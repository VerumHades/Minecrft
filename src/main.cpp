#include "game/blocks.hpp"
#include "game/chunk.hpp"
#include "structure/bytearray.hpp"
#include "structure/octree.hpp"
#include "structure/record_store.hpp"
#include "structure/serialization/octree_serializer.hpp"
#include "structure/streams/file_stream.hpp"
#include "vec_hash.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>

#include <memory>
#include <ui/core.hpp>
#include <ui/loader.hpp>
#include <scene.hpp>

#include <game/main_scene.hpp>
#include <game/menu_scene.hpp>

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

    /// glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LogError("Failed to initialize glad!");
        return -1;
    }
    const GLubyte* version = glGetString(GL_VERSION);
    std::string versionStr(reinterpret_cast<const char*>(version));
    LogInfo("OpenGL version: {}", versionStr);

    // Check if OpenGL 4.6 is supported
    int major = 0, minor = 0;
    sscanf(reinterpret_cast<const char*>(version), "%d.%d", &major, &minor);

    if (major >= 4 && minor >= 6) {

    } else
        LogWarning("Version of Opengl 4.6 is not supported. This is a pretty serious issue, maybe update drivers?");

    if (glfwExtensionSupported("GL_ARB_multi_draw_indirect") != GLFW_TRUE)
        LogError("Missing dependency, glMultiDrawElementsIndirect will not work.");

    GL_CALL(glEnable(GL_DEPTH_TEST));
    GL_CALL(glDepthFunc(GL_LEQUAL));

    GL_CALL(glEnable(GL_CULL_FACE)); // Enable backface culling
    GL_CALL(glCullFace(GL_BACK));    // Cull back faces
    GL_CALL(glFrontFace(GL_CCW));    // Set counterclockwise winding order as front*/
    GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    // glEnable(GL_FRAMEBUFFER_SRGB);
    GL_CALL(glEnable(GL_MULTISAMPLE)); // Redundant perhaps
    // glDepthMask(GL_FALSE);
    //GL_CALL(glEnable(GL_DEBUG_OUTPUT));
    //GL_CALL(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS));
    //GL_CALL(glDebugMessageCallback(GLDebugMessageCallback, NULL));

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        s->resize(window, width, height); // Call resize on the instance
    });
    
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) { s->mouseMove(window, static_cast<int>(x), static_cast<int>(y)); });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) { s->mouseEvent(window, button, action, mods); });
    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) { s->scrollEvent(window, xoffset, yoffset); });
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) { s->keyEvent(window, key, scancode, action, mods); });
    glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int codepoint) { s->keyTypedEvent(window, codepoint); });

    {
        SceneManager sceneManager{window};

        s = &sceneManager;

        UICore::get().lua().set_function("setScene", [](std::string name) { s->setScene(name); });

        UICore::get().lua().set_function("setLayer", [](std::string name) { s->getCurrentScene()->setUILayer(name); });

        BlockRegistry::get().setTextureSize(160, 160);
        BlockRegistry::get().loadFromFolder("resources/textures/blocks");
        BlockRegistry::get().loadPrototypesFromFile("resources/blocks.xml");
        CraftingRecipeRegistry::get().LoadRecipesFromXML("resources/recipes.xml");
        ItemRegistry::LoadFromXML("resources/items.xml");

        sceneManager.createScene<MenuScene>("menu");
        sceneManager.createScene<MainScene>("game");

        // sceneManager.createScene<TestScene>("test_scene");
        sceneManager.setScene("menu");
        // menuScene->setUILayer("default");

        double last    = glfwGetTime();
        double current = glfwGetTime();
        float deltatime;

        sceneManager.resize(window, 1920, 1080);
        while (!glfwWindowShouldClose(window)) {
            current   = glfwGetTime();
            deltatime = (float)(current - last);

            if (sceneManager.isFPSLocked() && deltatime < sceneManager.getTickTime()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<size_t>((sceneManager.getTickTime() - deltatime) * 1000)));
                continue;
            }
            last = current;

            // std::cout << "VRAM usage:" << GLBufferStatistics::getMemoryUsage() << std::endl;

            GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
            GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

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
