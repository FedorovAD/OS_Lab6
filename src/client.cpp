#include <string.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <csignal>
#include "wrap_zmq.h"
#include "socket.h"
#include "client.h"
#include <signal.h>
#include <sys/types.h>
using namespace std;

Client::Client(int new_id, string parent_endpoint, int new_parent_id){
	id = new_id;
	parent_id = new_parent_id;
	context = create_zmq_ctx();
	string endpoint = create_endpoint(EndpointType::CHILD_PUB_LEFT, getpid());
	child_publisher_left = new Socket(context, SocketType::PUBLISHER, endpoint);
	endpoint = create_endpoint(EndpointType::CHILD_PUB_RIGHT, getpid());
	child_publisher_right = new Socket(context, SocketType::PUBLISHER, endpoint);
	endpoint = create_endpoint(EndpointType::PARENT_PUB, getpid());
	parent_publisher = new Socket(context, SocketType::PUBLISHER, endpoint);
	parent_subscriber = new Socket(context, SocketType::SUBSCRIBER, parent_endpoint);
	left_subscriber = nullptr;
	right_subscriber = nullptr;
	terminated = false;
}

Client::~Client(){
	if(terminated) return;
	terminated = true;
	try{
		delete child_publisher_left;
		delete child_publisher_right;
		delete parent_publisher;
		delete parent_subscriber;
		if(left_subscriber)
			delete left_subscriber;
		if(right_subscriber)
			delete right_subscriber;
		destroy_zmq_ctx(context);
	} catch(runtime_error& err){
		cout << "Server wasn't stopped " << err.what() << endl;
	}
}

bool& Client::get_status(){
	return terminated;
}

void Client::send_up(Message msg){
	msg.to_up = true;
	parent_publisher->send(msg);
}

void Client::send_down(Message msg){
	msg.to_up = false;
	child_publisher_left->send(msg);
	child_publisher_right->send(msg);
}

int Client::get_id(){
	return id;
}

