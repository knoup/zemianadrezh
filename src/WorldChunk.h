#ifndef WORLDCHUNK_H_INCLUDED
#define WORLDCHUNK_H_INCLUDED

#include <vector>
#include <map>

#include "Block.h"
#include "Direction.h"

class WorldChunk {
  public:
	struct EncodedChunkData {
		EncodedChunkData(){};
		sf::Uint16  id;
		std::string blocks;
	};

	WorldChunk(int _id, bool _empty);

	int getID() const;

	const std::vector<Block>& getBlocks() const;
	void                      setBlockType(int _id, BlockData::Type _t);

	const EncodedChunkData encodeData() const;
	void                   parseData(const EncodedChunkData& _data);

  private:
  	//Functions -----------------------------------
  	std::map<Direction, Block*> getNeighboringBlocks(Block* _b);
  	//---------------------------------------------

	//Data members --------------------------------
	int                m_id;
	std::vector<Block> m_blocks{};
	//---------------------------------------------
};

//Forward declare Packet here and define the overloading in WorldChunk.cpp, so
//we don't have to include SFML/Networking.hpp in WorldChunk.h
namespace sf {
	class Packet;
}
sf::Packet& operator<<(sf::Packet& _p, const WorldChunk::EncodedChunkData& _d);
sf::Packet& operator>>(sf::Packet& _p, WorldChunk::EncodedChunkData& _d);

#endif // WORLDCHUNK_H_INCLUDED
