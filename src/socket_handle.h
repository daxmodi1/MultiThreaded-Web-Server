#ifndef SOCKET_HANDLE_H_
#define SOCKET_HANDLE_H_

#include <sys/socket.h>
#include <unistd.h>

class SocketHandle
{
public:
	SocketHandle() : descriptor(-1) {}
	explicit SocketHandle(int fd) : descriptor(fd) {}

	SocketHandle(const SocketHandle&) = delete;
	SocketHandle& operator=(const SocketHandle&) = delete;

	SocketHandle(SocketHandle&& other) : descriptor(other.descriptor)
	{
		other.descriptor = -1;
	}

	SocketHandle& operator=(SocketHandle&& other)
	{
		if (this != &other) {
			close();
			descriptor = other.descriptor;
			other.descriptor = -1;
		}

		return *this;
	}

	~SocketHandle()
	{
		close();
	}

	bool valid() const
	{
		return descriptor != -1;
	}

	int get() const
	{
		return descriptor;
	}

	int release()
	{
		const int released = descriptor;
		descriptor = -1;
		return released;
	}

	void reset(int fd = -1)
	{
		if (descriptor != fd) {
			close();
			descriptor = fd;
		}
	}

	void close()
	{
		if (descriptor != -1) {
			::close(descriptor);
			descriptor = -1;
		}
	}

private:
	int descriptor;
};

#endif /* SOCKET_HANDLE_H_ */
