#include "WorldChunk.h"

#include <random>
#include <chrono>

#include <SFML/Network.hpp>
#include "Util/Coordinates.h"

//Packet operator overloading
//----------------------------------------------------------------------------------------------------------------
sf::Packet& operator<<(sf::Packet& _p, const WorldChunk::EncodedChunkData& _d) {
	return _p << _d.id << _d.blocks;
}

sf::Packet& operator>>(sf::Packet& _p, WorldChunk::EncodedChunkData& _d) {
	return _p >> _d.id >> _d.blocks;
}
//----------------------------------------------------------------------------------------------------------------

WorldChunk::WorldChunk(int _id, bool _empty) : m_id{_id} {
	//index represents a block's position in the chunk.
	//First, the x axis is filled out, then the y, so
	//it looks like this:

	int index{0};

	for (int y{0}; y < CHUNK_DIMENSIONS_Y; y++) {
		for (int x{0}; x < CHUNK_DIMENSIONS_X; x++) {
			bool air{false};

			if (_empty) {
				air = true;
			}

			else {
				auto seed = std::chrono::high_resolution_clock::now()
				              .time_since_epoch()
				              .count();
				static std::mt19937                mt_rand(seed);
				std::uniform_int_distribution<int> uniform_dist(1, 20);
				int                                mean = uniform_dist(mt_rand);

				if (mean == 1) {
					air = true;
				}
			}

			BlockData::Type t;
			if (!air) {
				t = BlockData::Type::DIRT;
			}
			else {
				t = BlockData::Type::AIR;
			}

			Block block{index, t};

			++index;

			m_blocks.push_back(block);
		}
	}

	adjustBorders();
}

int WorldChunk::getID() const {
	return m_id;
}

const std::vector<Block>& WorldChunk::getBlocks() const {
	return m_blocks;
}

void WorldChunk::setBlockType(int _id, BlockData::Type _t) {
	m_blocks[_id].setType(_t);
}

const WorldChunk::EncodedChunkData WorldChunk::encodeData() const {
	/*
	chunkID will contain the chunk's ID

	blocks will contain both the type (BlockData::Type enum)
	and position of all non-air (non-0) blocks in the chunk,
	with the block types delimited by , and positions delimited
	by .
	For example:

	1,14.1,156.1,332.1,745.1,1020

	All the blocks in the example are of type 1, which
	corresponds to BlockData::Type::DIRT, so they will be dirt blocks.
	*/

	EncodedChunkData data;

	data.id = sf::Uint16(m_id);

	auto blocks = getBlocks();

	for (size_t z{0}; z < blocks.size(); z++) {
		int type{blocks[z].getType()};
		if (type != 0) {
			data.blocks += std::to_string(type);
			data.blocks += ",";
			data.blocks += std::to_string(z);
			data.blocks += ".";
		}
	}

	return data;
}

void WorldChunk::parseData(const WorldChunk::EncodedChunkData& _data) {
	m_id = _data.id;
	std::string currentNumber;

	int         currentType;
	std::string currentTypeStr{""};
	bool        readingType{true};

	for (const char& c : _data.blocks) {
		if (c == ',') {
			readingType = false;
			currentType = std::stoi(currentTypeStr);
			currentTypeStr.clear();
		}

		else if (c == '.') {
			int id = std::stoi(currentNumber);
			currentNumber.clear();
			setBlockType(id, BlockData::Type(currentType));
			readingType = true;
		}

		else if (readingType) {
			currentTypeStr += c;
		}

		else if (!readingType) {
			currentNumber += c;
		}
	}

	adjustBorders();
}

