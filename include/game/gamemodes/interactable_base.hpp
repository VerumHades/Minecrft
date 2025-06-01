#pragma once

#include <game/gamemodes/gamemode.hpp>
#include <atomic>

/**
 * @brief An intermidiate gamemode that provides the cursor
 * 
 */
class GameModeInteractable: public GameMode{
    protected:
        struct CursorState{
            glm::ivec3 blockUnderCursorPosition = {0,0,0};
            glm::vec3 blockUnderCursorEmpty = {0,0,0}; // Block before the selected block, air where a block will be placed
            Block* blockUnderCursor = nullptr;
        } cursor_state;

        glm::vec3 lastPlayerPosition = {0,0,0};

        std::function<void()> onCursorTargetChange;
        std::atomic<bool> pending_cursor_update;
        
        void UpdateCursor();

    public:
        GameModeInteractable(GameModeState& state, const std::string& name): GameMode(state,name) {}
        virtual void PhysicsUpdate(double deltatime) override;
        virtual void Render(double deltatime) override;
};