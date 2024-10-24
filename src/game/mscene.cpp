#include <game/mscene.hpp>

void MainScene::initialize(Scene* mainScene){
    modelManager.initialize();
    camera.getProjectionUniform().attach(modelManager.getModelProgram());
    camera.getViewUniform().attach(modelManager.getModelProgram());

    fpsLock = false;
    
    std::unique_ptr<Command> tpCommand = std::make_unique<Command>(
        std::vector<CommandArgumentType>({INT,INT,INT}),
        [this](std::vector<CommandArgument> arguments){
            Entity& player = world->getEntities()[0];
            
            std::cout << arguments[0].intValue << " " <<  arguments[1].intValue  << " " << arguments[3].intValue << std::endl;
            player.setPosition({arguments[0].intValue,arguments[1].intValue,arguments[2].intValue});
        }
    );

    std::unique_ptr<Command> summonCommand = std::make_unique<Command>(
        std::vector<CommandArgumentType>({STRING}),
        [this](std::vector<CommandArgument> arguments){
            Entity& player = world->getEntities()[0];
            std::string& name = arguments[0].stringValue;

            if(name == "bob"){
                world->getEntities().emplace_back(player.getPosition(), glm::vec3(0.6, 1.8, 0.6));
                world->getEntities()[world->getEntities().size() - 1].setModel("bob");
            }
        }
    );

    std::unique_ptr<Command> saveWorldCommand = std::make_unique<Command>(
        std::vector<CommandArgumentType>({STRING}),
        [this](std::vector<CommandArgument> arguments){
            std::string& name = arguments[0].stringValue;

            //world->save("saves/" + name + ".bin");
        }
    );

    commandProcessor.addCommand({"teleport","tp"},std::move(tpCommand));
    commandProcessor.addCommand({"summon"},std::move(summonCommand));
    commandProcessor.addCommand({"save_world"}, std::move(saveWorldCommand));

    this->getUILayer("default").cursorMode = GLFW_CURSOR_DISABLED;
    this->getUILayer("chat").eventLocks = {true, true, true, true};
    this->getUILayer("menu").eventLocks = {true, true, true, true};
    this->getUILayer("settings").eventLocks = {true, true, true, true};

    auto chatInput = std::make_shared<UICommandInput>(
        &commandProcessor,
        TValue(PIXELS,0),
        TValue(OPERATION_MINUS,{FRACTIONS, 100}, {MFRACTION, 100}),
        TValue(FRACTIONS,100),
        TValue(PIXELS,50),
        glm::vec4(0,0,0,1)
    );
    this->chatInput = chatInput.get();

    this->chatInput->onSubmit = [this](std::string command) {
        commandProcessor.processCommand(command);
        this->chatInput->setText("");
    };
    
    this->setUILayer("chat");
    this->addElement(chatInput);
    
    /*
        Create the pause menu
    */
    this->setUILayer("menu");

    auto pauseMenuFrame = std::make_shared<UIFrame>(
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(PIXELS, 400),
        TValue(PIXELS, 700),
        glm::vec4(0.1,0.1,0.1,0.8)
    );

    auto exitButton = std::make_shared<UILabel>(
        "Exit", 
        TValue(OPERATION_MINUS,{PFRACTION, 50}, {MFRACTION, 50}),
        TValue(OPERATION_MINUS,{PFRACTION, 100}, {MFRACTION, 200}),
        TValue(PIXELS, 200),
        TValue(PIXELS, 40),
        glm::vec4(0.3,0.3,0.3,1)
    );
    exitButton->setHoverColor(glm::vec4(0.0,0.1,0.5,1.0));
    exitButton->onClicked = [this]() {
        this->sceneManager->setScene("menu");
    };

    auto resumeButton = std::make_shared<UILabel>(
        "Resume", 
        TValue(OPERATION_MINUS,{PFRACTION, 50}, {MFRACTION, 50}),
        TValue(MFRACTION, 100),
        TValue(PIXELS, 200),
        TValue(PIXELS, 40),
        glm::vec4(0.3,0.3,0.3,1)
    );
    resumeButton->setHoverColor(glm::vec4(0.0,0.1,0.5,1.0));
    resumeButton->onClicked = [this]() {
        this->setUILayer("default");
        menuOpen = false;
    };

    auto settingsButton = std::make_shared<UILabel>(
        "Settings", 
        TValue(OPERATION_MINUS,{PFRACTION, 50}, {MFRACTION, 50}),
        TValue(MFRACTION, 300),
        TValue(PIXELS, 200),
        TValue(PIXELS, 40),
        glm::vec4(0.3,0.3,0.3,1)
    );
    settingsButton->setHoverColor(glm::vec4(0.0,0.1,0.5,1.0));
    settingsButton->onClicked = [this]() {
        this->setUILayer("settings");
    };

    pauseMenuFrame->setBorderWidth({{PIXELS,8},{PIXELS,8},{PIXELS,8},{PIXELS,8}});

    pauseMenuFrame->appendChild(exitButton);
    pauseMenuFrame->appendChild(resumeButton);
    pauseMenuFrame->appendChild(settingsButton);

    this->addElement(pauseMenuFrame);

    /*
        ======
    */
    this->setUILayer("settings");

    auto settingsMenuFrame = std::make_shared<UIFrame>(
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(OPERATION_MINUS,{FRACTIONS, 50}, {MFRACTION, 50}),
        TValue(PIXELS, 700),
        TValue(PIXELS, 700),
        glm::vec4(0.1,0.1,0.1,0.8)
    );

    auto renderDistanceSlider = std::make_shared<UISlider>(
        TValue(OPERATION_MINUS,{PFRACTION, 50}, {MFRACTION, 50}),
        TValue(100),
        TValue(PFRACTION, 80),
        TValue(PIXELS, 20),
        &renderDistance,
        static_cast<uint32_t>(4), static_cast<uint32_t>(32),
        glm::vec4(0.3,0.3,0.3,1)
    );

    auto renderDistanceLabel = std::make_shared<UILabel>(
        "Render distance: ", 
        TValue(OPERATION_MINUS,{PFRACTION, 50}, {MFRACTION, 50}),
        TValue(10),
        TValue(PFRACTION, 80),
        TValue(PIXELS, 40),
        glm::vec4(0.3,0.3,0.3,0.0)
    );
    renderDistanceLabel->setBorderWidth({{PIXELS,0},{PIXELS,0},{PIXELS,0},{PIXELS,0}});

    settingsMenuFrame->setBorderWidth({{PIXELS,8},{PIXELS,8},{PIXELS,8},{PIXELS,8}});
    settingsMenuFrame->appendChild(renderDistanceSlider);
    settingsMenuFrame->appendChild(renderDistanceLabel);

    this->addElement(settingsMenuFrame);
    mainScene->getUILayer("settings").addElement(settingsMenuFrame);

    this->setUILayer("default");

    auto allocatorVisVertex = std::make_shared<UIAllocatorVisualizer>(
        TValue(PIXELS,10),
        TValue(PIXELS,10),
        TValue(FRACTIONS, 33),
        TValue(PIXELS, 20),
        glm::vec4(0.3,0.3,0.3,0.0),
        &chunkBuffer.getVertexAllocator()
    );

    auto allocatorVisIndex = std::make_shared<UIAllocatorVisualizer>(
        TValue(PIXELS,10),
        TValue(PIXELS,40),
        TValue(FRACTIONS, 33),
        TValue(PIXELS, 20),
        glm::vec4(0.3,0.3,0.3,0.0),
        &chunkBuffer.getIndexAllocator()
    );

    this->addElement(allocatorVisVertex);
    this->addElement(allocatorVisIndex);

    Model& bob = modelManager.createModel("bob");
    bob.loadFromFile("models/test.gltf", "");
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

    chunkBuffer.initialize(renderDistance);

    camera.setModelPosition({0,0,0});
    terrainProgram.updateUniform("modelMatrix");

    suncam.setCaptureSize(renderDistance * 2 * CHUNK_SIZE);

    threadPool = std::make_unique<ThreadPool>(10);
    //world->load("saves/worldsave.bin");
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
    Entity& player = world->getEntities()[0];

    RaycastResult hit = world->raycast(camPosition.x,camPosition.y,camPosition.z,camDirection.x, camDirection.y, camDirection.z,10);
    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && hit.hit){
        world->setBlock(hit.x, hit.y, hit.z, {BlockTypes::Air});

        auto chunk = world->getChunkFromBlockPosition(hit.x, hit.y, hit.z);
        if(!chunk) return;
        regenerateChunkMesh(*chunk,world->getGetChunkRelativeBlockPosition(hit.x, hit.y, hit.z));
        
    }
    else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && hit.hit){
        CollisionCheckResult result = world->checkForPointCollision(hit.lastX, hit.lastY, hit.lastZ, 1);

        world->setBlock(result.x,  result.y,  result.z, {static_cast<BlockTypes>(selectedBlock)});
        if(
            player.checkForCollision(*world, false).collision ||
            player.checkForCollision(*world, true).collision
        ){
            world->setBlock(result.x,  result.y,  result.z, {BlockTypes::Air});
            return;
        }

        auto chunk = world->getChunkFromBlockPosition(result.x, result.y,result.z);
        if(!chunk) return;
        regenerateChunkMesh(*chunk,world->getGetChunkRelativeBlockPosition(result.x, result.y,result.z));
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

void MainScene::unlockedKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(key == GLFW_KEY_T && action == GLFW_PRESS && !chatOpen){
        chatOpen = true;
        this->setUILayer("chat");
        uiManager->setFocus(this->chatInput);
    }
    else if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS && chatOpen){
        chatOpen = false;
        this->setUILayer("default");
    }
    else if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        menuOpen = !menuOpen;
        if(menuOpen) this->setUILayer("menu");
        else this->setUILayer("default");
    }
}

