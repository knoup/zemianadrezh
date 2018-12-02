#ifndef NETWORKMANAGERSERVER_H_INCLUDED
#define NETWORKMANAGERSERVER_H_INCLUDED

#include <memory>
#include <map>

#include "SFML/Network.hpp"

#include "PacketTypes.h"
#include "PacketSender.h"

class Server;

class NetworkManagerServer {
	public:
		NetworkManagerServer(Server& _server);

		//The second and third arguments here are optional.
		//Depending on whether _exclude is true or false,
		//the packet will either be sent to everyone but the
		//specificed recipient (true), or only include the specified
		//recipient (false, default).
		void sendPacket(Packet::TCPPacket _type,
						sf::TcpSocket* _recipient = nullptr,
						bool _exclude = false);

		void sendPacket(Packet::UDPPacket _type,
						sf::IpAddress _recipient = sf::IpAddress::None,
						bool _exclude = false);

		void receiveTCPPackets();
		void receiveUDPPackets();

		//Sends a message to all connected players
		void sendMessage(std::string _message, std::string _sender);

		const std::vector<std::unique_ptr<sf::TcpSocket>>& getClients();

		void listen();
		void accept();
		void update();
	private:
		//Since the clients that connect to us via UDP will have varying
		//port, we'll use IPInfo to conveniently store it alongside their IP
		struct IPInfo {
			IPInfo(sf::IpAddress _i,
				unsigned short _p) :
				ipAddress{ _i },
				port{ _p }{};

			sf::IpAddress ipAddress;
			unsigned short port;
		};

		Server& m_server;
		sf::TcpListener m_listener;
		sf::UdpSocket m_udpSocket;
		std::vector<std::unique_ptr<sf::TcpSocket>> m_clientConnections;
		std::map<std::string, IPInfo> m_clientIPs;
		std::vector<std::pair<std::string, std::string>> m_messages;
};


#endif // NETWORKMANAGERSERVER_H_INCLUDED
