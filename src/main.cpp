#include <iostream>
#include <cstdlib>

#include "Server.hpp"

static void usage(const char *prog)
{
	std::cerr << "Usage: " << prog << " <port> <password>" << std::endl;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	int port = std::atoi(argv[1]);
	if (port <= 0 || port > 65535)
	{
		std::cerr << "Invalid port: " << argv[1] << std::endl;
		return EXIT_FAILURE;
	}

	try
	{
		Server server(port);
		server.start();
	}
	catch (const std::exception &e)
	{
		std::cerr << "Fatal: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
