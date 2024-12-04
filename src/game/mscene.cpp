#include <game/mscene.hpp>

void MainScene::initialize(Scene* menuScene, UILoader* uiLoader){
    fpsLock = false;
    
    this->getUILayer("default").cursorMode = GLFW_CURSOR_DISABLED;
    this->getUILayer("chat").eventLocks = {true, true, true, true};
    this->getUILayer("menu").eventLocks = {true, true, true, true};
    this->getUILayer("settings").eventLocks = {true, true, true, true};

    this->setUILayer("default");

    auto crosshair = uiManager->createElement<UICrosshair>();
    crosshair->setSize(60,60);
    crosshair->setPosition(
        {OPERATION_MINUS, {PERCENT,50}, {MY_PERCENT, 50}},
        {OPERATION_MINUS, {PERCENT,50}, {MY_PERCENT, 50}}
    );
    crosshair->setAttribute(&UIFrame::Style::textColor, {255,255,255});

    getUILayer("default").addElement(crosshair);
    //bob.loadFromFile("models/dio_brando/scene.gltf", "models/dio_brando");
    
    std::array<std::string,6> skyboxPaths = {
        "skybox/stars/right.png",
        "skybox/stars/left.png",
        "skybox/stars/top.png",
        "skybox/stars/bottom.png",
        "skybox/stars/front.png",
        "skybox/stars/back.png"
    };  

    itemPrototypeRegistry.addPrototype(ItemPrototype("diamond","textures/diamond32.png"));
    itemPrototypeRegistry.addPrototype(ItemPrototype("crazed","textures/crazed32.png"));

    held_item_slot = std::make_shared<ItemSlot>(itemTextureAtlas,*uiManager);

    std::shared_ptr<ItemInventory> inventory = std::make_shared<ItemInventory>(itemTextureAtlas,*uiManager, 10,5, held_item_slot);
    inventory->setPosition(
        {OPERATION_MINUS, {PERCENT,50}, {MY_PERCENT,50}},
        {OPERATION_MINUS, {PERCENT,50}, {MY_PERCENT,50}}
    );
    inventory->setAttribute(&UIFrame::Style::backgroundColor, {20,20,20,100});
    inventory->setAttribute(&UIFrame::Style::borderWidth, {3,3,3,3});
    inventory->setAttribute(&UIFrame::Style::borderColor, {UIColor{0,0,0},{0,0,0},{0,0,0},{0,0,0}});

    auto item = itemPrototypeRegistry.createItem("diamond");
    auto item2 = itemPrototypeRegistry.createItem("crazed");
    inventory->setItem(1,0,item);
    inventory->setItem(0,4,item2);

    setUILayer("menu");
    addElement(inventory);
    addElement(held_item_slot);
    setUILayer("default");

    skyboxProgram.use();

    skybox.load(skyboxPaths);
    
    terrainProgram.use();
    
    blockTextureRegistry.addTexture("grass_top"     ,"textures/grass_top.png");
    blockTextureRegistry.addTexture("grass_side"    ,"textures/grass_side.png");
    blockTextureRegistry.addTexture("dirt"          ,"textures/dirt.png");
    blockTextureRegistry.addTexture("stone"         ,"textures/stone.png");
    blockTextureRegistry.addTexture("oak_log_top"   ,"textures/oak_log_top.png");
    blockTextureRegistry.addTexture("oak_log    "   ,"textures/oak_log.png");
    blockTextureRegistry.addTexture("oak_leaves"    ,"textures/oak_leaves.png");
    blockTextureRegistry.addTexture("birch_leaves"  ,"textures/birch_leaves.png");
    blockTextureRegistry.addTexture("birch_log"     ,"textures/birch_log.png");
    blockTextureRegistry.addTexture("birch_log_top" ,"textures/birch_log_top.png");
    blockTextureRegistry.addTexture("blue_wool"     ,"textures/blue_wool.png");
    blockTextureRegistry.addTexture("sand"          ,"textures/sand.png");
    blockTextureRegistry.addTexture("grass_bilboard","textures/grass_bilboard.png");
    
    blockTextureRegistry.setTextureSize(160,160);
    blockTextureRegistry.load();


    blockRegistry.addFullBlock("dirt", "dirt");
    blockRegistry.addFullBlock("stone", "stone");
    blockRegistry.addFullBlock("grass", {"grass_top","dirt","grass_side","grass_side","grass_side","grass_side"});
    blockRegistry.addFullBlock("oak_leaves", "oak_leaves", true);

    terrainProgram.setSamplerSlot("textureArray", 0);
    terrainProgram.setSamplerSlot("shadowMap", 1);

    modelProgram.setSamplerSlot("textureIn",0);

    camera.setPosition(0.0f,160.0f,0.0f);

    sunDirUniform.setValue({ 
        cos(glm::radians(sunAngle)), // X position (cosine component)
        sin(glm::radians(sunAngle)), // Y position (sine component for vertical angle)
        0  // Z position (cosine component)
    });

    terrainProgram.updateUniforms();
    skyboxProgram.updateUniforms();

    chunkMeshRegistry.initialize(renderDistance);

    camera.setModelPosition({0,0,0});
    suncam.setCaptureSize(renderDistance * 2 * CHUNK_SIZE);

    threadPool = std::make_unique<ThreadPool>(12);
    //world->load("saves/worldsave.bin");

    inputManager.bindKey(GLFW_KEY_W    , MOVE_FORWARD , "Move forward ");
    inputManager.bindKey(GLFW_KEY_S    , MOVE_BACKWARD, "Move backward");
    inputManager.bindKey(GLFW_KEY_A    , STRAFE_LEFT  , "Strafe left");
    inputManager.bindKey(GLFW_KEY_D    , STRAFE_RIGHT , "Strafe right");
    inputManager.bindKey(GLFW_KEY_SPACE, MOVE_UP      , "Jump");
    inputManager.bindKey(GLFW_KEY_LEFT_CONTROL, MOVE_DOWN, "Move down when flying");
    inputManager.bindKey(GLFW_KEY_LEFT_SHIFT, SCROLL_ZOOM, "Hold to zoom");

    auto settingsFrame = menuScene->getUILayer("settings").getElementById("keybind_container");

    for(auto& [key,action]: inputManager.getBoundKeys()){
        std::string kename = getKeyName(key,0);

        auto frame = uiManager->createElement<UIFrame>();
        frame->setSize({OPERATION_MINUS,{PERCENT,100},{10}}, 40);
        frame->setIdentifiers({"controlls_member"});

        auto name = uiManager->createElement<UILabel>();
        name->setText(action.name);
        name->setSize({PERCENT,80},40);
        name->setPosition(0,0);
        name->setHoverable(false);
        name->setIdentifiers({"controlls_member_name"});
        
        auto keyname = uiManager->createElement<UILabel>();
        keyname->setText(kename);
        keyname->setSize({OPERATION_MINUS,{PERCENT,20},5},40);
        keyname->setPosition({PERCENT,80},0);
        keyname->setFocusable(true);
        keyname->setIdentifiers({"controlls_member_keyname"});

        keyname->onKeyEvent = [this, key, action, keyname](GLFWwindow* window, int new_key, int /*scancode*/, int /*action*/, int /*mods*/){
            inputManager.unbindKey(key);
            inputManager.bindKey(new_key, action.action, action.name);

            std::string new_name = getKeyName(new_key,0);
            keyname->setText(new_name);
        };

        frame->appendChild(name);
        frame->appendChild(keyname);
        settingsFrame->appendChild(frame);
    
        uiLoader->getCurrentStyle().applyTo(frame);
        uiLoader->getCurrentStyle().applyTo(name);
        uiLoader->getCurrentStyle().applyTo(keyname);
        
    }

    menuScene->getUILayer("settings").getElementById("settings_scrollable")->calculateTransforms();

    auto mouse_settings = menuScene->getUILayer("settings").getElementById("mouse_sensitivity_container");

    auto sensitivity_slider = uiManager->createElement<UISlider>();
    sensitivity_slider->setValuePointer(&sensitivity);
    sensitivity_slider->setSize({PERCENT,60},{PERCENT,100});
    sensitivity_slider->setPosition({PERCENT,20},0);
    sensitivity_slider->setMin(1);
    sensitivity_slider->setMax(100);
    sensitivity_slider->setDisplayValue(true);
    sensitivity_slider->setIdentifiers({"mouse_sensitivity_slider"});
    
    mouse_settings->appendChild(sensitivity_slider);

    uiLoader->getCurrentStyle().applyTo(sensitivity_slider);
}

