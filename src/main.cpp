#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#include <world.hpp>
#include <rendering/buffer.hpp>
#include <rendering/shaders.hpp>
#include <rendering/texture.hpp>
#include <rendering/camera.hpp>
#include <entity.hpp>

PerspectiveCamera camera;
DepthCamera suncam;

ModelManager modelManager;

ShaderProgram mainProgram;
ShaderProgram skyboxProgram;

int lastMouseX = 0;
int lastMouseY = 0;
float sensitivity = 0.1f;

int renderDistance = 16;

float camSpeed = 0.01f;

glm::vec3 camOffset = {0.3f,1.6f,0.3f};

float camFOV = 70.0f;
float maxFOV = 70.0f; 
float minFOV = 2.0f;

int sunDistance = ((DEFAULT_CHUNK_SIZE * renderDistance) / 2) ;
float sunAngle = 70.0f;
World world;

struct BoundKey{
    int key;
    bool isDown;
} boundKeys[] = {
    {GLFW_KEY_SPACE},
    {GLFW_KEY_LEFT_SHIFT},
    {GLFW_KEY_W},
    {GLFW_KEY_S},
    {GLFW_KEY_A},
    {GLFW_KEY_D}
};
int boundKeysCount = sizeof(boundKeys) / sizeof(boundKeys[0]);

static bool lineMode = true;

void key_callback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/){
    for(int i = 0; i < boundKeysCount;i++){
        if(key == boundKeys[i].key && action == GLFW_PRESS) boundKeys[i].isDown = true; 
        else if(key == boundKeys[i].key && action == GLFW_RELEASE) boundKeys[i].isDown = false; 
    }

    if(key == GLFW_KEY_M && action == GLFW_PRESS){
        lineMode = !lineMode;
        if(!lineMode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height){
    glViewport(0,0,width,height);
    camera.resizeScreen(width, height, camFOV);
    
    mainProgram.updateUniforms();
    skyboxProgram.updateUniforms();
    //printf("%i %i\n",width,height);
}

void cursor_position_callback(GLFWwindow* /*window*/, double mouseX, double mouseY)
{
    float xoffset = (float)mouseX - lastMouseX;
    float yoffset = lastMouseY - (float)mouseY; // Reversed since y-coordinates go from bottom to top
    lastMouseX = (int)mouseX;
    lastMouseY = (int)mouseY;

    xoffset *= sensitivity;
    yoffset *= sensitivity * 2;

    float camYaw = camera.getYaw() + xoffset;
    float camPitch = camera.getPitch() + yoffset;

    camYaw = std::fmod((camYaw + xoffset), (GLfloat)360.0f);

    // Constrain the pitch to avoid gimbal lock
    if (camPitch > 89.0f)
        camPitch = 89.0f;
    if (camPitch < -89.0f)
        camPitch = -89.0f;

    camera.setRotation(camPitch, camYaw);

    mainProgram.updateUniforms();
    skyboxProgram.updateUniforms();
}

static int selectedBlock = 1;
void mouse_button_callback(GLFWwindow* /*window*/, int button, int action, int /*mods*/)
{
    glm::vec3& camDirection = camera.getDirection();
    glm::vec3 camPosition = camera.getPosition();
    Entity& player = world.getEntities()[0];

    RaycastResult hit = world.raycast(camPosition.x,camPosition.y,camPosition.z,camDirection.x, camDirection.y, camDirection.z,10);
    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && hit.hit){
        world.setBlock(hit.x, hit.y, hit.z, {BlockTypes::Air});

        auto chunk = world.getChunkFromBlockPosition(hit.x, hit.z);
        if(!chunk) return;
        chunk->regenerateMesh(world.getBlockInChunkPosition(hit.x, hit.z));
        
    }
    else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && hit.hit){
        CollisionCheckResult result = world.checkForPointCollision(hit.lastX, hit.lastY, hit.lastZ, 1);

        world.setBlock(result.x,  result.y,  result.z, {static_cast<BlockTypes>(selectedBlock)});
        if(
            player.checkForCollision(world, false).collision ||
            player.checkForCollision(world, true).collision
        ){
            world.setBlock(result.x,  result.y,  result.z, {BlockTypes::Air});
            return;
        }

        auto chunk = world.getChunkFromBlockPosition(result.x, result.z);
        if(!chunk) return;
        chunk->regenerateMesh(world.getBlockInChunkPosition(result.x, result.z));
    }
}

