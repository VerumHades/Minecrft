#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#include <ui/manager.hpp>
#include <scene.hpp>
#include <game/mscene.hpp>
#include <rendering/allocator.hpp>

SceneManager sceneManager;
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

    sceneManager.setWindow(window);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        sceneManager.resize(window, width, height); // Call resize on the instance
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) {
        sceneManager.mouseMove(window, static_cast<int>(x), static_cast<int>(y));
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        sceneManager.mouseEvent(window, button, action, mods);
    });
    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
        sceneManager.scrollEvent(window, xoffset, yoffset);
    });
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        sceneManager.keyEvent(window, key, scancode, action, mods);
    });
    glfwSetCharCallback(window, [](GLFWwindow* window, unsigned int codepoint) {
        sceneManager.keyTypedEvent(window, codepoint);
    });
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize glad!" << std::endl;
        return -1;
    }


    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    
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

    /*
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(GLDebugMessageCallback, NULL);
    */
   
    sceneManager.initialize();

    std::unique_ptr<Scene> mainMenu = std::make_unique<Scene>();
    std::unique_ptr<MainScene> mainScene = std::make_unique<MainScene>();
    MainScene* mainSceneTemp = mainScene.get();

    sceneManager.addScene("game",std::move(mainScene));
    sceneManager.addScene("menu",std::move(mainMenu));

    Scene* menuScene = sceneManager.getScene("menu");
    mainSceneTemp->initialize(menuScene);


    auto mainFlexFrame = std::make_shared<UIFlexFrame>(
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(PIXELS, 300),
        TValue(PIXELS, 500),
        glm::vec4(0.1,0.1,0.1,0.0)
    );
    mainFlexFrame->setBorderWidth({0,0,0,0});
    mainFlexFrame->setElementDirection(UIFlexFrame::ROWS);
    mainFlexFrame->setElementMargin(20);

    auto startButton = std::make_shared<UILabel>(
        "New Game", 
        TValue(PIXELS, 200),
        TValue(PIXELS, 40),
        glm::vec4(0.0,0.3,0.3,1.0)
    );
    startButton->setHoverColor(glm::vec4(0.0,0.1,0.5,1.0));
    mainFlexFrame->appendChild(startButton);

    auto settingsButton = std::make_shared<UILabel>(
        "Settings", 
        TValue(PIXELS, 200),
        TValue(PIXELS, 40),
        glm::vec4(0.0,0.3,0.3,1.0)
    );
    settingsButton->setHoverColor(glm::vec4(0.0,0.1,0.5,1.0));
    settingsButton->onClicked = [menuScene]{
        menuScene->setUILayer("settings");
    };
    auto settingsReturnButton = std::make_shared<UILabel>(
        "Back", 
        TValue(10),
        TValue(10),
        TValue(PIXELS, 200),
        TValue(PIXELS, 40),
        glm::vec4(0.0,0.3,0.3,1.0)
    );
    settingsReturnButton->setHoverColor(glm::vec4(0.0,0.1,0.5,1.0));
    settingsReturnButton->onClicked = [menuScene]{
        menuScene->setUILayer("default");
    };
    mainFlexFrame->appendChild(settingsButton);

    auto worldSelection = std::make_shared<UIFlexFrame>(
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(FRACTIONS, 0),
        TValue(PFRACTION, 100),
        TValue(1000),
        glm::vec4(0.1,0.1,0.1,0.0)
    );
    worldSelection->setElementDirection(UIFlexFrame::ROWS);
    worldSelection->setElementMargin(20);
    worldSelection->setBorderWidth({{PIXELS,0},{PIXELS,0},{PIXELS,0},{PIXELS,0}});

    auto worldSelectionScrollFrame = std::make_shared<UIScrollableFrame>(
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(FRACTIONS, 0),
        TValue(PFRACTION, 60),
        TValue(PFRACTION, 80),
        glm::vec4(0.05,0.05,0.05,1.0),
        worldSelection
    );

    UIFrame* worldSelectionRaw = worldSelection.get();

    startButton->onClicked = [menuScene, worldSelectionRaw, mainSceneTemp] {
        //sceneManager.setScene("game");
        menuScene->setUILayer("world_menu");
        worldSelectionRaw->clearChildren();

        for (const auto& dirEntry : std::filesystem::recursive_directory_iterator("saves")){
            std::string filepath = dirEntry.path().string();
            WorldStream stream(filepath);
            
            auto frame = std::make_shared<UIFrame>(
                800px,
                100px,
                glm::vec4(0.0,0.2,0.2,1.0)
            );
            frame->setColor(glm::vec4(0.0,0.2,0.2,0.6));

            auto temp = std::make_shared<UILabel>(
                stream.getName(),
                10px,
                10px,
                100ps - 20px,
                50ps,
                glm::vec4(0.0,0.0,0.0,0.0)
            );
            temp->setTextPosition(LEFT);
            temp->setBorderWidth(0px);
            temp->setHoverable(false);

            frame->onClicked = [menuScene, worldSelectionRaw, mainSceneTemp, filepath] {
                mainSceneTemp->setWorldPath(filepath);
                sceneManager.setScene("game");
            };

            frame->appendChild(temp);
            worldSelectionRaw->appendChild(frame);
        }
    };

    menuScene->setUILayer("settings");
    menuScene->addElement(settingsReturnButton);
    menuScene->setUILayer("world_menu");
    menuScene->addElement(worldSelectionScrollFrame);
    menuScene->setUILayer("default");
    menuScene->addElement(mainFlexFrame);
    
    sceneManager.setScene("game");

    double last = glfwGetTime();
    double current = glfwGetTime();
    float deltatime;

    while (!glfwWindowShouldClose(window)) {
        current = glfwGetTime();
        deltatime = (float)(current - last);

        if(sceneManager.isFPSLocked() && deltatime < sceneManager.getTickTime()) continue;
        last = current;

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sceneManager.render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}