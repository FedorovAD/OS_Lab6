all_done: server client

server: server.cpp socket.cpp wrap_zmq.cpp tree.cpp
	g++ server.cpp socket.cpp wrap_zmq.cpp tree.cpp -lzmq -o server -pthread
	
client: client.cpp socket.cpp wrap_zmq.cpp tree.cpp
	g++ client.cpp socket.cpp wrap_zmq.cpp tree.cpp -lzmq -o client