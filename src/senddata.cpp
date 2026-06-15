#include "../src/senddata.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <vector>

#include "../src/myhttpd.h"
#include "../src/socket_handle.h"

namespace
{
bool sortDirectory(const std::string& left, const std::string& right)
{
	return std::lexicographical_compare(
	    left.begin(), left.end(), right.begin(), right.end(),
	    [](unsigned char a, unsigned char b) { return std::tolower(a) < std::tolower(b); });
}

std::string reasonFor(int statusCode)
{
	switch (statusCode) {
		case 200:
			return "OK";
		case 400:
			return "Bad Request";
		case 404:
			return "Not Found";
		case 405:
			return "Method Not Allowed";
		default:
			return "Internal Server Error";
	}
}
}

bool SendData::sendAll(int socket, const char *data, size_t size)
{
	size_t sent = 0;
	while (sent < size) {
		const ssize_t bytesSent = send(socket, data + sent, size - sent, 0);
		if (bytesSent <= 0) {
			if (errno != EPIPE && errno != ECONNRESET) {
				perror("send");
			}
			return false;
		}
		sent += static_cast<size_t>(bytesSent);
	}

	return true;
}

bool SendData::sendAll(int socket, const std::string& data)
{
	return sendAll(socket, data.c_str(), data.size());
}

std::string SendData::httpDate(time_t value)
{
	tm *utc = gmtime(&value);
	char buffer[64] = {};

	if (utc == NULL || strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", utc) == 0) {
		return "";
	}

	return std::string(buffer);
}

std::string SendData::contentTypeFor(const std::string& filename)
{
	const std::string::size_type dot = filename.find_last_of('.');
	if (dot == std::string::npos) {
		return "application/octet-stream";
	}

	std::string extension = filename.substr(dot + 1);
	std::transform(extension.begin(), extension.end(), extension.begin(),
	               [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

	if (extension == "html" || extension == "htm") {
		return "text/html";
	}
	if (extension == "txt" || extension == "text") {
		return "text/plain";
	}
	if (extension == "css") {
		return "text/css";
	}
	if (extension == "js") {
		return "application/javascript";
	}
	if (extension == "jpeg" || extension == "jpg") {
		return "image/jpeg";
	}
	if (extension == "gif") {
		return "image/gif";
	}
	if (extension == "png") {
		return "image/png";
	}

	return "application/octet-stream";
}

std::string SendData::buildHeader(const clientInfo& c, int statusCode, const std::string& reason, size_t contentLength)
{
	std::ostringstream header;
	header << "HTTP/1.1 " << statusCode << " " << reason << "\r\n";
	header << "Date: " << httpDate(time(NULL)) << "\r\n";
	header << "Server: myhttpd/1.0\r\n";

	struct stat fileInfo;
	if (c.status_file && stat(c.r_filename.c_str(), &fileInfo) != -1) {
		header << "Last-Modified: " << httpDate(fileInfo.st_mtime) << "\r\n";
	}

	header << "Content-Type: " << contentTypeFor(c.r_filename) << "\r\n";
	header << "Content-Length: " << contentLength << "\r\n";
	header << "Connection: close\r\n\r\n";

	return header.str();
}

void SendData::sendData(clientInfo c)
{
	SocketHandle client(c.r_acceptid);

	if (c.status_code == 400 || c.status_code == 405) {
		const int status = c.status_code;
		const std::string body = reasonFor(status) + "\n";
		sendAll(client.get(), buildHeader(c, status, reasonFor(status), body.size()));
		if (c.r_type != "HEAD") {
			sendAll(client.get(), body);
		}

		if (logging) {
			generatingLog(c);
		}

		if (logging || r_daemon) {
			displaylog(c);
		}
		return;
	}

	if (c.status_file && (c.r_type == "GET" || c.r_type == "HEAD")) {
		c.status_code = 200;
		sendAll(client.get(), buildHeader(c, 200, "OK", static_cast<size_t>(c.r_filesize)));

		if (c.r_type == "GET") {
			std::ifstream file(c.r_filename.c_str(), std::ios::binary);
			std::vector<char> buffer(8192);

			while (file) {
				file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
				const std::streamsize bytesRead = file.gcount();
				if (bytesRead > 0 && !sendAll(client.get(), buffer.data(), static_cast<size_t>(bytesRead))) {
					break;
				}
			}
		}
	} else {
		c.status_code = 404;
		listingDir(c);
	}

	if (logging) {
		generatingLog(c);
	}

	if (logging || r_daemon) {
		displaylog(c);
	}
}

void SendData::generatingLog(clientInfo c)
{
	std::ofstream logfile(l_file.c_str(), std::ios::app);
	if (!logfile) {
		std::cout << "Error: log file could not be opened" << std::endl;
		return;
	}

	logfile << c.r_ip << "  [" << c.r_time << "]  [" << c.r_servetime << "]  "
	        << c.r_firstline << " " << c.status_code << " " << c.r_filesize << std::endl;
}

void SendData::displaylog(clientInfo c)
{
	time_t now = time(NULL);
	tm *utc = gmtime(&now);
	char currentTime[50] = {};

	if (utc != NULL && strftime(currentTime, sizeof(currentTime), "%x:%X", utc) != 0) {
		c.r_servetime = currentTime;
	}

	std::cout << c.r_ip << "  [" << c.r_time << "]  [" << c.r_servetime << "]  "
	          << c.r_firstline << " " << c.status_code << " " << c.r_filesize << std::endl;
}

void SendData::listingDir(clientInfo c)
{
	const std::string::size_type lastSlash = c.r_filename.find_last_of('/');
	const std::string dir = lastSlash == std::string::npos ? rootdir : c.r_filename.substr(0, lastSlash);
	std::vector<std::string> entries;

	DIR *directory = opendir(dir.c_str());
	if (directory != NULL) {
		while (dirent *entry = readdir(directory)) {
			entries.push_back(entry->d_name);
		}
		closedir(directory);
	}

	std::sort(entries.begin(), entries.end(), sortDirectory);

	std::ostringstream body;
	if (entries.empty()) {
		body << "404 Not Found\n";
	} else {
		body << "Files Listing:\n";
		for (std::vector<std::string>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
			body << *it << "\n";
		}
	}

	const std::string responseBody = body.str();
	sendAll(c.r_acceptid, buildHeader(c, 404, "Not Found", responseBody.size()));
	if (c.r_type != "HEAD") {
		sendAll(c.r_acceptid, responseBody);
	}
}
