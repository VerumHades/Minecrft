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

int lastMouseX = 0;
int lastMouseY = 0;
float sensitivity = 0.1f;

int renderDistance = 8;

float camSpeed = 0.01f;

glm::vec3 camOffset = {0.3f,1.6f,0.3f};

float camFOV = 90.0f;
float maxFOV = 90.0f;
float minFOV = 10.0f;

int sunDistance = ((DEFAULT_CHUNK_SIZE * renderDistance) / 2) ;
float sunAngle = 45.0f;
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
    camera.updateUniforms();
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
}

//static int selectedBlock = 1;
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

        world.setBlock(result.x,  result.y,  result.z, {BlockTypes::BlueWool});
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
    /*else if(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS){
        selectedBlock = (selectedBlock + 1) % 5 + 1;
    }*/
}

void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset)
{
    camFOV -= (float) yoffset * 5.0f;

    if(camFOV < minFOV) camFOV = minFOV;
    else if(camFOV > maxFOV) camFOV = maxFOV;

    camera.adjustFOV(camFOV);
}

int clampAngle(int angle) {
    return (angle % 360 + 360) % 360;
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
    //glDepthMask(GL_FALSE);

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    camera.addShader("main", "shaders/vertex.vs","shaders/fragment.fs");
    std::array<std::string,6> skyboxPaths = {
        "skybox/stars/right.png",
        "skybox/stars/left.png",
        "skybox/stars/top.png",
        "skybox/stars/bottom.png",
        "skybox/stars/front.png",
        "skybox/stars/back.png"
    };

    camera.addSkybox("shaders/skybox.vs","shaders/skybox.fs", skyboxPaths);
    std::cout << "Loaded skybox" << std::endl;
   // GLSkybox skybox = GLSkybox(skyboxPaths);

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

    camera.useProgram("main");
    GLTextureArray tilemap = GLTextureArray();
    tilemap.loadFromFiles(texturePaths,160,160);
    std::cout << "Loaded textures" << std::endl;

    suncam.initialize();
    suncam.getTexture()->bind(1);

    int lightSpaceMatrixLoc = camera.getProgram("main").getUniformLocation("lightSpaceMatrix");
    int lightDirLoc = camera.getProgram("main").getUniformLocation("lightDir");

    int sunDirLoc = camera.getProgram("skybox").getUniformLocation("sunDir");
    int camPosSkyboxLoc = camera.getProgram("skybox").getUniformLocation("camPos");
    int camDirSkyboxLoc = camera.getProgram("skybox").getUniformLocation("camDir");
    
    int camPosLoc = camera.getProgram("main").getUniformLocation("camPos");

    //int TimeLoc = camera.getProgram("main").getUniformLocation("time");

    glUniform1i(camera.getProgram("main").getUniformLocation("textureArray"),0);
    glUniform1i(camera.getProgram("main").getUniformLocation("shadowMap"),1);

    double last = glfwGetTime();
    double current = glfwGetTime();

    int pregenDistance = renderDistance + 2;

    camera.setPosition(0.0f,160.0f,0.0f);

    world.getEntities().emplace_back(glm::vec3(0,160,0), glm::vec3(0.6, 1.8, 0.6));

    double seconds;
    while (!glfwWindowShouldClose(window)) {
        current = glfwGetTime();
        seconds = current - last;
        last = current;

        Entity& player = world.getEntities()[0];
        const glm::vec3& playerPosition = player.getPosition();
        glm::vec3 camPosition = playerPosition + camOffset;
        
        //std::cout << player.getVelocity().x << " " << player.getVelocity().y << " " << player.getVelocity().z << std::endl;
        camera.setPosition(camPosition);
        camera.updateUniforms();

        /*if(seconds >= 0.01){
            last = current;
            
            glUniform1f(TimeLoc, time);
            time = (time + 1) % 2400;
            //std::cout << time << std::endl;
        }*/

        //time = (time + 1) % 2400;
        //sunAngle = ((float)time / 2400.0) * 180.0;


        //std::cout << seconds << std::endl;
        //double fps = 1.0 / seconds;
        //std::cout << "FPS: " << fps << "Chunks loaded: " << world.chunksTotal() <<  std::endl;
        //int time = 0;

        int total = 0;
        for(int x = -pregenDistance; x <= pregenDistance; x++){
            for(int z = -pregenDistance; z <= pregenDistance; z++){
                int chunkX = x;
                int chunkZ = z;

                Chunk* meshlessChunk = world.getChunk(chunkX, chunkZ);
                if(!meshlessChunk){
                    meshlessChunk = world.generateAndGetChunk(chunkX, chunkZ);
                }
                total++;

                //std::cout << "\r Generation chunks: " << total << "/" << pow(pregenDistance * 2,2);
            }
        }
       // std::cout << std::endl;
        
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //if(boundKeys[0].isDown) accelY += 0.0006;
        //if(boundKeys[1].isDown) accelY -= 0.0006;
        glm::vec3 camDir = glm::normalize(camera.getDirection());
        glm::vec3 horizontalDir = {camDir.x, 0, camDir.z};
        glm::vec3 rightDir = normalize(glm::cross(camera.getUp(), horizontalDir));

        if(boundKeys[4].isDown) player.accelerate(rightDir * camSpeed);
        if(boundKeys[5].isDown) player.accelerate(-rightDir * camSpeed);
        if(boundKeys[3].isDown) player.accelerate(-horizontalDir * camSpeed);
        if(boundKeys[2].isDown) player.accelerate(horizontalDir * camSpeed);
        if(boundKeys[0].isDown) player.accelerate(camera.getUp());

        player.update(world);

        //printf("x:%f y:%f z:%f ax:%f ay:%f az:%f\ n",camX,camY,camZ,accelX,accelY,accelZ);

        //suncam.setPosition(c0,400, camera.getPosition().z);

        if(world.updated){
            float offsetX = camera.getPosition().x;
            float offsetZ = camera.getPosition().z;

            suncam.setPosition(
                offsetX + sunDistance * cos(glm::radians(sunAngle)), // X position (cosine component)
                sunDistance * sin(glm::radians(sunAngle)), // Y position (sine component for vertical angle)
                offsetZ  // Z position (cosine component)
            );
            suncam.setTarget(
                offsetX,
                0,
                offsetZ
            );
            suncam.updateProjection();

            glUniform3f(lightDirLoc, 
                -sunDistance * cos(glm::radians(sunAngle)), // X position (cosine component)
                -sunDistance * sin(glm::radians(sunAngle)), // Y position (sine component for vertical angle)
                0  // Z position (cosine component)
            );
            glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(suncam.getLightSpaceMatrix()));
            //suncam.updateProjection();
            //glDisable( GL_CULL_FACE );
            suncam.prepareForRender();
            world.drawChunks(suncam, renderDistance);
            //glEnable( GL_CULL_FACE );

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0,0,camera.getScreenWidth(),camera.getScreenHeight());

            world.updated = false;
        }


        camera.drawSkybox();
        glUniform3f(sunDirLoc, 
            -sunDistance * cos(glm::radians(sunAngle)), // X position (cosine component)
            -sunDistance * sin(glm::radians(sunAngle)), // Y position (sine component for vertical angle)
            0  // Z position (cosine component)
        );
        glUniform3f(camPosSkyboxLoc, camPosition.x, camPosition.y, camPosition.z);
        glUniform3f(camDirSkyboxLoc, camDir.x, camDir.y, camDir.z);

        camera.useProgram("main");
        glUniform3f(camPosLoc, camPosition.x, camPosition.y, camPosition.z);
        tilemap.bind(0);

        world.drawChunks(camera, renderDistance);
        //std::cout << "Drawn: " << total << "/" << pow(renderDistance * 2,2) << std::endl;
        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}