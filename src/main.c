#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>

#include <world.h>
#include <buffer.h>
#include <shaders.h>
#include <view.h>
#include <texture.h>
#include <standard.h>

ShaderProgram mainProgram;
ShaderProgram skyboxProgram;

int lastMouseX = 0;
int lastMouseY = 0;

int camAngleX = 0;
int camAngleY = 0;

float camX = 0;
float camY = 150;
float camZ = 0;

int renderDistance = 6;

float accelX = 0;
float accelZ = 0;
float accelY = 0;

float camSpeed = 0.002;
float M_PI_D180 = M_PI / 180;

float camFOV = 90;

RectangularCollider playerCollider = {.x = -0.3, .y = -1.7, .z = -0.3, .width = 0.6,.height = 1.8, .depth = 0.6};

World* world;

struct BoundKey{
    int key;
    int isDown;
} boundKeys[] = {
    {.key = GLFW_KEY_SPACE},
    {.key = GLFW_KEY_LEFT_SHIFT},
    {.key = GLFW_KEY_W},
    {.key = GLFW_KEY_S},
    {.key = GLFW_KEY_A},
    {.key = GLFW_KEY_D}
};
int boundKeysCount = arrayLen(boundKeys);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    for(int i = 0; i < boundKeysCount;i++){
        if(key == boundKeys[i].key && action == GLFW_PRESS) boundKeys[i].isDown = 1; 
        else if(key == boundKeys[i].key && action == GLFW_RELEASE) boundKeys[i].isDown = 0; 
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0,0,width,height);
    recalculateProjectionMatrix(&mainProgram, width, height, camFOV);
    recalculateProjectionMatrix(&skyboxProgram, width, height, camFOV);
    //printf("%i %i\n",width,height);
}

void cursor_position_callback(GLFWwindow* window, double mouseX, double mouseY)
{
    int mouseXDelta = mouseX - lastMouseX;
    int mouseYDelta = mouseY - lastMouseY;

    camAngleY = clampAngle(camAngleY + mouseXDelta);
    camAngleX = clampAngle(camAngleX + mouseYDelta);
    if(camAngleX > 270) camAngleX = 270;
    if(camAngleX < 110) camAngleX = 110;

    setViewRotation(&mainProgram, camAngleX + 180, camAngleY,0);
    setViewRotation(&skyboxProgram, camAngleX + 180, camAngleY,0);

    lastMouseX = mouseX;
    lastMouseY = mouseY;
}

static int selectedBlock = 1;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    RaycastResult hit = raycastFromAngles(world,camX,camY,camZ,camAngleX,camAngleY,10);
    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && hit.hitBlock){
        setWorldBlock(world, hit.x, hit.y, hit.z, 0);

        Chunk* chunk = getChunkFromBlockPosition(world, hit.x, hit.z);
        regenerateChunkMesh(chunk);
        chunk = getWorldChunk(world, chunk->worldX + 1, chunk->worldZ   ); regenerateChunkMesh(chunk);
        chunk = getWorldChunk(world, chunk->worldX - 1, chunk->worldZ   ); regenerateChunkMesh(chunk);
        chunk = getWorldChunk(world, chunk->worldX    , chunk->worldZ + 1); regenerateChunkMesh(chunk);
        chunk = getWorldChunk(world, chunk->worldX    , chunk->worldZ - 1); regenerateChunkMesh(chunk);

    }
    else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && hit.hitBlock){
        CollisionCheckResult result = checkForPointCollision(world, hit.lastX, hit.lastY, hit.lastZ, 1);

        setWorldBlock(world, result.x,  result.y,  result.z, selectedBlock);
        if(
            checkForRectangularCollision(world, camX,camY,camZ, &playerCollider).collision ||
            checkForRectangularCollision(world, camX + accelX,camY + accelY,camZ + accelZ, &playerCollider).collision
        ){
            setWorldBlock(world,  result.x,  result.y,  result.z, 0);
            return;
        }

        Chunk* chunk = getChunkFromBlockPosition(world, result.x, result.z);
        regenerateChunkMesh(chunk);
        chunk = getWorldChunk(world, chunk->worldX + 1, chunk->worldZ   ); regenerateChunkMesh(chunk);
        chunk = getWorldChunk(world, chunk->worldX - 1, chunk->worldZ   ); regenerateChunkMesh(chunk);
        chunk = getWorldChunk(world, chunk->worldX    , chunk->worldZ + 1); regenerateChunkMesh(chunk);
        chunk = getWorldChunk(world, chunk->worldX    , chunk->worldZ - 1); regenerateChunkMesh(chunk);

    }
    else if(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS){
        selectedBlock = (selectedBlock + 1) % 5 + 1;
    }
}

