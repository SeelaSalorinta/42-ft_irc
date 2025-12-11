#pragma once 

#include <string>
#include <vector>
#include <sys/socket.h> // send
#include <algorithm>

class Client;

class Channel
{
	private:
		std::string _name;
		std::vector<Client*> _clients;

	public:
		Channel(const std::string &name);
		const std::string&	getName() const;
		const std::vector<Client*>&	getClients() const;

		void	addClient(Client* client);
		void	removeClient(Client* client);

		void	broadcast(const std::string &message);
};
