#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include <entt/entity/registry.hpp>

#include "NetworkManagerClient.h"
#include "GameInstance.h"
#include "ChatBox.h"
#include "UserInterface.h"

/*
The client class takes an optional pointer to a server.
This pointer should only be passed if the server is local;
if it's passed, the client will use the same resources as the
server.
*/

class Server;

class Client : public GameInstance {
	friend class NetworkManagerClient;

  public:
	Client(sf::RenderWindow& _window,
	       sf::IpAddress     _serverIP,
	       Server*           _localServer = nullptr);

	~Client();

	void getInput(sf::Event& _event);
	void update(int _timeslice);

	void sendPackets();
	void receivePackets();

	entt::entity         getPlayerId() const;
	sf::Vector2f         getPlayerPosition() const;
	const ChatBox*       getChatBox() const;
	const UserInterface* getUserInterface() const;

	bool isConnected() const;
	bool isLocal() const;

	std::pair<std::string, std::string> getPendingMessage() const;

  public:
	NetworkManagerClient m_networkManager;

  private:
	sf::IpAddress m_serverIP;
	Server*       m_localServer;
	entt::entity  m_player;

	ChatBox       m_chatBox;
	UserInterface m_userInterface;

	std::pair<std::string, std::string> m_pendingMessage;

	void handleIncomingMessages();
	void handleOutgoingMessages();
};

#endif // CLIENT_H_INCLUDED
