#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

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

#include <ui/manager.hpp>
#include <ui/font.hpp>
#include <ui/loader.hpp>

#include <game/world/world.hpp>
#include <game/world/mesh_generation.hpp>
#include <game/entity.hpp>
#include <game/commands.hpp>
#include <game/threadpool.hpp>
#include <game/input.hpp>
#include <game/items/cube_model.hpp>
#include <game/items/sprite_model.hpp>
#include <game/items/item.hpp>
#include <game/items/item_renderer.hpp>

#include <scene.hpp>
#include <set>
#include <memory>

class MainScene: public Scene{
    private:
        PerspectiveCamera camera = PerspectiveCamera("player");
        DepthCamera suncam = DepthCamera("sun");

        GBuffer gBuffer = GBuffer(1920,1080);
        GLFullscreenQuad fullscreen_quad;
        
        GLSSAO ssao;
        GLFramebuffer blured_ssao_framebuffer = GLFramebuffer(1920,1080,{{GL_RED,GL_RED,GL_FLOAT}});

        ShaderProgram modelProgram = ShaderProgram("shaders/graphical/model/model.vs","shaders/graphical/model/model.fs");
        ShaderProgram terrainProgram = ShaderProgram(ShaderProgramSource::fromFile("shaders/terrain.sp"));
        ShaderProgram skyboxProgram  = ShaderProgram("shaders/graphical/skybox.vs", "shaders/graphical/skybox.fs");
        ShaderProgram gBufferProgram = ShaderProgram("shaders/graphical/deffered_shading/gbuffer.vs","shaders/graphical/deffered_shading/gbuffer.fs");
        ShaderProgram blur_program = ShaderProgram("shaders/graphical/quad.vs","shaders/graphical/blur.fs");
        
        std::unique_ptr<ThreadPool> threadPool;

        GLSkybox skybox;
        std::unique_ptr<World> world;

        WireframeCubeRenderer wireframeRenderer;

        TextureRegistry blockTextureRegistry;
        BlockRegistry blockRegistry = BlockRegistry(blockTextureRegistry);
        
        ChunkMeshRegistry chunkMeshRegistry;
        ChunkMeshGenerator chunkMeshGenerator = ChunkMeshGenerator(blockRegistry);
        
        ItemTextureAtlas itemTextureAtlas;
        ItemPrototypeRegistry itemPrototypeRegistry;
        std::shared_ptr<ItemSlot> held_item_slot;
        std::shared_ptr<ItemInventory> inventory;

        std::string worldPath = "saves/worldsave.bin";
        int renderDistance = 3;
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

        float camSpeed = 0.01f;
        float camFOV = 90.0f;
        float maxFOV = 90.0f; 
        float minFOV = 2.0f;
        glm::vec3 camOffset = {0.3f,1.6f,0.3f};

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

        Font testFont = Font("fonts/JetBrainsMono/fonts/variable/JetBrainsMono[wght].ttf", 24);

        void physicsUpdate();
        void generateSurroundingChunks();
        void processMouseMovement();

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
        virtual void getRenderingInformation(UIRenderBatch& batch);
};
