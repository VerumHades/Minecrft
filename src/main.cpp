#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>

#include <ui/core.hpp>
#include <ui/loader.hpp>
#include <scene.hpp>

#include <game/main_scene.hpp>
#include <game/menu_scene.hpp>

#include <rendering/allocator.hpp>
#include <test_scene.hpp>

#include <path_config.hpp>
#include <logging.hpp>
#include <format> 

SceneManager* s;
int main() {
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit()) {
        LogError("Failed to initialize glfw!");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SAMPLES, 4); // anti-alliasing
    
    ///glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1920, 1080, "Hello Terrain", NULL, NULL);

    if (!window) {
        LogError("Failed to initialize glfw window!");
        glfwTerminate();
        return -1;
    }
    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LogError("Failed to initialize glad!");
        return -1;
    }
    const GLubyte* version = glGetString(GL_VERSION);
    std::string versionStr(reinterpret_cast<const char*>(version));
    LogInfo("OpenGL version: ", versionStr);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    glEnable(GL_CULL_FACE);  // Enable backface culling
    glCullFace(GL_BACK);     // Cull back faces
    glFrontFace(GL_CCW);     // Set counterclockwise winding order as front*/

    //glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_MULTISAMPLE);  // Redundant perhaps
    //glDepthMask(GL_FALSE);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(GLDebugMessageCallback, NULL);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        s->resize(window, width, height); // Call resize on the instance
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) {
        s->mouseMove(window, static_cast<int>(x), static_cast<int>(y));
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        s->mouseEvent(window, button, action, mods);
    });
    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
        s->scrollEvent(window, xoffset, yoffset);
    });
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        s->keyEvent(window, key, scancode, action, mods);
    });
    glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int codepoint) {
        s->keyTypedEvent(window, codepoint);
    });


    glLineWidth(2.0f);
    
    {
        SceneManager sceneManager{window};

        s = &sceneManager;

        UICore::get().lua().set_function("setScene", [](std::string name){
            s->setScene(name);
        });

        UICore::get().lua().set_function("setLayer", [](std::string name){
            s->getCurrentScene()->setUILayer(name);
        });


        BlockRegistry::get().setTextureSize(160,160);
        BlockRegistry::get().loadFromFolder("resources/textures");
        BlockRegistry::get().loadPrototypesFromFile("resources/blocks.xml");
        CraftingRecipeRegistry::get().LoadRecipesFromXML("resources/recipes.xml");
        ItemRegistry::LoadFromXML("resources/items.xml");

        sceneManager.createScene<MenuScene>("menu");
        sceneManager.createScene<MainScene>("game");

        //sceneManager.createScene<TestScene>("test_scene");
        sceneManager.setScene("menu");
        //menuScene->setUILayer("default");

        double last = glfwGetTime();
        double current = glfwGetTime();
        float deltatime;

        sceneManager.resize(window, 1920, 1080);

        while (!glfwWindowShouldClose(window)) {
            current = glfwGetTime();
            deltatime = (float)(current - last);

            if(
                sceneManager.isFPSLocked() && 
                deltatime < sceneManager.getTickTime()
            ){
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<size_t>((sceneManager.getTickTime() - deltatime) * 1000)));
                continue;
            }
            last = current;

            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            sceneManager.render();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        sceneManager.setScene("menu"); // Wait for game threads to stop if running

        Model::CleanupAll();
        UICore::get().cleanup();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}