void MainScene::resize(GLFWwindow* window, int width, int height){
    camera.resizeScreen(width, height, camFOV);
    
    terrainProgram.updateUniforms();
    skyboxProgram.updateUniforms();
}

void MainScene::mouseMove(GLFWwindow* window, int mouseX, int mouseY){
    this->mouseX = mouseX;
    this->mouseY = mouseY;
    mouseMoved = true;
}

void MainScene::unlockedMouseMove(GLFWwindow* window, int mouseX, int mouseY){
    if(menuOpen){
        if(held_item_slot){
            held_item_slot->setPosition(mouseX,mouseY);
            held_item_slot->calculateTransforms();
            held_item_slot->update();
        }
    }
}
void MainScene::processMouseMovement(){
    if(!mouseMoved) return;
    else mouseMoved = false;

    float xoffset = (float)mouseX - lastMouseX;
    float yoffset = lastMouseY - (float)mouseY; // Reversed since y-coordinates go from bottom to top
    lastMouseX = (int)mouseX;
    lastMouseY = (int)mouseY;

    float real_sensitivity = static_cast<float>(sensitivity) / 100.0f;

    xoffset *= real_sensitivity;
    yoffset *= real_sensitivity * 2;

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


    updateCursor();
}

void MainScene::updateCursor(){
    glm::vec3& camDirection = camera.getDirection();
    glm::vec3 camPosition = camera.getPosition();

    RaycastResult hit = world->raycast(camPosition,camDirection,10);

    blockUnderCursor = hit.hitBlock;
    blockUnderCursorPosition = hit.position;
    blockUnderCursorEmpty = hit.lastPosition;

    if(!blockUnderCursor || blockUnderCursor->id == BLOCK_AIR_INDEX) wireframeRenderer.removeCube(0);
    else wireframeRenderer.setCube(0,glm::vec3(hit.position) - 0.005f, {1.01,01.01,1.01},{0,0,0});
    
    //wireframeRenderer.setCube(1,glm::vec3(hit.lastPosition) - 0.005f, {1.01,01.01,1.01},{1.0,0,0});
}

