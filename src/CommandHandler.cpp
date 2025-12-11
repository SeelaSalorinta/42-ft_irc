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
	//if (cmd.name == "QUIT")
	//	return handleQUIT(cmd);

	// fallback for unknown commands
	sendReply(_client, "ERROR", "Unknown command");
}

/*void	CommandHandler::handleQUIT(const Command &cmd)
{
	if (!cmd.params.empty())
	{
		sendERR_(_client, "QUIT");
		return;
	}
}*/

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
		sendERR_NOSUCHCHANNEL(_client, cmd.params[0]);
		return;
	}
	//poista client kyseiseltä kanavalta
	_client.leaveChannel(channel);
	std::string partMsg = ":" + _client._nickname + " PART " + cmd.params[0]+ "\r\n";
	channel->broadcast(partMsg);
	std::cout << "[PART] " << _client._nickname << " part " << cmd.params[0] << "\n";

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
	Channel	*channel = _server.getOrCreateChannel(cmd.params[0]);
	channel->addClient(&_client);
	_client.joinChannel(channel);
	std::string joinMsg = ":" + _client._nickname + " JOIN " + cmd.params[0]+ "\r\n";
	channel->broadcast(joinMsg);
	std::cout << "[JOIN] " << _client._nickname << " joined " << cmd.params[0] << "\n";
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