#include "ProgramState_SPPlay.h"

#include "Keybinds.h"
#include "LoggerNetwork.h"

ProgramState_SPPlay::ProgramState_SPPlay(Program& _program)
    : 	ProgramState(_program),
       m_localServer(false),
       m_client(*m_program.m_window),
       m_rendererChunk(*m_program.m_window),
       m_rendererPlayer(*m_program.m_window),
       m_rendererChatbox(*m_program.m_window),
       m_view{sf::FloatRect(0, 0,
                            float(m_program.m_window->getSize().x),
                            float(m_program.m_window->getSize().y))} {

    /*
    The reason the client calls connect() twice is because of the way
    SFML connection statuses work: the client won't return
    sf::Socket::Done unless it's called AFTER a connection with the server
    has been estalished.

    Therefore, we'll call connect the first time in order for the log entry
    to register "awaiting response...", then a second time after the
    connection is established for "connection established!"

    See: https://en.sfml-dev.org/forums/index.php?topic=7118.0
    */

    m_client.m_networkManager.connect(sf::IpAddress::LocalHost, 7777);
    m_localServer.m_networkManager.accept();
    m_client.m_networkManager.connect(sf::IpAddress::LocalHost, 7777);

    m_client.m_networkManager.sendPacket(Packet::Type::REQUEST_WORLD);
    m_localServer.m_networkManager.receivePacket();
    m_client.m_networkManager.receivePacket();

    m_client.respawnPlayer();

    auto worldData = m_client.getWorld().getChunks();
    for(auto& chunk : worldData) {
        m_rendererChunk.update(&chunk);
    }

    m_rendererPlayer.addObject(m_client.getPlayer());
    m_rendererChatbox.addObject(m_client.getChatBox());
}

ProgramState_SPPlay::~ProgramState_SPPlay() {

}

void ProgramState_SPPlay::getInput(sf::Event& _event) {
    ProgramState::getInput(_event);

    if(_event.type == sf::Event::KeyPressed) {
		if(_event.key.code == Key::PAUSE_KEY){
			m_program.pushState_Pause();
			return;
		}
	}

    m_client.getInput(_event);
}

void ProgramState_SPPlay::update() {
    m_view.setCenter(m_client.getPlayer()->getPosition());

    m_client.sendPackets();
    m_localServer.receivePackets();
    m_client.receivePackets();

    m_client.update();
    m_localServer.update();
}

void ProgramState_SPPlay::draw() {
    m_program.m_window->setView(m_view);
    m_rendererPlayer.draw();
    m_rendererChunk.draw();
    m_rendererChatbox.draw();
}

void ProgramState_SPPlay::onResize(sf::Vector2u _newSize){
    ProgramState::onResize(_newSize);
    sf::Vector2f newSizeF{float(_newSize.x), float(_newSize.y)};
    m_view.setSize(newSizeF);
}
