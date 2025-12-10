#include "CommandHandler.hpp"

CommandHandler::CommandHandler(Server &server, Client &client)
	: _server(server), _client(client) {}

void CommandHandler::handleCommand(const Command &cmd)
{
	if (cmd.name == "CAP")
		return;

	if (cmd.name == "PASS")
		return handlePass(cmd);
	if (!_client._hasPass)
	{
		std::string msg = "ERROR :You must send PASS before other commands\r\n";
		send(_client._fd, msg.c_str(), msg.size(), 0);
		return;
	}

	if (cmd.name == "NICK")
		return handleNick(cmd);
	if (cmd.name == "USER")
		return handleUser(cmd);
	if (cmd.name == "PING")
		return handlePing(cmd);

	// fallback for unknown commands
	std::string msg = "ERROR :Unknown command\r\n";
	send(_client._fd, msg.c_str(), msg.size(), 0);
}

void CommandHandler::handlePass(const Command &cmd)
{
	if (cmd.params.size() != 1)
	{
		std::string msg = "ERROR :PASS command takes exactly one parameter\r\n";
		send(_client._fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (_client._isRegistered || _client._hasNick || _client._hasUser)
	{
		std::string msg = "ERROR :You may not reregister\r\n";
		send(_client._fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (cmd.params[0] != _server.getPassword())
	{
		std::string msg = "ERROR :Password incorrect\r\n";
		send(_client._fd, msg.c_str(), msg.size(), 0);
		return;
	}
	_client._hasPass = true;
	std::cout << "[PASS] accepted for fd " << _client._fd << std::endl;
	tryRegister();
}

void CommandHandler::handleNick(const Command &cmd)
{
	if (cmd.params.size() != 1)
	{
		std::string msg = "ERROR :NICK command takes exactly one parameter\r\n";
		send(_client._fd, msg.c_str(), msg.size(), 0);
		return;
	}
	_client._nickname = cmd.params[0];
	_client._hasNick = true;
	std::cout << "[NICK] set to \"" << _client._nickname << "\" for fd " << _client._fd << std::endl;
	tryRegister();
}

void CommandHandler::handleUser(const Command &cmd)
{
	if (cmd.params.size() != 4 || cmd.params[1] != "0" || cmd.params[2] != "*")
	{
		std::string msg = "ERROR :Usage: USER <username> 0 * :<realname>\r\n";
		send(_client._fd, msg.c_str(), msg.size(), 0);
		return;
	}
	_client._username = cmd.params[0];
	_client._realname = cmd.params[3];
	_client._hasUser = true;
	std::cout << "[USER] set to username \"" << _client._username
			  << "\", realname \"" << _client._realname
			  << "\" for fd " << _client._fd << std::endl;
	tryRegister();
}

void CommandHandler::handlePing(const Command &cmd)
{
	if (cmd.params.empty())
	{
		std::string msg = "ERROR :PING requires a parameter\r\n";
		send(_client._fd, msg.c_str(), msg.size(), 0);
		return;
	}
	std::string reply = "PONG " + cmd.params[0] + "\r\n";
	send(_client._fd, reply.c_str(), reply.size(), 0);
	std::cout << "[PING] from fd " << _client._fd << " -> reply \"" << reply << "\"\n";
}

void CommandHandler::tryRegister()
{
	if (_client._isRegistered)
		return;
	if (!_client._hasPass || !_client._hasNick || !_client._hasUser)
		return;

	_client._isRegistered = true;

	std::string msg = ":ft_irc 001 " + _client._nickname + " :Welcome to ft_irc\r\n";
	send(_client._fd, msg.c_str(), msg.size(), 0);
	std::cout << "[REGISTER] fd " << _client._fd << " registered as " << _client._nickname << "\n";
}