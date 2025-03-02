#pragma once

#include <memory>
#include <functional>

#include <game/game_state.hpp>
#include <game/input.hpp>
#include <rendering/camera.hpp>

#include <ui/core.hpp>
#include <ui/elements.hpp>


struct GameModeState{
    GameState& game_state;
    WireframeCubeRenderer& wireframe_renderer;
    KeyInputManager<ControllActions>& input_manager;
    const PerspectiveCamera& camera;
    const std::function<void(const std::string& name)>& setUILayer;
    const std::function<bool(const std::string& name)>& isActiveLayer;
};

class GameMode{
    private:
        std::unordered_map<std::string, std::shared_ptr<UILayer>> ui_layers;
        std::string name;

        std::string getLocalLayerName(const std::string& name);

    protected:
        UILayer& getLayerLocal(const std::string& name);
        bool isActiveLayerLocal(const std::string& name, GameModeState& state);

    public:
        GameMode(const std::string& name);

        virtual void KeyEvent(GameModeState& state, int key, int scancode, int action, int mods) = 0;
        virtual void MouseEvent(GameModeState& state, int button, int action, int mods) = 0;
        virtual void MouseMoved(GameModeState& state, int xoffset, int yoffset) = 0;
        virtual void MouseScroll(GameModeState& state, double xoffset, double yoffset) = 0;
};

