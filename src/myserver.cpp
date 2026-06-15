#include "../src/myserver.h"

#include <cerrno>
#include <cstring>
#include <ctime>
#include <iostream>
#include <unistd.h>

#include "../src/myhttpd.h"
#include "../src/parse.h"
#include "../src/socket_handle.h"

RunServer::RunServer()
{
	memset(&inValue, 0, sizeof(inValue));
	inValue.ai_family = AF_UNSPEC;
	inValue.ai_socktype = SOCK_STREAM;
	inValue.ai_flags = AI_PASSIVE;
	yes = 1;
}

uint16_t RunServer::get_port_number(struct sockaddr *s)
{
	if (s->sa_family == AF_INET) {
		return ntohs(((struct sockaddr_in *)s)->sin_port);
	}

	return ntohs(((struct sockaddr_in6 *)s)->sin6_port);
}

void *RunServer::get_ip_address(sockaddr *s)
{
	if (s->sa_family == AF_INET) {
		return &((sockaddr_in *)s)->sin_addr;
	}

	return &((sockaddr_in6 *)s)->sin6_addr;
}

void RunServer::accept_connection()
{
	SocketHandle listener;
	const int addressStatus = getaddrinfo(NULL, port.c_str(), &inValue, &serverInfo);
	if (addressStatus != 0) {
		std::cerr << "getaddrinfo: " << gai_strerror(addressStatus) << std::endl;
		return;
	}

	for (validInfo = serverInfo; validInfo != NULL; validInfo = validInfo->ai_next) {
		listener.reset(socket(validInfo->ai_family, validInfo->ai_socktype, validInfo->ai_protocol));
		sockId = listener.get();
		if (!listener.valid()) {
			perror("socket");
			continue;
		}

		if (setsockopt(listener.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
			perror("setsockopt");
			listener.close();
			sockId = listener.get();
			continue;
		}

		if (bind(listener.get(), validInfo->ai_addr, validInfo->ai_addrlen) == -1) {
			perror("bind");
			listener.close();
			sockId = listener.get();
			continue;
		}

		break;
	}

	if (validInfo == NULL || !listener.valid()) {
		std::cerr << "Unable to bind to port " << port << std::endl;
		freeaddrinfo(serverInfo);
		return;
	}

	if (listen(listener.get(), MAXCONNECTION) == -1) {
		perror("listen");
		freeaddrinfo(serverInfo);
		listener.close();
		sockId = listener.get();
		return;
	}

	sockId = listener.get();
	freeaddrinfo(serverInfo);

	while (true) {
		address = sizeof(clientAddr);
		acceptId = accept(listener.get(), reinterpret_cast<struct sockaddr *>(&clientAddr),
		                  &address);
		if (acceptId == -1) {
			if (errno == EINTR) {
				continue;
			}
			perror("accept");
			continue;
		}

		inet_ntop(clientAddr.ss_family, get_ip_address(reinterpret_cast<struct sockaddr *>(&clientAddr)),
		          ip1, sizeof(ip1));

		time_t now = time(NULL);
		tm *utc = gmtime(&now);
		char currentTime[50] = {};
		if (utc != NULL && strftime(currentTime, sizeof(currentTime), "%x:%X", utc) == 0) {
			perror("strftime");
		}

		clientIdentity cid;
		cid.acceptId = acceptId;
		cid.ip = ip1;
		cid.portno = get_port_number(reinterpret_cast<struct sockaddr *>(&clientAddr));
		cid.requesttime = currentTime;

		P->parseRequest(cid);
	}
}
