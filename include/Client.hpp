#pragma once

#include <string>

class Channel;

class Client
{
	private:
		std::vector<Channel*> _joinedChannels;
	public:
		int	_fd; // socket file descriptor
		std::string	_recvBuffer; // unfinished incoming data
		std::string	_sendBuffer; // outgoing messages

		std::string	_nickname;
		std::string	_username;
		std::string	_realname;

		bool	_hasPass;
		bool	_hasNick;
		bool	_hasUser;
		bool	_isRegistered;


		Client(int fd);

		const std::vector<Channel*>&	getJoinedChannels() const;
		void	joinChannel(Channel* channel);
		void	leaveChannel(Channel* channel);
};