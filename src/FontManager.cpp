#include "FontManager.h"

FontManager::FontManager() {
	sf::Font andy;

	andy.loadFromFile("assets/fonts/andy.ttf");
	m_fonts.emplace(Type::ANDY, andy);
}

const sf::Font& FontManager::getFont(Type _type) {
	return m_fonts[_type];
}

float FontManager::getLineSpacing(Type _type, unsigned int _characterSize) {
	return m_fonts[_type].getLineSpacing(_characterSize);
}
