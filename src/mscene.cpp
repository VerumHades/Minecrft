#include <mscene.hpp>

void MainScene::initialize(){
    modelManager.initialize();
    camera.getProjectionUniform().attach(modelManager.getModelProgram());
    camera.getViewUniform().attach(modelManager.getModelProgram());

    Model& bob = modelManager.createModel("bob");
    bob.loadFromFile("models/test.gltf", "models/dio_brando");
    //bob.loadFromFile("models/dio_brando/scene.gltf", "models/dio_brando");
    
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

    skybox.load(skyboxPaths);
    
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
        "textures/sand.png",
        "textures/pig.png"
    };
    
    terrainProgram.initialize();
    terrainProgram.addShader("shaders/terrain.vs", GL_VERTEX_SHADER);
    terrainProgram.addShader("shaders/terrain.fs", GL_FRAGMENT_SHADER);
    terrainProgram.compile();
    terrainProgram.use();

    tilemap.loadFromFiles(texturePaths,160,160);

    suncam.initialize();
    suncam.getLightSpaceMatrixUniform().attach(terrainProgram);
    suncam.getLightSpaceMatrixUniform().attach(modelManager.getModelProgram());
    suncam.getLightSpaceMatrixUniform().attach(modelManager.getModelDepthProgram());
    suncam.getModelUniform().attach(modelManager.getModelDepthProgram());

    //int sunDirLoc = camera.getProgram("skybox").getUniformLocation("sunDir");
    sunDirUniform.attach(skyboxProgram);
    sunDirUniform.attach(terrainProgram);
    
    //int TimeLoc = terrainProgram.getUniformLocation("time");

    glUniform1i(terrainProgram.getUniformLocation("textureArray"),0);
    glUniform1i(terrainProgram.getUniformLocation("shadowMap"),1);

    camera.initialize({skyboxProgram, terrainProgram, modelManager.getModelProgram()});
    camera.setPosition(0.0f,160.0f,0.0f);

    world.getEntities().emplace_back(glm::vec3(-1,50,0), glm::vec3(0.6, 1.8, 0.6));
    
    world.getEntities().emplace_back(glm::vec3(0,160,0), glm::vec3(0.6, 1.8, 0.6));
    world.getEntities()[1].setModel("bob");

    sunDirUniform.setValue({ 
        cos(glm::radians(sunAngle)), // X position (cosine component)
        sin(glm::radians(sunAngle)), // Y position (sine component for vertical angle)
        0  // Z position (cosine component)
    });

    terrainProgram.updateUniforms();
    skyboxProgram.updateUniforms();

    boundKeys = {
        {GLFW_KEY_SPACE},
        {GLFW_KEY_LEFT_SHIFT},
        {GLFW_KEY_W},
        {GLFW_KEY_S},
        {GLFW_KEY_A},
        {GLFW_KEY_D}
    };
}

void MainScene::resize(GLFWwindow* window, int width, int height){
    camera.resizeScreen(width, height, camFOV);
    
    terrainProgram.updateUniforms();
    skyboxProgram.updateUniforms();
}

void MainScene::mouseMove(GLFWwindow* window, int mouseX, int mouseY){
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

    terrainProgram.updateUniforms();
    skyboxProgram.updateUniforms();
}

void MainScene::mouseEvent(GLFWwindow* window, int button, int action, int mods){
    glm::vec3& camDirection = camera.getDirection();
    glm::vec3 camPosition = camera.getPosition();
    Entity& player = world.getEntities()[0];

    RaycastResult hit = world.raycast(camPosition.x,camPosition.y,camPosition.z,camDirection.x, camDirection.y, camDirection.z,10);
    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && hit.hit){
        world.setBlock(hit.x, hit.y, hit.z, {BlockTypes::Air});

        auto chunk = world.getChunkFromBlockPosition(hit.x, hit.y, hit.z);
        if(!chunk) return;
        chunk->regenerateMesh(world.getGetChunkRelativeBlockPosition(hit.x, hit.y, hit.z));
        
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

        auto chunk = world.getChunkFromBlockPosition(result.x, result.y,result.z);
        if(!chunk) return;
        chunk->regenerateMesh(world.getGetChunkRelativeBlockPosition(result.x, result.y,result.z));
    }
}

void MainScene::scrollEvent(GLFWwindow* window, double xoffset, double yoffset){
    if(boundKeys[1].isDown){
        camFOV -= (float) yoffset * 5.0f;

        if(camFOV < minFOV) camFOV = minFOV;
        else if(camFOV > maxFOV) camFOV = maxFOV;

        camera.adjustFOV(camFOV);
    }
    else{
        selectedBlock = (selectedBlock + 1) % predefinedBlocks.size();
    }
}