void MainScene::open(GLFWwindow* window){
    running = true;

    world = std::make_unique<World>(worldPath);
    world->getEntities().emplace_back(glm::vec3(-1,0,0), glm::vec3(0.6, 1.8, 0.6));

    std::thread physicsThread(std::bind(&MainScene::pregenUpdate, this));
    std::thread pregenThread(std::bind(&MainScene::physicsUpdate, this));

    std::cout << "Threads started" << std::endl;
    physicsThread.detach();
    pregenThread.detach();
}

void MainScene::close(GLFWwindow* window){
    threadsStopped = 0;
    running = false;

    while(threadsStopped < 2){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } 

    world = nullptr;
    chunkBuffer.clear();
}

void MainScene::render(){
    current = glfwGetTime();
    deltatime = (float)(current - last);
    last = current;

    glEnable(GL_DEPTH_TEST);
    glEnable( GL_CULL_FACE );

    Entity& player = world->getEntities()[0];
    const glm::vec3& playerPosition = player.getPosition();
    glm::vec3 camPosition = playerPosition + camOffset;
    glm::vec3 camDir = glm::normalize(camera.getDirection());

    //std::cout << player.getVelocity().x << " " << player.getVelocity().y << " " << player.getVelocity().z << std::endl;
    camera.setPosition(camPosition);
    // std::cout << std::endl;
    //if(boundKeys[0].isDown) accelY += 0.0006;
    //if(boundKeys[1].isDown) accelY -= 0.0006;

    //printf("x:%f y:%f z:%f ax:%f ay:%f az:%f\ n",camX,camY,camZ,accelX,accelY,accelZ);

    //suncam.setPosition(c0,400, camera.getPosition().z);
    
    int offsetX = ((int) camera.getPosition().x / 64) * 64;
    int offsetY = ((int) camera.getPosition().y / 64) * 64;
    int offsetZ = ((int) camera.getPosition().z / 64) * 64;

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
    suncam.setModelPosition({0,0,0});
    terrainProgram.updateUniform("modelMatrix");
    suncam.prepareForRender();
    chunkBuffer.draw();
    
    world->drawEntities(modelManager, suncam, true);
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

    camera.setModelPosition({0,0,0});
    terrainProgram.updateUniform("modelMatrix");
    chunkBuffer.draw();

    world->drawEntities(modelManager, camera);
    //std::cout << "Drawn: " << total << "/" << pow(renderDistance * 2,2) << std::endl;
    /* Swap front and back buffers */

    glClear(GL_DEPTH_BUFFER_BIT);

    glDisable( GL_CULL_FACE );
    glDisable(GL_DEPTH_TEST);

    uiManager->getFontManager().renderText("FPS: " + std::to_string(1.0 / deltatime), 10,40, 1.0, {0,0,0}, testFont);
    uiManager->getFontManager().renderText("Selected block: " + getBlockTypeName(static_cast<BlockTypes>(selectedBlock)), 10, 80, 1.0, {0,0,0}, testFont);
    uiManager->getFontManager().renderText("X: " + std::to_string(playerPosition.x) + " Y: " + std::to_string(playerPosition.y) + "  Z: " + std::to_string(playerPosition.z), 10, 120, 1.0, {0,0,0}, testFont);

    glEnable(GL_DEPTH_TEST);
    glEnable( GL_CULL_FACE );

    generateMeshes();
}

