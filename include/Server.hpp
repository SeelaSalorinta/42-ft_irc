#pragma once

#include <vector>
#include <string>
#include <poll.h>
#include <map>

class Client;
class Channel;

class Server
{
	private:
		Server(const Server &);
		Server &operator=(const Server &);

		int _port;
		std::string _password;
		int _listenFd;
		bool _running;

		std::vector<struct pollfd> _pollFds;

		void setupListeningSocket();
		void eventLoop();
		void acceptNewClients();
		void handleClient(std::size_t index);
		void dropClient(int fd, const char *reason);

		static bool setNonBlocking(int fd);

		std::map<std::string, Channel*> _channels;
		std::map<int, Client*> _clients; //servers clientS

		public:
			explicit Server(int port, const std::string &password);
			~Server();

			void start();
			const std::string&	getPassword() const;
			Channel*	getOrCreateChannel(const std::string &name);
			Channel*	getChannel(const std::string &name);
};