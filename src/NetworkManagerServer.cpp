#include "NetworkManagerServer.h"

#include <iostream>

#include "LoggerNetwork.h"

#include "Server.h"

NetworkManagerServer::NetworkManagerServer(Server& _server)
	: m_server(_server),
	  m_listener() {

	m_listener.setBlocking(false);
	listen();
}

void NetworkManagerServer::sendPacket(Packet::Type _type, sf::TcpSocket* _recipient, bool _exclude) {
	int packetCode = Packet::toInt(_type);

	std::vector<sf::TcpSocket*> recipients;

	if(_recipient == nullptr) {
		for(auto& client : m_clientConnections) {
			recipients.push_back(client.get());
		}
	}
	else {
		if(!_exclude) {
			recipients.push_back(_recipient);
		}
		else {
			for(auto& client : m_clientConnections) {
				if(client.get() != _recipient) {
					recipients.push_back(client.get());
				}
			}
		}
	}

	sf::Packet packet;
	packet << packetCode;

	switch(_type) {
	//////////////////////////////////////////////////////////////////////////////
	case Packet::Type::DATA_WORLD: {

		World::EncodedWorldData worldData = m_server.getWorld().encodeData();

		for(auto& recipient : recipients) {
			packet << worldData.chunkIDs;
			packet << worldData.invisibleBlocks;
			recipient->send(packet);
			LoggerNetwork::get_instance().logConsole(LoggerNetwork::LOG_SENDER::SERVER,
					LoggerNetwork::LOG_PACKET_DATATRANSFER::PACKET_SENT,
					packetCode);

		}

		break;
	}
	//////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////
	case Packet::Type::DATA_PLAYER: {

		for(auto& recipient : recipients) {
			for(auto& player : *m_server.getOtherPlayers()) {
				Player::EncodedPlayerData playerData = player->encodeData();

				packet << playerData.playerName;

				//Here we do a quick check to see if the playerdata generated
				//belongs to the recipient's player; if it does, we'll cancel
				//and not redundantly send it back to them
				auto i = m_clientNames.find(playerData.playerName);
				if(i != m_clientNames.end()) {
					if(i->second == recipient) {
						continue;
					}
				}

				packet << playerData.facingLeft;
				packet << playerData.speed;
				packet << playerData.positionX;
				packet << playerData.positionY;

				recipient->send(packet);
				LoggerNetwork::get_instance().logConsole(LoggerNetwork::LOG_SENDER::SERVER,
						LoggerNetwork::LOG_PACKET_DATATRANSFER::PACKET_SENT,
						packetCode);
			}
		}

		break;
	}
		//////////////////////////////////////////////////////////////////////////////
	}
}

void NetworkManagerServer::receivePacket() {
	int packetCode;
	sf::Packet packet;

	for(auto& connection : m_clientConnections) {
		if(connection->receive(packet) == sf::Socket::Status::Done) {
			packet >> packetCode;
			Packet::Type packetType{Packet::toType(packetCode)};
			LoggerNetwork::get_instance().logConsole(LoggerNetwork::LOG_SENDER::SERVER,
					LoggerNetwork::LOG_PACKET_DATATRANSFER::PACKET_RECEIVED,
					packetCode);

			switch(packetType) {
			//////////////////////////////////////////////////////////////////////////////
			case Packet::Type::REQUEST_WORLD: {
				sendPacket(Packet::Type::DATA_WORLD, connection.get());
				break;
			}
			//////////////////////////////////////////////////////////////////////////////

			//////////////////////////////////////////////////////////////////////////////
			case Packet::Type::DATA_PLAYER: {
				Player::EncodedPlayerData playerData;

				packet >> playerData.playerName;
				packet >> playerData.facingLeft;
				packet >> playerData.speed;
				packet >> playerData.positionX;
				packet >> playerData.positionY;

				m_clientNames.insert(std::make_pair(playerData.playerName, connection.get()));

				m_server.updateOtherPlayers(playerData);

				sendPacket(Packet::Type::DATA_PLAYER, connection.get());
				break;
			}
			//////////////////////////////////////////////////////////////////////////////
			default:
				break;
			}
		}
	}
}

const std::vector<std::unique_ptr<sf::TcpSocket>>& NetworkManagerServer::getClients() {
	return m_clientConnections;
}

void NetworkManagerServer::listen() {
	if(m_listener.listen(7777) != sf::Socket::Done) {
		LoggerNetwork::get_instance().log(LoggerNetwork::LOG_SENDER::SERVER,
										  LoggerNetwork::LOG_MESSAGE::LISTEN_PORT_FAILURE);
	} else {
		LoggerNetwork::get_instance().log(LoggerNetwork::LOG_SENDER::SERVER,
										  LoggerNetwork::LOG_MESSAGE::LISTEN_PORT_SUCCESS);
	}
}

void NetworkManagerServer::accept() {
	m_clientConnections.push_back(std::unique_ptr<sf::TcpSocket>(new sf::TcpSocket()));
	m_clientConnections.back()->setBlocking(false);

	sf::Socket::Status status = m_listener.accept(*m_clientConnections.back());
	if(status != sf::Socket::Done) {
		//LoggerNetwork::get_instance().log(LoggerNetwork::LOG_SENDER::SERVER,
										  //LoggerNetwork::LOG_MESSAGE::CONNECTION_FAILURE);
		m_clientConnections.pop_back();
	}
	else {
		if (m_clientConnections.back()->getRemoteAddress() != sf::IpAddress::LocalHost
			&&
			!m_server.connectionsAllowed()){
				LoggerNetwork::get_instance().log(LoggerNetwork::LOG_SENDER::SERVER,
										  LoggerNetwork::LOG_MESSAGE::CONNECTION_BLOCKED);
				m_clientConnections.pop_back();
				return;
			}

		LoggerNetwork::get_instance().log(LoggerNetwork::LOG_SENDER::SERVER,
										  LoggerNetwork::LOG_MESSAGE::CONNECTION_SUCCESS);

	}
}
