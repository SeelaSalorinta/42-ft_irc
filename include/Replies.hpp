#pragma once

#include <string>
#include <sys/socket.h> // send()

class Client;

void	sendReply(Client &client, const std::string &code, const std::string &message);

void	sendRPL_WELCOME(Client &client);
void	sendERR_ALREADYREGISTERED(Client &client);
void	sendERR_NEEDMOREPARAMS(Client &client, const std::string &command);
void	sendERR_PASSWDMISMATCH(Client &client);
void	sendERR_CLIENTNOTINCHANNEL(Client &client, const std::string &command);
void	sendERR_NOSUCHCHANNEL(Client &client, const std::string &command);
void	sendERR_NOSUCHNICK(Client& client, const std::string& nick);
void	sendERR_CANNOTSENDTOCHAN(Client& client, const std::string& channel);
void	sendERR_NORECIPIENT(Client& client, const std::string& cmd);
void	sendERR_NOTEXTTOSEND(Client& client);
void	sendERR_CHANOPRIVSNEEDED(Client& client, const std::string& channel);
void	sendERR_UNKNOWNMODE(Client& client, char mode, const std::string& channel);
void	sendERR_NOSUCHNICK(Client& client, const std::string& nick);
void	sendERR_USERNOTINCHANNEL(Client& client, const std::string& nick, const std::string& channel);
void	sendRPL_CHANNELMODEIS(Client& client, const std::string& channel, const std::string& modes, const std::string& args);
void	sendERR_CHANNELISFULL(Client& client, const std::string& channel);
void	sendERR_BADCHANNELKEY(Client& client, const std::string& channel);
void	sendERR_INVITEONLYCHAN(Client& client, const std::string& channel);