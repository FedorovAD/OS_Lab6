#include <string.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <csignal>
#include "server.h"
#include "socket.h"
#include "wrap_zmq.h"

void* subscriber_thread(void* server){
	Server* server_ptr = (Server*) server;
	pid_t serv_pid = server_ptr->get_pid();
	try{
		pid_t child_pid = fork();
		if(child_pid == -1) throw runtime_error("Can not fork.");
		if(child_pid == 0){
			execl("client", "client", "0", server_ptr->get_publisher()->get_endpoint().data(), "-2", nullptr);
			throw runtime_error("Can not execl");
			server_ptr->~Server();
			return (void*)-1;
		}
		string endpoint = create_endpoint(EndpointType::PARENT_PUB, child_pid);
		server_ptr->get_subscriber() = new Socket(server_ptr->get_context(), SocketType::SUBSCRIBER, endpoint);
		server_ptr->get_tree().insert(0);
		for(;;){
			Message msg;
			try{
				msg = server_ptr->get_subscriber()->receive();
			} catch(...){
				msg = Message();
			}	
			server_ptr->last_msg = msg;
			//cout << "Message on server: " << msg.uniq_num << " " << (int)msg.command << '\n';
			switch(msg.command){
				case CommandType::CREATE_BROTHER:
				case CommandType::CREATE_CHILD:
					cout << "OK:" << msg.get_create_id() << '\n';
					break;
				case CommandType::REMOVE_BROTHER:
				case CommandType::REMOVE_CHILD:
					cout << "OK\n";
					break;
				case CommandType::RETURN:
					break;
				case CommandType::EXEC_CHILD:{
					//cout << "OK:" << msg.get_create_id() << ":" << msg.value[0] << '\n';
					if(msg.size == -2){
						cout << "Error:" << msg.get_create_id() << ":Timer not set!\n";
					} else {
						cout << "Ok:" << msg.get_create_id();
						if(msg.size != -1){
							cout << ": " << msg.size * 1000;
						}
						cout << '\n';
					}
					break;
				}
				default:
					break;
			}
		}
	} catch(runtime_error& err){
		cout << "Server wasn't started " << err.what() << '\n';
	}
	return nullptr;
}

Server::Server(){
	context = create_zmq_ctx();
	pid = getpid();
	string endpoint = create_endpoint(EndpointType::CHILD_PUB_LEFT, getpid());
	publisher = new Socket(context, SocketType::PUBLISHER, endpoint);
	if(pthread_create(&receive_msg, 0, subscriber_thread, this) != 0){
		throw runtime_error("Can not run second thread.");
	}
	working = true;
}

Server::~Server(){
	if(!working) return;
	working = false;
	send(Message(CommandType::REMOVE_CHILD, 0, 0));
	try{
		delete publisher;
		delete subscriber;
		publisher = nullptr;
		subscriber = nullptr;
		destroy_zmq_ctx(context);
		sleep(2);
	} catch (runtime_error &err){
		cout << "Server wasn't stopped " << err.what() << '\n';
	}
}

pid_t Server::get_pid(){
	return pid;
}

void Server::print_tree(){
	t.print();
}

bool Server::check(int id){
	string path = t.find(id).path;
	Message msg(CommandType::RETURN, id, path.size(), path.data(), 0);
	//cout << "sent " << msg.uniq_num << '\n';
	send(msg);
	//cout << "Sleep zzz!" << '\n';
	//usleep(msg_wait_time);
	sleep(2);
	//cout << "Alarm!" << '\n';
	msg.get_to_id() = SERVER_ID;
	//cout << msg.to_id << " " << msg.create_id << " " << msg.uniq_num << " " << '\n';
	//cout << last_msg.to_id << " " << last_msg.create_id << " " << last_msg.uniq_num << " " << '\n';
	return last_msg == msg;
}

void Server::send(Message msg){
	msg.to_up = false;
	publisher->send(msg);
}

Message Server::receive(){
	return subscriber->receive();
}

Socket*& Server::get_publisher(){
	return publisher;
}
	
