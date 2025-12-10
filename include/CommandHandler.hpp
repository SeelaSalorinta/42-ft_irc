#pragma once

#include <iostream>
#include <sstream> // istringstream
#include <sys/types.h>  // socket
#include <sys/socket.h> // send

class Server;
class Client;

struct	Command
{
	std::string	name;
	std::vector<std::string>	params; // parametrit
};

Command	parseCommand(const std::string &line);

void	processLine(Server &server, Client &client, const std::string &line);
