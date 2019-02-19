#include "NetworkManagerClient.h"

#include <iostream>
#include <sstream>

#include "LoggerNetwork.h"
#include "Client.h"

NetworkManagerClient::NetworkManagerClient(Client& _client)
	:m_client{ _client },
	 m_connectionActive{ true },
	 m_udpSocket{} {

	m_udpSocket.setBlocking(false);

	if (m_udpSocket.bind(sf::Socket::AnyPort) != sf::Socket::Done) {
		LoggerNetwork::get_instance().log(LoggerNetwork::LOG_SENDER::CLIENT,
			LoggerNetwork::LOG_MESSAGE::BIND_PORT_FAILURE);
	}
	else {
		LoggerNetwork::get_instance().log(LoggerNetwork::LOG_SENDER::CLIENT,
			LoggerNetwork::LOG_MESSAGE::BIND_PORT_SUCCESS);
	}

	m_serverConnection.setBlocking(false);
}

void NetworkManagerClient::sendPacket(Packet::TCPPacket _type) {
	int packetCode = Packet::toInt(_type);
	PacketSharedPtr packet(new sf::Packet());
	*packet << packetCode;

	switch(_type) {

	//////////////////////////////////////////////////////////////////////////////
	case Packet::TCPPacket::JUSTJOINED: {
			Player::EncodedPlayerData playerData = m_client.getPlayer()->encodeData();
			sf::Uint16 port{ m_udpSocket.getLocalPort() };
			*packet << playerData.playerName;
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
			auto message = m_client.getPendingMessage();

			*packet << message.first;
			*packet << message.second;

			PacketSender::get_instance().send(&m_serverConnection, packet);

			break;
		}
	//////////////////////////////////////////////////////////////////////////////

	default:
		break;
	}


	LoggerNetwork::get_instance().logConsole(LoggerNetwork::LOG_SENDER::CLIENT,
			LoggerNetwork::LOG_PACKET_DATATRANSFER::PACKET_SENT,
			packetCode);
}

void NetworkManagerClient::sendPacket(Packet::UDPPacket _type) {
	int packetCode = Packet::toInt(_type);
	PacketSharedPtr packet(new sf::Packet());
	*packet << packetCode;
	unsigned short port{ Packet::Port_UDP_Server };

	switch (_type) {
	//////////////////////////////////////////////////////////////////////////////
	case Packet::UDPPacket::DATA_PLAYER: {
		Player::EncodedPlayerData playerData = m_client.getPlayer()->encodeData();
		*packet << playerData;

		PacketSender::get_instance().send(&m_udpSocket, packet, m_serverConnection.getRemoteAddress(), port);

		break;
	}
	//////////////////////////////////////////////////////////////////////////////
	}
}


void NetworkManagerClient::receiveTCPPackets() {
	int packetCode;
	PacketUPtr packet(new sf::Packet());

	if (m_serverConnection.receive(*packet) == sf::Socket::Status::Done) {
		*packet >> packetCode;
		Packet::TCPPacket packetType{Packet::toTCPType(packetCode)};
		LoggerNetwork::get_instance().logConsole(LoggerNetwork::LOG_SENDER::CLIENT,
				LoggerNetwork::LOG_PACKET_DATATRANSFER::PACKET_RECEIVED,
				packetCode);

		switch(packetType) {
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

				World::EncodedWorldData worldData;

				*packet >> worldData;

				//We won't want to parse the world data
				//if we're local, since the world will
				//be a shared pointer anyway, and re-parsing
				//will involve calling addChunks and mess up
				//the world by un-randomizing it
				if(!m_client.isLocal()) {
					m_client.parseWorldData(worldData);
				}

				stringIDsToVector(worldData.chunkIDs);
				setChunkDataProcessed(false);
				break;
			}
		//////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////////
		case Packet::TCPPacket::CHAT_MESSAGE: {
				std::string message;
				std::string sender;
				*packet >> message;
				*packet >> sender;

				m_lastReceivedMessage = std::make_pair(message, sender);
				break;
			}
		//////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////////
		case Packet::TCPPacket::RESPAWN_PLAYER: {
				m_client.respawnPlayer();
				break;
			}
		//////////////////////////////////////////////////////////////////////////////


		default:
			break;
		}
	}
}

