#pragma once

#include "Server.hpp"
#include "Client.hpp"
#include "Parser.hpp"
#include <iostream>
#include <sstream>
#include <sys/socket.h> // send

class CommandHandler
{
	private:
		Server&	_server;
		Client&	_client;

		void handlePass(const Command &cmd);
		void handleNick(const Command &cmd);
		void handleUser(const Command &cmd);
		void handlePing(const Command &cmd);
		void tryRegister();
	public:
		CommandHandler(Server &server, Client &client);
		void handleCommand(const Command &cmd);
};