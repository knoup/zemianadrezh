#ifndef BLOCKDATA_H_INCLUDED
#define BLOCKDATA_H_INCLUDED

#include <string>
#include <SFML/Graphics.hpp>

constexpr int BLOCK_DIMENSIONS_X = 16;
constexpr int BLOCK_DIMENSIONS_Y = 16;

constexpr int CHUNK_DIMENSIONS_X = 32;
constexpr int CHUNK_DIMENSIONS_Y = 32;

/*
BlockData contains all the information we need to know
about a certain block type.

assets/gfx/blocks.png is a texture atlas that contains all the
block textures. Every row will contain 10 textures, one for
each possible border, as follows:

(the number represents the block in the order it should appear
in its row, ~ represents the border)

  ~
~ 0 ~
  ~

  ~ ~ ~
~ 1 2 3 ~
~ 4 5 6 ~
~ 7 8 9 ~
  ~ ~ ~

*/

class BlockData {
  public:
  	enum BorderType
  	{
  		ALL           = 0,
		TOP_LEFT      = 1,
		TOP           = 2,
		TOP_RIGHT     = 3,
		LEFT          = 4,
		NONE          = 5,
		RIGHT         = 6,
		BOTTOM_LEFT   = 7,
		BOTTOM        = 8,
		BOTTOM_RIGHT  = 9
  	};

	enum Type
	{
		AIR  = 0,
		DIRT = 1
	};

	//Note:
	//The opacity parameter in the constructor is taken in percentage
	//because it makes more sense to specify it that way. Before
	//m_opacity is set, that percentage is converted to the actual
	//alpha value (0-255).

	BlockData(std::string _name, int _opacity, sf::Vector2f _textureIndeces);

	const std::string& getName() const;
	int                getOpacityAlpha() const;
	sf::Vector2f       getTextureIndeces() const;

  private:
	//Data members --------------------------------
	std::string  m_name;
	int          m_opacityAlpha;
	sf::Vector2f m_textureIndeces;
	//---------------------------------------------
};

#endif // BLOCKDATA_H_INCLUDED
