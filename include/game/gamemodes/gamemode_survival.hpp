#pragma once

#include <game/gamemodes/gamemode.hpp>

class GameModeSurvival: public GameMode{
    private:

    public:
        GameModeSurvival();

        void KeyEvent(GameState* state, CursorState* cursor_state, WireframeCubeRenderer* wireframe_renderer, int key, int scancode, int action, int mods) override;
        void MouseEvent(GameState* state, CursorState* cursor_state, WireframeCubeRenderer* wireframe_renderer, int button, int action, int mods) override;

        void updateCursor();
};