void MainScene::regenerateChunkMesh(Chunk& chunk){
    chunk.updateMesh();
    //chunkBuffer.unloadChunkMesh(chunk.getWorldPosition());
}


#define regenMesh(x,y,z) { \
    Chunk* temp = this->world->getChunk(x, y, z);\
    if(temp) regenerateChunkMesh(*temp);\
}
void MainScene::regenerateChunkMesh(Chunk& chunk, glm::vec3 blockCoords){
    regenerateChunkMesh(chunk);
    if(blockCoords.x == 0)              regenMesh((int) chunk.getWorldPosition().x - 1, (int) chunk.getWorldPosition().y, (int) chunk.getWorldPosition().z);
    if(blockCoords.x == CHUNK_SIZE - 1) regenMesh((int) chunk.getWorldPosition().x + 1, (int) chunk.getWorldPosition().y, (int) chunk.getWorldPosition().z);

    if(blockCoords.y == 0)              regenMesh((int) chunk.getWorldPosition().x, (int) chunk.getWorldPosition().y - 1, (int) chunk.getWorldPosition().z);
    if(blockCoords.y == CHUNK_SIZE - 1) regenMesh((int) chunk.getWorldPosition().x, (int) chunk.getWorldPosition().y + 1, (int) chunk.getWorldPosition().z);

    if(blockCoords.z == 0)              regenMesh((int) chunk.getWorldPosition().x, (int) chunk.getWorldPosition().y, (int) chunk.getWorldPosition().z - 1);
    if(blockCoords.z == CHUNK_SIZE - 1) regenMesh((int) chunk.getWorldPosition().x, (int) chunk.getWorldPosition().y, (int) chunk.getWorldPosition().z + 1);
}
#undef regenMesh