std::map<Direction, Block*> WorldChunk::getNeighboringBlocks(Block* _b) {
	static const sf::Vector2i dim{CHUNK_DIMENSIONS_X, CHUNK_DIMENSIONS_Y};
	std::map<Direction, Block*> result{};

	const auto position{_b->getPosition()};

	sf::Vector2i northPosition{position.x, position.y - 1};
	sf::Vector2i southPosition{position.x, position.y + 1};
	sf::Vector2i eastPosition{position.x + 1, position.y};
	sf::Vector2i westPosition{position.x - 1, position.y};

	result.insert({Direction::NORTH, nullptr});
	result.insert({Direction::SOUTH, nullptr});
	result.insert({Direction::EAST, nullptr});
	result.insert({Direction::WEST, nullptr});

	if(!Utility::Coordinates::coordinatesOutOfRange(northPosition, dim)) {
		int i {Utility::Coordinates::coordinatesToIndex(northPosition, dim.x)};
		if(m_blocks[i].getType() != BlockData::Type::AIR){
			result[Direction::NORTH] = &m_blocks[i];
		}
	}

	if(!Utility::Coordinates::coordinatesOutOfRange(southPosition, dim)) {
		int i {Utility::Coordinates::coordinatesToIndex(southPosition, dim.x)};
		if(m_blocks[i].getType() != BlockData::Type::AIR){
			result[Direction::SOUTH] = &m_blocks[i];
		}
	}

	if(!Utility::Coordinates::coordinatesOutOfRange(eastPosition, dim)) {
		int i {Utility::Coordinates::coordinatesToIndex(eastPosition, dim.x)};
		if(m_blocks[i].getType() != BlockData::Type::AIR){
			result[Direction::EAST] = &m_blocks[i];
		}
	}

	if(!Utility::Coordinates::coordinatesOutOfRange(westPosition, dim)) {
		int i {Utility::Coordinates::coordinatesToIndex(westPosition, dim.x)};
		if(m_blocks[i].getType() != BlockData::Type::AIR){
			result[Direction::WEST] = &m_blocks[i];
		}
	}

	return result;
}

//This function determines each block's border type.
//It does so by ruling out, one by one, the border
//types the block CANNOT have, depending on its
//neighbors, until it is left with one final
//result.

void WorldChunk::adjustBorders() {
	//The 16 types correspond to
	//BlockData::BorderType
	std::vector<int> possibleBorders;
	for(int i{0}; i <= 15; i++) {
		possibleBorders.push_back(i);
	}

	for(auto& block : m_blocks) {
		if(block.getType() == BlockData::Type::AIR) {
			continue;
		}

		auto neighbors{getNeighboringBlocks(&block)};
		auto borders{possibleBorders};

		if(neighbors[Direction::NORTH] != nullptr) {
			borders.erase(std::remove_if(
			borders.begin(), borders.end(),
			[](const int& x) {
				return x == 10
				|| (x >= 0 && x <= 3)
				|| (x >= 13 && x <= 15);
			}), borders.end());
		}
		else {
			borders.erase(std::remove_if(
			borders.begin(), borders.end(),
			[](const int& x) {
				return x == 11
				|| x == 12
				|| (x >= 4 && x <= 9);
			}), borders.end());
		}

		if(neighbors[Direction::SOUTH] != nullptr) {
			borders.erase(std::remove_if(
			borders.begin(), borders.end(),
			[](const int& x) {
				return x == 0
				|| x == 12
				|| x == 14
				|| x == 15
				|| (x >= 7 && x <= 10);
			}), borders.end());
		}
		else {
			borders.erase(std::remove_if(
			borders.begin(), borders.end(),
			[](const int& x) {
				return x == 11
				|| x == 13
				|| (x >= 1 && x <= 6);
			}), borders.end());
		}

		if(neighbors[Direction::EAST] != nullptr) {
			borders.erase(std::remove_if(
			borders.begin(), borders.end(),
			[](const int& x) {
				return x == 0
				|| x == 3
				|| x == 6
				|| x == 9
				|| x == 15
				|| (x >= 11 && x <= 13);
			}), borders.end());
		}
		else {
			borders.erase(std::remove_if(
			borders.begin(), borders.end(),
			[](const int& x) {
				return x == 1
				|| x == 2
				|| x == 4
				|| x == 5
				|| x == 7
				|| x == 8
				|| x == 10
				|| x == 14;
			}), borders.end());
		}

		if(neighbors[Direction::WEST] != nullptr) {
			borders.erase(std::remove_if(
			borders.begin(), borders.end(),
			[](const int& x) {
				return x == 0
				|| x == 1
				|| x == 4
				|| x == 7
				|| (x >= 11 && x <= 14);
			}), borders.end());
		}
		else {
			borders.erase(std::remove_if(
			borders.begin(), borders.end(),
			[](const int& x) {
				return x == 2
				|| x == 3
				|| x == 5
				|| x == 6
				|| x == 15
				|| (x >= 8 && x <= 10);
			}), borders.end());
		}

		//By now, all but 1 border types should have been removed
		//from the borders vector.
		block.setBorderType(BlockData::BorderType(borders[0]));
	}
}