void MainScene::mouseEvent(GLFWwindow* window, int button, int action, int mods){
    if(!blockUnderCursor || blockUnderCursor->id == BLOCK_AIR_INDEX) return;

    glm::vec3& camDirection = camera.getDirection();
    glm::vec3 camPosition = camera.getPosition();

    if (
        button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS 
    ){
        world->setBlock(blockUnderCursorPosition, {BLOCK_AIR_INDEX});

        auto chunk = world->getChunkFromBlockPosition(blockUnderCursorPosition);
        if(!chunk) return;
        regenerateChunkMesh(chunk,world->getGetChunkRelativeBlockPosition(blockUnderCursorPosition));
    }
    else if(
        button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS
    ){
        glm::ivec3 blockPosition = glm::floor(blockUnderCursorEmpty);

        world->setBlock(blockPosition, {static_cast<BlockID>(selectedBlock)});
        if(
            player->checkForCollision(world.get(), false) ||
            player->checkForCollision(world.get(), true)
        ){
            world->setBlock(blockPosition, {BLOCK_AIR_INDEX});
            return;
        }

        auto* chunk = world->getChunkFromBlockPosition(blockPosition);
        if(!chunk) return;
        regenerateChunkMesh(chunk,world->getGetChunkRelativeBlockPosition(blockPosition));
    }

    updateCursor();
}

