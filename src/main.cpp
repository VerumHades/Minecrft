#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#include <ui/manager.hpp>
#include <ui/loader.hpp>
#include <scene.hpp>
#include <game/mscene.hpp>
#include <rendering/allocator.hpp>


/*void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        if(menuOpen){
            uiManager.setScene("main");
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else{
            uiManager.setScene("internal_default");
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        menuOpen = !menuOpen;
    }
    uiManager.keyEvent(key,action);
}*/

void APIENTRY GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length,
                            const GLchar *message, const void *param)
{
	const char *source_, *type_, *severity_;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             source_ = "API";             break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source_ = "WINDOW_SYSTEM";   break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: source_ = "SHADER_COMPILER"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     source_ = "THIRD_PARTY";     break;
	case GL_DEBUG_SOURCE_APPLICATION:     source_ = "APPLICATION";     break;
	case GL_DEBUG_SOURCE_OTHER:           source_ = "OTHER";           break;
	default:                              source_ = "<SOURCE>";        break;
	}

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               type_ = "ERROR";               break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_ = "DEPRECATED_BEHAVIOR"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_ = "UDEFINED_BEHAVIOR";   break;
	case GL_DEBUG_TYPE_PORTABILITY:         type_ = "PORTABILITY";         break;
	case GL_DEBUG_TYPE_PERFORMANCE:         type_ = "PERFORMANCE";         break;
	case GL_DEBUG_TYPE_OTHER:               type_ = "OTHER";               break;
	case GL_DEBUG_TYPE_MARKER:              type_ = "MARKER";              break;
	default:                                type_ = "<TYPE>";              break;
	}

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         severity_ = "HIGH";         break;
	case GL_DEBUG_SEVERITY_MEDIUM:       severity_ = "MEDIUM";       break;
	case GL_DEBUG_SEVERITY_LOW:          severity_ = "LOW";          break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: severity_ = "NOTIFICATION"; break;
	default:                             severity_ = "<SEVERITY>";   break;
	}

	printf("%d: GL %s %s (%s): %s\n",
		   id, severity_, type_, source_, message);
}