const double sqrtof3 = sqrt(3);

void MainScene::generateMeshes(){
    int camWorldX = (int) camera.getPosition().x / CHUNK_SIZE;
    int camWorldY = (int) camera.getPosition().y / CHUNK_SIZE;
    int camWorldZ = (int) camera.getPosition().z / CHUNK_SIZE;

    std::unordered_set<glm::vec3, Vec3Hash, Vec3Equal> currentPositions;
    
    for(int x = -renderDistance; x <= renderDistance; x++) for(int y = -renderDistance; y <= renderDistance; y++) for(int z = -renderDistance; z <= renderDistance; z++){
        int chunkX = x + camWorldX;
        int chunkY = y + camWorldY;
        int chunkZ = z + camWorldZ;

        //std::cout << "Drawing chunk: " << chunkX << " " << chunkZ << " " << camera.getPosition().x << " " << camera.getPosition().z <<std::endl;

        Chunk* meshlessChunk = world->getChunk(chunkX, chunkY, chunkZ);
        //std::cout << "Got chunk!" << std::endl;
        if(!meshlessChunk) continue;
        if(!camera.isVisible(*meshlessChunk)) continue;
        if(meshlessChunk->isEmpty()) continue;

        meshlessChunk->generateMesh(chunkBuffer, *threadPool);

        glm::vec3 position = {chunkX,chunkY,chunkZ};

        if(!chunkBuffer.isChunkLoaded(position)) continue;
        //chunkBuffer.addDrawCall(position);
        currentPositions.emplace(position);
        //if(chunk->solidBuffer) chunk->solidBuffer->getBuffer().draw();
    }

    bool changed = false;
    for(auto& pos: currentPositions){
        if(loadedPositions.find(pos) != loadedPositions.end()) continue;

        chunkBuffer.addDrawCall(pos);
        changed = true;
    }

    for(auto& pos: loadedPositions){
        if(currentPositions.find(pos) != currentPositions.end()) continue;

        chunkBuffer.removeDrawCall(pos);
        changed = true;
    }

    loadedPositions = currentPositions;

    if(changed){
        //circumscribed sphere of the render distance

        glm::vec3 camWorldPosition = {camWorldX, camWorldY, camWorldZ};
        float radius = glm::distance(glm::vec3(0,0,0), glm::vec3(static_cast<float>(renderDistance)));
        
        chunkBuffer.unloadFarawayChunks(camWorldPosition, radius);
        chunkBuffer.updateDrawCalls();
    }
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

        int camWorldX = (int) camera.getPosition().x / CHUNK_SIZE;
        int camWorldY = (int) camera.getPosition().y / CHUNK_SIZE;
        int camWorldZ = (int) camera.getPosition().z / CHUNK_SIZE;

        if(!world->getChunk(camWorldX, camWorldY,camWorldZ)) continue;

        Entity& player = world->getEntities()[0];

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

        world->updateEntities();
    }

    threadsStopped++;
}

