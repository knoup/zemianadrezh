//////////////////////////////////////////////|
//Contains tests at the end of the file       |
//////////////////////////////////////////////|
#include "debug.h"
//--------------------------------------------|
// (*) Client - Server connection
//--------------------------------------------|

#include "Client.h"
#include "Server.h"

#include "LoggerNetwork.h"
#include "TextureManager.h"

#include "Tags/PlayerTag.h"
#include "Components/ComponentAnimation.h"
#include "Components/ComponentsPlayer.h"

Client::Client(sf::IpAddress _serverIP, Server* _localServer)
            : GameInstance(),
              m_networkManager(*this),
              m_serverIP(_serverIP),
              m_localServer(_localServer),
              m_player{},
              //TODO:
              //ChatBox's second parameter should take in the player's name more...
              //elegantly
              m_interface(m_networkManager,
                          _localServer == nullptr ? "RemotePlayer" :
                                                    "LocalPlayer"),
              m_systemAnimation{},
              m_systemDrawing{},
              m_systemPhysics{},
              m_systemPlayerMovement{getPlayerId()} {
	if (m_localServer != nullptr) {
		initialise(m_localServer);
	}
	else {
		initialise();
	}

	initialisePlayer();

	m_networkManager.connect(m_serverIP, Packet::Port_TCP_Server);
}

Client::~Client() {
	//We won't need to send a QUIT packet if the server was local, as that means
	//the server is already down.
	if (!isLocal()) {
		m_networkManager.sendPacket(Packet::TCPPacket::QUIT);
	}
}

void Client::getInput(sf::Event& _event) {
	m_interface.getInput(_event);
	m_systemPlayerMovement.getInput(*m_registry);
}

void Client::update(int _timeslice) {
	updateSystems(_timeslice);

	m_world->update(_timeslice);
	m_networkManager.update();
	m_interface.update(_timeslice);
}

void Client::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	target.draw(m_interface, states);
	m_systemDrawing.draw(*m_registry, target, states);
}

void Client::updateSystems(int _timeslice) {
	m_systemAnimation.update(_timeslice, *m_registry);
	m_systemPhysics.update(_timeslice, *m_registry);
	m_systemPlayerMovement.update(_timeslice, *m_registry);
}

void Client::sendPlayerPacket() {
	//Ensure that a reasonable rate of packet sending is maintained.
	static sf::Clock c;
	static float     secondsBetweenPackets{0.015};

	if (c.getElapsedTime().asSeconds() >= secondsBetweenPackets) {
		m_networkManager.sendPacket(Packet::UDPPacket::DATA_PLAYER);
		c.restart();
	}
}

void Client::receivePackets() {
	m_networkManager.receiveUDPPackets();
	m_networkManager.receiveTCPPackets();
}

const entt::entity& Client::getPlayerId() const {
	return m_player;
}

sf::Vector2f Client::getPlayerPosition() const {
	return m_registry->get<ComponentPosition>(m_player).m_position;
}

bool Client::isConnected() const {
	return m_networkManager.connectionActive();
}

bool Client::isLocal() const {
	return m_localServer != nullptr;
}

void Client::initialisePlayer() {
	m_player = m_registry->create();
	ComponentsPlayer p{};
	p.compName.m_name =
	  (m_localServer == nullptr ? "RemotePlayer" : "LocalPlayer");
	addPlayer(p, m_player);
}

void Client::respawnPlayer() {
	auto& playerPos = m_registry->get<ComponentPosition>(m_player).m_position;
	playerPos.x     = m_world->getCenter().x;
	playerPos.y     = 0;
}

/////////////////////////////////////////////////////////////////////////////////////
//---------------------------------------TESTS-------------------------------------//
/////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Testing client-server connection") {
	sf::IpAddress ip{sf::IpAddress::LocalHost};
	Server        server{true};

	SUBCASE("testing server size before joining") {
		CHECK(server.connectedPlayers() == 0);
	}

	SUBCASE("creating client and connecting to server") {
		Client client{ip, &server};
		server.acceptConnections();

		CHECK(client.isConnected());
		CHECK(client.isLocal());

		SUBCASE("testing server size after joining") {
			CHECK(server.connectedPlayers() == 1);
		}
	}

	//Client is out of scope, the destructor should have been called
	//and thus a QUIT packet sent to the server
	server.receivePackets();

	SUBCASE("testing server size after client quit") {
		CHECK(server.connectedPlayers() == 0);
	}
}
/////////////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------------------//
/////////////////////////////////////////////////////////////////////////////////////
