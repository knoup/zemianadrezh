#ifndef PLAYERTAG_H_INCLUDED
#define PLAYERTAG_H_INCLUDED

//Just an identifier class used to retrieve Player entities
//Additionally, m_local can be used to specify if the Player
//is a local player (the one the user controls), or another
//remotely connected player

class PlayerTag final {
  public:
	PlayerTag(bool _isLocal = false) : m_local{_isLocal} {};
	PlayerTag& operator=(const PlayerTag&) noexcept {
		return *this;
	};

	bool m_local;
};

#endif // PLAYERTAG_H_INCLUDED
