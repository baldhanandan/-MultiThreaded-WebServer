#ifndef MAIN_H_
#define MAIN_H_

#include <pthread.h>
#include "manageRequest.h"
#include "server.h"

extern pthread_mutex_t rqueue_lock;
extern pthread_cond_t rqueue_cond;
extern pthread_mutex_t print_lock;
extern pthread_cond_t print_cond;
extern ManageRequest *M;
class RunServer;
extern int sockId;
extern bool consoleLog;
extern RunServer *run;

extern int port;
extern bool logging;
extern string scheduling;
extern int threadnum;
extern bool summary;
extern string l_file;
extern string rootdir;
extern bool consoleLog;

#endif // ALLHEADERS_H_

