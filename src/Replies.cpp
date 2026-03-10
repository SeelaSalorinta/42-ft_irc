#include "Replies.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include <sstream>

static const std::string SERVERNAME = "ft_irc";

void sendNumeric(Client& client, const std::string& code,
	const std::vector<std::string>& params, const std::string& trailing)
{
	std::string full = ":" + SERVERNAME + " " + code + " " + client._nickname;

	for (size_t i = 0; i < params.size(); ++i)
		full += " " + params[i];

	if (!trailing.empty())
		full += " :" + trailing;

	full += "\r\n";

	if (client._server)
		client._server->queueMessage(&client, full);
}

void sendReply(Client &client, const std::string &code, const std::string &trailing)
{
	std::string full = ":" + SERVERNAME + " " + code + " " +
		client._nickname + " :" + trailing + "\r\n";

	if (client._server)
		client._server->queueMessage(&client, full);
}

void	sendRPL_WELCOME(Client &client)
{
	sendReply(client, "001", "Welcome to " + SERVERNAME);
}

void	sendERR_ALREADYREGISTERED(Client &client)
{
	sendReply(client, "462", "You may not reregister");
}

void	sendERR_NOSUCHCHANNEL(Client &client, const std::string &channel)
{
	sendNumeric(client, "403", std::vector<std::string>(1, channel), "No such channel");
}

void	sendERR_CLIENTNOTINCHANNEL(Client &client, const std::string &channel)
{
	sendNumeric(client, "442", std::vector<std::string>(1, channel), "Client is not part of channel");
}

void	sendERR_NEEDMOREPARAMS(Client &client, const std::string &command)
{
	sendNumeric(client, "461", std::vector<std::string>(1, command), "Not enough parameters");
}

void	sendERR_PASSWDMISMATCH(Client &client)
{
	sendReply(client, "464", "Password incorrect");
}

void sendERR_NOSUCHNICK(Client &client, const std::string &nick)
{
	sendNumeric(client, "401", std::vector<std::string>(1, nick), "No such nick/channel");
}

void sendERR_CANNOTSENDTOCHAN(Client& client, const std::string& channel)
{
	sendNumeric(client, "404", std::vector<std::string>(1, channel), "Cannot send to channel");
}

void sendERR_NORECIPIENT(Client& client, const std::string& command)
{
	sendNumeric(client, "411", std::vector<std::string>(1, command), "No recipient given");
}

void sendERR_NOTEXTTOSEND(Client& client)
{
	sendReply(client, "412", "No text to send");
}

void sendERR_CHANOPRIVSNEEDED(Client& client, const std::string& channel)
{
	sendNumeric(client, "482", std::vector<std::string>(1, channel), "You're not channel operator");
}

void sendERR_UNKNOWNMODE(Client& client, char mode, const std::string& channel)
{
	std::vector<std::string> params;
	params.push_back(std::string(1, mode));

	sendNumeric(client, "472", params, "is unknown mode char to me for " + channel);
}

void sendERR_USERNOTINCHANNEL(Client& client, const std::string& nick, const std::string& channel)
{
	std::vector<std::string> params;
	params.push_back(nick);
	params.push_back(channel);

	sendNumeric(client, "441", params, "They aren't on that channel");
}

void sendERR_CHANNELISFULL(Client& client, const std::string& channel)
{
	sendNumeric(client, "471", std::vector<std::string>(1, channel), "Cannot join channel (+l)");
}

void sendERR_INVITEONLYCHAN(Client& client, const std::string& channel)
{
	sendNumeric(client, "473", std::vector<std::string>(1, channel), "Cannot join channel (+i)");
}

void sendERR_BADCHANNELKEY(Client& client, const std::string& channel)
{
	sendNumeric(client, "475", std::vector<std::string>(1, channel), "Cannot join channel (+i)");
}

void sendRPL_CHANNELMODEIS(Client& client, const std::string& channel,
	const std::string& modes, const std::string& args)
{
	std::vector<std::string> params;
	params.push_back(channel);
	params.push_back(modes);

	std::istringstream iss(args);
	std::string arg;
	while (iss >> arg)
		params.push_back(arg);

	sendNumeric(client, "324", params, "");
}
