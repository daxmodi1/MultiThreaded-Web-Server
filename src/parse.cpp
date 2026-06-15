#include "../src/parse.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>
#include <mutex>
#include <sstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "../src/myhttpd.h"
#include "../src/senddata.h"

namespace
{
std::string joinPath(const std::string& base, const std::string& relative)
{
	if (relative.empty()) {
		return base;
	}

	if (!base.empty() && base[base.size() - 1] == '/') {
		return base + relative;
	}

	return base + "/" + relative;
}

std::string normalizeRequestedPath(std::string path)
{
	const std::string querySeparator = "?";
	const std::string::size_type query = path.find(querySeparator);
	if (query != std::string::npos) {
		path.erase(query);
	}

	while (!path.empty() && path[0] == '/') {
		path.erase(path.begin());
	}

	std::vector<std::string> parts;
	std::stringstream stream(path);
	std::string part;
	while (std::getline(stream, part, '/')) {
		if (part.empty() || part == ".") {
			continue;
		}
		if (part == "..") {
			return "";
		}
		parts.push_back(part);
	}

	std::string normalized;
	for (size_t i = 0; i < parts.size(); ++i) {
		if (i != 0) {
			normalized += "/";
		}
		normalized += parts[i];
	}

	return normalized;
}

int fileSize(const std::string& filename)
{
	struct stat fileInfo;
	if (stat(filename.c_str(), &fileInfo) == -1 || !S_ISREG(fileInfo.st_mode)) {
		return 0;
	}

	return static_cast<int>(fileInfo.st_size);
}

std::string currentTimeString()
{
	time_t now = time(NULL);
	tm *utc = gmtime(&now);
	char buffer[50] = {};

	if (utc == NULL || strftime(buffer, sizeof(buffer), "%x:%X", utc) == 0) {
		return "";
	}

	return std::string(buffer);
}
}

void Parse::parseRequest(clientIdentity c_id)
{
	std::string request;
	char buffer[1024];

	while (request.size() < 8192) {
		const ssize_t bytesRead = recv(c_id.acceptId, buffer, sizeof(buffer), 0);
		if (bytesRead <= 0) {
			if (bytesRead < 0) {
				perror("Receive");
			}
			close(c_id.acceptId);
			return;
		}

		request.append(buffer, static_cast<size_t>(bytesRead));
		if (request.find("\r\n\r\n") != std::string::npos || request.find('\n') != std::string::npos) {
			break;
		}
	}

	clientInfo cInfo;
	cInfo.r_acceptid = c_id.acceptId;
	cInfo.r_portno = c_id.portno;
	cInfo.r_ip = c_id.ip;
	cInfo.r_time = c_id.requesttime;

	const std::string::size_type lineEnd = request.find_first_of("\r\n");
	if (lineEnd == std::string::npos) {
		cInfo.r_firstline = request;
		cInfo.status_code = 400;
		if (r_daemon) {
			SendData sender;
			sender.sendData(cInfo);
			return;
		}

		readyQueue(cInfo);
		return;
	}

	cInfo.r_firstline = request.substr(0, lineEnd);

	std::istringstream requestLine(cInfo.r_firstline);
	std::string requestedPath;
	requestLine >> cInfo.r_type >> requestedPath >> cInfo.r_method;

	if (cInfo.r_type.empty() || requestedPath.empty() || cInfo.r_method.empty()) {
		cInfo.status_code = 400;
		if (r_daemon) {
			SendData sender;
			sender.sendData(cInfo);
			return;
		}

		readyQueue(cInfo);
		return;
	}

	const std::string relativePath = normalizeRequestedPath(requestedPath);
	cInfo.rootcheck = relativePath.find('/') == std::string::npos;
	cInfo.r_filename = joinPath(rootdir, relativePath);

	changeFolder(cInfo);
}

void Parse::changeFolder(clientInfo c)
{
	const std::string::size_type tilde = c.r_filename.find('~');
	if (tilde != std::string::npos && tilde + 1 < c.r_filename.size()) {
		const std::string::size_type slash = c.r_filename.find('/', tilde);
		if (slash != std::string::npos) {
			const std::string requestFor = c.r_filename.substr(tilde + 1, slash - tilde - 1);
			const std::string rest = c.r_filename.substr(slash);
			c.r_filename.erase(tilde);
			c.r_filename += requestFor + "/myhttpd" + rest;
		}
	}

	checkRequest(c);
}

bool Parse::fileExists(const std::string& filename)
{
	struct stat fileInfo;
	return stat(filename.c_str(), &fileInfo) != -1 && S_ISREG(fileInfo.st_mode);
}

void Parse::checkRequest(clientInfo cInfo)
{
	if (cInfo.status_code == 400) {
		if (r_daemon) {
			SendData sender;
			sender.sendData(cInfo);
			return;
		}

		readyQueue(cInfo);
		return;
	}

	std::string method = cInfo.r_type;
	std::transform(method.begin(), method.end(), method.begin(),
	               [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
	cInfo.r_type = method;

	if (method != "GET" && method != "HEAD") {
		cInfo.status_code = 405;
		if (r_daemon) {
			SendData sender;
			sender.sendData(cInfo);
			return;
		}

		readyQueue(cInfo);
		return;
	}

	cInfo.status_file = fileExists(cInfo.r_filename);
	cInfo.r_filesize = cInfo.status_file ? fileSize(cInfo.r_filename) : 0;

	if (r_daemon) {
		SendData sender;
		sender.sendData(cInfo);
		return;
	}

	readyQueue(cInfo);
}

bool sortRequest(const clientInfo& lhs, const clientInfo& rhs)
{
	return lhs.r_filesize < rhs.r_filesize;
}

void Parse::readyQueue(clientInfo cInfo)
{
	{
		std::lock_guard<std::mutex> lock(rqueue_mutex);
		clientlist.push_back(cInfo);
	}
	rqueue_cond.notify_one();
}

void Parse::popRequest()
{
	std::this_thread::sleep_for(std::chrono::seconds(r_time));

	while (true) {
		clientInfo c;
		std::string selectedScheduling = scheduling;
		std::transform(selectedScheduling.begin(), selectedScheduling.end(), selectedScheduling.begin(),
		               [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });

		{
			std::unique_lock<std::mutex> lock(rqueue_mutex);
			rqueue_cond.wait(lock, [this] { return !clientlist.empty(); });

			if (selectedScheduling == "SJF") {
				clientlist.sort(sortRequest);
			}

			c = clientlist.front();
			clientlist.pop_front();
		}

		{
			std::lock_guard<std::mutex> lock(request_mutex);
			requestlist.push_back(c);
		}
		request_cond.notify_one();
	}
}

void Parse::serveRequest()
{
	while (true) {
		clientInfo c;
		{
			std::unique_lock<std::mutex> lock(request_mutex);
			request_cond.wait(lock, [this] { return !requestlist.empty(); });
			c = requestlist.front();
			requestlist.pop_front();
		}
		c.r_servetime = currentTimeString();

		SendData sender;
		sender.sendData(c);
	}
}
