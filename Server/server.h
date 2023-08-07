#ifndef SERVER_H_
#define SERVER_H_

#ifndef MAXCONNECTION
#define MAXCONNECTION 10
#endif
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

class RunServer
{
	public:
		int sockId;
		struct sockaddr_in self;
		void
		accept_connection ();
};

#endif 