void MainScene::scrollEvent(GLFWwindow* window, double xoffset, double yoffset){
    if(inputManager.isActive(SCROLL_ZOOM)){
        camFOV -= (float) yoffset * 5.0f;

        if(camFOV < minFOV) camFOV = minFOV;
        else if(camFOV > maxFOV) camFOV = maxFOV;

        camera.adjustFOV(camFOV);
    }
    //else{
    //    selectedBlock = (selectedBlock + 1) % predefinedBlocks.size();
    //}
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
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        menuOpen = !menuOpen;
        if(menuOpen) this->setUILayer("menu");
        else this->setUILayer("default");
    }
}

void MainScene::open(GLFWwindow* window){
    running = true;
    allGenerated = false;

    player = std::make_shared<Entity>(glm::vec3(0,0,0), glm::vec3(0.6, 1.8, 0.6));
    
    world = std::make_unique<World>(worldPath, blockRegistry);
    world->addEntity(player);


    for(int i = 0;i < 20;i++){
        for(int j = 0;j < 20;j++){
            auto entity =  std::make_shared<DroppedItem>(itemPrototypeRegistry.createItem("crazed"), glm::vec3((float)i, 50, (float)j));
            world->addEntity(entity);
        }
    }

    chunkMeshGenerator.setWorld(world.get());

    //std::thread generationThread(std::bind(&MainScene::generateSurroundingChunks, this));
    //generationThread.detach();

    generateSurroundingChunks();

    //std::thread physicsThread(std::bind(&MainScene::pregenUpdate, this));
    std::thread physicsThread(std::bind(&MainScene::physicsUpdate, this));

    std::cout << "Threads started" << std::endl;
    physicsThread.detach();

    this->setUILayer("default");
    //pregenThread.detach();
}

void MainScene::close(GLFWwindow* window){
    threadsStopped = 0;
    running = false;

    chunkMeshGenerator.setWorld(nullptr);

    while(threadsStopped < 1){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } 

    world = nullptr;
    chunkMeshRegistry.clear();
}

