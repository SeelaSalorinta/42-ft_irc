#include "CommandHandler.hpp"
#include "Replies.hpp"

CommandHandler::CommandHandler(Server &server, Client &client)
	: _server(server), _client(client) {}

void	CommandHandler::handleCommand(const Command &cmd)
{
	if (cmd.name == "CAP")
		return;

	if (cmd.name == "PASS")
		return handlePass(cmd);
	if (!_client._hasPass)
	{
		sendReply(_client, "ERROR", "You must send PASS before other commands");
		return;
	}

	if (cmd.name == "NICK")
		return handleNick(cmd);
	if (cmd.name == "USER")
		return handleUser(cmd);
	if (cmd.name == "JOIN")
		return handleJoin(cmd);
	if (cmd.name == "PING")
		return handlePing(cmd);

	// fallback for unknown commands
	sendReply(_client, "ERROR", "Unknown command");
}

void	CommandHandler::handlePass(const Command &cmd)
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

void	CommandHandler::handleNick(const Command &cmd)
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

void	CommandHandler::handleUser(const Command &cmd)
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

void	CommandHandler::handleJoin(const Command &cmd)
{
	if (cmd.params.empty())
	{
		sendERR_NEEDMOREPARAMS(_client, "JOIN");
		return;
	}
	Channel	*channel = _server.getOrCreateChannel(cmd.params[0]);
	channel->addClient(&_client);
	std::string joinMsg = ":" + _client._nickname + " JOIN " + cmd.params[0]+ "\r\n";
	channel->broadcast(joinMsg);
	std::cout << "[JOIN] " << _client._nickname << " joined " << cmd.params[0] << "\n";
}

void	CommandHandler::handlePing(const Command &cmd)
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