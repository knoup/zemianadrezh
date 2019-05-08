#include "GameInstance.h"

#include "TextureManager.h"

#include "Tags/PlayerTag.h"
#include "Components/ComponentAnimation.h"
#include "Components/ComponentDirection.h"
#include "Components/ComponentName.h"
#include "Components/ComponentPhysics.h"
#include "Components/ComponentPosition.h"

#include <iostream>
GameInstance::GameInstance() : m_world() {
}

const World& GameInstance::getWorld() const {
	return m_world;
}

/*
const std::shared_ptr<std::vector<std::shared_ptr<Player>>>
GameInstance::getPlayers() const {
    return m_players;
}
*/

const encodedChunks GameInstance::encodeWorldChunks() const {
	return m_world.encodeChunks();
}

void GameInstance::parseWorldChunk(const WorldChunk::EncodedChunkData& _data) {
	m_world.parseChunk(_data);
}

void GameInstance::updatePlayer(const ComponentsPlayer& _data) {
	auto view {m_registry.view<PlayerTag>()};
	for(auto& entity : view) {
		auto name {m_registry.get<ComponentName>(entity).m_name};
		if(name == _data.compName.m_name) {
			m_registry.replace<ComponentPhysics>(entity, _data.compVel);
			m_registry.replace<ComponentDirection>(entity, _data.compDir);
			m_registry.replace<ComponentPosition>(entity, _data.compPos);
			return;
		}
	}

	//If we got this far, that means no player with a matching name
	//was found. Let's create it.
	addPlayer(_data);
}

entt::entity GameInstance::getPlayer(std::string& _name) {
	auto view {m_registry.view<const PlayerTag>()};
	for(auto& entity : view) {
		auto name {m_registry.get<ComponentName>(entity).m_name};
		if(name == _name) {
			return entity;
		}
	}
}

void GameInstance::removePlayer(entt::entity _e) {
	m_registry.remove<PlayerTag>(_e);
	m_registry.remove<ComponentAnimation>(_e);
	m_registry.remove<ComponentName>(_e);
	m_registry.remove<ComponentPhysics>(_e);
	m_registry.remove<ComponentDirection>(_e);
	m_registry.remove<ComponentPosition>(_e);
}

void GameInstance::removePlayer(std::string& _name) {
	auto view = m_registry.view<PlayerTag>();
	for (auto& entity : view) {
		const auto name = m_registry.get<ComponentName>(entity).m_name;
		if (name == _name) {
			removePlayer(entity);
		}
	}
}

void GameInstance::addPlayer(const ComponentsPlayer& _data) {
	entt::entity e{m_registry.create()};
	m_registry.assign<ComponentAnimation>(
	  e,
	  TextureManager::get_instance().getTexture(TextureManager::Type::PLAYER));

	m_registry.assign<PlayerTag>(e);
	m_registry.assign<ComponentName>(e, _data.compName);
	m_registry.assign<ComponentPhysics>(e, _data.compVel);
	m_registry.assign<ComponentDirection>(e, _data.compDir);
	m_registry.assign<ComponentPosition>(e, _data.compPos);
}
