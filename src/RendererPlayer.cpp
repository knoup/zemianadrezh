#include "RendererPlayer.h"


RendererPlayer::RendererPlayer(sf::RenderWindow& _window)
    : m_window(_window),
      m_view{sf::FloatRect(0, 0, float(m_window.getSize().x), float(m_window.getSize().y))} {
}

void RendererPlayer::addPlayer(const Player* _player) const {

    auto it = std::find(m_players.begin(), m_players.end(), _player);

    if(it == m_players.end()) {
        m_players.push_back(_player);
    }
}

void RendererPlayer::update(){
    if(!m_players.empty()){
        m_view.setCenter(m_players[0]->getPosition());
    }
}

void RendererPlayer::draw() const {
	m_window.setView(m_view);

    for(auto& player : m_players) {
        m_window.draw(*player);
    }
}
