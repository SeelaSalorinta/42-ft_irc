#include "CommandHandler.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Parser.hpp"
#include "Replies.hpp"
#include "Channel.hpp"

// Forward declaration for helper used across handlers
static std::string	makePrefix(const Client& c);

CommandHandler::CommandHandler(Server &server, Client &client)
	: _server(server), _client(client) {}

void	CommandHandler::handleCommand(const Command &cmd)
{
	if (cmd.name == "CAP")
		return;

	if (cmd.name == "PASS")
		return handlePASS(cmd);
	if (!_client._hasPass)
	{
		sendReply(_client, "ERROR", "You must send PASS before other commands");
		return;
	}
	if (!_client._isRegistered
		&& cmd.name != "NICK"
		&& cmd.name != "USER"
		&& cmd.name != "QUIT"
		&& cmd.name != "PING")
	{
		// ERR_NOTREGISTERED
		sendReply(_client, "451", "You have not registered");
		return;
	}

	//THINK BETTER WAY TO HANDLE
	struct Entry { const char* name; void (CommandHandler::*fn)(const Command&); };
	static const Entry table[] = {
		{"NICK", &CommandHandler::handleNICK},
		{"USER", &CommandHandler::handleUSER},
		{"JOIN", &CommandHandler::handleJOIN},
		{"PART", &CommandHandler::handlePART},
		{"PING", &CommandHandler::handlePING},
		{"INVITE", &CommandHandler::handleINVITE},
		{"KICK", &CommandHandler::handleKICK},
		{"TOPIC", &CommandHandler::handleTOPIC},
		{"PRIVMSG", &CommandHandler::handlePRIVMSG},
		{"NOTICE", &CommandHandler::handleNOTICE},
		{"MODE", &CommandHandler::handleMODE},
		{"QUIT", &CommandHandler::handleQUIT},
	};

	for (std::size_t i = 0; i < sizeof(table) / sizeof(table[0]); ++i)
	{
		if (cmd.name == table[i].name)
			return (this->*(table[i].fn))(cmd);
	}

	// fallback for unknown commands
	sendReply(_client, "ERROR", "Unknown command");
}

void CommandHandler::handleQUIT(const Command& cmd)
{
	std::string reason = "Client Quit";
	if (!cmd.params.empty())
		reason = cmd.params[0];

	std::vector<Channel*> chans = _client.getJoinedChannels();
	std::string msg = makePrefix(_client) + "QUIT :" + reason + "\r\n";

	for (std::size_t i = 0; i < chans.size(); ++i)
	{
		Channel* ch = chans[i];
		if (ch && ch->hasClient(&_client))
			ch->broadcastExcept(msg, &_client);
		_client.leaveChannel(ch);
	}

	_server.disconnectClient(_client._fd, reason);
}


std::string	makePrefix(const Client& c)
{
	return ":" + c._nickname + "!" + c._username + "@localhost ";
}