void MainScene::pregenUpdate(){
    while(running){
        int camWorldX = (int) camera.getPosition().x / CHUNK_SIZE;
        int camWorldY = (int) camera.getPosition().y / CHUNK_SIZE;
        int camWorldZ = (int) camera.getPosition().z / CHUNK_SIZE;

        std::vector<std::thread> openThreads = {};

        int pregenDistance = renderDistance + 1; 
        
        for(int x = -pregenDistance; x <= pregenDistance; x++) for(int y = -pregenDistance; y <= pregenDistance; y++) for(int z = -pregenDistance; z <= pregenDistance; z++){
            int chunkX = camWorldX + x;
            int chunkY = camWorldY + y;
            int chunkZ = camWorldZ + z;

            Chunk* meshlessChunk = world->getChunk(chunkX, chunkY, chunkZ);
            if(!meshlessChunk){
                //meshlessChunk = world->generateAndGetChunk(chunkX, chunkY, chunkZ);

                if(world->isChunkLoadable(chunkX,chunkY,chunkZ)){
                    //std::cout << "Loading chunk" << std::endl;
                    world->loadChunk(chunkX,chunkY,chunkZ);
                    continue;
                }

                World* worldp = world.get();
                //std::cout << "Generating chunk" << std::endl;

                LODLevel level = FAR;
                float distance = glm::length(glm::vec3(x,y,z));
                if(distance < 6 ) level = CLOSE;
                else if(distance < 10 ) level = MID;
                else if(distance < 13) level = MID_FAR;

                threadPool->deploy([worldp,chunkX,chunkY,chunkZ, level](){
                    worldp->generateChunk(chunkX, chunkY, chunkZ, level);
                });
            }
        }
    }

    threadsStopped++;
}

std::vector<UIRenderInfo> UIAllocatorVisualizer::getRenderingInformation(UIManager& manager){
    std::vector<UIRenderInfo> out = {};

    auto memsize = watched->getMemorySize();
    auto t = getTransform(manager);
    int currentPosition = 0;

    for(auto& block: watched->getBlocks()){
        size_t bw = round((static_cast<float>(block.size) / static_cast<float>(memsize)) * static_cast<float>(t.width));
        
        UIColor color = block.used ? UIColor(1,0,0,1) : UIColor(0,1,0,1);

        out.push_back(UIRenderInfo::Rectangle(t.x + currentPosition, t.y, bw, t.height, color));
        currentPosition += bw;
    }

    std::vector<UIRenderInfo> temp = manager.buildTextRenderingInformation("Blocks total: " + std::to_string(watched->getBlocks().size()),t.x + t.width, t.y,1,{1,1,1,1});
    out.insert(out.end(), temp.begin(), temp.end());

    return out;
}