#include "NetworkManagerClient.h"

#include <iostream>
#include <sstream>

#include "LoggerNetwork.h"
#include "Client.h"

#include "Tags/PlayerTag.h"
#include "Components/ComponentsPlayer.h"

NetworkManagerClient::NetworkManagerClient(Client& _client)
            : m_client{_client}, m_playerSpawned{false}, m_connectionActive{false}, m_udpSocket{} {
	m_udpSocket.setBlocking(false);

	if (m_udpSocket.bind(sf::Socket::AnyPort) != sf::Socket::Done) {
		LoggerNetwork::get_instance().log(
		  LoggerNetwork::LOG_SENDER::CLIENT,
		  LoggerNetwork::LOG_MESSAGE::BIND_PORT_FAILURE);
	}
	else {
		LoggerNetwork::get_instance().log(
		  LoggerNetwork::LOG_SENDER::CLIENT,
		  LoggerNetwork::LOG_MESSAGE::BIND_PORT_SUCCESS);
	}

	m_serverConnection.setBlocking(false);
}

void NetworkManagerClient::update() {
	PacketSender::get_instance().update();
}

void NetworkManagerClient::sendPacket(Packet::TCPPacket _type) {
	int             packetCode = Packet::toInt(_type);
	PacketSharedPtr packet(new sf::Packet());
	*packet << packetCode;

	switch (_type) {
	//////////////////////////////////////////////////////////////////////////////
	case Packet::TCPPacket::JUSTJOINED: {
		entt::entity e{m_client.m_player};
		sf::Uint16   port{m_udpSocket.getLocalPort()};

		const auto dir{m_client.m_registry->get<ComponentDirection>(e)};
		const auto name{m_client.m_registry->get<ComponentName>(e)};
		const auto vel{m_client.m_registry->get<ComponentPhysics>(e)};
		const auto pos{m_client.m_registry->get<ComponentPosition>(e)};

		ComponentsPlayer data{dir, name, vel, pos};

		*packet << data;
		*packet << port;

		PacketSender::get_instance().send(&m_serverConnection, packet);
		break;
	}
	//////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////
	case Packet::TCPPacket::QUIT: {
		PacketSender::get_instance().send(&m_serverConnection, packet);
		break;
	}
	//////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////
	case Packet::TCPPacket::REQUEST_WORLD: {
		PacketSender::get_instance().send(&m_serverConnection, packet);
		break;
	}
	//////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////
	case Packet::TCPPacket::CHAT_MESSAGE: {
		//At this stage, m_messageToSend should have been set (usually by
		//UserInterface)
		*packet << m_messageToSend.sender;
		*packet << m_messageToSend.content;

		PacketSender::get_instance().send(&m_serverConnection, packet);

		break;
	}
		//////////////////////////////////////////////////////////////////////////////

	default:
		break;
	}
}

void NetworkManagerClient::sendPacket(Packet::UDPPacket _type) {
	if(!m_playerSpawned) {
		return;
	}

	int             packetCode = Packet::toInt(_type);
	PacketSharedPtr packet(new sf::Packet());
	*packet << packetCode;
	unsigned short port{Packet::Port_UDP_Server};

	switch (_type) {
	//////////////////////////////////////////////////////////////////////////////
	case Packet::UDPPacket::DATA_PLAYER: {
		entt::entity e{m_client.m_player};

		auto dir{m_client.m_registry->get<ComponentDirection>(e)};
		auto name{m_client.m_registry->get<ComponentName>(e)};
		auto vel{m_client.m_registry->get<ComponentPhysics>(e)};
		auto pos{m_client.m_registry->get<ComponentPosition>(e)};

		*packet << ComponentsPlayer{dir, name, vel, pos};

		PacketSender::get_instance().send(
		  &m_udpSocket, packet, m_serverConnection.getRemoteAddress(), port);

		break;
	}
		//////////////////////////////////////////////////////////////////////////////
	}
}

