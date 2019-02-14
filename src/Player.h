#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "EntityMoving.h"
#include "Animation.h"

class Player : public EntityMoving, public sf::Drawable {
	public:
		struct EncodedPlayerData {
			std::string playerName;
			bool facingLeft;
			float velocityX;
			float velocityY;
			float positionX;
			float positionY;
		};
			
		Player(const std::string& _name);

		const std::string& getName() const;

		void getInput();
		void update(int _timeslice);
		void draw(sf::RenderTarget& target, sf::RenderStates states) const;

		const EncodedPlayerData encodeData() const;
		void parseData(const EncodedPlayerData& _data);
	private:
		sf::Sprite m_sprite;
		Animation m_animationSheet;
		std::string m_playerName;
		bool m_spawned{false};
};

//Forward declare Packet here and define the overloading in Player.cpp, so
//we don't have to include SFML/Networking.hpp in Player.h
namespace sf {
	class Packet;
}
sf::Packet& operator <<(sf::Packet& _p, const Player::EncodedPlayerData& _d);
sf::Packet& operator >>(sf::Packet& _p, Player::EncodedPlayerData& _d);
#endif // PLAYER_H_INCLUDED
