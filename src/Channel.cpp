#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(const std::string &name) : _name(name) {}

const std::string&	Channel::getName() const
{
	return _name;
}

const std::vector<Client*>&	Channel::getClients() const
{
	return _clients;
}

void	Channel::addClient(Client* client)
{
	if (std::find(_clients.begin(), _clients.end(), client) == _clients.end())
		_clients.push_back(client);
}

void	Channel::removeClient(Client* client)
{
	_clients.erase(std::remove(_clients.begin(), _clients.end(), client), _clients.end());
}

void	Channel::broadcast(const std::string &message)
{
	for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		send((*it)->_fd, message.c_str(), message.size(), 0);
}