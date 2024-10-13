#ifndef MAINSCENE_H
#define MAINSCENE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#include <rendering/buffer.hpp>
#include <rendering/shaders.hpp>
#include <rendering/texture.hpp>
#include <rendering/camera.hpp>

#include <ui/manager.hpp>
#include <ui/font.hpp>

#include <game/world.hpp>
#include <game/entity.hpp>
#include <game/commands.hpp>
#include <game/threadpool.hpp>

#include <scene.hpp>
#include <set>

struct BoundKey{
    int key;
    bool isDown;
};

class MainScene: public Scene{
    private:
        PerspectiveCamera camera;
        DepthCamera suncam;
        ModelManager modelManager;

        ThreadPool threadPool = ThreadPool(8);

        ShaderProgram terrainProgram;
        ShaderProgram skyboxProgram;
        CommandProcessor commandProcessor;

        GLTextureArray tilemap;
        GLSkybox skybox;
        std::unique_ptr<World> world;

        MultiChunkBuffer chunkBuffer;

        std::string worldPath = "saves/worldsave.bin";
        int renderDistance = 4;
        int selectedBlock = 2;

        bool running = false;
        int threadsStopped = 0;
        
        bool lineMode = true;
        bool chatOpen = false;
        bool menuOpen = false;
        UIInput* chatInput;
        //int sunDistance = ((CHUNK_SIZE * renderDistance) / 2) ;
        int sunDistance = 100;
        float sunAngle = 70.0f;
        Uniform<glm::vec3> sunDirUniform = Uniform<glm::vec3>("sunDir");

        float camSpeed = 0.01f;
        float camFOV = 90.0f;
        float maxFOV = 90.0f; 
        float minFOV = 2.0f;
        glm::vec3 camOffset = {0.3f,1.6f,0.3f};

        int lastMouseX = 0;
        int lastMouseY = 0;
        float sensitivity = 0.1f;

        std::vector<BoundKey> boundKeys;

        Font testFont = Font("fonts/JetBrainsMono/fonts/variable/JetBrainsMono[wght].ttf", 24);

        void physicsUpdate();
        void pregenUpdate();
        void generateMeshes();

        double last = glfwGetTime();
        double current = glfwGetTime();
        double deltatime;

        std::unordered_set<glm::vec3, Vec3Hash, Vec3Equal> loadedPositions;
        
    public:
        void initialize();
        void setWorldPath(std::string path) {worldPath = path;}

        void render() override;
        void open(GLFWwindow* window)  override;
        void close(GLFWwindow* window)  override;
        void resize(GLFWwindow* window, int width, int height)  override;

        void regenerateChunkMesh(Chunk& chunk);
        void regenerateChunkMesh(Chunk& chunk,glm::vec3 blockCoords);

        void mouseMove(GLFWwindow* window, int x, int y)  override;
        void mouseEvent(GLFWwindow* window, int button, int action, int mods)  override;
        void scrollEvent(GLFWwindow* window, double xoffset, double yoffset) override;

        void keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) override;
        void unlockedKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) override;
};

class UIAllocatorVisualizer: public UIFrame{
    private:
        Allocator* watched;
    public:
        UIAllocatorVisualizer(TValue x, TValue y, TValue width, TValue height, UIColor color, Allocator* watched): UIFrame(x,y,width,height,color), watched(watched) {};
        std::vector<UIRenderInfo> getRenderingInformation(UIManager& manager) override;
};

#endif