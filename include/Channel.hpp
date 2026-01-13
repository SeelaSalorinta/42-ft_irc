#pragma once 

#include <string>
#include <vector>
#include <set>
#include <sys/socket.h> // send
#include <algorithm>

class Client;

class Channel
{
	private:
		std::string _name;
		std::vector<Client*> _clients;

		std::string _topic;

		bool _inviteOnly;   // +i
		bool _topicOpOnly;  // +t
		std::string _key;   // +k
		bool _hasKey;
		size_t _userLimit;  // +l
		bool _hasLimit;

		std::set<Client*> _operators;
		std::set<std::string> _invitedNicks;

	public:
		Channel(const std::string &name);
		const std::string&	getName() const;
		const std::vector<Client*>&	getClients() const;

		void	addClient(Client* client);
		void	removeClient(Client* client);

		void	broadcast(const std::string &message);
		void	broadcastExcept(const std::string &message, Client* except);

		bool	hasClient(Client* client);

		const std::string& getTopic() const;
		void setTopic(const std::string& t);
	
		bool isOperator(Client* c) const;
		void addOperator(Client* c);
		void removeOperator(Client* c);
	
		bool inviteOnly() const;
		void setInviteOnly(bool b);
	
		bool topicOpOnly() const;
		void setTopicOpOnly(bool b);
	
		bool hasKey() const;
		const std::string& getKey() const;
		void setKey(const std::string& k);
		void clearKey();
	
		bool hasLimit() const;
		size_t getLimit() const;
		void setLimit(size_t l);
		void clearLimit();
	
		void inviteNick(const std::string& nick);
		bool isInvited(const std::string& nick) const;
		void consumeInvite(const std::string& nick);

};
