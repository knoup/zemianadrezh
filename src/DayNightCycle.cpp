#include "DayNightCycle.h"

#include "TextureManager.h"
#include "FontManager.h"

#include <iostream>

constexpr int DAY_BEGIN_HOUR {5};
constexpr int DAY_END_HOUR {19};

constexpr int NIGHT_BEGIN_HOUR {19};
constexpr int NIGHT_END_HOUR {5};

DayNightCycle::DayNightCycle(const World& _world)
: 	m_world{_world},
	m_sunMoonSprite{},
	m_skyBackground{ sf::PrimitiveType::Quads, 4 },
	m_timeText{},
	m_targetWidth{0},
	m_targetHeight{0},
	m_shader{}
{
	m_timeText.setFont(FontManager::get_instance().getFont(FontManager::Type::ANDY));
	m_timeText.setCharacterSize(30);
	m_timeText.setOutlineThickness(1);
	m_timeText.setOrigin(m_timeText.getGlobalBounds().width / 2,
						 m_timeText.getGlobalBounds().height / 2);

	//making the sky blue (temporary until shader is implemented properly)
	/*
	for(int i{0}; i < 4; ++i) {
		m_skyBackground[i].color = sf::Color(90,112,255);
	}
	*/

	if (!m_shader.loadFromFile("assets/shaders/cycle_vertex.glsl", "assets/shaders/cycle_fragment.glsl")){
		std::cout << "ERROR!" << std::endl;
	}

	m_shader.setUniform("sky", TextureManager::get_instance().getTexture(TextureManager::Type::CYCLE_SKY_GRADIENT));
	m_shader.setUniform("glow", TextureManager::get_instance().getTexture(TextureManager::Type::CYCLE_GLOW_GRADIENT));

	update();
}

