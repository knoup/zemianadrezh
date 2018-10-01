#ifndef PACKETSENDER_H_INCLUDED
#define PACKETSENDER_H_INCLUDED

#include <memory>
#include <SFML/Network.hpp>

#include "Singleton.h"

/*
When using non-blocking TcpSockets, it is important to send
the same packet object over again if TcpSocket.send() returns
sf::Socket::Status::Partial. We'll use this class as a wrapper
around all packet sending operations to ensure that packets
are being sent properly, even if it takes several frames to
do so.

See the footnote on the following page for a detailed explanation:
https://www.sfml-dev.org/tutorials/2.5/network-socket.php
*/

typedef std::unique_ptr<sf::Packet> PacketUPtr;

class PacketSender : public Singleton<PacketSender> {
    public:
        struct PacketData {
            PacketData(sf::TcpSocket* _socket, PacketUPtr _packet)
                :   m_socket{_socket},
                    m_packet{std::move(_packet)} {}

            sf::TcpSocket* m_socket;
            PacketUPtr m_packet;
        };

        PacketSender();

        void send(sf::TcpSocket* _socket, PacketUPtr _packet);
        void update();

    private:
        std::vector<PacketData> m_packetData;
};


#endif // PACKETSENDER_H_INCLUDED
