#include "ProgramState_Play.h"

ProgramState_Play::ProgramState_Play(Program& _program)
    : 	ProgramState(_program),
		m_server(),
		m_rendererChunk(*m_program.m_window){

	auto worldChunks = m_server.getWorld().getChunks();

	for(auto& chunk : worldChunks){
		m_rendererChunk.update(&chunk);
	}
}

ProgramState_Play::~ProgramState_Play() {

}

void ProgramState_Play::getInput() {
	sf::Event event;

	while(m_program.m_window->pollEvent(event)){

		if(event.type == sf::Event::Closed){
			m_program.m_window->close();
		}

		if(event.type == sf::Event::KeyPressed){
			if(event.key.code == sf::Keyboard::Escape){
				m_program.m_window->close();
			}
		}

	}

}

void ProgramState_Play::update() {

}

void ProgramState_Play::draw() {
	m_rendererChunk.draw();
}
