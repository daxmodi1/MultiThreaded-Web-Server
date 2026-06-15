#ifndef PARSE_H_
#define PARSE_H_
#include <cstdint>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>

//enum RequestMethod {GET,HEAD,OTHER};

//typedef struct clientInfo;
//typedef struct clientIdentity;
struct clientIdentity
{
	int acceptId = -1;
	std::string ip;
	uint16_t portno = 0;
	std::string requesttime;
};

struct clientInfo
{
	std::string r_method;
	std::string r_type;
	std::string r_version;
	std::string r_firstline;
	std::string r_filename;
	std::string r_time;
	std::string r_servetime;
	int r_acceptid = -1;
	std::string r_ip;
	uint16_t r_portno = 0;
	int r_filesize = 0;
	bool status_file = false;
	std::string r_ctype;
	bool rootcheck = true;
	int status_code = 0;
};

class Parse
{
public:
		std::list<clientInfo> clientlist;
		std::list<clientInfo> requestlist;

		bool fileExists(const std::string& filename);
		void parseRequest(clientIdentity);
		void readyQueue(clientInfo );
		void checkRequest(clientInfo);
		void popRequest();
		void serveRequest();
		void changeFolder(clientInfo);
};
#endif /* PARSE_H_ */
