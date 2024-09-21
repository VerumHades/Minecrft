#ifndef MAINSCENE_H
#define MAINSCENE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#include <world.hpp>
#include <rendering/buffer.hpp>
#include <rendering/shaders.hpp>
#include <rendering/texture.hpp>
#include <rendering/camera.hpp>
#include <ui/manager.hpp>
#include <ui/font.hpp>
#include <entity.hpp>

struct BoundKey{
    int key;
    bool isDown;
};

class MainScene: public UIScene{
    private:
        PerspectiveCamera camera;
        DepthCamera suncam;
        ModelManager modelManager;

        ShaderProgram terrainProgram;
        ShaderProgram skyboxProgram;

        GLTextureArray tilemap;
        GLSkybox skybox;
        World world;

        int renderDistance = 8;
        int selectedBlock = 2;
        bool running = false;
        bool lineMode = true;
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

        double last = glfwGetTime();
        double current = glfwGetTime();
        double deltatime;
        
    public:
        void render(UIManager& manager) override;
        void initialize();
        void open(UIManager& manager) override;
        void close(UIManager& manager) override;
        
        void resize(UIManager& manager, int width, int height) override;
        void mouseMove(UIManager& manager, int x, int y) override;
        void mouseEvent(UIManager& manager, int button, int action) override;
        void mouseScroll(UIManager& manager, int yoffset) override;
        void keyEvent(UIManager& manager, int key, int action) override;
};

#endif