void CommandHandler::handleMODE(const Command& cmd)
{
	if (cmd.params.size() < 1)
		return sendERR_NEEDMOREPARAMS(_client, "MODE");

	const std::string& channelName = cmd.params[0];
	Channel* channel = _server.getChannel(channelName);
	if (!channel)
		return sendERR_NOSUCHCHANNEL(_client, channelName);

	//is necessary that you are on the channel or no?
	if (!channel->hasClient(&_client))
		return sendERR_CLIENTNOTINCHANNEL(_client, channelName);

	// MODE #chan  -> query current modes
	if (cmd.params.size() == 1)
	{
		std::string modes = "+";
		std::string args;

		if (channel->inviteOnly())
			modes += "i";
		if (channel->topicOpOnly())
			modes += "t";
		if (channel->hasKey())
			modes += "k";
		if (channel->hasLimit())
			modes += "l";

		if (channel->hasKey())
			args += " " + channel->getKey();
		if (channel->hasLimit())
		{
			std::ostringstream oss;
			oss << channel->getLimit();
			args += " " + oss.str();
		}

		//debug print
		std::cout
	<< "[MODE QUERY] nick=" << _client._nickname
	<< " channel=" << channelName
	<< " modes=" << modes
	<< " args=" << args
	<< std::endl;

		return sendRPL_CHANNELMODEIS(_client, channelName, modes, args);
	}

	// changing modes needs an operator
	if (!channel->isOperator(&_client))
	{
		//debug print delete laterr
		std::cout << "[MODE] denied: not operator nick=" << _client._nickname << " chan=" << channelName << "\n";
		return sendERR_CHANOPRIVSNEEDED(_client, channelName);
	}

	const std::string& modeStr = cmd.params[1];

	bool adding = true;
	std::size_t paramIndex = 2; // next params used by modes needing args

	std::string appliedModes; // for broadcasting
	std::string appliedArgs;

	for (std::size_t i = 0; i < modeStr.size(); ++i)
	{
		char m = modeStr[i];

		if (m == '+') { adding = true; continue; }
		if (m == '-') { adding = false; continue; }

		// i: invite-only (no param)
		if (m == 'i')
		{
			channel->setInviteOnly(adding);
			appliedModes += (adding ? "+i" : "-i");
		}
		// t: topic op-only (no param)
		else if (m == 't')
		{
			channel->setTopicOpOnly(adding);
			appliedModes += (adding ? "+t" : "-t");
		}
		// k: key (param when adding, no param when removing)
		else if (m == 'k')
		{
			if (adding)
			{
				if (paramIndex >= cmd.params.size())
					return sendERR_NEEDMOREPARAMS(_client, "MODE");
				const std::string& key = cmd.params[paramIndex++];
				channel->setKey(key);
				appliedModes += "+k";
				appliedArgs += " " + key;
			}
			else
			{
				channel->clearKey();
				appliedModes += "-k";
			}
		}
		// l: limit (param when adding, no param when removing)
		else if (m == 'l')
		{
			if (adding)
			{
				if (paramIndex >= cmd.params.size())
					return sendERR_NEEDMOREPARAMS(_client, "MODE");

				const std::string& limStr = cmd.params[paramIndex++];
				std::istringstream iss(limStr);
				long lim = 0;
				iss >> lim;
				if (lim <= 0)
					return; //should throw error or not?

				channel->setLimit(static_cast<size_t>(lim));
				appliedModes += "+l";
				appliedArgs += " " + limStr;
			}
			else
			{
				channel->clearLimit();
				appliedModes += "-l";
			}
		}
		// o: operator (needs nick param)
		else if (m == 'o')
		{
			if (paramIndex >= cmd.params.size())
				return sendERR_NEEDMOREPARAMS(_client, "MODE");

			const std::string& nick = cmd.params[paramIndex++];
			Client* target = _server.getClientByNick(nick);
			if (!target)
				return sendERR_NOSUCHNICK(_client, nick);
			if (!channel->hasClient(target))
				return sendERR_USERNOTINCHANNEL(_client, nick, channelName);

			if (adding)
			{
				channel->addOperator(target);
				appliedModes += "+o";
			}
			else
			{
				channel->removeOperator(target);
				appliedModes += "-o";
			}
			appliedArgs += " " + nick;
		}
		else
		{
			return sendERR_UNKNOWNMODE(_client, m, channelName);
		}
	}

	if (appliedModes.empty())
		return;

	// Broadcast to channel: :nick!user@localhost MODE #chan <appliedModes> <args>
	std::string msg = makePrefix(_client) + "MODE " + channelName + " " + appliedModes + appliedArgs + "\r\n";
	channel->broadcast(msg);
}

void CommandHandler::handleNOTICE(const Command& cmd)
{
	if (cmd.params.size() < 2)
		return; // no errors for NOTICE

	const std::string& target = cmd.params[0];
	const std::string& message = cmd.params[1];
	if (target.empty())
		return;

	if (target[0] == '#')
	{
		Channel* channel = _server.getChannel(target);
		if (!channel)
			return;
		if (!channel->hasClient(&_client))
			return;

		std::string msg = makePrefix(_client) + "NOTICE " + target + " :" + message + "\r\n";
		channel->broadcastExcept(msg, &_client);
		return;
	}

	Client* dest = _server.getClientByNick(target);
	if (!dest) return;

	std::string msg = makePrefix(_client) + "NOTICE " + target + " :" + message + "\r\n";
	_server.queueMessage(dest, msg);
}

void CommandHandler::handlePRIVMSG(const Command &cmd)
{
	if (cmd.params.size() < 1)
		return sendERR_NORECIPIENT(_client, "PRIVMSG");
	if (cmd.params.size() < 2)
		return sendERR_NOTEXTTOSEND(_client);

	const std::string& target = cmd.params[0];
	const std::string& message = cmd.params[1];

	if (target.empty())
		return sendERR_NORECIPIENT(_client, "PRIVMSG");

	if (target[0] == '#')
	{
		Channel* channel = _server.getChannel(target);
		if (!channel)
			return sendERR_NOSUCHCHANNEL(_client, target);
		if (!channel->hasClient(&_client))
			return sendERR_CANNOTSENDTOCHAN(_client, target); // tai 442, mutta 404 on “send” kontekstissa parempi

		std::string msg = makePrefix(_client) + "PRIVMSG " + target + " :" + message + "\r\n";
		channel->broadcastExcept(msg, &_client);
		return;
	}

	Client* dest = _server.getClientByNick(target);
	if (!dest)
		return sendERR_NOSUCHNICK(_client, target);

	std::string msg = makePrefix(_client) + "PRIVMSG " + target + " :" + message + "\r\n";
	_server.queueMessage(dest, msg);
}