void NetworkManagerClient::receiveUDPPackets() {
	static sf::Clock clock{};
    static int receivedCount{0};
    if(clock.getElapsedTime().asSeconds() >= 1) {
		std::cout << "[CLIENT] UDP packets received: " << receivedCount << std::endl;
		std::cout << "-----------------------------------------------" << std::endl;
		receivedCount = 0;
		clock.restart();
    }
    //--------------------------------------------------------------------------
	int packetCode;
	PacketUPtr packet(new sf::Packet());
	unsigned short port{ m_udpSocket.getLocalPort() };

	sf::IpAddress address{ m_serverConnection.getRemoteAddress() };

	while (m_udpSocket.receive(*packet, address, port) == sf::Socket::Status::Done) {
		receivedCount++;
		*packet >> packetCode;
		Packet::UDPPacket packetType{ Packet::toUDPType(packetCode) };
		LoggerNetwork::get_instance().logConsole(LoggerNetwork::LOG_SENDER::CLIENT,
			LoggerNetwork::LOG_PACKET_DATATRANSFER::PACKET_RECEIVED,
			packetCode);

		switch (packetType) {
		//////////////////////////////////////////////////////////////////////////////
		case Packet::UDPPacket::DATA_PLAYER: {
			Player::EncodedPlayerData playerData;
			*packet >> playerData;

			m_client.updatePlayer(playerData);
			break;
		}
		//////////////////////////////////////////////////////////////////////////////
		}
	}
}

void NetworkManagerClient::connect(const sf::IpAddress& _ip, unsigned short _port) {
	bool successfullyConnected{false};

	m_serverConnection.setBlocking(true);
	sf::Socket::Status status = m_serverConnection.connect(_ip, _port);

	if(status != sf::Socket::Done) {
		LoggerNetwork::get_instance().log(LoggerNetwork::LOG_SENDER::CLIENT,
										  LoggerNetwork::LOG_MESSAGE::CONNECTION_WAITING);
	}
	else {
		LoggerNetwork::get_instance().log(LoggerNetwork::LOG_SENDER::CLIENT,
										  LoggerNetwork::LOG_MESSAGE::CONNECTION_SUCCESS);
		successfullyConnected = true;
	}

	m_serverConnection.setBlocking(false);

	if(successfullyConnected){
		sendPacket(Packet::TCPPacket::JUSTJOINED);
	}
}

bool NetworkManagerClient::connectionActive() const {
	bool invalidHost { m_serverConnection.getRemoteAddress() == sf::IpAddress::None };
	return m_connectionActive && !invalidHost;
}

bool NetworkManagerClient::receivedMessage(std::pair<std::string, std::string>* _ptr) {
	bool valid {m_lastReceivedMessage.first != ""};

	if(_ptr != nullptr) {
		*_ptr = m_lastReceivedMessage;
	}

	return valid;
}

void NetworkManagerClient::clearLastReceivedMessage() {
	m_lastReceivedMessage.first = "";
	m_lastReceivedMessage.second = "";
}


bool NetworkManagerClient::chunkDataReceived(std::vector<int>* _ids) const {
	if(_ids != nullptr) {
		*_ids = m_lastReceivedChunks;
	}

	return m_chunkDataReceived;
}

void NetworkManagerClient::setChunkDataProcessed(bool _val) {
	m_chunkDataReceived = !_val;
}

void NetworkManagerClient::update() {
	PacketSender::get_instance().update();
}


void NetworkManagerClient::stringIDsToVector(std::string _ids) {
	m_lastReceivedChunks.clear();

	std::replace(_ids.begin(), _ids.end(), '%', ' ' );

	std::stringstream ss(_ids);
	std::copy(	std::istream_iterator<int>(ss),
				std::istream_iterator<int>(),
				back_inserter(m_lastReceivedChunks));
}
