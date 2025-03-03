#pragma once

#include <game/gamemodes/gamemode.hpp>

class GameModeInteractable: public GameMode{
    protected:
        struct CursorState{
            glm::ivec3 blockUnderCursorPosition = {0,0,0};
            glm::vec3 blockUnderCursorEmpty = {0,0,0}; // Block before the selected block, air where a block will be placed
            Block* blockUnderCursor = nullptr;
        } cursor_state;
        
        void UpdateCursor();

    public:
        GameModeInteractable(GameModeState& state, const std::string& name): GameMode(state,name) {}
};