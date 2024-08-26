#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#include <world.hpp>
#include <rendering/buffer.hpp>
#include <rendering/shaders.hpp>
#include <rendering/texture.hpp>
#include <rendering/camera.hpp>

Camera camera;

int lastMouseX = 0;
int lastMouseY = 0;
float sensitivity = 0.1;

float camPitch = 0;
float camYaw = 0;

float camX = 0;
float camY = 150;
float camZ = 0;

int renderDistance = 8;

float accelX = 0;
float accelZ = 0;
float accelY = 0;

float camSpeed = 0.002;
float M_PI_D180 = M_PI / 180;

float camFOV = 90;
float halfCamFov = 50;

RectangularCollider playerCollider = {-0.3,-1.7, -0.3, 0.6, 1.8, 0.6};
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

static bool lineMode = false;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0,0,width,height);
    camera.resizeScreen(width, height, camFOV);
    camera.updateUniforms();
    //printf("%i %i\n",width,height);
}

void cursor_position_callback(GLFWwindow* window, double mouseX, double mouseY)
{
    float xoffset = mouseX - lastMouseX;
    float yoffset = lastMouseY - mouseY; // Reversed since y-coordinates go from bottom to top
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camYaw += xoffset;
    camPitch += yoffset;

    camYaw = std::fmod((camYaw + xoffset), (GLfloat)360.0f);

    // Constrain the pitch to avoid gimbal lock
    if (camPitch > 89.0f)
        camPitch = 89.0f;
    if (camPitch < -89.0f)
        camPitch = -89.0f;

    camera.setRotation(camPitch, camYaw);
}

