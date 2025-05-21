#pragma once

#include <memory>
#include <functional>

#include <game/game_state.hpp>
#include <game/input.hpp>
#include <game/terrain_manager.hpp>

#include <rendering/camera.hpp>
#include <rendering/cubes.hpp>
#include <rendering/wireframes.hpp>

#include <ui/core.hpp>
#include <ui/elements.hpp>


#include <scene.hpp>

class Chunk;

struct GameModeState{
    GameState* game_state;
    WireframeCubeRenderer& wireframe_renderer;
    CubeRenderer& cube_renderer;
    KeyInputManager<ControllActions>& input_manager;
    const PerspectiveCamera& camera;
    TerrainManager& terrain_manager;
    Scene& scene;
    std::function<void(Chunk* chunk, const glm::ivec3 position)> regenerateChunkMesh;
};


class GameMode{
    protected:
        GameModeState& state;

    private:
        std::string name;
        std::string getLocalLayerName(const std::string& name);

    protected:
        std::string base_layer_override = "";

        UILayer& getLayerLocal(const std::string& name);
        void setLayerLocal(const std::string& name);
        bool isActiveLayerLocal(const std::string& name);
        bool IsBaseLayerActive();
        
    public:
        GameMode(GameModeState& state, const std::string& name);

        UILayer& GetBaseLayer();

        virtual bool NoClip() { return false; }

        virtual void Initialize() = 0;

        virtual void Open() = 0;
        virtual void Render(double deltatime) = 0;
        virtual void PhysicsUpdate(double deltatime) = 0;

        virtual void KeyEvent(int key, int scancode, int action, int mods) = 0;
        virtual void MouseEvent(int button, int action, int mods) = 0;
        virtual void MouseMoved(int xoffset, int yoffset) = 0;
        virtual void MouseScroll(double xoffset, double yoffset) = 0;
};

