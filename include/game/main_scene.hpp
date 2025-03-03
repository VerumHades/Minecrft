#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#include <glm/gtc/random.hpp>

#include <rendering/opengl/buffer.hpp>
#include <rendering/opengl/shaders.hpp>
#include <rendering/opengl/texture.hpp>
#include <rendering/camera.hpp>
#include <rendering/wireframes.hpp>
#include <rendering/texture_registry.hpp>
#include <rendering/gbuffer.hpp>
#include <rendering/opengl/quad.hpp>
#include <rendering/ssao.hpp>

#include <parsing/block_loader.hpp>

#include <ui/core.hpp>
#include <ui/font.hpp>
#include <ui/loader.hpp>
#include <ui/form.hpp>
#include <ui/elements/selection.hpp>

#include <game/world/terrain.hpp>
#include <game/world/mesh_generation.hpp>
#include <game/entity.hpp>
#include <game/commands.hpp>
#include <game/threadpool.hpp>
#include <game/input.hpp>
#include <game/items/sprite_model.hpp>
#include <game/items/item.hpp>
#include <game/items/item_renderer.hpp>
#include <game/structure.hpp>
#include <game/game_state.hpp>
#include <game/items/crafting.hpp>
#include <game/terrain_manager.hpp>
#include <game/models/generic_model.hpp>

#include <game/gamemodes/gamemode.hpp>
#include <game/gamemodes/survival.hpp>
#include <game/gamemodes/structure_capture.hpp>

#include <indexing.hpp>
#include <path_config.hpp>

#include <scene.hpp>
#include <set>
#include <memory>
#include <random>
#include <atomic>
#include <queue>

class UIHotbar;
class UILoading;

/*
    Never make two instances of this class or weird things are going to happen!
*/
class MainScene: public Scene{
    private:
        PerspectiveCamera camera = PerspectiveCamera("player");
        DepthCamera suncam = DepthCamera("sun");

        SpiralIndexer3D indexer;

        GBuffer gBuffer = GBuffer(1920,1080);
        GLFullscreenQuad fullscreen_quad;

        ShaderProgram modelProgram = ShaderProgram("resources/shaders/graphical/model/model.vs","resources/shaders/graphical/model/model.fs");
        ShaderProgram terrainProgram = ShaderProgram("resources/shaders/terrain.vs","resources/shaders/terrain.fs");
        ShaderProgram skyboxProgram  = ShaderProgram("resources/shaders/graphical/skybox.vs", "resources/shaders/graphical/skybox.fs");
        ShaderProgram gBufferProgram = ShaderProgram("resources/shaders/graphical/deffered_shading/gbuffer.vs","resources/shaders/graphical/deffered_shading/gbuffer.fs");

        GLSkybox skybox{};
        std::unique_ptr<GameState> game_state;

        WireframeCubeRenderer wireframeRenderer{};
        
        TerrainManager terrain_manager{};

        std::unique_ptr<GLTextureArray> block_texture_array = nullptr;
        std::shared_ptr<UILoading> generation_progress;

        std::string worldPath = "saves";

        std::vector<std::shared_ptr<GameMode>> game_modes;
        int selected_game_mode = -1;

        std::atomic<bool> update_render_distance = false;

        GameModeState gamemodeState = {
            game_state.get(),
            wireframeRenderer,
            inputManager,
            camera,
            terrain_manager,
            *this,
            [this](Chunk* chunk, glm::ivec3 position){
                terrain_manager.regenerateChunkMesh(chunk, position);
                updateVisibility = 1;
            }
        };

        template <typename T, typename ...Args>
        void AddGameMode(Args... args){
            auto mode = std::make_shared<T>(gamemodeState,args...);
            int temp = selected_game_mode;
            
            selected_game_mode = game_modes.size();
            game_modes.push_back(mode);

            HandleGamemodeEvent(&GameMode::Initialize);
            selected_game_mode = temp;
        }

        template <typename FuncName, typename ...Args>
        void HandleGamemodeEventSelective(GameMode* game_mode, FuncName func, Args... args){
            (game_mode->*func)(args...);
        }

        template <typename FuncName, typename ...Args>
        void HandleGamemodeEvent(FuncName func, Args... args){
            if(selected_game_mode < 0 || static_cast<size_t>(selected_game_mode) >= game_modes.size()) return;
            
            HandleGamemodeEventSelective(game_modes.at(selected_game_mode).get(), func, args...);
        }

        GameMode* CurrentGameMode(){
            if(selected_game_mode < 0 || static_cast<size_t>(selected_game_mode) >= game_modes.size()) return nullptr;
            return game_modes.at(selected_game_mode).get();
        }

        UILayer& GetCurrentBaseLayer(){
            if(!CurrentGameMode()) return getUILayer("default");
            return CurrentGameMode()->GetBaseLayer();
        }

        void ResetToBaseLayer(){
            setUILayer(GetCurrentBaseLayer().name);
        }

        int renderDistance = 4;
        int selectedBlock = 4;

        bool allGenerated = false;
        bool running = false;
        int threadsStopped = 0;
        
        bool lineMode = false;
        bool menuOpen = false;
        //int sunDistance = ((CHUNK_SIZE * renderDistance) / 2) ;
        int sunDistance = 100;
        float sunAngle = 70.0f;
        Uniform<glm::vec3> sunDirUniform = Uniform<glm::vec3>("sun_direction");

        float camAcceleration = 50.00f;
        float camFOV = 90.0f;
        float maxFOV = 90.0f; 
        float minFOV = 2.0f;
        glm::vec3 camOffset = {0.3f,1.6f,0.3f};
        glm::ivec3 lastCamWorldPosition = {0,0,0};

        int lastMouseX = 0;
        int lastMouseY = 0;
        int mouseX = 0;
        int mouseY = 0;

        bool mouseMoved = false;

        int sensitivity = 10;

        int lastCamPitch = 0;
        int lastCamYaw = 0;
        float chunkVisibilityUpdateThreshold = 10.0f;

        int updateVisibility = 0; // If greater than 0 decreses and the chunk draw calls update

        KeyInputManager<ControllActions> inputManager;

        Font testFont = Font("resources/fonts/JetBrainsMono[wght].ttf", 24);

        void physicsUpdate();
        void updateLoadedLocations(glm::ivec3 old_location, glm::ivec3 new_location);

        double last = glfwGetTime();
        double current = glfwGetTime();
        double deltatime;

        float targetTPS = 240;
        float tickTime = 1.0f / targetTPS;

        double last_tick_time;
        Uniform<float> interpolation_time = Uniform<float>("model_interpolation_time");
        
    public:
        MainScene(){}
        void initialize() override;
        void setWorldPath(const std::string& path) {worldPath = path;}

        void render() override;
        void open(GLFWwindow* window)  override;
        void close(GLFWwindow* window)  override;
        void resize(GLFWwindow* window, int width, int height)  override;

        void mouseMove(GLFWwindow* window, int x, int y)  override;
        void mouseEvent(GLFWwindow* window, int button, int action, int mods)  override;
        void scrollEvent(GLFWwindow* window, double xoffset, double yoffset) override;

        void keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) override;
};

class UILoading: public UIFrame{
    private:
        std::array<std::atomic<int>, 8>* values = nullptr;
        std::array<int, 8> max_values{};
    public:
        void setValues(std::array<std::atomic<int>, 8>* ptr) {values = ptr;}
        void getRenderingInformation(UIRenderBatch& batch) override;
};