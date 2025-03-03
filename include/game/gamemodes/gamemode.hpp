#pragma once

#include <memory>
#include <functional>

#include <game/game_state.hpp>
#include <game/input.hpp>
#include <game/terrain_manager.hpp>
#include <scene.hpp>

#include <rendering/camera.hpp>

#include <ui/core.hpp>
#include <ui/elements.hpp>

class Chunk;

struct GameModeState{
    GameState* game_state;
    WireframeCubeRenderer& wireframe_renderer;
    KeyInputManager<ControllActions>& input_manager;
    const PerspectiveCamera& camera;
    TerrainManager& terrain_manager;
    Scene& scene;
    std::function<void(Chunk* chunk, const glm::ivec3 position)> regenerateChunkMesh;
};

#define UNWRAP_PROP(name,type) type name = state.name

#define UNWRAP_STATE() \
UNWRAP_PROP(game_state         , auto*); \
UNWRAP_PROP(wireframe_renderer , auto&);\
UNWRAP_PROP(input_manager      , auto&);\
UNWRAP_PROP(camera             , const auto&);\
UNWRAP_PROP(terrain_manager    , auto&);\
UNWRAP_PROP(scene              , auto&);\
UNWRAP_PROP(regenerateChunkMesh, auto&);\

class GameMode{
    private:
        std::string name;
        std::string getLocalLayerName(const std::string& name);

    protected:
        std::string base_layer_override = "";
        GameModeState& state;

        UILayer& getLayerLocal(const std::string& name);
        bool isActiveLayerLocal(const std::string& name);
        bool IsBaseLayerActive();
        
    public:
        GameMode(GameModeState& state, const std::string& name);

        UILayer& GetBaseLayer();

        virtual bool NoClip() { return false; }

        virtual void Initialize() = 0;

        virtual void Open() = 0;
        virtual void Render(double deltatime) = 0;
        virtual void PhysicsUpdate() = 0;

        virtual void KeyEvent(int key, int scancode, int action, int mods) = 0;
        virtual void MouseEvent(int button, int action, int mods) = 0;
        virtual void MouseMoved(int xoffset, int yoffset) = 0;
        virtual void MouseScroll(double xoffset, double yoffset) = 0;
};