void NetworkManagerClient::receiveTCPPackets() {
	int        packetCode;
	PacketUPtr packet(new sf::Packet());

	while (m_serverConnection.receive(*packet) == sf::Socket::Status::Done) {
		*packet >> packetCode;
		Packet::TCPPacket packetType{Packet::toTCPType(packetCode)};

		switch (packetType) {
		//////////////////////////////////////////////////////////////////////////////
		case Packet::TCPPacket::QUIT: {
			std::string name{};
			*packet >> name;
			m_client.removePlayer(name);
			break;
		}
		//////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////////
		case Packet::TCPPacket::CONNECTIONLOST: {
			m_connectionActive = false;
			break;
		}
		//////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////////
		case Packet::TCPPacket::DATA_WORLD: {
			WorldChunk::EncodedChunkData chunkData;

			*packet >> chunkData;

			//We won't want to parse the world data
			//if we're local, since the world will
			//be a shared pointer anyway, and re-parsing
			//will involve calling addChunks and mess up
			//the world by un-randomizing it
			if (!m_client.isLocal()) {
				m_client.parseWorldChunk(chunkData);
			}

			m_client.renderUpdatedChunk(chunkData.id);
			break;
		}
		//////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////////
		case Packet::TCPPacket::CHAT_MESSAGE: {
			std::string sender;
			std::string message;
			*packet >> sender;
			*packet >> message;

			m_lastReceivedMessage = {sender, message};
			break;
		}
		//////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////////
		case Packet::TCPPacket::RESPAWN_PLAYER: {
			m_client.respawnPlayer();
			m_playerSpawned = true;
			break;
		}
			//////////////////////////////////////////////////////////////////////////////

		default:
			break;
		}
	}
}

//This function is used to display the number of received packets per second as long
//as they're not 0. Used in the beginning of receiveUDPPackets()
void logToConsole(int& _i) {
	static sf::Clock clock{};
	if (clock.getElapsedTime().asSeconds() >= 1) {
		if (_i > 0) {
			std::cout << "[CLIENT] UDP packets received: " << _i << std::endl;
			std::cout << "----------------------------------" << std::endl;
			_i = 0;
		}
		clock.restart();
	}
}

void NetworkManagerClient::receiveUDPPackets() {
	static int UDPPacketsReceived{0};
	logToConsole(UDPPacketsReceived);

	int            packetCode;
	PacketUPtr     packet(new sf::Packet());
	unsigned short port{m_udpSocket.getLocalPort()};

	sf::IpAddress address{m_serverConnection.getRemoteAddress()};

	while (m_udpSocket.receive(*packet, address, port) ==
	       sf::Socket::Status::Done) {
		UDPPacketsReceived++;
		*packet >> packetCode;
		Packet::UDPPacket packetType{Packet::toUDPType(packetCode)};

		switch (packetType) {
		//////////////////////////////////////////////////////////////////////////////
		case Packet::UDPPacket::DATA_PLAYER: {
			ComponentsPlayer p;
			*packet >> p;
			//When we receive a packet containing player data, we're going to update or
			//add the player to our registry. Note that under normal circumstances, the
			//client's own player shouldn't have its data sent back to it.
			m_client.updatePlayer(p);
			break;
		}
			//////////////////////////////////////////////////////////////////////////////
		}
	}
}

void NetworkManagerClient::connect(const sf::IpAddress& _ip,
                                   unsigned short       _port) {
	bool successfullyConnected{false};

	m_serverConnection.setBlocking(true);
	sf::Socket::Status status = m_serverConnection.connect(_ip, _port);

	if (status != sf::Socket::Done) {
		LoggerNetwork::get_instance().log(
		  LoggerNetwork::LOG_SENDER::CLIENT,
		  LoggerNetwork::LOG_MESSAGE::CONNECTION_WAITING);
	}
	else {
		LoggerNetwork::get_instance().log(
		  LoggerNetwork::LOG_SENDER::CLIENT,
		  LoggerNetwork::LOG_MESSAGE::CONNECTION_SUCCESS);
		successfullyConnected = true;
	}

	m_serverConnection.setBlocking(false);

	if (successfullyConnected) {
		m_connectionActive = true;
		sendPacket(Packet::TCPPacket::JUSTJOINED);
	}
}

bool NetworkManagerClient::connectionActive() const {
	bool invalidHost{m_serverConnection.getRemoteAddress() ==
	                 sf::IpAddress::None};
	return m_connectionActive && !invalidHost;
}

bool NetworkManagerClient::receivedMessage(Message* _ptr) {
	bool valid{m_lastReceivedMessage.sender != ""};

	if (_ptr != nullptr) {
		*_ptr = m_lastReceivedMessage;
	}

	return valid;
}

void NetworkManagerClient::clearLastReceivedMessage() {
	m_lastReceivedMessage.sender  = "";
	m_lastReceivedMessage.content = "";
}

void NetworkManagerClient::setMessageToSend(Message _msg) {
	m_messageToSend = _msg;
}