int Client::add_child(int new_id){
	pid_t pid = fork();
	if(pid == -1) throw runtime_error("Can not fork.");
	if(!pid){
		string endpoint = child_publisher_left->get_endpoint();
		execl("client", "client", to_string(new_id).data(), 
			endpoint.data(), to_string(id).data(), nullptr);
		throw runtime_error("Can not execl.");
	}
	string endpoint = create_endpoint(EndpointType::PARENT_PUB, pid);
	int timeout = 1500;
	left_subscriber = new Socket(context, SocketType::SUBSCRIBER, endpoint);
	zmq_setsockopt( left_subscriber->get_socket(), ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
	return pid;
}

int Client::add_brother(int new_id){
	pid_t pid = fork();
	if(pid == -1) throw runtime_error("Can not fork.");
	if(!pid){
		string endpoint = child_publisher_right->get_endpoint();
		execl("client", "client", to_string(new_id).data(), 
			endpoint.data(), to_string(id).data(), nullptr);
		throw runtime_error("Can not execl.");
	}
	string endpoint = create_endpoint(EndpointType::PARENT_PUB, pid);
	int timeout = 1500;
	right_subscriber = new Socket(context, SocketType::SUBSCRIBER, endpoint);
	zmq_setsockopt(right_subscriber->get_socket(), ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
	return pid;
}


void process_msg(Client& client, Message msg){
	switch(msg.command){
		case CommandType::ERROR:
			throw runtime_error("Error message received.");
		case CommandType::RETURN:{
			//cout << client.get_id() << " return msg!\n"; 
			msg.get_to_id() = SERVER_ID;
			client.send_up(msg);
			break;
		}
		case CommandType::CREATE_CHILD:{
			msg.get_create_id() = client.add_child(msg.get_create_id());
			msg.get_to_id() = SERVER_ID;
			client.send_up(msg);
			break;
		}
		case CommandType::CREATE_BROTHER:{
			msg.get_create_id() = client.add_brother(msg.get_create_id());
			msg.get_to_id() = SERVER_ID;
			client.send_up(msg);
			break;
		}
		case CommandType::REMOVE_BROTHER:
		case CommandType::REMOVE_CHILD:{
			if(msg.to_up){
				client.send_up(msg);
				break;
			}
			if(msg.to_id != client.get_id() && msg.to_id != UNIVERSAL_MSG){
				client.send_down(msg);
				break;
			}
			msg.get_to_id() = PARENT_SIGNAL;
			msg.get_create_id() = client.get_id(); 
			client.send_up(msg);
			msg.get_to_id() = UNIVERSAL_MSG;
			client.send_down(msg);
			client.~Client();
			throw invalid_argument("Exiting child...");
			break;
		}
		case CommandType::EXEC_CHILD: {
			time_t tmp;
			switch(msg.get_create_id()){
				case 0:{
					client.cur_timer = true;
					client.has_time = true;
					client.timestamp = time(&tmp);
					msg.size = -1;
					break;
				}
				case 1:{
					if(client.cur_timer){
						client.timestamp = time(&tmp) - client.timestamp;
						client.cur_timer = false;
					}
					msg.size = -1;
					break;
				}
				case 2:{
					if(client.has_time){
						if(client.cur_timer){
							msg.size = time(&tmp) - client.timestamp;
						} else{
							msg.size = client.timestamp;
						}
					} else{
						msg.size = -2;
					}
					break;
				}
				default:{
					break;
				}	
			}
			msg.get_create_id() = client.get_id();
			client.send_up(msg);
			break;
		}
		default:
			throw runtime_error("Undefined command.");
	}
}

Client* client_ptr = nullptr;
void TerminateByUser(int) {
	if (client_ptr != nullptr) {
		client_ptr->~Client();
	}
	cout << to_string(getpid()) + " Terminated by user" << endl;
	exit(0);
}

int main (int argc, char const *argv[]) 
{
	if(argc != 4){
		cout << "-1" << endl;
		return -1;
	}
	try{
		if (signal(SIGINT, TerminateByUser) == SIG_ERR) {
			throw runtime_error("Can not set SIGINT signal");
		}
		if (signal(SIGSEGV, TerminateByUser) == SIG_ERR) {
			throw runtime_error("Can not set SIGSEGV signal");
		}
		if (signal(SIGTERM, TerminateByUser) == SIG_ERR) {
			throw runtime_error("Can not set SIGTERM signal");
		}
		Client client(stoi(argv[1]), string(argv[2]), stoi(argv[3]));
		client_ptr = &client;
		cout << getpid() << ": " "Client started. "  << "Id:" << client.get_id() << endl;
		for(;;){
			//cout << client.get_id() << ": " "Client ready. " << '\n';
			Message msg = client.parent_subscriber->receive();
			//cout << client.get_id() << "! !" << msg.to_id << " " << int(msg.command) << '\n';
			if(msg.to_id != client.get_id() && msg.to_id != UNIVERSAL_MSG){
				if(msg.to_up){
					client.send_up(msg);
				} else{
					try{
						msg.to_up = false;
						if(msg.value[msg.offset++] == 'c'){
							client.child_publisher_left->send(msg);
							msg = client.left_subscriber->receive();
						} else{
							client.child_publisher_right->send(msg);
							msg = client.right_subscriber->receive();
						}
						if((msg.command == CommandType::REMOVE_CHILD || msg.command == CommandType::REMOVE_BROTHER)
							 && msg.to_id == PARENT_SIGNAL){
							msg.to_id = SERVER_ID;
							//cout << "!!\n"; 
							if(msg.command == CommandType::REMOVE_BROTHER){
								delete client.right_subscriber;
								client.right_subscriber = nullptr;
							} else{
								delete client.left_subscriber;
								client.left_subscriber = nullptr;
							}
						}
						//cout << "(" << client.get_id() << ' ' << msg.to_id << int(msg.command) << '\n';
						client.send_up(msg);
					}catch(exception& e){
						//cout << e.what() << '\n';
						if(client.parent_id == -2){
							//cout << client.get_id() <<  " sent " << msg.uniq_num << '\n';
							client.send_up(Message());
						}
					}
				}
			} else{
				process_msg(client, msg);
			}
		}
	} catch(runtime_error& err){
		cout << getpid() << ": " << err.what() << '\n';
	} catch(invalid_argument& inv){
		cout << getpid() << ": " << inv.what() << '\n';
	}
	return 0;
}
