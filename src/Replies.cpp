#include "Replies.hpp"
#include "Client.hpp"

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

void	sendERR_NOSUCHCHANNEL(Client &client, const std::string &command)
{
	sendReply(client, "403", command + " :No such channel");
}

void	sendERR_CLIENTNOTINCHANNEL(Client &client, const std::string &command)
{
	sendReply(client, "442", command + " :Client is not part of channel");
}

void	sendERR_NEEDMOREPARAMS(Client &client, const std::string &command)
{
	sendReply(client, "461", command + " :Not enough parameters");
}

void	sendERR_PASSWDMISMATCH(Client &client)
{
	sendReply(client, "464", "Password incorrect");
}

void sendERR_NOSUCHNICK(Client &client, const std::string &nick)
{
	sendReply(client, "401", nick + " :No such nick/channel");
}
void sendERR_CANNOTSENDTOCHAN(Client& client, const std::string& channel)
{
	sendReply(client, "404", channel + " :Cannot send to channel");
}
void sendERR_NORECIPIENT(Client& client, const std::string& cmd)
{
	sendReply(client, "411", cmd + " :No recipient given");
}
void sendERR_NOTEXTTOSEND(Client& client)
{
	sendReply(client, "412", ":No text to send");
}

void sendERR_CHANOPRIVSNEEDED(Client& client, const std::string& channel)
{
	sendReply(client, "482", channel + " :You're not channel operator");
}

void sendERR_UNKNOWNMODE(Client& client, char mode, const std::string& channel)
{
	std::string m(1, mode);
	sendReply(client, "472", m + " :is unknown mode char to me for " + channel);
}

void sendERR_USERNOTINCHANNEL(Client& client, const std::string& nick, const std::string& channel)
{
	sendReply(client, "441", nick + " " + channel + " :They aren't on that channel");
}

void sendERR_CHANNELISFULL(Client& client, const std::string& channel)
{
	sendReply(client, "471", channel + " :Cannot join channel (+l)");
}

void sendERR_INVITEONLYCHAN(Client& client, const std::string& channel)
{
	sendReply(client, "473", channel + " :Cannot join channel (+i)");
}

void sendERR_BADCHANNELKEY(Client& client, const std::string& channel)
{
	sendReply(client, "475", channel + " :Cannot join channel (+k)");
}

void sendRPL_CHANNELMODEIS(Client& client, const std::string& channel, const std::string& modes, const std::string& args)
{
	// 324 <nick> <channel> <modes> [mode params]
	sendReply(client, "324", channel + " " + modes + args);
}

