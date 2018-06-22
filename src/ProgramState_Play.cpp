#include "ProgramState_Play.h"

#include "LoggerNetwork.h"

ProgramState_Play::ProgramState_Play(Program& _program)
    : 	ProgramState(_program),
       m_localServer(),
       m_client(),
       m_rendererChunk(*m_program.m_window),
       m_rendererPlayer(*m_program.m_window),
       m_chatBox(*m_program.m_window, m_client.getPlayer()->getName()),
       m_view{sf::FloatRect(0, 0,
                            float(m_program.m_window->getSize().x),
                            float(m_program.m_window->getSize().y))} {

    m_client.m_networkManager.connect(sf::IpAddress::LocalHost, 7777);
    m_localServer.m_networkManager.accept();

    m_client.m_networkManager.sendPacket(Packet::Type::REQUEST_WORLD);
    m_localServer.m_networkManager.receivePacket();
    m_client.m_networkManager.receivePacket();

    m_client.respawnPlayer();

    auto worldData = m_client.getWorld().getChunks();
    for(auto& chunk : worldData) {
        m_rendererChunk.update(&chunk);
    }

    m_rendererPlayer.addObject(m_client.getPlayer());
}

ProgramState_Play::~ProgramState_Play() {

}

void ProgramState_Play::getInput(sf::Event& _event) {
    ProgramState::getInput(_event);

    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
        m_program.popState();
    }

    m_client.getInput();
    m_rendererPlayer.getInput(_event);
    m_chatBox.getInput(_event);
}

void ProgramState_Play::update() {
    m_view.setCenter(m_client.getPlayer()->getPosition());

    m_client.sendPackets();
    m_localServer.receivePackets();
    m_client.receivePackets();

    m_client.update();
    m_chatBox.update();
    m_localServer.update();
    m_rendererPlayer.update();
}

void ProgramState_Play::draw() {
    m_program.m_window->setView(m_view);
    m_rendererPlayer.draw();
    m_rendererChunk.draw();
    m_program.m_window->draw(m_chatBox);
}

void ProgramState_Play::onResize(sf::Vector2u _newSize){
    ProgramState_Play::onResize(_newSize);
    sf::Vector2f newSizeF{float(_newSize.x), float(_newSize.y)};
    m_view.setSize(newSizeF);
}
