#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "server.h"
#include "manageRequest.h"
#include "main.h"
#include <unistd.h>
#include <cstdlib>
using namespace std;

// This function will create the socket for all the incoming client connections & update the client structure and pass
// this structure to manageRequest class
void
RunServer::accept_connection ()
{
	if ((sockId = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror ("Socket");
		exit (1);
	}

	/*---Initialize address/port structure---*/
	bzero (&self, sizeof(self));
	self.sin_family = AF_INET;
	self.sin_port = htons(port);
	self.sin_addr.s_addr = INADDR_ANY;
	int option_value = 1;
	if (setsockopt (sockId, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(int)) == -1)
	{
		perror ("setsockopt");
		exit (1);
	}
	/*---Assign a port number to the socket---*/
	if (bind (sockId, (struct sockaddr*) &self, sizeof(self)) != 0)
	{
		perror ("socket--bind");
		exit (1);
	}

	/*---Make it a "listening socket"---*/
	if (listen (sockId, 20) != 0)
	{
		perror ("socket--listen");
		exit (1);
	}

	while (true)
	{		// Main thread will listen continuously
		int acceptId;
		struct sockaddr_in client_addr;
		int addrlen = sizeof(client_addr);

		/*---accept a connection (creating a data pipe)---*/
		acceptId = accept (sockId, (struct sockaddr*) &client_addr, (socklen_t *) &addrlen);
		time_t tim = time (NULL);
		tm *now = localtime (&tim);
		char currtime[50];
		//cout<<"here\n"<<clientport<<"here\n";
		if (strftime (currtime, 50, "%x:%X", now) == 0)
			perror ("Date Error");
		string requesttime (currtime);
		clientIdentity cid;
		cid.acceptId = acceptId;
		cid.ip = inet_ntoa (client_addr.sin_addr);
		cid.portno = ntohs(client_addr.sin_port);
		cid.requesttime = requesttime;
		cout << cid.ip << " " << cid.portno << " " << cid.requesttime << endl;
		M->parseRequest (cid);
	}
}