Socket*& Server::get_subscriber(){
	return subscriber;
}

void* Server::get_context(){
	return context;
}

tree& Server::get_tree(){
	return t;
}

void Server::create_child(int id, int parent_val){
	if(t.find(id).found){
		throw runtime_error("Error:" + to_string(id) + ":Node with that number already exists.");
	}
	int parent_id = t.insert(id, parent_val);
	if(parent_id == -1){
		throw runtime_error("Error:" + to_string(id) + ":Parent node doesn't exist.");
	}
	if(parent_id && !check(parent_id)){ //not zero node
		throw runtime_error("Error:" + to_string(id) + ":Parent node is unavailable.");
	}
	string path = t.find(parent_id).path;
	if(parent_val == parent_id){
		send(Message(CommandType::CREATE_CHILD, parent_id, path.size(), path.data(), id));
	} else{
		send(Message(CommandType::CREATE_BROTHER, parent_id, path.size(), path.data(), id));
	}
}

void Server::remove_child(int id){
	if(!t.find(id).found){
		throw runtime_error("Error:" + to_string(id) + ":Node with that number doesn't exist.");
	}
	if(!check(id)){
		throw runtime_error("Error:" + to_string(id) + ":Node is unavailable.");
	}
	string path = t.find(id).path;
	int parent_id = t.delete_el(id);
	if(parent_id == -255){
		throw runtime_error("Error:" + to_string(id) + ":Parent node doesn't exist.");
	}
	//cout << id << '!' << path <<'\n';
	if(parent_id <= 0){
		send(Message(CommandType::REMOVE_CHILD, id, path.size(), path.data(), id));
	} else{
		send(Message(CommandType::REMOVE_BROTHER, id, path.size(), path.data(), id));
	}
}

void Server::exec_child(int id){
	string n;
	cin >> n;
	int state = -1;
	if(n == "start"){
		state = 0;
	} else if(n == "stop"){
		state = 1;
	} else if(n == "time"){
		state = 2;
	}
	if(state == -1){
		throw runtime_error("Error: Unknown subcommand.");
	}
	string path = t.find(id).path;
	send(Message(CommandType::EXEC_CHILD, id, path.size(), path.data(), state));
}

void process_cmd(Server& server, string cmd){
	if(cmd == "create"){
		int id, parent_id;
		cin >> id >> parent_id;
		if(parent_id == -1)
			parent_id = 0;
		server.create_child(id, parent_id);
	} else if (cmd == "remove"){
		int id;
		cin >> id;
		server.remove_child(id);
	} else if (cmd == "exec"){
		int id;
		cin >> id;
		server.exec_child(id);
	} else if(cmd == "exit"){
		throw invalid_argument("Exiting...");
	} else if(cmd == "ping"){
		int id;
		cin >> id;
		if(!server.get_tree().find(id).found){
			cout << "Error: Not found\n";
		}
		cout << "Ok: ";
		cout << server.check(id) << '\n';
	} else if(cmd == "status"){
		int id;
		cin >> id;
		if(!server.get_tree().find(id).found){
			throw runtime_error("Error:" + to_string(id) + ":Node with that number doesn't exist.");
		}
		if(server.check(id)){
			cout << "OK" << '\n';
		} else{
			cout << "Node is unavailable" << '\n';
		}
	} else {
		cout << "It is not a command!\n";
	}
}

Server* server_ptr = nullptr;
void TerminateByUser(int) {
	if (server_ptr != nullptr) {
		server_ptr->~Server();
	}
	cout << to_string(getpid()) + " Terminated by user" << '\n';
	exit(0);
}

int main (int argc, char const *argv[]) 
{
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
		Server server;
		server_ptr = &server;
		cout << getpid() << " server started correctly!\n";
		for(;;){
			try{
				string cmd;
				while(cin >> cmd){
					process_cmd(server, cmd);
				}
			} catch(const runtime_error& arg){
				cout << arg.what() << '\n';
			}
		}
	} catch(const runtime_error& arg){
		cout << arg.what() << '\n';
	} catch(...){}
	sleep(8);
	return 0;
}
