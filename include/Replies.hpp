#pragma once

#include <string>
#include <sys/socket.h> // send()
#include "Client.hpp"


void sendReply(Client &client, const std::string &code, const std::string &message);

void sendRPL_WELCOME(Client &client);
void sendERR_ALREADYREGISTERED(Client &client);
void sendERR_NEEDMOREPARAMS(Client &client, const std::string &command);
void sendERR_PASSWDMISMATCH(Client &client);
