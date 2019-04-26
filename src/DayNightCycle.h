#ifndef DAYNIGHTCYCLE_H_INCLUDED
#define DAYNIGHTCYCLE_H_INCLUDED

#include <SFML/Graphics.hpp>

#include "World.h"

/*
This class handles everything to do with the VISUAL effects of the day-night cycle, including:

-The sun/moon's position
-Shaders

Just give it a world, and it will handle everything else. It inherits from sf::Drawable so as
to decouple the aesthetic portions of the day-night cycle from everything else. Simply draw it.

Saving this for future reference:
https://csc.lsu.edu/~kooima/misc/cs594/final/part2.html
*/

class DayNightCycle : public sf::Drawable {
	public:
		DayNightCycle(const World& _world);

		void draw(sf::RenderTarget& target, sf::RenderStates states) const;
		//This function will deal with managing either the sun or the moon's
        //sprite and position, depending on the world time
        void update();

	private:
		const World& m_world;
        sf::Sprite m_sunMoonSprite;
        //m_skyBackground will simply contain four vertices at the corners of the
        //window. Shaders will determine the appropriate gradients depending on the
        //time of day and position of the sun in the sky.
        sf::VertexArray m_skyBackground;
        sf::Text m_timeText;
        //In order to avoid having a lot of extra code to handle resizing, we'll
        //simply update these values to the values of the width and height of
        //the RenderTarget passed into the draw function.
        mutable float m_targetWidth;
        mutable float m_targetHeight;
        mutable sf::Shader m_shader;

		void updateSkyVertices();
};

#endif // DAYNIGHTCYCLE_H_INCLUDED