void	CommandHandler::handlePART(const Command &cmd)
{
	if (cmd.params.size() != 1)
	{
		sendERR_NEEDMOREPARAMS(_client, "PART");
		return;
	}
	std::string channelName = cmd.params[0];
	if (channelName.empty() || channelName[0] != '#')
	{
		sendERR_NOSUCHCHANNEL(_client, channelName);
		return;
	}

	Channel* channel = _server.getChannel(channelName);
	if (!channel)
	{
		sendERR_NOSUCHCHANNEL(_client, channelName);
		return;
	}
	if (!channel->hasClient(&_client))
		return sendERR_CLIENTNOTINCHANNEL(_client, channelName);
	std::string msg = makePrefix(_client) + "PART " + channelName + "\r\n";
	channel->broadcast(msg);
	std::cout << "[PART] " << _client._nickname << " part " << channelName << "\n";
		//poista client kyseiseltä kanavalta
	_client.leaveChannel(channel);

}

void	CommandHandler::handlePASS(const Command &cmd)
{
	if (cmd.params.size() != 1)
	{
		sendERR_NEEDMOREPARAMS(_client, "PASS");
		return;
	}
	if (_client._isRegistered || _client._hasNick || _client._hasUser)
	{
		sendERR_ALREADYREGISTERED(_client);
		return;
	}
	if (cmd.params[0] != _server.getPassword())
	{
		sendERR_PASSWDMISMATCH(_client);
		return;
	}
	_client._hasPass = true;
	std::cout << "[PASS] accepted for fd " << _client._fd << std::endl;
	tryRegister();
}

void	CommandHandler::handleNICK(const Command &cmd)
{
	if (cmd.params.size() != 1)
	{
		sendERR_NEEDMOREPARAMS(_client, "NICK");
		return;
	}
	const std::string& newNick = cmd.params[0];
	Client* existing = _server.getClientByNick(newNick);
	if (existing && existing != &_client)
	{
		std::string cur = _client._hasNick ? _client._nickname : "*";
		return sendReply(_client, "433", cur + " " + newNick + " :Nickname is already in use");
	}

	_client._nickname = newNick;
	_client._hasNick = true;
	std::cout << "[NICK] set to \"" << _client._nickname << "\" for fd " << _client._fd << std::endl;
	tryRegister();
}

void	CommandHandler::handleUSER(const Command &cmd)
{
	if (cmd.params.size() != 4)
	{
		sendERR_NEEDMOREPARAMS(_client, "USER");
		return;
	}
	if (cmd.params[1] != "0" || cmd.params[2] != "*")
		return;
	_client._username = cmd.params[0];
	_client._realname = cmd.params[3];
	_client._hasUser = true;
	std::cout << "[USER] set to username \"" << _client._username
			  << "\", realname \"" << _client._realname
			  << "\" for fd " << _client._fd << std::endl;
	tryRegister();
}

void	CommandHandler::handleJOIN(const Command &cmd)
{
	if (cmd.params.empty())
	{
		sendERR_NEEDMOREPARAMS(_client, "JOIN");
		return;
	}
	std::string channelName = cmd.params[0];
	Channel	*channel = _server.getOrCreateChannel(channelName);
	if (channel->hasClient(&_client))
		return;
	bool firstJoiner = channel->getClients().empty();

	if (channel->inviteOnly() && !channel->isInvited(_client._nickname))
		return sendERR_INVITEONLYCHAN(_client, channelName);

	if (channel->hasLimit() && channel->getClients().size() >= channel->getLimit())
		return sendERR_CHANNELISFULL(_client, channelName);

	std::string key = (cmd.params.size() >= 2) ? cmd.params[1] : "";
	if (channel->hasKey() && key != channel->getKey())
		return sendERR_BADCHANNELKEY(_client, channelName);

	channel->addClient(&_client);
	_client.joinChannel(channel);
	if (channel->inviteOnly())
		channel->consumeInvite(_client._nickname);	
	if (firstJoiner) //first joiner gets operaator rights?
		channel->addOperator(&_client);
	std::string msg = makePrefix(_client) + "JOIN " + channelName + "\r\n";
	channel->broadcast(msg);	
	std::cout << "[JOIN] " << _client._nickname << " joined " << channelName << "\n";
}

