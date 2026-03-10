#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <climits>

#include "Server.hpp"

static void usage(const char *prog)
{
	std::cerr << "Usage: " << prog << " <port> <password>" << std::endl;
}

static bool parsePort(const char *str, int &port)
{
	char *end = NULL;
	errno = 0;

	long value = std::strtol(str, &end, 10);

	if (str == end)
		return false; // no digits at all

	if (*end != '\0')
		return false; // extra junk after number

	if (errno == ERANGE || value < 1 || value > 65535)
		return false; // out of valid port range

	port = static_cast<int>(value);
	return true;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	int port;
	if (!parsePort(argv[1], port))
	{
		std::cerr << "Invalid port: " << argv[1] << std::endl;
		return EXIT_FAILURE;
	}

	std::string password = argv[2];

	try
	{
		Server server(port, password);
		server.start();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Fatal: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}