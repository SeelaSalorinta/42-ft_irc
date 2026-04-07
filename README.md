# ft_irc 🛰️ A small IRC server written in C++
## Made by [@Seela Salorinta](https://github.com/SeelaSalorinta) and [@Jimi Karhu](https://github.com/gitenjoyer95)

**IRC (Internet Relay Chat)** is a text-based communication protocol where users can:
- connect to a server
- join channels (chat rooms)
- send messages to users or groups

Think of it as:
> Discord… but in the terminal and from the 90s 😎


## What It Does 🎯

- Lets users connect and register with the server
- Supports private messages and channel conversations
- Includes channel management features like topic changes, invites, kicks, and modes
- Handles client quits, disconnects, and channel cleanup


## Server Behavior 🤖

- To complete registration, a client must send valid `PASS`, `NICK`, and `USER`
- Most commands are only available after registration is complete
- `PASS` must match the server password or the client stays unregistered
- Invite-only channels (`+i`) only allow users with an active invite
- Invites are consumed on join, so rejoining later requires a new invite
- If a channel has mode `+l` and is full, new users cannot join until a spot is free
- Lowering the limit does not remove users already in the channel
- The first user to join a brand new channel gets operator rights automatically
- Operators can manage the channel with commands like `KICK`, `INVITE`, and supported `MODE` changes
- If everyone leaves a channel, the server removes that empty channel automatically


## Technical Notes ⚙️

- The server uses non-blocking sockets
- Client connections are handled with `poll()`
- The architecture is single-threaded and event-driven
- Multiple clients are served in the same event loop
- Empty channels are destroyed automatically
- Disconnected clients are removed and cleaned up by the server


## Supported Commands 🛠️

- `PASS` (set the server password)
- `NICK` (choose or change a nickname)
- `USER` (finish user registration)
- `PING` (keep the connection alive)
- `JOIN` (join a channel)
- `PART` (leave a channel)
- `PRIVMSG` (send a message to a user or channel)
- `NOTICE` (send a notice without error replies)
- `QUIT` (disconnect from the server)
- `MODE` (view or change channel modes)
- `INVITE` (invite a user to an invite-only channel)
- `KICK` (remove a user from a channel)
- `TOPIC` (view or change the channel topic)


## Supported Modes 🔐

Channel modes currently implemented:

- `+i` / `-i` (invite-only channel)
- `+t` / `-t` (only channel operators can change topic)
- `+k` / `-k` (set or remove channel key/password)
- `+l` / `-l` (set or remove user limit; full channels block new joins, but existing users stay)
- `+o` / `-o` (give or remove channel operator status)


## Build ⚙️

```bash
make
```


## Run ▶️

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 mysecretpass
```

Notes:

- Port must be between `1024` and `65535`
- The server listens on all interfaces
- Password is required before registration completes


## Try It Out 💻🧪

Start the server:

```bash
./ircserv 6667 mysecretpass
```

You can mix IRC clients however you want: `irssi` with `irssi`, `irssi` with `nc`, or multiple `nc` terminals.

Example setup:

Terminal 1:
```bash
irssi -c localhost -p 6667 -n seela -w mysecretpass
```

Terminal 2:
```bash
nc 127.0.0.1 6667
```
Useful commands to try:

```bash
PASS mysecretpass
NICK jimi
USER jimi 0 * :Jimbo
JOIN #mychannel
PRIVMSG #mychannel :hello channel from jimi
PRIVMSG seela :hello to seela its jimi
```

Terminal 3:
```bash
irssi -c localhost -p 6667 -n meme -w mysecretpass
```

Useful commands to try:

```text
/JOIN #mychannel
/MSG #mychannel hello from me
/MSG jimi hello to jimi from me
/NOTICE #mychannel quiet but important message
/TOPIC #mychannel this is my topic yey
/PART #mychannel :myreason for parting
/QUIT :myreason for quitting
```

## Project Structure 📁

- `src/Server.cpp`: socket setup, poll loop, client lifecycle
- `src/CommandHandler.cpp`: IRC command dispatch and behavior
- `src/Channel.cpp`: channel membership and channel modes
- `src/Client.cpp`: per-client state
- `src/Parser.cpp`: raw line parsing into commands
- `src/Replies.cpp`: numeric replies and errors
- `include/`: headers