void MainScene::keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods){
    for(int i = 0; i < boundKeys.size();i++){
        if(key == boundKeys[i].key && action == GLFW_PRESS) boundKeys[i].isDown = true; 
        else if(key == boundKeys[i].key && action == GLFW_RELEASE) boundKeys[i].isDown = false; 
    }

    if(key == GLFW_KEY_M && action == GLFW_PRESS){
        lineMode = !lineMode;
        if(!lineMode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void MainScene::open(GLFWwindow* window){
    running = true;

    std::thread physicsThread(std::bind(&MainScene::pregenUpdate, this));
    std::thread pregenThread(std::bind(&MainScene::physicsUpdate, this));

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    std::cout << "Threads started" << std::endl;
    physicsThread.detach();
    pregenThread.detach();
}

void MainScene::close(GLFWwindow* window){
    running = false;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void MainScene::render(){
    current = glfwGetTime();
    deltatime = (float)(current - last);
    last = current;

    glEnable(GL_DEPTH_TEST);
    glEnable( GL_CULL_FACE );

    Entity& player = world.getEntities()[0];
    const glm::vec3& playerPosition = player.getPosition();
    glm::vec3 camPosition = playerPosition + camOffset;
    glm::vec3 camDir = glm::normalize(camera.getDirection());

    //std::cout << player.getVelocity().x << " " << player.getVelocity().y << " " << player.getVelocity().z << std::endl;
    camera.setPosition(camPosition);
    // std::cout << std::endl;

    world.updateBuffers();
    //if(boundKeys[0].isDown) accelY += 0.0006;
    //if(boundKeys[1].isDown) accelY -= 0.0006;

    //printf("x:%f y:%f z:%f ax:%f ay:%f az:%f\ n",camX,camY,camZ,accelX,accelY,accelZ);

    //suncam.setPosition(c0,400, camera.getPosition().z);
    
    int offsetX = (int) camera.getPosition().x;
    int offsetY = 0;
    int offsetZ = (int) camera.getPosition().z;

    suncam.getTexture()->bind(1);
    
    suncam.setPosition(
        (float) offsetX + sunDistance * cos(glm::radians(sunAngle)), // X position (cosine component)
        (float) offsetY + sunDistance * sin(glm::radians(sunAngle)), // Y position (sine component for vertical angle)
        (float) offsetZ  // Z position (cosine component)
    );
    suncam.setTarget(
        (float) offsetX,
        (float) offsetY,
        (float) offsetZ
    );
    suncam.updateProjection();

    //suncam.updateProjection();
    glDisable( GL_CULL_FACE );
    suncam.prepareForRender();
    world.drawChunks(suncam, suncam.getProgram(), renderDistance);
    world.drawEntities(modelManager, suncam, true);
    glEnable( GL_CULL_FACE );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0,0,camera.getScreenWidth(),camera.getScreenHeight());


    //skyboxProgram.updateUniforms();
    
    glDisable(GL_CULL_FACE);
    skyboxProgram.use();
    skybox.draw();
    glEnable(GL_CULL_FACE);

    terrainProgram.updateUniforms();
    terrainProgram.use();
    tilemap.bind(0);

    world.drawChunks(camera, terrainProgram, renderDistance);
    world.drawEntities(modelManager, camera);
    //std::cout << "Drawn: " << total << "/" << pow(renderDistance * 2,2) << std::endl;
    /* Swap front and back buffers */

    glClear(GL_DEPTH_BUFFER_BIT);

    glDisable( GL_CULL_FACE );
    glDisable(GL_DEPTH_TEST);

    manager->getFontManager().renderText("FPS: " + std::to_string(1.0 / deltatime), 10,40, 1.0, {0,0,0}, testFont);
    manager->getFontManager().renderText("Selected block: " + getBlockTypeName(static_cast<BlockTypes>(selectedBlock)), 10, 80, 1.0, {0,0,0}, testFont);

    glEnable(GL_DEPTH_TEST);
    glEnable( GL_CULL_FACE );
    
}

void MainScene::physicsUpdate(){
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
        glm::vec3 rightDir = glm::normalize(glm::cross(camera.getUp(), horizontalDir));

        if(boundKeys[4].isDown) player.accelerate(rightDir * camSpeed);
        if(boundKeys[5].isDown) player.accelerate(-rightDir * camSpeed);
        if(boundKeys[3].isDown) player.accelerate(-horizontalDir * camSpeed);
        if(boundKeys[2].isDown) player.accelerate(horizontalDir * camSpeed);

        if(boundKeys[1].isDown) player.accelerate(-camera.getUp() * 0.2f);
        if(boundKeys[0].isDown) player.accelerate(camera.getUp() * 0.2f);
        /*if(
            boundKeys[0].isDown 
            && player.checkForCollision(world, false, {0,-0.1f,0}).collision
            && player.getVelocity().y == 0
        ) player.accelerate(camera.getUp() * 0.2f);*/

        world.updateEntities();
    }
}

void MainScene::pregenUpdate(){
    int pregenDistance = renderDistance + 2;

    while(running){
        int camWorldX = (int) camera.getPosition().x / CHUNK_SIZE;
        int camWorldZ = (int) camera.getPosition().z / CHUNK_SIZE;

        for(int x = -pregenDistance; x <= pregenDistance; x++){
            for(int z = -pregenDistance; z <= pregenDistance; z++){
                int chunkX = camWorldX + x;
                int chunkZ = camWorldZ + z;

                Chunk* meshlessChunk = world.getChunk(chunkX, 0, chunkZ);
                if(!meshlessChunk){
                    meshlessChunk = world.generateAndGetChunk(chunkX, 0, chunkZ);
                }
            }
        }
    }
}