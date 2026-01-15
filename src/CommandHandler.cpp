#include "CommandHandler.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Parser.hpp"
#include "Replies.hpp"
#include "Channel.hpp"

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

	//THINK BETTER WAY TO HANDLE
	if (cmd.name == "NICK")
		return handleNICK(cmd);
	if (cmd.name == "USER")
		return handleUSER(cmd);
	if (cmd.name == "JOIN")
		return handleJOIN(cmd);
	if (cmd.name == "PART")
		return handlePART(cmd);
	if (cmd.name == "PING")
		return handlePING(cmd);
	if (cmd.name == "PRIVMSG")
		return handlePRIVMSG(cmd);
	if (cmd.name == "NOTICE")
		return handleNOTICE(cmd);
	if (cmd.name == "MODE")
		return handleMODE(cmd);
	
	if (cmd.name == "QUIT")
		return handleQUIT(cmd);

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
	send(dest->_fd, msg.c_str(), msg.size(), 0);
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
	send(dest->_fd, msg.c_str(), msg.size(), 0);
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
	_client._nickname = cmd.params[0];
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
	if (firstJoiner) //first joiner gets operaator rights?
		channel->addOperator(&_client);
	std::string msg = makePrefix(_client) + "JOIN " + channelName + "\r\n";
	channel->broadcast(msg);	
	std::cout << "[JOIN] " << _client._nickname << " joined " << channelName << "\n";
}

void	CommandHandler::handlePING(const Command &cmd)
{
	if (cmd.params.empty())
	{
		sendERR_NEEDMOREPARAMS(_client, "PING");
		return;
	}
	std::string reply = "PONG " + cmd.params[0] + "\r\n";
	send(_client._fd, reply.c_str(), reply.size(), 0);
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