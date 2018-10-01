#ifndef PROGRAMSTATE_PAUSE_H_INCLUDED
#define PROGRAMSTATE_PAUSE_H_INCLUDED

#include "ProgramState_Menu.h"

class ProgramState_Pause : public ProgramState_Menu {
    public:
        ProgramState_Pause(Program& _program);

        void getInput(sf::Event& _event);
};

#endif // PROGRAMSTATE_PAUSE_H_INCLUDED
