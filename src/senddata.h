#ifndef SENDDATA_H_
#define SENDDATA_H_

#include <cstddef>
#include <ctime>
#include <string>
#include <sys/types.h>

#include "../src/parse.h"

class SendData
{
public:
	void sendData(clientInfo c);
	void generatingLog(clientInfo c);
	void listingDir(clientInfo c);
	void displaylog(clientInfo c);

private:
	bool sendAll(int socket, const char *data, size_t size);
	bool sendAll(int socket, const std::string& data);
	std::string buildHeader(const clientInfo& c, int statusCode, const std::string& reason, size_t contentLength);
	std::string contentTypeFor(const std::string& filename);
	std::string httpDate(time_t value);
};

#endif /* SENDDATA_H_ */
