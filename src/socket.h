#ifndef _SOCKET_H
#define _SOCKET_H

#include <string>
#include "wrap_zmq.h" 
using namespace std;

class Socket {
public:
    Socket(void* context, SocketType new_socket_type, string new_endpoint);
    ~Socket();
    void send(Message message);
    Message receive();
    void subscribe(string new_endpoint);
    string get_endpoint() const;
    void*& get_socket();
private:
    void* socket;
    SocketType socket_type;
    string endpoint;
};

#endif