void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{
    if(boundKeys[1].isDown){
        camFOV -= (float) yoffset * 5.0f;

        if(camFOV < minFOV) camFOV = minFOV;
        else if(camFOV > maxFOV) camFOV = maxFOV;

        camera.adjustFOV(camFOV);
    }
    else{
        selectedBlock = (selectedBlock + 1) % predefinedBlocks.size() + 1;
        std::cout << "Selected block: " << getBlockTypeName(static_cast<BlockTypes>(selectedBlock)) << std::endl;
    }
}

int clampAngle(int angle) {
    return (angle % 360 + 360) % 360;
}

void physicsUpdate();
void pregenUpdate();
static bool running = true;

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
    //glfwSwapInterval(0);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
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
    glFrontFace(GL_CW);     // Set counterclockwise winding order as front*/

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_MULTISAMPLE);  // Redundant perhaps
    //glDepthMask(GL_FALSE);

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    modelManager.initialize();
    camera.getProjectionUniform().attach(modelManager.getModelProgram());
    camera.getViewUniform().attach(modelManager.getModelProgram());
    
    std::array<std::string,6> skyboxPaths = {
        "skybox/stars/right.png",
        "skybox/stars/left.png",
        "skybox/stars/top.png",
        "skybox/stars/bottom.png",
        "skybox/stars/front.png",
        "skybox/stars/back.png"
    };

    skyboxProgram.initialize();
    skyboxProgram.addShader("shaders/skybox.vs", GL_VERTEX_SHADER);
    skyboxProgram.addShader("shaders/skybox.fs", GL_FRAGMENT_SHADER);
    skyboxProgram.compile();
    skyboxProgram.use();
    
    GLSkybox skybox = GLSkybox(skyboxPaths);
    
    std::vector<std::string> texturePaths = {
        "textures/grass_top.png",
        "textures/grass_side.png",
        "textures/dirt.png",
        "textures/stone.png",
        "textures/oak_log_top.png",
        "textures/oak_log.png",
        "textures/oak_leaves.png",
        "textures/birch_leaves.png",
        "textures/birch_log.png",
        "textures/birch_log_top.png",
        "textures/blue_wool.png",
        "textures/sand.png"
    };
    
    mainProgram.initialize();
    mainProgram.addShader("shaders/vertex.vs", GL_VERTEX_SHADER);
    mainProgram.addShader("shaders/fragment.fs", GL_FRAGMENT_SHADER);
    mainProgram.compile();
    mainProgram.use();

    GLTextureArray tilemap = GLTextureArray();
    tilemap.loadFromFiles(texturePaths,160,160);
    std::cout << "Loaded textures" << std::endl;

    suncam.initialize();
    suncam.getTexture()->bind(1);

    int lightSpaceMatrixLoc = mainProgram.getUniformLocation("lightSpaceMatrix");


    //int sunDirLoc = camera.getProgram("skybox").getUniformLocation("sunDir");
    auto sunDirUniform = Uniform<glm::vec3>("sunDir");
    sunDirUniform.attach(skyboxProgram);
    
    //int TimeLoc = mainProgram.getUniformLocation("time");

    glUniform1i(mainProgram.getUniformLocation("textureArray"),0);
    glUniform1i(mainProgram.getUniformLocation("shadowMap"),1);

    double last = glfwGetTime();
    double current = glfwGetTime();

    camera.initialize({skyboxProgram, mainProgram});
    camera.setPosition(0.0f,160.0f,0.0f);

    world.getEntities().emplace_back(glm::vec3(0,160,0), glm::vec3(0.6, 1.8, 0.6));


    sunDirUniform.setValue({ 
        -sunDistance * cos(glm::radians(sunAngle)), // X position (cosine component)
        -sunDistance * sin(glm::radians(sunAngle)), // Y position (sine component for vertical angle)
        0  // Z position (cosine component)
    });

    mainProgram.updateUniforms();
    skyboxProgram.updateUniforms();
    
    std::thread physicsThread(physicsUpdate);
    std::thread pregenThread(pregenUpdate);

    float deltatime;
    while (!glfwWindowShouldClose(window)) {
        current = glfwGetTime();
        deltatime = (float)(current - last);
        last = current;

        Entity& player = world.getEntities()[0];
        const glm::vec3& playerPosition = player.getPosition();
        glm::vec3 camPosition = playerPosition + camOffset;
        glm::vec3 camDir = glm::normalize(camera.getDirection());

        //std::cout << player.getVelocity().x << " " << player.getVelocity().y << " " << player.getVelocity().z << std::endl;
        camera.setPosition(camPosition);
        mainProgram.updateUniforms();
       // std::cout << std::endl;

        world.updateBuffers();
        
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //if(boundKeys[0].isDown) accelY += 0.0006;
        //if(boundKeys[1].isDown) accelY -= 0.0006;

        //printf("x:%f y:%f z:%f ax:%f ay:%f az:%f\ n",camX,camY,camZ,accelX,accelY,accelZ);

        //suncam.setPosition(c0,400, camera.getPosition().z);

        if(world.updated){
            int offsetX = (int) (camera.getPosition().x + -sunDistance * cos(glm::radians(sunAngle)));
            int offsetZ = (int) camera.getPosition().z;

            suncam.setPosition(
                (float) offsetX + sunDistance * cos(glm::radians(sunAngle)), // X position (cosine component)
                sunDistance * sin(glm::radians(sunAngle)), // Y position (sine component for vertical angle)
                (float) offsetZ  // Z position (cosine component)
            );
            suncam.setTarget(
                (float) offsetX - sunDistance * cos(glm::radians(sunAngle)),
                -sunDistance * sin(glm::radians(sunAngle)),
                (float) offsetZ
            );
            suncam.updateProjection();

            glUniform3f(lightDirLoc, 
                -sunDistance * cos(glm::radians(sunAngle)), // X position (cosine component)adorkastock
                -sunDistance * sin(glm::radians(sunAngle)), // Y position (sine component for vertical angle)
                0  // Z position (cosine component)
            );
            glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(suncam.getLightSpaceMatrix()));
            //suncam.updateProjection();
            //glDisable( GL_CULL_FACE );
            suncam.prepareForRender();
            world.drawChunks(suncam, suncam.getProgram(), renderDistance);
            //glEnable( GL_CULL_FACE );

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0,0,camera.getScreenWidth(),camera.getScreenHeight());

            world.updated = false;
        }


        //skyboxProgram.updateUniforms();
        
        glDisable(GL_CULL_FACE);
        skyboxProgram.use();
        skybox.draw();
        glEnable(GL_CULL_FACE);

        mainProgram.use();
        tilemap.bind(0);

        world.drawChunks(camera, mainProgram, renderDistance);
        //std::cout << "Drawn: " << total << "/" << pow(renderDistance * 2,2) << std::endl;
        /* Swap front and back buffers */
        glfwSwapBuffers(window);
        /* Poll for and process events */
        glfwPollEvents();
    }

    running = false;
    physicsThread.join();
    pregenThread.join();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void physicsUpdate(){
    double last = glfwGetTime();
    double current = glfwGetTime();
    float deltatime;

    float targetTPS = 60;
    float tickTime = 1.0f / targetTPS;

    while(running){
        current = glfwGetTime();
        deltatime = (float)(current - last);

        //std::cout << deltatime << "/" << tickTime << std::endl;
        if(deltatime < tickTime) continue;
        last = current;

        Entity& player = world.getEntities()[0];

        glm::vec3 camDir = glm::normalize(camera.getDirection());
        glm::vec3 horizontalDir = glm::normalize(glm::vec3(camDir.x, 0, camDir.z));
        glm::vec3 rightDir = normalize(glm::cross(camera.getUp(), horizontalDir));

        if(boundKeys[4].isDown) player.accelerate(rightDir * camSpeed);
        if(boundKeys[5].isDown) player.accelerate(-rightDir * camSpeed);
        if(boundKeys[3].isDown) player.accelerate(-horizontalDir * camSpeed);
        if(boundKeys[2].isDown) player.accelerate(horizontalDir * camSpeed);
        if(
            boundKeys[0].isDown 
            && player.checkForCollision(world, false, {0,-0.1f,0}).collision
            && player.getVelocity().y == 0
        ) player.accelerate(camera.getUp() * 0.2f);

        world.updateEntities();
    }
}

void pregenUpdate(){
    int pregenDistance = renderDistance + 2;

    while(running){
        int camWorldX = (int) camera.getPosition().x / DEFAULT_CHUNK_SIZE;
        int camWorldZ = (int) camera.getPosition().z / DEFAULT_CHUNK_SIZE;

        for(int x = -pregenDistance; x <= pregenDistance; x++){
            for(int z = -pregenDistance; z <= pregenDistance; z++){
                int chunkX = camWorldX + x;
                int chunkZ = camWorldZ + z;

                Chunk* meshlessChunk = world.getChunk(chunkX, chunkZ);
                if(!meshlessChunk){
                    meshlessChunk = world.generateAndGetChunk(chunkX, chunkZ);
                }
            }
        }
    }
}