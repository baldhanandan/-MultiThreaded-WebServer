#include "main.h"
#include "server.h"
#include "manageRequest.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <string>
#include <cstring>
#include <unistd.h>

using namespace std;
#define MAXTHREADS 30

int port = 8080;
bool consoleLog = false;
bool logging = false;
string l_file = "log.txt";
string scheduling = "FCFS";
int threadnum = 10;
string rootdir = "./resources/";

int sockId = 0;
ManageRequest *M = new ManageRequest ();
pthread_mutex_t rqueue_lock;
pthread_cond_t rqueue_cond;
pthread_mutex_t print_lock;
pthread_cond_t print_cond;
pthread_t thread_scheduler;
pthread_t threads[MAXTHREADS];
RunServer *run = new RunServer ();

void
printhelp ()
{
	printf ("\n************************************************************************************************************\n");
	printf ("HELP: server -c -h -l logFileName -p portNo -r rootDirectory -n threadnumber -s scheduling\n");
	printf ("Use -c option to display file requests log on console\n");
	printf ("Use -h option to display the this help menu\n");
	printf ("Use -l followed by filename of log file\n");
	printf ("Use -p followed by port number to change default port number (8080)\n");
	printf ("Use -r followed by root directory to change default root directory (./resources/)\n");
	printf ("Use -n followed by number of threads to change the default value (10)\n");
	printf ("Use -s followed by name of scheduling algorithm to change default algorithm (SJF)\n");
	printf ("\nScheduling algorithms : SJF (Shortest Job First), FCFS (First Come First Serve)\n");
	printf ("Press Ctrl+C to exit the server\n");
	printf ("\n");
	printf ("************************************************************************************************************\n\n");
}

void
sigint_handler (int signum)
{
	close (sockId);
	delete M;
	M = NULL;
	delete run;
	run = NULL;
	pthread_mutex_destroy (&rqueue_lock);
	pthread_cond_destroy (&rqueue_cond);
	pthread_mutex_destroy (&print_lock);
	pthread_cond_destroy (&print_cond);
	pthread_cancel (thread_scheduler);
	for (int i = 0; i < threadnum; i++)
		pthread_cancel (threads[i]);
	exit (signum);
}

int
main (int argc, char *argv[])
{
	int opt = 0;
	while ((opt = getopt (argc, argv, "chl:p:r:n:s:")) != -1)
	{
		switch (opt)
		{
			case 'c':
				consoleLog = true;
				break;
			case 'h':
				printhelp ();
				return 0;
			case 'l':
				l_file.assign (optarg);
				logging = true;
				break;
			case 'p':
				port = atoi (optarg);
				break;
			case 'r':
				rootdir.assign (optarg);
				break;
			case 'n':
				threadnum = atoi (optarg);
				break;
			case 's':
				scheduling.assign (optarg);
				break;
			default:
				break;
		}
	}
	signal (SIGINT, sigint_handler);

	pthread_mutex_init (&rqueue_lock, NULL);
	pthread_mutex_init (&print_lock, NULL);
	pthread_cond_init (&rqueue_cond, NULL);
	pthread_cond_init (&print_cond, NULL);
	pthread_create (&thread_scheduler, NULL, &ManageRequest::popRequest_helper, M);
	for (int i = 0; i < threadnum; i++)
		pthread_create (&threads[i], NULL, &ManageRequest::serveRequest_helper, M);
	run->accept_connection ();

	return 0;
}
