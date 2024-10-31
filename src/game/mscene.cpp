#include <game/mscene.hpp>

void MainScene::initialize(Scene* menuScene, UILoader* uiLoader){
    modelManager.initialize();
    camera.getProjectionUniform().attach(modelManager.getModelProgram());
    camera.getViewUniform().attach(modelManager.getModelProgram());

    fpsLock = false;
    
    std::unique_ptr<Command> tpCommand = std::make_unique<Command>(
        std::vector<CommandArgument::CommandArgumentType>({CommandArgument::INT,CommandArgument::INT,CommandArgument::INT}),
        [this](std::vector<CommandArgument> arguments){
            Entity& player = world->getEntities()[0];
            
            std::cout << arguments[0].intValue << " " <<  arguments[1].intValue  << " " << arguments[3].intValue << std::endl;
            player.setPosition({arguments[0].intValue,arguments[1].intValue,arguments[2].intValue});
        }
    );

    std::unique_ptr<Command> summonCommand = std::make_unique<Command>(
        std::vector<CommandArgument::CommandArgumentType>({CommandArgument::STRING}),
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
        std::vector<CommandArgument::CommandArgumentType>({CommandArgument::STRING}),
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
        "textures/grass_bilboard.png"
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

    chunkBuffer.initialize(renderDistance);

    camera.setModelPosition({0,0,0});
    terrainProgram.updateUniform("modelMatrix");

    suncam.setCaptureSize(renderDistance * 2 * CHUNK_SIZE);

    threadPool = std::make_unique<ThreadPool>(10);
    //world->load("saves/worldsave.bin");

    inputManager.bindKey(GLFW_KEY_W    , MOVE_FORWARD , "Move forward ");
    inputManager.bindKey(GLFW_KEY_S    , MOVE_BACKWARD, "Move backward");
    inputManager.bindKey(GLFW_KEY_A    , STRAFE_LEFT  , "Strafe left");
    inputManager.bindKey(GLFW_KEY_D    , STRAFE_RIGHT , "Strafe right");
    inputManager.bindKey(GLFW_KEY_SPACE, MOVE_UP      , "Jump");
    inputManager.bindKey(GLFW_KEY_LEFT_SHIFT, SCROLL_ZOOM, "Hold to zoom");

    auto settingsFrame = menuScene->getUILayer("settings").getElementById("keybind_container");

    for(auto& [key,action]: inputManager.getBoundKeys()){
        std::string kename = getKeyName(key,0);

        std::cout << kename << std::endl;

        auto frame = uiManager->createElement<UIFrame>();
        frame->setSize({OPERATION_MINUS,{PERCENT,100},{10}}, 40);

        auto name = uiManager->createElement<UILabel>();
        name->setText(action.name);
        name->setSize({PERCENT,80},40);
        name->setPosition(0,0);
        name->setHoverable(false);
        
        auto keyname = uiManager->createElement<UILabel>();
        keyname->setText(kename);
        keyname->setSize({OPERATION_MINUS,{PERCENT,20},5},40);
        keyname->setPosition({PERCENT,80},0);
        keyname->setFocusable(true);

        keyname->onKeyEvent = [this, key, action, keyname](GLFWwindow* window, int new_key, int /*scancode*/, int /*action*/, int /*mods*/){
            inputManager.unbindKey(key);
            inputManager.bindKey(new_key, action.action, action.name);

            std::string new_name = getKeyName(new_key,0);
            keyname->setText(new_name);
        };

        if(uiLoader->getCurrentStyle()){
            uiLoader->getCurrentStyle()->applyTo(frame, "frame", "", {"controlls_member"});
            uiLoader->getCurrentStyle()->applyTo(name, "label", "", {"controlls_member_name"});
            uiLoader->getCurrentStyle()->applyTo(keyname, "label", "", {"controlls_member_keyname"});
        }

        frame->appendChild(name);
        frame->appendChild(keyname);
        settingsFrame->appendChild(frame);
    }
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

    if(abs(lastCamPitch - camPitch) > chunkVisibilityUpdateThreshold){
        updateVisibility = 1;
        lastCamPitch = camPitch;
    }
    if(abs(lastCamYaw - camYaw) > chunkVisibilityUpdateThreshold){
        updateVisibility = 1;
        lastCamYaw = camYaw;
    }

    camera.setRotation(camPitch, camYaw);

    //std::cout << "Camera rotated: " << camPitch << " " << camYaw << std::endl;

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
    if(inputManager.isActive(SCROLL_ZOOM)){
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
    inputManager.keyEvent(window,key,scancode,action,mods);

    if(key == GLFW_KEY_M && action == GLFW_PRESS){
        lineMode = !lineMode;
        if(!lineMode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if(key == GLFW_KEY_K && action == GLFW_PRESS){
        updateVisibility = 1;
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
    generateSurroundingChunks();

    //std::thread physicsThread(std::bind(&MainScene::pregenUpdate, this));
    std::thread physicsThread(std::bind(&MainScene::physicsUpdate, this));

    std::cout << "Threads started" << std::endl;
    physicsThread.detach();
    //pregenThread.detach();
}

void MainScene::close(GLFWwindow* window){
    threadsStopped = 0;
    running = false;

    while(threadsStopped < 1){
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

    generateMeshes();

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
}

void MainScene::regenerateChunkMesh(Chunk& chunk){
    chunk.updateMesh();
    chunk.syncGenerateMesh(chunkBuffer);
    this->updateVisibility = 1;
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
    glm::ivec3 camWorldPosition = {
        camera.getPosition().x / CHUNK_SIZE,
        camera.getPosition().y / CHUNK_SIZE,
        camera.getPosition().z / CHUNK_SIZE
    };


    if(updateVisibility > 0){
        std::vector<DrawElementsIndirectCommand> drawCommands = {};
        drawCommands.reserve(20000);
        auto* drawcmdnptr = &drawCommands;
        auto istart = std::chrono::high_resolution_clock::now();

        //int consideredTotal = 0;
        //int *tr = &consideredTotal;

        findVisibleChunks(
            camera.getLocalFrustum(), 
            renderDistance, 
            [drawcmdnptr, camWorldPosition, this](glm::ivec3 local_position){
                auto pos = local_position + camWorldPosition;

                //(*tr)++;

                Chunk* meshlessChunk = world->getChunk(pos.x, pos.y, pos.z);
                if(!meshlessChunk) return;
                if(meshlessChunk->isEmpty()) return;
                if(meshlessChunk->needsMeshReload())  meshlessChunk->generateMesh(chunkBuffer, *threadPool);
                if(meshlessChunk->isMeshEmpty()) return;

                drawcmdnptr->push_back(this->chunkBuffer.getCommandFor(pos));
            }
        );

        //std::cout << (consideredTotal / pow(renderDistance*2,3)) * 100 << "%" << std::endl;
        //End time point
        auto iend = std::chrono::high_resolution_clock::now();
        std::cout << "Iterated over all chunks in: " << std::chrono::duration_cast<std::chrono::microseconds>(iend - istart).count() << " microseconds" << std::endl;
        
        //auto start = std::chrono::high_resolution_clock::now();

        chunkBuffer.updateDrawCalls(drawCommands);
        updateVisibility--;
    }
    //End time point
    //auto end = std::chrono::high_resolution_clock::now();

    //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //std::cout << "Updated draw calls in: " << duration << " microseconds" << std::endl;

    //float radius = glm::distance(glm::vec3(0,0,0), glm::vec3(static_cast<float>(renderDistance)));
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
        glm::vec3 leftDir = glm::normalize(glm::cross(camera.getUp(), horizontalDir));

        if(inputManager.isActive(STRAFE_RIGHT )) player.accelerate(-leftDir * camSpeed);
        if(inputManager.isActive(STRAFE_LEFT  )) player.accelerate(leftDir * camSpeed);
        if(inputManager.isActive(MOVE_BACKWARD)) player.accelerate(-horizontalDir * camSpeed);
        if(inputManager.isActive(MOVE_FORWARD )) player.accelerate(horizontalDir * camSpeed);

        //if(boundKeys[1].isDown) player.accelerate(-camera.getUp() * 0.2f);
        if(inputManager.isActive(MOVE_UP)) player.accelerate(camera.getUp() * 0.2f);
        /*if(
            boundKeys[0].isDown 
            && player.checkForCollision(world, false, {0,-0.1f,0}).collision
            && player.getVelocity().y == 0
        ) player.accelerate(camera.getUp() * 0.2f);*/

        world->updateEntities();
    }

    threadsStopped++;
}

void MainScene::generateSurroundingChunks(){
    int camWorldX = (int) camera.getPosition().x / CHUNK_SIZE;
    int camWorldY = (int) camera.getPosition().y / CHUNK_SIZE;
    int camWorldZ = (int) camera.getPosition().z / CHUNK_SIZE;

    std::vector<std::thread> openThreads = {};

    int pregenDistance = renderDistance + 1; 
    
    int generatedTotal = 0;
    int* gptr = &generatedTotal;

    bool notAllGenerated = true;
    while(notAllGenerated){
        notAllGenerated = false;
        for(int x = -pregenDistance; x <= pregenDistance; x++) for(int y = -pregenDistance; y <= pregenDistance; y++) for(int z = -pregenDistance; z <= pregenDistance; z++){
            int chunkX = camWorldX + x;
            int chunkY = camWorldY + y;
            int chunkZ = camWorldZ + z;

            //if(chunkY > 4) return;

            Chunk* meshlessChunk = world->getChunk(chunkX, chunkY, chunkZ);
            if(!meshlessChunk){
                notAllGenerated = true;
                //meshlessChunk = world->generateAndGetChunk(chunkX, chunkY, chunkZ);

                if(world->isChunkLoadable(chunkX,chunkY,chunkZ)){
                    //std::cout << "Loading chunk" << std::endl;
                    world->loadChunk(chunkX,chunkY,chunkZ);
                    generatedTotal++;
                    continue;
                }

                World* worldp = world.get();
                auto* bp = &chunkBuffer;
                //std::cout << "Generating chunk" << std::endl;

                LODLevel level = FAR;
                float distance = glm::length(glm::vec3(x,y,z));
                if(distance < 6 ) level = CLOSE;
                else if(distance < 10 ) level = MID;
                else if(distance < 13) level = MID_FAR;

                bool success = threadPool->deploy([worldp,chunkX,chunkY,chunkZ, level, bp,gptr](){
                    worldp->generateChunk(chunkX, chunkY, chunkZ, level);
                    (*gptr)++;
                });
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Wating for chunks to generate... " << generatedTotal << "/" << pow(pregenDistance*2,3) << std::endl;
    }
    std::cout << "Generating meshes..." << std::endl;
    for(int x = -renderDistance; x <= renderDistance; x++) for(int y = -renderDistance; y <= renderDistance; y++) for(int z = -renderDistance; z <= renderDistance; z++){
        int chunkX = camWorldX + x;
        int chunkY = camWorldY + y;
        int chunkZ = camWorldZ + z;
        
        Chunk* meshlessChunk = world->getChunk(chunkX, chunkY, chunkZ);
        if(!meshlessChunk){
            std::cout << "Thats a bug mate!" << std::endl;
            continue;
        }
        meshlessChunk->syncGenerateMesh(chunkBuffer);
    }
}

void UIAllocatorVisualizer::getRenderingInformation(RenderYeetFunction& yeet){
    auto memsize = watched->getMemorySize();
    int currentPosition = 0;

    for(auto& block: watched->getBlocks()){
        size_t bw = round((static_cast<float>(block.size) / static_cast<float>(memsize)) * static_cast<float>(transform.width));
        
        UIColor color = block.used ? UIColor(1,0,0,1) : UIColor(0,1,0,1);

        yeet(UIRenderInfo::Rectangle(transform.x + currentPosition, transform.y, bw, transform.height, color),clipRegion);
        currentPosition += bw;
    }

    manager.buildTextRenderingInformation(yeet,clipRegion, "Blocks total: " + std::to_string(watched->getBlocks().size()),transform.x + transform.width, transform.y,1,{1,1,1,1});
}