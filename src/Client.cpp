#include "Client.hpp"
#include "Channel.hpp"
#include "Server.hpp"

Client::Client(int fd, Server* server)
	: _fd(fd), _server(server), _hasPass(false), _hasNick(false),
	_hasUser(false), _isRegistered(false)
{}

const std::vector<Channel*>&	Client::getJoinedChannels() const
{
	return _joinedChannels;
}

void	Client::joinChannel(Channel* channel)
{
	if (std::find(_joinedChannels.begin(), _joinedChannels.end(), channel) == _joinedChannels.end())
		_joinedChannels.push_back(channel);
}

void	Client::leaveChannel(Channel* channel)
{
	channel->removeClient(this);
	channel->removeOperator(this);

	_joinedChannels.erase(
	std::remove(_joinedChannels.begin(), _joinedChannels.end(), channel),
	_joinedChannels.end());
	if (_server)
		_server->destroyChannelIfEmpty(channel);
	
}
