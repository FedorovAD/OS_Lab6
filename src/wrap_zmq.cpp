#include <tuple>
#include <cstring>
#include <iostream>
#include "socket.h"
#include "wrap_zmq.h"
#include <unistd.h>
using namespace std;

void* create_zmq_ctx(){
	void* context  = zmq_ctx_new();
	if(context == nullptr){
		throw runtime_error("Can not create new context.");
	}
	return context;
}

void destroy_zmq_ctx(void* context){
	sleep(1);
	if(zmq_ctx_destroy(context) != 0){
		throw runtime_error("Can not destroy context.");
	}
}

int get_zmq_socket_type(SocketType type){
	switch(type){
		case SocketType::PUBLISHER:
			return ZMQ_PUB;
		case SocketType::SUBSCRIBER:
			return ZMQ_SUB;
		default:
			throw runtime_error("Undefined socket type.");
	}
}

void* create_zmq_socket(void* context, SocketType type){
	int zmq_type = get_zmq_socket_type(type);
	void* socket = zmq_socket(context, zmq_type);
	if(socket == nullptr){
		throw runtime_error("Can not create socket.");
	}
	return socket;
}

void close_zmq_socket(void* socket){
	sleep(1);
	if(zmq_close(socket) != 0){
		throw runtime_error("Can not close socket.");
	}
}

string create_endpoint(EndpointType type, pid_t id){
	switch(type){
		case EndpointType::PARENT_PUB:
			return "ipc:///tmp/parrent_pub_" + to_string(id);
		case EndpointType::CHILD_PUB_LEFT:
			return "ipc:///tmp/child_pub_left_" + to_string(id);
		case EndpointType::CHILD_PUB_RIGHT:
			return "ipc:///tmp/child_pub_right" + to_string(id);
		default:
			throw runtime_error("Wrong Endpoint type.");
	}
}

void bind_zmq_socket(void* socket, string endpoint){
	if(zmq_bind(socket, endpoint.data()) != 0){
		//zmq_connect(socket, endpoint.data());
		throw runtime_error("Can not bind socket.");	
	}
}

void unbind_zmq_socket(void* socket, string endpoint){
	sleep(1);
	if(zmq_unbind(socket, endpoint.data()) != 0){
		throw runtime_error("Can not unbind socket.");	
	}
}

void connect_zmq_socket(void* socket, string endpoint){
	if(zmq_connect(socket, endpoint.data()) != 0){
		throw runtime_error("Can not connect socket.");	
	}
	zmq_setsockopt(socket, ZMQ_SUBSCRIBE, 0, 0);
}

void disconnect_zmq_socket(void* socket, string endpoint){
	if(zmq_disconnect(socket, endpoint.data()) != 0){
		throw runtime_error("Can not disconnect socket.");	
	}
}

atomic<int> Message::counter = 0;

Message::Message(){
	command = CommandType::ERROR;
	to_id = SERVER_ID;
	uniq_num = ++counter;
	to_up = true;
}

Message::Message(CommandType new_command, int new_to_id, int new_size, char* new_value, int new_id)
: command(new_command), to_id(new_to_id), size(new_size), uniq_num(++counter), to_up(false), create_id(new_id){
	for(int i = 0; i < size; ++i){
		value[i] = new_value[i];
	}
}

Message::Message(CommandType new_command, int new_to_id, int new_id)
: command(new_command), to_id(new_to_id), uniq_num(++counter), to_up(false), create_id(new_id){}

bool operator==(const Message& lhs, const Message& rhs){
	return tie(lhs.command, lhs.to_id, lhs.create_id, lhs.uniq_num) == 
		   tie(rhs.command, rhs.to_id, rhs.create_id, rhs.uniq_num);
}

int& Message::get_create_id(){
	return create_id;
}

int& Message::get_to_id(){
	return to_id;
}

void create_zmq_msg(zmq_msg_t* zmq_msg, Message& msg){
	zmq_msg_init_size(zmq_msg, sizeof(msg));
	memcpy(zmq_msg_data(zmq_msg), &msg, sizeof(msg));
}

void send_zmq_msg(void* socket, Message& msg){
	zmq_msg_t zmq_msg;
	create_zmq_msg(&zmq_msg, msg);
	if(!zmq_msg_send(&zmq_msg, socket, 0)){
		throw runtime_error("Can not send message.");
	}
	zmq_msg_close(&zmq_msg);
}

Message get_zmq_msg(void* socket){
	zmq_msg_t zmq_msg;
	zmq_msg_init(&zmq_msg);
	if(zmq_msg_recv(&zmq_msg, socket, 0) == -1){
		//return Message(); //ERROR message
		throw runtime_error("ERROR message");
	}
	Message msg;
	memcpy(&msg, zmq_msg_data(&zmq_msg),  sizeof(msg));
	zmq_msg_close(&zmq_msg);
	return msg;
}