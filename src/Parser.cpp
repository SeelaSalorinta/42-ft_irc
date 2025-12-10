#include "Parser.hpp"

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