void MainScene::render(){
    current = glfwGetTime();
    deltatime = (float)(current - last);
    last = current;

    threadPool->deployPendingJobs();
    if(!allGenerated){
        glDisable(GL_DEPTH_TEST);
        glDisable( GL_CULL_FACE );
        uiManager->getFontManager().renderText(
            "Generating chunks: " + std::to_string(world->chunksTotal()) + "/" + std::to_string(pow((renderDistance + 1)*2+1,3)),
        10,40, 1.0, {1.0,1.0,1.00}, testFont);
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable( GL_CULL_FACE );

    const glm::vec3& playerPosition = player->getPosition();
    glm::vec3 camPosition = playerPosition + camOffset;
    glm::vec3 camDir = glm::normalize(camera.getDirection());

    //std::cout << player.getVelocity().x << " " << player.getVelocity().y << " " << player.getVelocity().z << std::endl;
    camera.setPosition(camPosition);
    // std::cout << std::endl;
    //if(boundKeys[0].isDown) accelY += 0.0006;
    //if(boundKeys[1].isDown) accelY -= 0.0006;

    chunkMeshGenerator.loadMeshFromQueue(chunkMeshRegistry);

    processMouseMovement();

    bool updatedVisibility = false;
    if(updateVisibility > 0){
        auto istart = std::chrono::high_resolution_clock::now();

        //int consideredTotal = 0;
        //int *tr = &consideredTotal;

        chunkMeshRegistry.updateDrawCalls(camera.getPosition(), camera.getFrustum());

        //std::cout << (consideredTotal / pow(renderDistance*2,3)) * 100 << "%" << std::endl;
        //End time point
        auto iend = std::chrono::high_resolution_clock::now();
        //std::cout << "Iterated over all chunks in: " << std::chrono::duration_cast<std::chrono::microseconds>(iend - istart).count() << " microseconds" << std::endl;

        auto start = std::chrono::high_resolution_clock::now();
        updateVisibility = 0;
        updatedVisibility = true;
    }

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
    terrainProgram.updateUniforms();
    suncam.prepareForRender();
    chunkMeshRegistry.draw();
    
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
    blockTextureRegistry.getLoadedTextureArray().bind(0);

    //auto start = std::chrono::high_resolution_clock::now();

    camera.setModelPosition({0,0,0});
    terrainProgram.updateUniforms();
    chunkMeshRegistry.draw();

    //auto end = std::chrono::high_resolution_clock::now();
    //std::cout << "Drawn terrain in: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " microseconds" << std::endl;

    //std::cout << "Drawn: " << total << "/" << pow(renderDistance * 2,2) << std::endl;
    /* Swap front and back buffers */

    itemPrototypeRegistry.updateModelsDrawRequestBuffer();
    modelProgram.updateUniforms();
    itemPrototypeRegistry.drawItemModels();
    
    glDisable( GL_CULL_FACE );

    wireframeRenderer.draw();

    glClear(GL_DEPTH_BUFFER_BIT);

    glDisable( GL_CULL_FACE );
    glDisable(GL_DEPTH_TEST);   

    auto* selectedBlockDefinition = blockRegistry.getBlockPrototypeByIndex(selectedBlock);

    uiManager->getFontManager().renderText("FPS: " + std::to_string(1.0 / deltatime), 10,40, 1.0, {0,0,0}, testFont);
    if(selectedBlockDefinition) uiManager->getFontManager().renderText("Selected block: " + selectedBlockDefinition->name, 10, 80, 1.0, {0,0,0}, testFont);
    uiManager->getFontManager().renderText("X: " + std::to_string(playerPosition.x) + " Y: " + std::to_string(playerPosition.y) + "  Z: " + std::to_string(playerPosition.z), 10, 120, 1.0, {0,0,0}, testFont);

    glEnable(GL_DEPTH_TEST);
    glEnable( GL_CULL_FACE );
}

void MainScene::regenerateChunkMesh(Chunk* chunk){
    chunkMeshGenerator.syncGenerateSyncUploadMesh(chunk, chunkMeshRegistry);
    this->updateVisibility = 1;
    //chunkMeshRegistry.unloadChunkMesh(chunk->getWorldPosition());
}


#define regenMesh(position) { \
    Chunk* temp = this->world->getChunk(position);\
    if(temp) regenerateChunkMesh(temp);\
}
void MainScene::regenerateChunkMesh(Chunk* chunk, glm::vec3 blockCoords){
    regenerateChunkMesh(chunk);
    if(blockCoords.x == 0)              regenMesh(chunk->getWorldPosition() - glm::ivec3(1,0,0));
    if(blockCoords.x == CHUNK_SIZE - 1) regenMesh(chunk->getWorldPosition() + glm::ivec3(1,0,0));

    if(blockCoords.y == 0)              regenMesh(chunk->getWorldPosition() - glm::ivec3(0,1,0));
    if(blockCoords.y == CHUNK_SIZE - 1) regenMesh(chunk->getWorldPosition() + glm::ivec3(0,1,0));

    if(blockCoords.z == 0)              regenMesh(chunk->getWorldPosition() - glm::ivec3(0,0,1));
    if(blockCoords.z == CHUNK_SIZE - 1) regenMesh(chunk->getWorldPosition() + glm::ivec3(0,0,1));
}
#undef regenMesh

void MainScene::physicsUpdate(){
    double last = glfwGetTime();
    double current = glfwGetTime();
    float deltatime;

    float targetTPS = 60;
    float tickTime = 1.0f / targetTPS;

    while(running){
        current = glfwGetTime();
        deltatime = (float)(current - last);

        if(!allGenerated) continue;
        //std::cout << deltatime << "/" << tickTime << std::endl;
        if(deltatime < tickTime) continue;
        last = current;


        glm::ivec3 camWorldPosition = glm::floor(camera.getPosition() / static_cast<float>(CHUNK_SIZE));

        glm::vec3 camDir = glm::normalize(camera.getDirection());
        glm::vec3 horizontalDir = glm::normalize(glm::vec3(camDir.x, 0, camDir.z));
        glm::vec3 leftDir = glm::normalize(glm::cross(camera.getUp(), horizontalDir));

        if(inputManager.isActive(STRAFE_RIGHT )) player->accelerate(-leftDir * camSpeed);
        if(inputManager.isActive(STRAFE_LEFT  )) player->accelerate(leftDir * camSpeed);
        if(inputManager.isActive(MOVE_BACKWARD)) player->accelerate(-horizontalDir * camSpeed);
        if(inputManager.isActive(MOVE_FORWARD )) player->accelerate(horizontalDir * camSpeed);

        //if(boundKeys[1].isDown) player.accelerate(-camera.getUp() * 0.2f);
        if(inputManager.isActive(MOVE_UP)) player->accelerate(camera.getUp() * 0.2f);
        if(inputManager.isActive(MOVE_DOWN)) player->accelerate(-camera.getUp() * 0.2f);
        /*if(
            boundKeys[0].isDown 
            && player.checkForCollision(world, false, {0,-0.1f,0}).collision
            && player.getVelocity().y == 0
        ) player.accelerate(camera.getUp() * 0.2f);*/

        if(!world->getChunk(camWorldPosition)) continue;

        world->updateEntities();
        itemPrototypeRegistry.resetModelsDrawRequests();
        world->drawEntities();
        itemPrototypeRegistry.passModelsDrawRequests();
    }

    threadsStopped++;
}

