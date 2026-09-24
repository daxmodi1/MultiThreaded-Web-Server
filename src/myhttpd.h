#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include "../src/parse.h"
#include "myserver.h"


extern std::mutex rqueue_mutex; 
extern std::condition_variable rqueue_cond; 
extern std::mutex request_mutex;
extern std::condition_variable request_cond; 
extern Parse *P;  // parsing and serving the methods
class RunServer;    
extern int sockId; // for ctrl+c handling
extern bool r_daemon;  // debug mode
extern RunServer *run;  // start the listener through accept_connection() method of RunServer class
extern std::string port; // port no.
extern bool logging;  // logging option
extern std::string scheduling;  // scheduling type
extern int threadnum;  // no of threads in thread pool
extern bool summary;  // for the -h option
extern int r_time;   // initial wait before moving req to worker queue
extern std::string l_file;  // path to the log file
extern std::string rootdir; // root directory for serving files


