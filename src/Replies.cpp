#include "Replies.hpp"

static const std::string SERVERNAME = "ft_irc";

void	sendReply(Client &client, const std::string &code, const std::string &message)
{
	std::string full = ":" + SERVERNAME + " " + code + " " + client._nickname + " :" + message + "\r\n";
	send(client._fd, full.c_str(), full.size(), 0);
}

void	sendRPL_WELCOME(Client &client)
{
	sendReply(client, "001", "Welcome to " + SERVERNAME);
}

void	sendERR_ALREADYREGISTERED(Client &client)
{
	sendReply(client, "462", "You may not reregister");
}

void	sendERR_NEEDMOREPARAMS(Client &client, const std::string &command)
{
	sendReply(client, "461", command + " :Not enough parameters");
}

void	sendERR_PASSWDMISMATCH(Client &client)
{
	sendReply(client, "464", "Password incorrect");
}