void printOpenGLLimits() {
    GLint maxWorkGroupCount[3]; // [X, Y, Z]
    GLint maxWorkGroupSize[3];  // [X, Y, Z]
    GLint maxInvocations;       // Total invocations per work group
    GLint maxSharedMemorySize;  // Total shared memory size for compute shaders
    
    // Query GL_MAX_COMPUTE_WORK_GROUP_COUNT
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &maxWorkGroupCount[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &maxWorkGroupCount[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &maxWorkGroupCount[2]);
    
    // Query GL_MAX_COMPUTE_WORK_GROUP_SIZE
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxWorkGroupSize[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxWorkGroupSize[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxWorkGroupSize[2]);
    
    // Query GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxInvocations);
    
    // Query GL_MAX_COMPUTE_SHARED_MEMORY_SIZE
    glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &maxSharedMemorySize);
    
    // Output the queried values
    std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT:" << std::endl;
    std::cout << "  X: " << maxWorkGroupCount[0] << ", Y: " << maxWorkGroupCount[1] << ", Z: " << maxWorkGroupCount[2] << std::endl;
    
    std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE:" << std::endl;
    std::cout << "  X: " << maxWorkGroupSize[0] << ", Y: " << maxWorkGroupSize[1] << ", Z: " << maxWorkGroupSize[2] << std::endl;
    
    std::cout << "GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS: " << maxInvocations << std::endl;
    
    std::cout << "GL_MAX_COMPUTE_SHARED_MEMORY_SIZE: " << maxSharedMemorySize << " bytes" << std::endl;
}


SceneManager* s;
int main() {
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit()) {
        std::cout << "Failed to initialize glfw!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SAMPLES, 4); // anti-alliasing
    
    ///glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1920, 1080, "Hello World", NULL, NULL);

    if (!window) {
        std::cout << "Failed to initialize glfw window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

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
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize glad!" << std::endl;
        return -1;
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    
    printOpenGLLimits();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    glEnable(GL_CULL_FACE);  // Enable backface culling
    glCullFace(GL_BACK);     // Cull back faces
    glFrontFace(GL_CCW);     // Set counterclockwise winding order as front*/

    //glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_MULTISAMPLE);  // Redundant perhaps
    //glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    

    uniformLinker.ignore("textureArray");
    uniformLinker.ignore("text");
    uniformLinker.ignore("textAtlas");
    uniformLinker.ignore("tex");
    uniformLinker.ignore("textureIn");

    /*
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(GLDebugMessageCallback, NULL);
    */

    glLineWidth(2.0f);
    
    {
        SceneManager sceneManager;

        sceneManager.setWindow(window);
        sceneManager.initialize();

        s = &sceneManager;

        std::unique_ptr<Scene> mainMenu = std::make_unique<Scene>();
        std::unique_ptr<MainScene> mainScene = std::make_unique<MainScene>();
        MainScene* mainSceneTemp = mainScene.get();

        sceneManager.addScene("game",std::move(mainScene));
        sceneManager.addScene("menu",std::move(mainMenu));

        Scene* menuScene = sceneManager.getScene("menu");

        UILoader loader = UILoader(sceneManager.getUIManager());
        loader.loadWindowFromXML(menuScene->getWindow(), "templates/menu.xml");
        auto* l = &loader;

        mainSceneTemp->initialize(menuScene, l);

        menuScene->setUILayer("default");

        auto startButton = menuScene->getUILayer("default").getElementById("new_world");
        auto scrollable = std::dynamic_pointer_cast<UIScrollableFrame>(menuScene->getUILayer("world_menu").getElementById("top_frame"));
        
        startButton->onClicked = [menuScene, mainSceneTemp, scrollable, l] {
            menuScene->setUILayer("world_menu");
            scrollable->clearChildren();

            for (const auto& dirEntry : std::filesystem::recursive_directory_iterator("saves")){
                std::string filepath = dirEntry.path().string();
                WorldStream stream(filepath);
                
                auto frame = s->getUIManager().createElement<UIFrame>();
                frame->setSize({PERCENT,80}, 200);
                
                auto temp = s->getUIManager().createElement<UILabel>();
                temp->setText(stream.getName());
                temp->setSize({PERCENT,100}, 40);
                temp->setHoverable(false);
                temp->setIdentifiers({"world_option_label"});

                auto chunkCountLabel = s->getUIManager().createElement<UILabel>();
                chunkCountLabel->setText("Number of saved chunks: " + std::to_string(stream.getChunkCount()));
                chunkCountLabel->setSize({PERCENT,100},40);
                chunkCountLabel->setPosition(0,45);
                chunkCountLabel->setHoverable(false);
                chunkCountLabel->setIdentifiers({"world_option_chunk_count_label"});

                frame->onClicked = [menuScene, mainSceneTemp, filepath] {
                    mainSceneTemp->setWorldPath(filepath);
                    s->setScene("game");
                };

                frame->appendChild(temp);
                frame->appendChild(chunkCountLabel);
                scrollable->appendChild(frame);

                l->getCurrentStyle().applyTo(frame);
                l->getCurrentStyle().applyTo(temp);
                l->getCurrentStyle().applyTo(chunkCountLabel);
            }
            
            scrollable->calculateTransforms();
            scrollable->update();
            scrollable->updateChildren();
        };

        auto toSettings = menuScene->getUILayer("default").getElementById("setttings");
        toSettings->onClicked = [menuScene]{
            menuScene->setUILayer("settings");
        };

        auto backButton = menuScene->getUILayer("world_menu").getElementById("back_to_menu");
        backButton->onClicked = [menuScene, mainSceneTemp] {
            menuScene->setUILayer("default");
        };

        auto settingsBackButton = menuScene->getUILayer("settings").getElementById("settings_back_button");
        settingsBackButton->onClicked = [menuScene, mainSceneTemp] {
            menuScene->setUILayer("default");
        };

        auto newWorldNameInput = std::dynamic_pointer_cast<UIInput>(menuScene->getUILayer("world_menu").getElementById("new_world_name"));
        
        auto newWorldFunc = [newWorldNameInput, startButton]{
            auto name = newWorldNameInput->getText();
            newWorldNameInput->setText("");
            if(name == "") return;

            WorldStream stream("saves/" + name + ".bin");
            stream.setName(name);

            startButton->onClicked();
        };

        newWorldNameInput->onSubmit = [newWorldFunc](std::string){newWorldFunc();};

        auto newWorldButton = menuScene->getUILayer("world_menu").getElementById("create_new_world");
        newWorldButton->onClicked = newWorldFunc;

        sceneManager.setScene("game");
        menuScene->setUILayer("default");

        double last = glfwGetTime();
        double current = glfwGetTime();
        float deltatime;

        while (!glfwWindowShouldClose(window)) {
            current = glfwGetTime();
            deltatime = (float)(current - last);

            if(sceneManager.isFPSLocked() && deltatime < sceneManager.getTickTime()){
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
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}