void DayNightCycle::update() {
	//A draw call needs to be attempted at least once to set these values
	//before updating can begin
	if(m_targetHeight == 0 || m_targetWidth == 0) {
		return;
	}

	updateSkyVertices();

	static const double pi {std::atan(1)*4};
	World::WorldTime worldTime = m_world.getTime();

	m_timeText.setString(worldTime.getString());
	m_timeText.setPosition(m_targetWidth - 1.2 * m_timeText.getGlobalBounds().width, 0);

	int HOUR_START 	{DAY_BEGIN_HOUR};
	int HOUR_END 	{DAY_END_HOUR};

	if(worldTime.hours >= DAY_BEGIN_HOUR && worldTime.hours < DAY_END_HOUR) {
		m_sunMoonSprite.setTexture(TextureManager::get_instance().getTexture(TextureManager::Type::SUN));
	}
	else{
		HOUR_START 	= NIGHT_BEGIN_HOUR;
		HOUR_END 	= NIGHT_END_HOUR;
		m_sunMoonSprite.setTexture(TextureManager::get_instance().getTexture(TextureManager::Type::MOON));
	}

	//We're going to set the origin to be at the rightmost point horizontally and the
	//centermost point vertically. Later, when determining xPos, we add the width of the
	//sprite to our calculation. This is to ensure a seamless transition (at the exact
	//moment the sun is out of our view, the moon starts to come in, and vice versa)
	m_sunMoonSprite.setOrigin(	m_sunMoonSprite.getGlobalBounds().width,
								m_sunMoonSprite.getGlobalBounds().height / 2);

	//These two if statements make sure that everything runs smoothly in cases such as the following:
	//HOUR_START = 19
	//HOUR_END = 5
	//worldTime.hours = 0
	//In such a case, the algorithm will get confused and not work as it should because of the way
	//24h wrapping works. Adding 24 will fix the logic so that the correct number of hours is given
	if(HOUR_START > HOUR_END) {
        HOUR_END += 24;
	}
	if(worldTime.hours < HOUR_START) {
		worldTime.hours += 24;
	}

	//Get the "percentage" of how far along the X axis the sun should be (for example, at 12, it should be
	//exactly 50% of the way.
	//First, calculate the difference between the hours.
	//[For example, from 6 to 18, this would be 12]
	int difference {abs(HOUR_START - HOUR_END)};
	//Then, calculate the offset from the begin hour.
	//[For example, if we take 12, 12 - 6 = 6]
	int offset {abs(HOUR_START - worldTime.hours)};
	//We'll now be able to take into consideration the minutes. We simply divide
	//the world time in minutes by 60 to get the percentage value between our
	//current hour and the next hour. For example, 30 would yield 0.5.
	float minutesPercentage {worldTime.minutes / 60.f };

	//We can divide our minutesPercentage by the hour difference to get the value we should
	//increment our final result by.
	float minuteProgress {minutesPercentage / difference};

	//Finally, divide the offset by the difference to get the percentage value for the hours, then
	//increment it by the minutes percentage.
	double progressInPercent {double(offset) / double(difference)};
	progressInPercent += minuteProgress;

	//For the xPos, we're going to want to invert progressInPercent so that the sun/moon goes
	//from the east to the west and not vice versa
	//Also, since the sprite's X origin is at its rightmost point, we'll add the sprite's width to
	//the view size in our xPos calculation

	//We're also going to invert the sin result in yPos so the movement will be down->up->down, rather
	//than up->down->up
	double xPos {(1 - progressInPercent) * (m_targetWidth + m_sunMoonSprite.getGlobalBounds().width)};
	double yPos {(1 - sin(progressInPercent * pi)) * m_targetHeight / 3};
	//This adjustment was made with some experimenting to get it to look right
	yPos += m_targetHeight / 10;

	m_sunMoonSprite.setPosition(xPos, yPos);

	//In SFML,	 the origin (0,0) is at the top left.
	//In OpenGL, the origin (0,0) is in the middle.
	//We'll do a couple of things to account for this
	sf::Vector2f openGLCoordinates{ m_sunMoonSprite.getPosition() };
	//First, since the sprite's x origin is at its rightmost point, we'll
	//subtract half its width from the x coordinate.
	openGLCoordinates.x -= m_sunMoonSprite.getGlobalBounds().width / 2;
	//Next, we'll subtract half the screen width and add half the height
	//to get the equivalent point in the OpenGL coordinate system
	openGLCoordinates.x -= m_targetWidth / 2;
	openGLCoordinates.y += m_targetHeight/ 2;

	//TODO: further refine this
	//We're going to let the shader determine the appropriate gradient from two
	//files, sky.png and glow.png, that represent the sun's position in the sky
	//over time
	m_shader.setUniform("sunPosition", sf::Vector3f{openGLCoordinates.x, openGLCoordinates.y, 0.f});
	m_shader.setUniform("lightIntensity", float(sin(progressInPercent * pi)));

}

void DayNightCycle::draw(sf::RenderTarget& target, sf::RenderStates states) const {
	m_targetWidth = target.getSize().x;
	m_targetHeight = target.getSize().y;

	sf::RenderStates skyStates{&m_shader};
	target.draw(m_skyBackground, skyStates);
	//target.draw(m_skyBackground, states);

	target.draw(m_sunMoonSprite, states);
	target.draw(m_timeText, states);

}

void DayNightCycle::updateSkyVertices() {
	//In SFML,	 the origin (0,0) is at the top left.
	//In OpenGL, the origin (0,0) is in the middle.
	//Therefore, we'll need to tweak some stuff.
	//top left
	m_skyBackground[0].position = { -m_targetWidth / 2, m_targetHeight / 2 };
	m_skyBackground[0].texCoords = { -m_targetWidth / 2, m_targetHeight / 2 };
	//top right
	m_skyBackground[1].position = { m_targetWidth / 2, m_targetHeight / 2 };
	m_skyBackground[1].texCoords = { m_targetWidth / 2, m_targetHeight / 2 };
	//bottom right
	m_skyBackground[2].position = { m_targetWidth / 2, -m_targetHeight / 2 };
	m_skyBackground[2].texCoords = { m_targetWidth / 2, -m_targetHeight / 2 };
	//bottom left
	m_skyBackground[3].position = { -m_targetWidth / 2, -m_targetHeight / 2 };
	m_skyBackground[3].texCoords = { -m_targetWidth / 2, -m_targetHeight / 2 };
}