//static int selectedBlock = 1;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    glm::vec3& camDirection = camera.getDirection();

    RaycastResult hit = world.raycast(camX,camY,camZ,camDirection.x, camDirection.y, camDirection.z,10);
    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && hit.hit){
        world.setBlock(hit.x, hit.y, hit.z, {BlockTypes::Air});

        auto chunk = world.getChunkFromBlockPosition(hit.x, hit.z);
        if(!chunk) return;
        chunk->regenerateMesh();
        
        /*chunk = getWorldChunk(chunk.worldX + 1, chunk.worldZ   ); regenerateChunkMesh(chunk);
        chunk = getWorldChunk(chunk.worldX - 1, chunk.worldZ   ); regenerateChunkMesh(chunk);
        chunk = getWorldChunk(chunk.worldX    , chunk.worldZ + 1); regenerateChunkMesh(chunk);
        chunk = getWorldChunk(chunk.worldX    , chunk.worldZ - 1); regenerateChunkMesh(chunk);*/
    }
    else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && hit.hit){
        CollisionCheckResult result = world.checkForPointCollision(hit.lastX, hit.lastY, hit.lastZ, 1);

        world.setBlock(result.x,  result.y,  result.z, {BlockTypes::BlueWool});
        if(
            world.checkForRectangularCollision(camX,camY,camZ, &playerCollider).collision ||
            world.checkForRectangularCollision(camX + accelX,camY + accelY,camZ + accelZ, &playerCollider).collision
        ){
            world.setBlock(result.x,  result.y,  result.z, {BlockTypes::Air});
            return;
        }

        auto chunk = world.getChunkFromBlockPosition(result.x, result.z);
        if(!chunk) return;
        chunk->regenerateMesh();
        /*chunk = getWorldChunk(world, chunk.worldX + 1, chunk.worldZ   ); regenerateChunkMesh(chunk);
        chunk = getWorldChunk(world, chunk.worldX - 1, chunk.worldZ   ); regenerateChunkMesh(chunk);
        chunk = getWorldChunk(world, chunk.worldX    , chunk.worldZ + 1); regenerateChunkMesh(chunk);
        chunk = getWorldChunk(world, chunk.worldX    , chunk.worldZ - 1); regenerateChunkMesh(chunk);*/

    }
    /*else if(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS){
        selectedBlock = (selectedBlock + 1) % 5 + 1;
    }*/
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(1920, 1080, "Hello World", NULL, NULL);

    if (!window) {
        std::cout << "Failed to initialize glfw window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize glad!" << std::endl;
        return -1;
    }

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

    camera.getProgram("main").use();
    GLTextureArray tilemap = GLTextureArray();
    tilemap.loadFromFiles(texturePaths,160,160);

    int camPosLoc = glGetUniformLocation(camera.getProgram("main").getID(),"camPos");

    clock_t last = clock();
    clock_t current = clock();

    int callsToExpand = 0;

    double seconds;
    while (!glfwWindowShouldClose(window)) {
        last = current;
        current = clock();
        seconds = (double)(current - last) / (double)CLOCKS_PER_SEC;

        double fps = 1.0 / seconds;
        //printf("FPS: %f\n",fps);


        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //if(boundKeys[0].isDown) accelY += 0.0006;
        //if(boundKeys[1].isDown) accelY -= 0.0006;

        int iCamAngleY = clampAngle(360 - camYaw);
        if(boundKeys[4].isDown){
            accelZ -= cos(M_PI_D180 * iCamAngleY) * camSpeed;
            accelX -= sin(M_PI_D180 * iCamAngleY) * camSpeed;
        }
        if(boundKeys[5].isDown){
            accelZ += cos(M_PI_D180 * iCamAngleY) * camSpeed;
            accelX += sin(M_PI_D180 * iCamAngleY) * camSpeed;
        }
        if(boundKeys[3].isDown){
            accelZ += cos(M_PI_D180 * clampAngle(iCamAngleY - 90)) * camSpeed;
            accelX += sin(M_PI_D180 * clampAngle(iCamAngleY - 90)) * camSpeed;
        }
        if(boundKeys[2].isDown){
            accelZ -= cos(M_PI_D180 * clampAngle(iCamAngleY - 90)) * camSpeed;
            accelX -= sin(M_PI_D180 * clampAngle(iCamAngleY - 90)) * camSpeed;
        }

        if(!boundKeys[2].isDown && !boundKeys[3].isDown && !boundKeys[4].isDown && !boundKeys[5].isDown){
            accelX = 0;
            accelZ = 0;
        }

        accelY -= 0.005;
        if(boundKeys[0].isDown && accelY < 1.0) accelY += 0.01;

        if(world.checkForRectangularCollision(camX + accelX,camY,camZ, &playerCollider).collision){
            accelX = 0;
        }
        else camX += accelX; 
        
        if(world.checkForRectangularCollision(camX,camY + accelY,camZ, &playerCollider).collision){
            accelY = 0;
        }
        else camY += accelY;

        if(world.checkForRectangularCollision(camX,camY,camZ + accelZ, &playerCollider).collision){
            accelZ = 0;
        }
        else camZ += accelZ;


        //printf("x:%f y:%f z:%f ax:%f ay:%f az:%f\n",camX,camY,camZ,accelX,accelY,accelZ);
        camera.setPosition(camX, camY, camZ);
        camera.updateUniforms();
        camera.drawSkybox();

        camera.getProgram("main").use();
        glUniform3f(camPosLoc, camX, camY, camZ);
        tilemap.bind();

        int camWorldX = camX / DEFAULT_CHUNK_SIZE;
        int camWorldZ = camZ / DEFAULT_CHUNK_SIZE;

        float offsetCamX = camX - cos(M_PI_D180 * camYaw) * (-camPitch - 90 + camY);
        float offsetCamZ = camZ - sin(M_PI_D180 * camYaw) * (-camPitch - 90 + camY);

        for(int x = -renderDistance; x <= renderDistance; x++){
            for(int z = -renderDistance; z <= renderDistance; z++){
                int chunkX = x + camWorldX;
                int chunkZ = z + camWorldZ;

                int realChunkX = chunkX * DEFAULT_CHUNK_SIZE + DEFAULT_CHUNK_SIZE/2;
                int realChunkZ = chunkZ * DEFAULT_CHUNK_SIZE + DEFAULT_CHUNK_SIZE/2;

                float angle = clampAngle(atan2(offsetCamZ - realChunkZ, offsetCamX - realChunkX) * 180 / M_PI - 90);
                float difference = abs(angle - (camYaw + 90));
                difference = fmin(difference, 360 - difference);

                if(difference > halfCamFov) continue; 

                Chunk* chunk = world.getChunkWithMesh(chunkX, chunkZ);
                if(!chunk){
                    if(!world.getChunk(chunkX, chunkZ)) world.generateAndGetChunk(chunkX, chunkZ);
                    continue;
                }
            
                
                if(!chunk->isDrawn) continue;

                camera.setModelPosition(chunkX * DEFAULT_CHUNK_SIZE,0,chunkZ * DEFAULT_CHUNK_SIZE);
                camera.updateUniforms();
                camera.getProgram("main").use();

                chunk->solidBuffer->getBuffer().draw();
            }
        }
        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}