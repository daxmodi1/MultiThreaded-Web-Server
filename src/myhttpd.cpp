#include "../src/myhttpd.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <signal.h>
#include <string>
#include <thread>
#include <unistd.h>

std::string port = "8080";
bool r_daemon = false;
bool logging = false;
std::string scheduling = "FCFS";
int threadnum = 10;
bool summary = false;
int r_time = 30;
std::string l_file = "log.txt";
std::string rootdir = "../files";

int sockId = -1;
std::mutex rqueue_mutex;
std::condition_variable rqueue_cond;
std::mutex request_mutex;
std::condition_variable request_cond;
sig_atomic_t flag = 0;

namespace
{
Parse parser;
RunServer server;



int toPositiveInt(const char *value, int fallback)
{
	char *end = NULL;
	const long parsed = strtol(value, &end, 10);
	if (end == value || *end != '\0' || parsed <= 0) {
		return fallback;
	}

	return static_cast<int>(parsed);
}
}

Parse *P = &parser;
RunServer *run = &server;

void signal_callback_handler(int signum)
{
	if (sockId != -1) {
		close(sockId);
	}
	_exit(signum);
}

void printh()
{
	std::cout << "\n**************************************************************************\n";
	std::cout << "Usage: myhttpd [-d] [-h] [-l file] [-p port] [-r rootdirectory] [-t time] [-n threadnumber] [-s FCFS|SJF]\n\n";
	std::cout << "-d  Run in single-request debug mode\n";
	std::cout << "-h  Show this summary and exit\n";
	std::cout << "-l  Append request logs to the given file\n";
	std::cout << "-p  Listen on the given port, default 8080\n";
	std::cout << "-r  Serve files from the given root directory, default ../files\n";
	std::cout << "-t  Scheduler initial wait time in seconds, default 30\n";
	std::cout << "-n  Worker thread count, default 10, max 30\n";
	std::cout << "-s  Scheduling policy: FCFS or SJF\n";
	std::cout << "Press Ctrl+C anytime to exit the server\n";
	std::cout << "**************************************************************************\n";
}

int main(int argc, char *argv[])
{
	signal(SIGINT, signal_callback_handler);
	signal(SIGPIPE, SIG_IGN);

	int opt = getopt(argc, argv, "dhl:p:r:t:n:s:");
	while (opt != -1) {
		switch (opt) {
			case 'd':
				r_daemon = true;
				break;
			case 'h':
				summary = true;
				break;
			case 'l':
				l_file.assign(optarg);
				logging = true;
				break;
			case 'p':
				port.assign(optarg);
				break;
			case 'r':
				rootdir.assign(optarg);
				break;
			case 's':
				scheduling.assign(optarg);
				break;
			case 't':
				r_time = toPositiveInt(optarg, r_time);
				break;
			case 'n':
				threadnum = toPositiveInt(optarg, threadnum);
				break;
			default:
				return 1;
		}

		opt = getopt(argc, argv, "dhl:p:r:t:n:s:");
	}

	if (summary) {
		printh();
		return 0;
	}

	std::transform(scheduling.begin(), scheduling.end(), scheduling.begin(),
	               [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
	if (scheduling != "FCFS" && scheduling != "SJF") {
		std::cerr << "Unknown scheduling policy: " << scheduling << ". Use FCFS or SJF.\n";
		return 1;
	}

	if (r_daemon) {
		run->accept_connection();
		return 0;
	}

	threadnum = std::min(threadnum, 30);

	std::thread scheduler(&Parse::popRequest, P);
	scheduler.detach();

	for (int i = 0; i < threadnum; ++i) {
		std::thread worker(&Parse::serveRequest, P);
		worker.detach();
	}

	run->accept_connection();

	return 0;
}