void MainScene::generateSurroundingChunks(){
    glm::ivec3 camWorldPosition = glm::floor(camera.getPosition() / static_cast<float>(CHUNK_SIZE));
    int pregenDistance = renderDistance + 1; 
    
    int generatedTotal = 0;
    int* gptr = &generatedTotal;

    /*for(int x = -pregenDistance ; x <= pregenDistance; x++) for(int y = -pregenDistance; y <= pregenDistance; y++) for(int z = -pregenDistance; z <= pregenDistance; z++){
        int chunkX = camWorldX + x;
        int chunkY = camWorldY + y;
        int chunkZ = camWorldZ + z;

            //if(chunkY > 4) return;
        if(world->isChunkLoadable(chunkX,chunkY,chunkZ)){
            //std::cout << "Loading chunk" << std::endl;
            world->loadChunk(chunkX,chunkY,chunkZ);
            generatedTotal++;
        }
    }*/

        
    for(int x = -pregenDistance ; x <= pregenDistance; x++) for(int y = -pregenDistance; y <= pregenDistance; y++) for(int z = -pregenDistance; z <= pregenDistance; z++){
        glm::ivec3 chunkPosition = camWorldPosition + glm::ivec3(x,y,z);

        //if(chunkY > 4) return;

        world->generateChunk(chunkPosition);
    }

    for(int x = -renderDistance ; x <= renderDistance; x++) for(int y = -renderDistance; y <= renderDistance; y++) for(int z = -renderDistance; z <= renderDistance; z++){
        glm::ivec3 chunkPosition = camWorldPosition + glm::ivec3(x,y,z);
        
        Chunk* meshlessChunk = world->getChunk(chunkPosition);
        if(!meshlessChunk){
            std::cerr << "Chunk not generated when generating meshes?" << std::endl;
            continue;
        }
        chunkMeshGenerator.syncGenerateSyncUploadMesh(meshlessChunk, chunkMeshRegistry);
    }
    while(!threadPool->finished()){ // Wait for everything to generate
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

   
    allGenerated = true;
}

void UICrosshair::getRenderingInformation(RenderYeetFunction& yeet){
    auto color = getAttribute(&UIFrame::Style::textColor);

    // Left
    yeet(UIRenderInfo::Rectangle(
        transform.x,
        transform.y + transform.height / 2 - thickness / 2,
        transform.width / 2 - part_margin,
        thickness,
        color
    ),clipRegion);
    // Right
    yeet(UIRenderInfo::Rectangle(
        transform.x + transform.width / 2 + part_margin,
        transform.y + transform.height / 2 - thickness / 2,
        transform.width / 2 - part_margin,
        thickness,
        color
    ),clipRegion);

    // Top
    yeet(UIRenderInfo::Rectangle(
        transform.x + transform.width / 2 - thickness / 2,
        transform.y,
        thickness,
        transform.height / 2 - part_margin,
        color
    ),clipRegion);
    
    //Bottom
    yeet(UIRenderInfo::Rectangle(
        transform.x + transform.width  / 2 - thickness / 2,
        transform.y + transform.height / 2 + part_margin,
        thickness,
        transform.height / 2 - part_margin,
        color
    ),clipRegion);
}