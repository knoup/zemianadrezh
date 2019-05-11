#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include <entt/entity/registry.hpp>

#include "Interface/UserInterface.h"
#include "NetworkManagerClient.h"
#include "GameInstance.h"

#include "Systems/SystemAnimation.h"
#include "Systems/SystemDrawing.h"
#include "Systems/SystemPhysics.h"
#include "Systems/SystemPlayerMovement.h"

/*
The client class takes an optional pointer to a server.
This pointer should only be passed if the server is local;
if it's passed, the client will use the same resources as the
server.
*/

class Server;

class Client
            : public GameInstance
            , public sf::Drawable {
	friend class NetworkManagerClient;
	//TODO: maybe get rid of ProgramState_Play
	//being a friend.
	friend class ProgramState_Play;

  public:
	Client(sf::IpAddress _serverIP, Server* _localServer = nullptr);

	~Client();

	void getInput(sf::Event& _event);
	void update(int _timeslice);
	//Client's draw function simply draws the interface
	void draw(sf::RenderTarget& target, sf::RenderStates states) const;

	void updateSystems(int _timeslice);

	void sendPlayerPacket();
	void receivePackets();

	const entt::entity& getPlayerId() const;
	sf::Vector2f        getPlayerPosition() const;

	bool isConnected() const;
	bool isLocal() const;

  private:
	//Functions -----------------------------------
	void initialisePlayer();
	void respawnPlayer();
	//---------------------------------------------

	//Data members --------------------------------
	NetworkManagerClient m_networkManager;
	sf::IpAddress        m_serverIP;
	Server*              m_localServer;
	entt::entity         m_player;
	UserInterface        m_interface;

	SystemAnimation      m_systemAnimation;
	SystemDrawing        m_systemDrawing;
	SystemPhysics        m_systemPhysics;
	SystemPlayerMovement m_systemPlayerMovement;
	//---------------------------------------------
};

#endif // CLIENT_H_INCLUDED
