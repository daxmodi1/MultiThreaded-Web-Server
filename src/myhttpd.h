#ifndef MYHTTPD_H_
#define MYHTTPD_H_

#include <condition_variable>
#include <mutex>
#include <string>

#include "../src/parse.h"
#include "../src/myserver.h"

extern std::mutex rqueue_mutex;
extern std::condition_variable rqueue_cond;
extern std::mutex request_mutex;
extern std::condition_variable request_cond;
extern Parse *P;
class RunServer;
extern int sockId;
extern bool r_daemon;
extern RunServer *run;

extern std::string port;
extern bool logging;
extern std::string scheduling;
extern int threadnum;
extern bool summary;
extern int r_time;
extern std::string l_file;
extern std::string rootdir;



#endif // ALLHEADERS_H_

