#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <string>
#include <poll.h>

class Server
{
public:
	explicit Server(int port);
	~Server();

	void start();

private:
	Server(const Server &);
	Server &operator=(const Server &);

	int _port;
	int _listenFd;
	bool _running;
	std::vector<struct pollfd> _pollFds;

	void setupListeningSocket();
	void eventLoop();
	void acceptNewClients();
	void handleClient(std::size_t index);
	void dropClient(int fd, const char *reason);

	static bool setNonBlocking(int fd);
};

#endif
