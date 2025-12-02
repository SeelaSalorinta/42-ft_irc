#include "Server.hpp"

#include <iostream>
#include <cstring>
#include <stdexcept>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

namespace
{
	struct ScopedFd
	{
		int fd;
		explicit ScopedFd(int f) : fd(f) {}
		~ScopedFd() { if (fd != -1) close(fd); }
		void release() { fd = -1; }
	};
}

Server::Server(int port)
	: _port(port), _listenFd(-1), _running(false)
{
}

Server::~Server()
{
	if (_listenFd != -1)
		close(_listenFd);
}

bool Server::setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return false;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

void Server::setupListeningSocket()
{
	_listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenFd < 0)
		throw std::runtime_error("socket failed");

	int opt = 1;
	if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("setsockopt failed");

	if (!setNonBlocking(_listenFd))
		throw std::runtime_error("failed to set non-blocking");

	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(static_cast<uint16_t>(_port));

	if (bind(_listenFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
		throw std::runtime_error("bind failed");
	if (listen(_listenFd, SOMAXCONN) < 0)
		throw std::runtime_error("listen failed");

	pollfd pfd;
	pfd.fd = _listenFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pollFds.push_back(pfd);

	std::cout << "Listening on port " << _port << std::endl;
}

void Server::start()
{
	setupListeningSocket();
	_running = true;
	eventLoop();
}

void Server::eventLoop()
{
	while (_running)
	{
		int ready = poll(&_pollFds[0], _pollFds.size(), -1);
		if (ready < 0)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("poll failed");
		}

		// Accept new clients if listener is ready
		if (_pollFds[0].revents & POLLIN)
			acceptNewClients();

		for (std::size_t i = 1; i < _pollFds.size(); ++i)
		{
			if (_pollFds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
			{
				dropClient(_pollFds[i].fd, "disconnect");
				continue;
			}
			if (_pollFds[i].revents & POLLIN)
				handleClient(i);
		}
	}
}

void Server::acceptNewClients()
{
	while (true)
	{
		int clientFd = accept(_listenFd, 0, 0);
		if (clientFd < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			continue;
		}
		if (!setNonBlocking(clientFd))
		{
			close(clientFd);
			continue;
		}

		pollfd pfd;
		pfd.fd = clientFd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		_pollFds.push_back(pfd);

		std::cout << "Client connected fd=" << clientFd << std::endl;
	}
}

void Server::handleClient(std::size_t index)
{
	int fd = _pollFds[index].fd;
	char buf[1024];

	ssize_t n = recv(fd, buf, sizeof(buf), 0);
	if (n <= 0)
	{
		if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
			dropClient(fd, "recv error");
		return;
	}

	ssize_t sent = send(fd, buf, static_cast<std::size_t>(n), 0);
	if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
	{
		dropClient(fd, "send error");
	}
}

void Server::dropClient(int fd, const char *reason)
{
	std::cout << "Closing fd=" << fd << " (" << reason << ")" << std::endl;
	close(fd);
	for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
	{
		if (it->fd == fd)
		{
			_pollFds.erase(it);
			break;
		}
	}
}
