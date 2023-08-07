#include "main.h"
#include "manageRequest.h"
#include "senddata.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fstream>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <dirent.h>
#include <algorithm>
#include <unistd.h>

using namespace std;

// This function have the current client structure, this function will parse the request and fill the fields
void
ManageRequest::parseRequest (clientIdentity c_id)
{
	int recvbytes = 0;
	char buffer[1024];
	clientInfo cInfo;

	fcntl (c_id.acceptId, O_NONBLOCK, 0);

	if ((recvbytes = (recv (c_id.acceptId, buffer, sizeof(buffer), 0))) == -1)
		perror ("Receive:");
	buffer[recvbytes] = '\0';
	string request (buffer);

	int current = 0, current2 = 0;
	int next = request.find_first_of ("\r\n", current);
	if (next < 0)
	{
		write (cInfo.r_acceptid, "Error: Bad Request, Retry", 25);
		close (cInfo.r_acceptid);
	}
	else
	{
		cInfo.r_firstline = request.substr (current, next - current);

		current = cInfo.r_firstline.find_first_of (" ");
		current2 = cInfo.r_firstline.find_last_of (" ");

		cInfo.r_acceptid = c_id.acceptId;
		cInfo.r_portno = c_id.portno;
		cInfo.r_ip = c_id.ip;
		cInfo.r_time = c_id.requesttime;
		cInfo.r_type = cInfo.r_firstline.substr (0, current);
		cInfo.r_filename = cInfo.r_firstline.substr (current + 1, current2 - current - 1);
		cInfo.r_method = cInfo.r_firstline.substr (current2 + 1, cInfo.r_firstline.length () - current2);
		cInfo.r_filename = rootdir + cInfo.r_filename;

		checkFilename (cInfo);
		checkRequest (cInfo);
	}

}

// If tilt is present in the client request then change the filename accordingly.
void
ManageRequest::checkFilename (clientInfo c)
{
	string homeDir = getenv ("HOME");
	int next = c.r_filename.find_first_of ("~", 0);
	int size = c.r_filename.size ();
	if (next >= 0 && next < size)
	{
		int pos = c.r_filename.find_first_of ("/", next + 1);
		string username = c.r_filename.substr (next + 1, pos - (next + 1));
		string restString = c.r_filename.substr (pos, c.r_filename.size () - pos);
		if (username.length () == 0)
			c.r_filename = homeDir + restString;
		else
		{
			int pos2 = c.r_filename.find_last_of ("/", 0);
			c.r_filename = homeDir.substr (0, pos2) + restString;
		}
	}
}

// if file requested exists or not
bool
ManageRequest::fileExists (const char *filename)
{
	struct stat filenamecheck;
	if (stat (filename, &filenamecheck) != -1)
		return true;
	return false;
}

// Check type of request
void
ManageRequest::checkRequest (clientInfo cInfo)
{
	int pos = cInfo.r_filename.find_last_of ("/");
	int next = cInfo.r_filename.find_first_of (".", pos + 1);
	if (next > 0 && fileExists (cInfo.r_filename.c_str ()))
	{
		if(cInfo.r_type == "GET" || cInfo.r_type == "HEAD")
		{
			ifstream file;
			file.open (cInfo.r_filename.c_str ());
			int size=0;
			if (file.is_open ())
			{
				file.seekg (0, ios::end);
				size = file.tellg ();
			}
			cInfo.status_file = true;
			cInfo.r_filesize = (int) size;
			file.close ();
		}
	}
	else
	{
		cInfo.r_filesize = 0;
		cInfo.status_file = false;
	}
	readyQueue (cInfo);
}

// Sort the request based on file size
bool
sortRequest (const clientInfo& lhs, const clientInfo& rhs)
{
	return lhs.r_filesize < rhs.r_filesize;
}

// Here main thread will put the request in the queue
void
ManageRequest::readyQueue (clientInfo cInfo)
{
	pthread_mutex_lock (&rqueue_lock);
	clientlist.push_back (cInfo);
	pthread_cond_signal (&rqueue_cond);
	pthread_mutex_unlock (&rqueue_lock);
}

// In this scheduler thread will fetch the request from the queue based on the scheduling policies
void
ManageRequest::popRequest ()
{
	while (true)
	{
		clientInfo c;
		transform (scheduling.begin (), scheduling.end (), scheduling.begin (), ::toupper);
		if (scheduling == "SJF")
		{
			pthread_mutex_lock (&rqueue_lock);
			while (clientlist.empty ())
				pthread_cond_wait (&rqueue_cond, &rqueue_lock);
			clientlist.sort (sortRequest);
			c = clientlist.front ();
			clientlist.pop_front ();
			pthread_mutex_unlock (&rqueue_lock);
		}
		else
		{
			pthread_mutex_lock (&rqueue_lock);
			while (clientlist.empty ())
				pthread_cond_wait (&rqueue_cond, &rqueue_lock);
			c = clientlist.front ();
			clientlist.pop_front ();
			pthread_mutex_unlock (&rqueue_lock);
		}
		pthread_mutex_lock (&print_lock);
		requestlist.push_back (c);
		pthread_cond_signal (&print_cond);
		pthread_mutex_unlock (&print_lock);
	}
}

void *
ManageRequest::popRequest_helper (void *c)
{
	((ManageRequest *) c)->popRequest ();
	return NULL;
}

void *
ManageRequest::serveRequest_helper (void *c)
{
	ManageRequest *M2 = (ManageRequest *) c;
	M2->serveRequest ();
	return NULL;
}

// In this function continuously threads are acting to serve the client request.
void
ManageRequest::serveRequest ()
{
	pthread_detach (pthread_self ());
	while (1)
	{
		pthread_mutex_lock (&print_lock);
		while (requestlist.empty ())
			pthread_cond_wait (&print_cond, &print_lock);
		SendData S;
		clientInfo c;
		c = requestlist.front ();
		requestlist.pop_front ();

		// Note serving time of the file request
		time_t tim = time (NULL);
		tm *now = localtime (&tim);
		char currtime[50];
		if (strftime (currtime, 50, "%x:%X", now) == 0)
			perror ("Date Format Error");
		string servetime (currtime);
		c.r_servetime = servetime;
		pthread_mutex_unlock (&print_lock);
		S.sendData (c);
	}
}
