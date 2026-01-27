#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(const std::string& name)
: _name(name), _topic(""),
  _inviteOnly(false), _topicOpOnly(false),
  _key(""), _hasKey(false),
  _userLimit(0), _hasLimit(false)
{}

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
		(*it)->_sendBuffer.append(message);
}

void	Channel::broadcastExcept(const std::string &message, Client* except)
{
	for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (*it == except)
			continue;
		(*it)->_sendBuffer.append(message);
	}
}

bool	Channel::hasClient(Client* client)
{
	return (std::find(_clients.begin(), _clients.end(), client) != _clients.end());
}

const std::string&	Channel::getTopic() const
{
	return _topic;
}

void	Channel::setTopic(const std::string& t)
{
	_topic = t;
}

bool	Channel::isOperator(Client* c) const
{
	return _operators.find(c) != _operators.end();
}

void	Channel::addOperator(Client* c)
{
	_operators.insert(c);
}

void	Channel::removeOperator(Client* c)
{
	_operators.erase(c);
}

bool	Channel::inviteOnly() const
{
	return _inviteOnly;
}

void	Channel::setInviteOnly(bool b)
{
	_inviteOnly = b;
}

bool	Channel::topicOpOnly() const
{
	return _topicOpOnly;
}

void	Channel::setTopicOpOnly(bool b)
{
	_topicOpOnly = b;
}

bool	Channel::hasKey() const
{
	return _hasKey;
}

const std::string&	Channel::getKey() const
{
	return _key;
}

void	Channel::setKey(const std::string& k)
{
	_hasKey = true;
	_key = k;
}

void	Channel::clearKey()
{
	_hasKey = false;
	_key = "";
}

bool	Channel::hasLimit() const
{
	return _hasLimit;
}

size_t	Channel::getLimit() const
{
	return _userLimit;
}

void	Channel::setLimit(size_t l)
{
	_hasLimit = true;
	_userLimit = l;
}

void	Channel::clearLimit()
{
	_hasLimit = false;
	_userLimit = 0;
}

void	Channel::inviteNick(const std::string& nick)
{
	_invitedNicks.insert(nick);
}

bool	Channel::isInvited(const std::string& nick) const
{
	return _invitedNicks.find(nick) != _invitedNicks.end();
}

void	Channel::consumeInvite(const std::string& nick)
{
	_invitedNicks.erase(nick);
}
