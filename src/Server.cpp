#include "Server.hpp"
#include "Parser.hpp"
#include "CommandHandler.hpp"
#include "Client.hpp"
#include "Channel.hpp"

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

Server::Server(int port, const std::string &password)
	: _port(port), _password(password), _listenFd(-1), _running(false)
{
}

Server::~Server()
{
	if (_listenFd != -1)
		close(_listenFd);
}

const std::string& Server::getPassword() const
{
	return _password;
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

		 //create new client object
		_clients[clientFd] = new Client(clientFd);

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

	if (_clients.count(fd) == 0) {
		dropClient(fd, "client not found");
		return;
	}
	
	Client* client = _clients[fd];
	//read from socket
	ssize_t n = recv(fd, buf, sizeof(buf), 0);
	if (n <= 0)
	{
		if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
			dropClient(fd, "recv error");
		return;
	}
	// append new data to this client's buffer
	client->_recvBuffer.append(buf, n);
	while (true)
	{
		std::size_t pos = client->_recvBuffer.find("\r\n"); //add r later!! this for netcat
		if (pos == std::string::npos)
			break; // no complete line yet, wait for more data

		// Take the line (without "\r\n")
		std::string line = client->_recvBuffer.substr(0, pos);
		// Remove that line + "\r\n" from the buffer
		client->_recvBuffer.erase(0, pos + 2); //add +2 later !!! this is fr netcat fornow

		if (line.empty())
			continue;

		std::cout << "REcieved line " << line << std::endl;
		Command cmd = parseCommand(line);
		CommandHandler handler(*this, *client);
		handler.handleCommand(cmd); 
	}

	//IN COMMENTS FOR NOW
	/*ssize_t sent = send(fd, buf, static_cast<std::size_t>(n), 0);
	if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
	{
		dropClient(fd, "send error");
	}*/
}

void	Server::dropClient(int fd, const char *reason)
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
	// poista client channeleilta
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it != _clients.end())
	{
		Client* client = it->second;

		const std::vector<Channel*>& chans = client->getJoinedChannels();
		for (std::size_t i = 0; i < chans.size(); ++i)
		{
			chans[i]->removeClient(client);
			client->leaveChannel(chans[i]);
		}

		delete client;
		_clients.erase(it);
	}
}

Channel*	Server::getOrCreateChannel(const std::string &name)
{
	if (_channels.count(name) == 0)
		_channels[name] = new Channel(name);
	return _channels[name];
}

Channel*	Server::getChannel(const std::string &name)
{
	if (_channels.count(name) == 0)
		return NULL;
	return _channels[name];
}
