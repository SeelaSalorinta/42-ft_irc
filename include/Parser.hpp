#pragma once

#include <string>
#include <vector>
#include <sstream>

struct Command
{
	std::string					name;
	std::vector<std::string>	params;
};

Command parseCommand(const std::string& line);