int main(void) {
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", glfwGetPrimaryMonitor(), NULL);
    if (!window) {
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
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    //glDepthMask(GL_FALSE);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 
    mainProgram = newShaderProgram();
    addVertexShader(&mainProgram, "shaders/vertex.vs");
    addFragmentShader(&mainProgram, "shaders/fragment.fs");
    compileShaderProgram(&mainProgram);
    setupProjection(&mainProgram, camFOV);

    skyboxProgram = newShaderProgram();
    addVertexShader(&skyboxProgram, "shaders/skybox.vs");
    addFragmentShader(&skyboxProgram, "shaders/skybox.fs");
    compileShaderProgram(&skyboxProgram);
    setupProjection(&skyboxProgram, camFOV);

    char* skyboxPaths[] = {
        "skybox/stars/right.png",
        "skybox/stars/left.png",
        "skybox/stars/top.png",
        "skybox/stars/bottom.png",
        "skybox/stars/front.png",
        "skybox/stars/back.png"
    };
    GLSkybox skybox = createSkybox(skyboxPaths, 6);
    GLTexture tilemap = createTexture(&mainProgram,"textures/tilemap.png");

    world = newWorld();

    clock_t last = clock();
    clock_t current = clock();

    int callsToExpand = 0;

    double seconds;
    while (!glfwWindowShouldClose(window)) {
        last = current;
        current = clock();
        seconds = (double)(current - last) / (double)CLOCKS_PER_SEC;

        double fps = 1.0 / seconds;

        if(fps >= 120) callsToExpand++;
        else callsToExpand = 0;

        if(callsToExpand > 10){
            printf("Expanding render distance %i\n", renderDistance);
            callsToExpand = 0;
            renderDistance++;
        }

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //if(boundKeys[0].isDown) accelY += 0.0006;
        //if(boundKeys[1].isDown) accelY -= 0.0006;

        int iCamAngleY = (360 - camAngleY);
        if(boundKeys[2].isDown){
            accelZ -= cos(M_PI_D180 * iCamAngleY) * camSpeed;
            accelX -= sin(M_PI_D180 * iCamAngleY) * camSpeed;
        }
        if(boundKeys[3].isDown){
            accelZ += cos(M_PI_D180 * iCamAngleY) * camSpeed;
            accelX += sin(M_PI_D180 * iCamAngleY) * camSpeed;
        }
        if(boundKeys[4].isDown){
            accelZ += cos(M_PI_D180 * clampAngle(iCamAngleY - 90)) * camSpeed;
            accelX += sin(M_PI_D180 * clampAngle(iCamAngleY - 90)) * camSpeed;
        }
        if(boundKeys[5].isDown){
            accelZ -= cos(M_PI_D180 * clampAngle(iCamAngleY - 90)) * camSpeed;
            accelX -= sin(M_PI_D180 * clampAngle(iCamAngleY - 90)) * camSpeed;
        }

        if(!boundKeys[2].isDown && !boundKeys[3].isDown && !boundKeys[4].isDown && !boundKeys[5].isDown){
            accelX = 0;
            accelZ = 0;
        }

        accelY -= 0.005;

        if(checkForRectangularCollision(world, camX + accelX,camY,camZ, &playerCollider).collision){
            accelX = 0;
        }
        else camX += accelX; 

        if(checkForRectangularCollision(world, camX,camY + accelY,camZ, &playerCollider).collision){
            accelY = 0;

            if(boundKeys[0].isDown) accelY += 0.15;
        }
        else camY += accelY;

        if(checkForRectangularCollision(world, camX,camY,camZ + accelZ, &playerCollider).collision){
            accelZ = 0;
        }
        else camZ += accelZ;

        //printf("x:%f y:%f z:%f ax:%f ay:%f az:%f\n",camX,camY,camZ,accelX,accelY,accelZ);

        useShaderProgram(&skyboxProgram);
        drawSkybox(&skybox);

        useShaderProgram(&mainProgram);
        bindTexture(&tilemap);

        int camWorldX = camX / DEFAULT_CHUNK_SIZE;
        int camWorldZ = camZ / DEFAULT_CHUNK_SIZE;

        for(int x = -renderDistance; x <= renderDistance; x++){
            for(int z = -renderDistance; z <= renderDistance; z++){
                int rx = x + camWorldX;
                int rz = z + camWorldZ;

                Chunk* chunk = getWorldChunkWithMesh(world, rx, rz);
                if(chunk == NULL) chunk = generateWorldChunk(world, rx, rz);
                if(!chunk->isDrawn) continue;

                setViewOffset(&mainProgram, -camX + rx * DEFAULT_CHUNK_SIZE, -camY, -camZ + rz * DEFAULT_CHUNK_SIZE);
                drawBuffer(&chunk->solidBuffer);
            }
        }
        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    destroySkybox(&skybox);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}