void CommandHandler::handleTOPIC(const Command& cmd)
{
	if (cmd.params.empty())
		return sendERR_NEEDMOREPARAMS(_client, "TOPIC");

	const std::string& channelName = cmd.params[0];
	Channel* channel = _server.getChannel(channelName);
	if (!channel)
		return sendERR_NOSUCHCHANNEL(_client, channelName);
	if (!channel->hasClient(&_client))
		return sendERR_CLIENTNOTINCHANNEL(_client, channelName);

	// View topic
	if (cmd.params.size() == 1)
	{
		if (channel->getTopic().empty())
			return sendReply(_client, "331", channelName + " :No topic is set"); // RPL_NOTOPIC
		return sendReply(_client, "332", channelName + " :" + channel->getTopic()); // RPL_TOPIC
	}

	// Change topic requires operator if +t
	if (channel->topicOpOnly() && !channel->isOperator(&_client))
		return sendERR_CHANOPRIVSNEEDED(_client, channelName);

	const std::string& topic = cmd.params[1];
	channel->setTopic(topic);

	std::string msg = makePrefix(_client) + "TOPIC " + channelName + " :" + topic + "\r\n";
	channel->broadcast(msg);
}

void CommandHandler::handleINVITE(const Command& cmd)
{
	if (cmd.params.size() < 2)
		return sendERR_NEEDMOREPARAMS(_client, "INVITE");

	const std::string& nick = cmd.params[0];
	const std::string& channelName = cmd.params[1];

	Channel* channel = _server.getChannel(channelName);
	if (!channel)
		return sendERR_NOSUCHCHANNEL(_client, channelName);
	if (!channel->hasClient(&_client))
		return sendERR_CLIENTNOTINCHANNEL(_client, channelName);
	if (!channel->isOperator(&_client))
		return sendERR_CHANOPRIVSNEEDED(_client, channelName);

	Client* target = _server.getClientByNick(nick);
	if (!target)
		return sendERR_NOSUCHNICK(_client, nick);
	if (channel->hasClient(target))
		return sendReply(_client, "443", nick + " " + channelName + " :is already on channel"); // ERR_USERONCHANNEL

	channel->inviteNick(nick);
	std::string inviteMsg = makePrefix(_client) + "INVITE " + nick + " " + channelName + "\r\n";
	_server.queueMessage(target, inviteMsg);
	sendReply(_client, "341", nick + " " + channelName); // RPL_INVITING
}

void CommandHandler::handleKICK(const Command& cmd)
{
	if (cmd.params.size() < 2)
		return sendERR_NEEDMOREPARAMS(_client, "KICK");

	const std::string& channelName = cmd.params[0];
	const std::string& nick = cmd.params[1];
	const std::string reason = (cmd.params.size() >= 3) ? cmd.params[2] : "Kicked";

	Channel* channel = _server.getChannel(channelName);
	if (!channel)
		return sendERR_NOSUCHCHANNEL(_client, channelName);
	if (!channel->hasClient(&_client))
		return sendERR_CLIENTNOTINCHANNEL(_client, channelName);
	if (!channel->isOperator(&_client))
		return sendERR_CHANOPRIVSNEEDED(_client, channelName);

	Client* target = _server.getClientByNick(nick);
	if (!target)
		return sendERR_NOSUCHNICK(_client, nick);
	if (!channel->hasClient(target))
		return sendERR_USERNOTINCHANNEL(_client, nick, channelName);

	std::string msg = makePrefix(_client) + "KICK " + channelName + " " + nick + " :" + reason + "\r\n";
	channel->broadcast(msg);
	target->leaveChannel(channel);
}

void	CommandHandler::handlePING(const Command &cmd)
{
	if (cmd.params.empty())
	{
		sendERR_NEEDMOREPARAMS(_client, "PING");
		return;
	}
	std::string reply = "PONG " + cmd.params[0] + "\r\n";
	_server.queueMessage(&_client, reply);
	std::cout << "[PING] from fd " << _client._fd << " -> reply \"" << reply << "\"\n";
}

void	CommandHandler::tryRegister()
{
	if (_client._isRegistered)
		return;
	if (!_client._hasPass || !_client._hasNick || !_client._hasUser)
		return;

	_client._isRegistered = true;
	sendRPL_WELCOME(_client);
	std::cout << "[REGISTER] fd " << _client._fd << " registered as " << _client._nickname << "\n";
}
