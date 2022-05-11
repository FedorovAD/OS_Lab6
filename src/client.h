#ifndef _CLIENT_H
#define _CLIENT_H

#include <string.h>
#include <iostream>
#include <unistd.h>
#include <time.h>
#include "wrap_zmq.h"
#include "socket.h"

using namespace std;

class Client{
private:
	int id;
	void* context;
	bool terminated;
public:
	Socket* child_publisher_left;
	Socket* child_publisher_right;
	Socket* parent_publisher;
	Socket* parent_subscriber;
	Socket* left_subscriber;
	Socket* right_subscriber;
	Client(int new_id, string parent_endpoint, int new_parent_id);
	~Client();
	bool& get_status();
	void send_up(Message msg);
	void send_down(Message msg);
	Message receive();
	int get_id();
	int add_child(int id);
	int add_brother(int new_id);
	int parent_id;
	bool cur_timer = false;
	bool has_time = false;
	time_t timestamp = 0;
};

#endif
