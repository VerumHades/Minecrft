#pragma once

#include <memory>

#include <game/game_state.hpp>
#include <game/input.hpp>

#include <ui/core.hpp>
#include <ui/elements.hpp>

struct CursorState{
    glm::ivec3 blockUnderCursorPosition = {0,0,0};
    glm::vec3 blockUnderCursorEmpty = {0,0,0}; // Block before the selected block, air where a block will be placed
    Block* blockUnderCursor = nullptr;
};

struct GameModeState{
    GameState* state;
    CursorState* cursor_state;
    WireframeCubeRenderer* wireframe_renderer;
};

class GameMode{
    protected:
        std::shared_ptr<UILayer> ui_layer;
    
    public:
        GameMode(const std::string& name);

        virtual void KeyEvent(GameState* state, CursorState* cursor_state, WireframeCubeRenderer* wireframe_renderer, int key, int scancode, int action, int mods) = 0;
        virtual void MouseEvent(GameState* state, CursorState* cursor_state, WireframeCubeRenderer* wireframe_renderer, int button, int action, int mods) = 0;

        std::shared_ptr<UILayer>& getInterfaceLayer(){ return ui_layer; }
};

