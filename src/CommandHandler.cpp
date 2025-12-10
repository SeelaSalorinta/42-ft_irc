#include "CommandHandler.hpp"
#include "Server.hpp"
#include "Client.hpp"

static void tryRegister(Client &client)
{
	if (client._isRegistered)
		return;
	if (!client._hasPass || !client._hasNick || !client._hasUser)
		return;

	client._isRegistered = true;
	std::string msg = ":ft_irc 001 " + client._nickname +
		" :Welcome to ft_irc\r\n";
	send(client._fd, msg.c_str(), msg.size(), 0);	
}

Command	parseCommand(const std::string &line)
{
	Command	cmd;
	std::istringstream	iss(line);
	std::string	word;

	if (!(iss >> cmd.name))
		return cmd;
	while (iss >> word)
	{
		if (!word.empty() && word[0] == ':')
		{
			std::string rest;
			std::getline(iss, rest);
			word.erase(0, 1); // poista ':' alusta
			cmd.params.push_back(word + rest);
			break;
		}
		cmd.params.push_back(word);
	}
	return cmd;
}

void	processLine(Server &server, Client &client, const std::string &line)
{
	(void)server; //myöhemmin

	//helper print
	std::cout << "[processLine] fd " << client._fd
		<< " line: \"" << line << "\"" << std::endl;

	Command cmd = parseCommand(line);
	if (cmd.name.empty())
		return;
	// ignoraa CAP (irssi lähettää CAP LS ekaks)
	if (cmd.name == "CAP")
		return;
	if (cmd.name == "PASS")
	{
		if (cmd.params.size() != 1)
		{
			std::string msg = "ERROR : PASS command takes exactly one parameter\r\n";
			send(client._fd, msg.c_str(), msg.size(), 0);
			return;
		}
		if (client._isRegistered || client._hasNick || client._hasUser)
		{
			std::string msg = "ERROR : You may not reregister\r\n";
			send(client._fd, msg.c_str(), msg.size(), 0);
			return;
		}
		if (cmd.params[0] != server.getPassword())
		{
			std::string msg = "ERROR : Password incorrect\r\n";
			send(client._fd, msg.c_str(), msg.size(), 0);
			// pitääkö drop client? eikai
			return;
		}
		client._hasPass = true;
		std::cout << "[PASS] accepted for fd " << client._fd << std::endl;
		tryRegister(client);
		return;
	}
	//ensin tarvtaan password
	if (!client._hasPass)
	{
		std::string msg = "ERROR : You must send PASS before other commands\r\n";
		send(client._fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (cmd.name == "NICK")
	{
		if (cmd.params.size() != 1)
		{
			std::string msg = "ERROR : NICK command takes exactly one parameter\r\n";
			send(client._fd, msg.c_str(), msg.size(), 0);
			return;
		}
		client._nickname = cmd.params[0];
		client._hasNick = true;
		std::cout << "[NICK] set to \"" << client._nickname
				  << "\" for fd " << client._fd << std::endl;
		tryRegister(client);
		return;
	}
	if (cmd.name == "USER")
	{
		if (cmd.params.size() != 4 || cmd.params[1] != "0"
			|| cmd.params[2] != "*")
		{
			std::string msg = "ERROR : Usage : USER <username> 0 * :<real name>\r\n";
			send(client._fd, msg.c_str(), msg.size(), 0);
			return;
		}
		client._username = cmd.params[0];
		client._realname = cmd.params[3];
		client._hasUser = true;
		std::cout << "[USER] set to username \"" << client._username << "\" and realname \"" << client._realname
				  << "\" for fd " << client._fd << std::endl;
		tryRegister(client);
		return;
	}
	if (cmd.name == "JOIN" || cmd.name == "PART" || cmd.name == "PRIVMSG" 
		|| cmd.name == "NOTICE" || cmd.name == "QUIT")
	{
		//handleCoreCommands();
		return;
	}
	if (cmd.name == "KICK" || cmd.name == "INVITE"
		|| cmd.name == "TOPIC" || cmd.name == "MODE")
	{
		//handleOperatorCommands();
		return;
	}
	if (cmd.name == "PING")
	{
		if (cmd.params.empty())
		{
			std::string msg = "ERROR: PING needs a parameter\r\n";
			send(client._fd, msg.c_str(), msg.size(), 0);
			return;
		}

		// vastaa PINGIIN lähettämällä PONGilla sama takaisin
		std::string reply = "PONG " + cmd.params[0] + "\r\n";
		send(client._fd, reply.c_str(), reply.size(), 0);

		std::cout << "[PING] from fd " << client._fd
				<< " -> reply \"" << reply << "\"\n";
		return;
	}

	std::string msg = "ERROR : Unknown command\r\n";
	send(client._fd, msg.c_str(), msg.size(), 0);
}