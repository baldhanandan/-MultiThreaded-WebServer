#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fstream>
#include <time.h>
#include <pthread.h>
#include <sstream>
#include <dirent.h>
#include <vector>
#include <algorithm>
#include "manageRequest.h"
#include "senddata.h"
#include "main.h"
#include <unistd.h>
#include <signal.h>
using namespace std;

void
SendData::sendData (clientInfo c)				// Sending data
{
	signal(SIGPIPE,SIG_IGN);
	cout << "Serving request " << c.r_filename << endl;
	if (c.status_file == true)
	{
		// Last Modified status of requested file
		struct stat buf;
		char lastmodify[50];
		stat (c.r_filename.c_str (), &buf);
		strcpy (lastmodify, ctime (&buf.st_mtime));
		string lmodify (lastmodify);

		// Store the size of the requsted file in string
		std::stringstream ss;
		ss << c.r_filesize;
		string filesize = ss.str ();

		// Type of file requested
		int pos = c.r_filename.find_last_of (".");
		string c_type = c.r_filename.substr (pos + 1, c.r_filename.size ());
		if (c_type == "txt" || c_type == "html" || c_type == "htm")
			c.r_ctype = "text/html";
		else if (c_type == "gif" || c_type == "jpeg")
			c.r_ctype = "image/" + c_type;
		else
			c.r_ctype = " ";

		//Building header
		string header = c.r_type + " " + c.r_method + " 200 OK\r\n";
		header = header + "Date:" + c.r_servetime + "\r\n" + "Server: httpserver 1.1\r\n";
		header = header + "Last-Modified:" + lmodify + "\r\n" + "Content-Type:" + c.r_ctype + "\r\n";
		header = header + "Content-Length:" + filesize + "\r\n\r\n";
		c.status_code = 200;
		if (send (c.r_acceptid, header.c_str (), strlen (header.c_str ()), 0) == -1)
			perror ("send");
		// Task of HEAD request is done now.
		// GET request sends a file after sending header.
		if (c.r_type == "GET")
		{
			//Send the file
			ifstream file;
			char *readblock;
			size_t size;
			file.open (c.r_filename.c_str ());
			if (file.is_open ())
			{
				file.seekg (0, ios::end);
				size = file.tellg ();
				readblock = new char[size];
				file.seekg (0, ios::beg);
				file.read (readblock, size);
			}
			else
				cout << "Error opening file " << c.r_filename << endl;
			int sent = 0;
			if ((sent = send (c.r_acceptid, readblock, size, 0)) == -1)
				perror ("send");
			file.close ();
			delete[] readblock;
		}
	}
	else
	{
		write (c.r_acceptid, "Error 404: File Not Found", 25);
		c.status_code = 404;
		listingDir (c);
	}
	cout << "Served request of " << c.r_filename << endl;

	// Note time of completion of service.
	time_t tim = time (NULL);
	tm *now = localtime (&tim);
	char currtime[50];
	if (strftime (currtime, 50, "%x:%X", now) == 0)
		perror ("Date Format Error");
	string servetime (currtime);
	c.r_servetime = servetime;
	if (logging) // If request need to be logged
		generatingLog (c);
	if (consoleLog)  // If console log need to be printed
		displaylog (c);

	close (c.r_acceptid);
}

void
SendData::generatingLog (clientInfo c)
{
	ofstream logfile;
	string file = l_file;
	logfile.open (file.c_str (), std::ios::app);
	if (!logfile.is_open())
		cout << "Error: file could not be opened" << endl;				// file couldn't be opened
	logfile << c.r_ip << "  [" << c.r_time << "]  [" << c.r_servetime << "]  " << c.r_firstline << " " << c.status_code << " " << c.r_filesize << endl;
	logfile.close ();
}

void
SendData::displaylog (clientInfo c)
{
	cout << c.r_ip << "  [" << c.r_time << "]  [" << c.r_servetime << "]  " << c.r_firstline << " " << c.status_code << " " << c.r_filesize << endl;
}

// Sort the list based on alphabets
bool
sortDirectory (const string &left, const string &right)
{
	for (string::const_iterator lit = left.begin (), rit = right.begin (); lit != left.end () && rit != right.end (); ++lit, ++rit)
		if (tolower (*lit) < tolower (*rit))
			return true;
		else if (tolower (*lit) > tolower (*rit))
			return false;
	if (left.size () < right.size ())
		return true;
	return false;
}

// List the directory
void
SendData::listingDir (clientInfo c)
{
	struct dirent *de = NULL;
	DIR *d = NULL;
	int last = c.r_filename.find_last_of ("/");
	string dir = c.r_filename.substr (0, last);
	vector<string> dirlist;
	char * dirname = new char[dir.size () + 1];
	std::copy (dir.begin (), dir.end (), dirname);
	dirname[dir.size ()] = '\0';
	d = opendir (dirname);
	if (d == NULL)
	{
		write (c.r_acceptid, "Error 404: Directory Not Found", 30);
		c.status_code = 404;
	}
	else
	{
		while ((de = readdir (d)))
		{
			string s (de->d_name);
			dirlist.push_back (s);
		}
		vector<string>::iterator it;
		std::sort (dirlist.begin (), dirlist.end (), sortDirectory);
		write (c.r_acceptid, "Files Listing:", 14);
		for (it = dirlist.begin (); it < dirlist.end (); it++)
		{
			write (c.r_acceptid, (*it).c_str (), strlen ((*it).c_str ()));
			write (c.r_acceptid, "\n", 1);
		}
		closedir (d);
		delete[] dirname;
		dirname = NULL;
	}
}

