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

#include <parsing/shader_parser.hpp>
#include <parsing/block_loader.hpp>

#include <ui/manager.hpp>
#include <ui/font.hpp>
#include <ui/loader.hpp>

#include <game/world/world.hpp>
#include <game/world/mesh_generation.hpp>
#include <game/entity.hpp>
#include <game/commands.hpp>
#include <game/threadpool.hpp>
#include <game/input.hpp>
#include <game/items/sprite_model.hpp>
#include <game/items/item.hpp>
#include <game/items/item_renderer.hpp>

#include <indexing.hpp>

#include <scene.hpp>
#include <set>
#include <memory>
#include <random>
#include <atomic>
#include <queue>

class UIHotbar;

class MainScene: public Scene{
    private:
        PerspectiveCamera camera = PerspectiveCamera("player");
        DepthCamera suncam = DepthCamera("sun");

        SpiralIndexer3D indexer;

        GBuffer gBuffer = GBuffer(1920,1080);
        GLFullscreenQuad fullscreen_quad;

        ShaderProgram modelProgram = ShaderProgram("resources/shaders/graphical/model/model.vs","resources/shaders/graphical/model/model.fs");
        ShaderProgram terrainProgram = ShaderProgram(ShaderProgramSource::fromFile("resources/shaders/terrain.sp"));
        ShaderProgram skyboxProgram  = ShaderProgram("resources/shaders/graphical/skybox.vs", "resources/shaders/graphical/skybox.fs");
        ShaderProgram gBufferProgram = ShaderProgram("resources/shaders/graphical/deffered_shading/gbuffer.vs","resources/shaders/graphical/deffered_shading/gbuffer.fs");
        ShaderProgram blur_program = ShaderProgram("resources/shaders/graphical/quad.vs","resources/shaders/graphical/blur.fs");
        
        std::unique_ptr<ThreadPool> threadPool;

        GLSkybox skybox;
        std::unique_ptr<World> world;

        WireframeCubeRenderer wireframeRenderer;
        
        ChunkMeshRegistry chunkMeshRegistry;
        ChunkMeshGenerator chunkMeshGenerator;
        std::queue<glm::ivec3> chunk_generation_queue = {};

        std::unique_ptr<GLTextureArray> block_texture_array = nullptr;
        
        std::shared_ptr<UILabel> fps_label;

        ItemTextureAtlas itemTextureAtlas;
        ItemPrototypeRegistry itemPrototypeRegistry;
        std::shared_ptr<ItemSlot> held_item_slot;
        std::shared_ptr<ItemInventory> inventory;
        std::shared_ptr<UIHotbar> hotbar;
        
        std::atomic<bool> update_hotbar = false;

        std::string worldPath = "saves/worldsave.bin";
        int renderDistance = 4;
        int selectedBlock = 4;

        bool allGenerated = false;
        bool running = false;
        int threadsStopped = 0;
        
        bool lineMode = true;
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

        enum ControllActions{
            MOVE_FORWARD,
            MOVE_BACKWARD,
            STRAFE_LEFT,
            STRAFE_RIGHT,
            MOVE_UP,
            MOVE_DOWN,
            SCROLL_ZOOM
        };

        KeyInputManager<ControllActions> inputManager;

        Font testFont = Font("resources/fonts/JetBrainsMono/fonts/variable/JetBrainsMono[wght].ttf", 24);

        void physicsUpdate();
        void processMouseMovement();

        void enqueueChunkGeneration(glm::ivec3 position);
        void updateLoadedLocations(glm::ivec3 old_location, glm::ivec3 new_location);

        double last = glfwGetTime();
        double current = glfwGetTime();
        double deltatime;

        void updateCursor();

        glm::ivec3 blockUnderCursorPosition = {0,0,0};
        glm::vec3 blockUnderCursorEmpty = {0,0,0}; // Block before the selected block, air where a block will be placed
        Block* blockUnderCursor = nullptr;
        
    public:
        void initialize() override;
        void setWorldPath(std::string path) {worldPath = path;}

        void render() override;
        void open(GLFWwindow* window)  override;
        void close(GLFWwindow* window)  override;
        void resize(GLFWwindow* window, int width, int height)  override;

        void regenerateChunkMesh(Chunk* chunk);
        void regenerateChunkMesh(Chunk* chunk,glm::vec3 blockCoords);

        void mouseMove(GLFWwindow* window, int x, int y)  override;
        void mouseEvent(GLFWwindow* window, int button, int action, int mods)  override;
        void scrollEvent(GLFWwindow* window, double xoffset, double yoffset) override;

        void keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) override;
        void unlockedKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) override;
        void unlockedMouseMove(GLFWwindow* window, int mouseX, int mouseY) override;
};

class UICrosshair: public UIFrame{
    private:
        int part_margin = 5;
        int thickness = 5;
    public:
        UICrosshair(UIManager& manager): UIFrame(manager){setFocusable(false);setHoverable(false);}
        void getRenderingInformation(UIRenderBatch& batch) override;
};

class UIHotbar: public ItemInventory{
    private:
        int selected_slot = 0;
        const int slots_total = 9;

    public:
        UIHotbar(ItemTextureAtlas& textureAtlas, UIManager& manager, std::shared_ptr<ItemSlot> held_item_ptr): 
        ItemInventory(textureAtlas, manager, 9, 1, held_item_ptr){}

        void getRenderingInformation(UIRenderBatch& batch) override;
        void selectSlot(int slot){
            selected_slot = (slot + slots_total) % slots_total;
        }
        int getSelectedSlotNumber(){return selected_slot;}

        LogicalItemSlot& getSelectedSlot() {
            return getItemSlot(selected_slot,0);
        };
};