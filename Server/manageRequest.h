#ifndef MANAGEREQUEST_H_
#define MANAGEREQUEST_H_
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <string>
#include <fstream>
#include <list>
#include <vector>
using namespace ::std;

struct clientIdentity
{

		int acceptId;
		string ip;
		int portno;
		string requesttime;
};
struct clientInfo
{
		string r_method;
		string r_type;
		string r_version;
		string r_firstline;
		string r_filename;
		string r_time;
		string r_servetime;
		int r_acceptid;
		string r_ip;
		u_int16_t r_portno;
		int r_filesize;
		bool status_file;
		string r_ctype;
		int status_code;

};

class ManageRequest
{
	public:
		list<clientInfo> clientlist;
		list<clientInfo> requestlist;
		bool
		fileExists (const char *filename);
		void
		parseRequest (clientIdentity);
		void
		readyQueue (clientInfo);
		void
		checkRequest (clientInfo);
		void
		popRequest ();
		static void *
		popRequest_helper (void *);
		void
		serveRequest ();
		static void *
		serveRequest_helper (void *);
		void
		checkFilename (clientInfo);
};
#endif
