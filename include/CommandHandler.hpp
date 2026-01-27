#pragma once

#include <iostream>
#include <sstream>
#include <sys/socket.h> // send

class Server;
class Client;
struct Command;

class CommandHandler
{
	private:
		Server&	_server;
		Client&	_client;

		void	handlePASS(const Command &cmd);
		void	handleNICK(const Command &cmd);
		void	handleUSER(const Command &cmd);
		void	handleJOIN(const Command &cmd);
		void	handlePING(const Command &cmd);
		void	handleQUIT(const Command &cmd);
		void	handlePART(const Command &cmd);
		void	handlePRIVMSG(const Command &cmd);
		void	handleNOTICE(const Command& cmd);
		void	handleMODE(const Command& cmd);
		void	handleINVITE(const Command& cmd);
		void	handleKICK(const Command& cmd);
		void	handleTOPIC(const Command& cmd);
		void	tryRegister();
	public:
		CommandHandler(Server &server, Client &client);
		void handleCommand(const Command &cmd);
};
