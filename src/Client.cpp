#include "Client.hpp"

Client::Client(int fd)
: _fd(fd), _hasPass(false), _hasNick(false), _hasUser(false), _isRegistered(false)
{}
