#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <bits/stdc++.h>
#include <arpa/inet.h> 
#include <pthread.h>

#define MAXLINE 256
#define MAXFILES 30
#define MAXSUB 10
#define MAXRESPONSE 10
using namespace std;
pthread_t *threads;
string ipaddress, exefile, input;
int noFiles;
string files[MAXFILES];
int data[MAXFILES];
long long received[MAXFILES], total[MAXFILES];
void
printhelp (char *outputfile)
{
	printf ("\nUsage: %s -i ipAddress -h -f inputFile \n\n", outputfile);
	printf ("\t-i followed by ip address of the server\n");
	printf ("\t-f followed by input file name, default input file is %s\n", input.c_str ());
	printf ("\t-h to print this help\n\n");
	printf ("Note : -i option is necessary\n");
	printf ("Input file format:\n");
	printf ("Number of files\n<file1>\n<file2>\n.\n.\n.\n<fileN>\n\n");
}
void *
clientRequest (void *xx)
{
	int sockfd = 0, n = 0;
	struct sockaddr_in serv_addr;

	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf ("\n Error : Could notd create socket \n");
		return NULL;
	}

	memset (&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(8080);

	if (inet_pton (AF_INET, ipaddress.c_str (), &serv_addr.sin_addr) <= 0)
	{
		printf ("\n inet_pton error occured\n");
		return NULL;
	}

	if (connect (sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{
		printf ("\n Error : Connect Failed \n");
		return NULL;
	}
	ofstream file;
	int y = *((int *) xx);
	// Form request
	char sendline[MAXLINE+1], recvline[MAXLINE+1];
	sprintf (sendline, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", files[y].c_str (), ipaddress.c_str ());

	file.open (files[y].c_str (), ios::ate);
	if (write (sockfd, sendline, strlen (sendline)) >= 0)
	{
		//Read the header
		if ((n = read (sockfd, recvline, MAXLINE)) > 0)
			recvline[n] = '\0';
		string header (recvline);
		string temp = "Content-Length:";
		string crlf = "\r\n\r\n";
		int pos = header.find (temp);
		int pos2 = header.find (crlf);
		total[y] = atoll (header.substr (pos + temp.length (), pos2 - pos).c_str ());
		if (pos2 + crlf.length () <= MAXLINE - 1)
		{
			received[y] += header.length () - pos2 - crlf.length ();
			// Showing progress of only first two files on th same line console in order to show parallelism.
			if (noFiles >= 2)
			{
				int percent = (100.00 * received[0]) / total[0];
				int percent2 = (100.00 * received[1]) / total[1];
				printf ("\r%s : %lld/%lld  %d %%  , %s : %lld/%lld  %d %%", files[0].c_str (), received[0], total[0], percent, files[1].c_str (), received[1], total[1], percent2);
			}
			file << header.substr (pos2 + crlf.length (), header.length () - pos2 - crlf.length () + 1);
		}
		// Read the response
		while ((n = read (sockfd, recvline, MAXLINE)) > 0)
		{
			received[y] += n;
			// Showing progress of only first two files on th same line console in order to show parallelism.
			if (noFiles >= 2)
			{
				int percent = (100.00 * received[0]) / total[0];
				int percent2 = (100.00 * received[1]) / total[1];
				printf ("\r%s : %lld/%lld  %d %%  , %s : %lld/%lld  %d %%", files[0].c_str (), received[0], total[0], percent, files[1].c_str (), received[1], total[1], percent2);
			}
			recvline[n] = '\0';
			file << recvline;
		}
	}
	file.close ();
	return NULL;
}
int
main (int argc, char *argv[])
{
	int opt = 0;
	input = "input.txt";
	bool flagIp = false;
	while ((opt = getopt (argc, argv, "i:f:h")) != -1)
	{
		switch (opt)
		{
			case 'i':
				ipaddress = optarg;
				flagIp = true;
				break;
			case 'f':
				input = optarg;
				break;
			case 'h':
				printhelp (argv[0]);
				return 0;
			default:
				return 0;
		}
	}
	if (flagIp == false)
	{
		printf ("Please provide -i <ip-address> \nUse -h option for help\n");
		return 0;
	}
	ifstream f1;
	f1.open (input.c_str());
	if (!f1.is_open ())
	{
		printf ("Error opening file\n");
		return 0;
	}
	char temp[256];
	f1.getline (temp, 256);
	noFiles = atoi (temp);
	for (int i = 0; i < noFiles; i++)
	{
		f1.getline (temp, 256);
		files[i].assign (temp);
		if (*files[i].rbegin () == '\r')
			files[i].erase (files[i].length () - 1);
		data[i] = i;
	}
	threads = new pthread_t[noFiles];
	if (noFiles >= 2)
		printf ("Showing progress of only first two files\n");
	for (int i = 0; i < noFiles; i++)
		pthread_create (&threads[i], NULL, clientRequest, (void *) &data[i]);
	for (int i = 0; i < noFiles; i++)
		pthread_join (threads[i], NULL);
	return 0;
}
