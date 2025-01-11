#include <game/main_scene.hpp>

void MainScene::initialize(){
    fpsLock = false;

    Scene* menuScene = sceneManager->getScene("menu");
    
    this->getUILayer("default").cursorMode = GLFW_CURSOR_DISABLED;
    this->getUILayer("chat").eventLocks = {true, true, true, true};
    this->getUILayer("menu").eventLocks = {true, true, true, true};
    this->getUILayer("settings").eventLocks = {true, true, true, true};
    this->getUILayer("structure_capture").cursorMode = GLFW_CURSOR_DISABLED;

    this->setUILayer("default");

    fps_label = uiManager->createElement<UILabel>();
    fps_label->setPosition(10,10);
    addElement(fps_label);

    auto crosshair = uiManager->createElement<UICrosshair>();
    crosshair->setSize(60,60);
    crosshair->setPosition(
        {OPERATION_MINUS, {PERCENT,50}, {MY_PERCENT, 50}},
        {OPERATION_MINUS, {PERCENT,50}, {MY_PERCENT, 50}}
    );
    crosshair->setAttribute(&UIFrame::Style::textColor, {255,255,255});

    getUILayer("default").addElement(crosshair);

    std::array<std::string,6> skyboxPaths = {
        "resources/skybox/stars/right.png",
        "resources/skybox/stars/left.png",
        "resources/skybox/stars/top.png",
        "resources/skybox/stars/bottom.png",
        "resources/skybox/stars/front.png",
        "resources/skybox/stars/back.png"
    };  

    itemPrototypeRegistry.addPrototype(ItemPrototype("diamond","resources/textures/diamond32.png"));
    itemPrototypeRegistry.addPrototype(ItemPrototype("crazed","resources/textures/crazed32.png"));

    held_item_slot = std::make_shared<ItemSlot>(itemTextureAtlas,*uiManager);

    inventory = std::make_shared<ItemInventory>(itemTextureAtlas,*uiManager, 10,5, held_item_slot);
    inventory->setPosition(
        {OPERATION_MINUS, {PERCENT,50}, {MY_PERCENT,50}},
        {OPERATION_MINUS, {PERCENT,50}, {MY_PERCENT,50}}
    );

    auto item = itemPrototypeRegistry.createItem("diamond");
    auto item2 = itemPrototypeRegistry.createItem("crazed");
    inventory->setItem(1,0,item);
    inventory->setItem(0,4,item2);

    hotbar = std::make_shared<UIHotbar>(itemTextureAtlas, *uiManager, held_item_slot);
    hotbar->setPosition(
        {OPERATION_MINUS, {PERCENT,50}, {MY_PERCENT,50}},
        {OPERATION_MINUS,{OPERATION_MINUS, {PERCENT,100}, {MY_PERCENT,100}},20}
    );

    auto structure_capture_frame = uiManager->createElement<UIFrame>();
    structure_capture_frame->setPosition(
        (TValue{PERCENT,100} - TValue{MY_PERCENT,100}) - 10,
        10
    );
    structure_capture_frame->setSize(200,350);
    structure_capture_frame->setAttribute(&UIFrame::Style::backgroundColor, {20,20,20});
    
    auto layout = std::make_shared<UIFlexLayout>();
    layout->setDirection(UIFlexLayout::VERTICAL);
    layout->setExpand(false);

    structure_capture_frame->setLayout(layout);

    structure_capture_start_label = uiManager->createElement<UILabel>();
    structure_capture_start_label->setAttribute(&UILabel::Style::fontSize, {12});
    structure_capture_start_label->setSize(180,30);

    structure_capture_end_label = uiManager->createElement<UILabel>();
    structure_capture_end_label->setAttribute(&UIFrame::Style::fontSize, {12});
    structure_capture_end_label->setSize(180,30);

    structure_capture_frame->appendChild(structure_capture_start_label);
    structure_capture_frame->appendChild(structure_capture_end_label);

    setUILayer("structure_capture");
    addElement(structure_capture_frame);
    setUILayer("menu");
    addElement(inventory);
    addElement(hotbar);
    addElement(held_item_slot);
    setUILayer("default");
    addElement(hotbar);

    skyboxProgram.use();

    skybox.load(skyboxPaths);
    
    terrainProgram.use();
    
    global_block_registry.setTextureSize(160,160);
    global_block_registry.loadFromFolder("resources/textures");
    block_texture_array = global_block_registry.load();

    BlockLoader::loadFromFile("resources/blocks.config");

    terrainProgram.setSamplerSlot("textureArray", 0);
    terrainProgram.setSamplerSlot("shadowMap", 1);

    modelProgram.setSamplerSlot("textureIn",0);

    gBufferProgram.setSamplerSlot("gPosition", 0);
    gBufferProgram.setSamplerSlot("gNormal", 1);
    gBufferProgram.setSamplerSlot("gAlbedoSpec", 2);
    gBufferProgram.setSamplerSlot("AmbientOcclusion", 3);

    blur_program.setSamplerSlot("input", 0);

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
}

void MainScene::resize(GLFWwindow* window, int width, int height){
    camera.resizeScreen(width, height, camFOV);
    gBuffer = GBuffer(width,height);
    
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
            held_item_slot->setPosition(
                mouseX,
                mouseY
            );
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
    
    if(isActiveLayer("structure_capture")){
        if(!structureCaptured) structureCaptureEnd = blockUnderCursorPosition;
        updateStructureCaptureDisplay();
    }
    //wireframeRenderer.setCube(1,glm::vec3(hit.lastPosition) - 0.005f, {1.01,01.01,1.01},{1.0,0,0});
}

void MainScene::mouseEvent(GLFWwindow* window, int button, int action, int mods){
    if(!blockUnderCursor || blockUnderCursor->id == BLOCK_AIR_INDEX) return;

    auto& player = world->getPlayer();

    if (
        button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS 
    ){  
        if(isActiveLayer("structure_capture")){
            structureCaptured = false;
            structureCaptureStart = blockUnderCursorPosition;
            updateStructureCaptureDisplay();
            return;
        }
        
        auto* block_prototype = global_block_registry.getBlockPrototypeByIndex(blockUnderCursor->id);
        auto* item_prototype = itemPrototypeRegistry.createPrototypeForBlock(block_prototype);

        auto entity = DroppedItem(itemPrototypeRegistry.createItem(item_prototype), glm::vec3(blockUnderCursorPosition) + glm::vec3(0.5,0.5,0.5));
        entity.accelerate({
            static_cast<float>(std::rand() % 200) / 100.0f - 1.0f,
            0.6f,
            static_cast<float>(std::rand() % 200) / 100.0f - 1.0f
        }, 1.0f);
        world->addEntity(entity);
        //auto& selected_slot = hotbar->getSelectedSlot();
        //inventory->addItem();

        world->setBlock(blockUnderCursorPosition, {BLOCK_AIR_INDEX});

        auto chunk = world->getChunkFromBlockPosition(blockUnderCursorPosition);
        if(!chunk) return;
        regenerateChunkMesh(chunk,world->getGetChunkRelativeBlockPosition(blockUnderCursorPosition));
    }
    else if(
        button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS
    ){
        if(isActiveLayer("structure_capture")){
            structureCaptureEnd = blockUnderCursorPosition;
            structureCaptured = true;
            updateStructureCaptureDisplay();
            return;
        }

        glm::ivec3 blockPosition = glm::floor(blockUnderCursorEmpty);

        auto& selected_slot = hotbar->getSelectedSlot();
        if(!selected_slot.hasItem()) return;

        auto* prototype = selected_slot.getItem()->getPrototype();
        if(!prototype || !prototype->isBlock()) return;

        world->setBlock(blockPosition, {prototype->getBlockID()});
        if(
            player.checkForCollision(world.get(), false) ||
            player.checkForCollision(world.get(), true)
        ){
            world->setBlock(blockPosition, {BLOCK_AIR_INDEX});
            return;
        }

        selected_slot.decreaseQuantity(1);
        hotbar->update();

        auto* chunk = world->getChunkFromBlockPosition(blockPosition);
        if(!chunk) return;
        regenerateChunkMesh(chunk,world->getGetChunkRelativeBlockPosition(blockPosition));
    }

    updateCursor();
}

template <typename T>
static inline void swapToOrder(T& a, T& b){
    if(a > b){
        T temp = a;
        a = b;
        b = temp;
    }
}

void MainScene::updateStructureCaptureDisplay(){
    //swapToOrder(structureCaptureStart.x, structureCaptureEnd.x);
    //swapToOrder(structureCaptureStart.y, structureCaptureEnd.y);
    //swapToOrder(structureCaptureStart.z, structureCaptureEnd.z);

    structure_capture_start_label->setText(
        "Capture start: " + std::to_string(structureCaptureStart.x) + " " + std::to_string(structureCaptureStart.y) + " " + std::to_string(structureCaptureStart.z)
    );
    structure_capture_end_label->setText(
        "Capture end: " + std::to_string(structureCaptureEnd.x) + " " + std::to_string(structureCaptureEnd.y) + " " + std::to_string(structureCaptureEnd.z)
    );
    structure_capture_start_label->update();
    structure_capture_end_label->update();

    glm::ivec3 size = structureCaptureEnd - structureCaptureStart;

    wireframeRenderer.setCube(1,glm::vec3(structureCaptureStart) - 0.005f, glm::vec3{0.01,0.01,0.01} + glm::vec3(size),{0,0.1,0.1});
}

void MainScene::scrollEvent(GLFWwindow* window, double xoffset, double yoffset){
    if(inputManager.isActive(SCROLL_ZOOM)){
        camFOV -= (float) yoffset * 5.0f;

        if(camFOV < minFOV) camFOV = minFOV;
        else if(camFOV > maxFOV) camFOV = maxFOV;

        camera.adjustFOV(camFOV);
    }
    else{
        int scroll = abs(yoffset) / yoffset;

        hotbar->selectSlot(hotbar->getSelectedSlotNumber() - scroll);
        hotbar->update();
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

    if(key == GLFW_KEY_Q && action == GLFW_PRESS){
        auto& slot = hotbar->getSelectedSlot();
        if(!slot.hasItem()) return;
        
        auto* item_prototype = slot.getItem()->getPrototype();
        if(!item_prototype) return;
        
        auto entity = DroppedItem(itemPrototypeRegistry.createItem(item_prototype), camera.getPosition() + camera.getDirection() * 0.5f);
        entity.accelerate(camera.getDirection(),1.0f);
        world->addEntity(entity);

        slot.decreaseQuantity(1);
        hotbar->update();
    }
}

void MainScene::unlockedKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        if(this->getCurrentUILayer().name != "default") 
            this->setUILayer("default");
    }
    else if(key == GLFW_KEY_N && action == GLFW_PRESS){
        if(this->getCurrentUILayer().name != "structure_capture")
            this->setUILayer("structure_capture");
    }
}

void MainScene::open(GLFWwindow* window){
    running = true;
    allGenerated = false;
    
    world = std::make_unique<World>(worldPath);

    auto& player = world->getPlayer();

    player.onCollision = [this](Entity* self, Entity* collided_with){
        if(collided_with->getTypename() != "dropped_item") return;

        const auto* data = reinterpret_cast<const DroppedItem::Data*>(collided_with->getData());
        
        if(!this->hotbar->addItem(data->item))
            this->inventory->addItem(data->item);

        this->update_hotbar = true;
        //else this->hotbar->update();
    };

    chunkMeshGenerator.setWorld(world.get());

    int pregenDistance = renderDistance + 1; 

    //world->getWorldGenerator().generateChunkRegion(*world, {0,0,0});
    for(int x = -pregenDistance; x <= pregenDistance; x++) 
    for(int y = -pregenDistance; y <= pregenDistance; y++) 
    for(int z = -pregenDistance; z <= pregenDistance; z++){
        glm::ivec3 chunkPosition = glm::ivec3(x,y,z);

        if(world->isChunkLoadable(chunkPosition)) world->loadChunk(chunkPosition);
        else world->generateChunk(chunkPosition);
    }

    //std::thread generationThread(std::bind(&MainScene::generateSurroundingChunks, this));
    //generationThread.detach();

    player.setPosition({0,255,0});
    while(
        !player.checkForCollision(world.get(), false, {0,-1,0}) && 
        player.getPosition().y > 0
    ) player.setPosition(player.getPosition() + glm::vec3{0,-1,0});


    for(auto& prototype: global_block_registry.prototypes()){
        if(prototype.id == 0) continue; // Dont make air

        auto* item_prototype = itemPrototypeRegistry.createPrototypeForBlock(&prototype);
        Item it = itemPrototypeRegistry.createItem(item_prototype);
        it.setQuantity(256);
        hotbar->addItem(it);
    }

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

    world->save();

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

    fps_label->setText(std::to_string(1.0f / deltatime) + "FPS");
    fps_label->update();
    
    glEnable(GL_DEPTH_TEST);
    glEnable( GL_CULL_FACE );

    glm::vec3 camPosition = world->getPlayer().getPosition() + camOffset;

    camera.setPosition(camPosition);
    chunkMeshGenerator.loadMeshFromQueue(chunkMeshRegistry);

    if(update_hotbar){
        hotbar->update();
        update_hotbar = false;
    } 

    if(!chunk_generation_queue.empty()){
        auto& position = chunk_generation_queue.front();
        world->generateChunk(position);
        chunk_generation_queue.pop();
    }

    if(chunk_generation_queue.empty() && indexer.getCurrentDistance() < renderDistance){
        auto position = indexer.next() + lastCamWorldPosition;
        Chunk* chunk = world->getChunk(position);

        if(!chunk) std::cerr << "Chunk not generated when generating meshes?" << std::endl;
        else if(!chunkMeshRegistry.isChunkLoaded(position)) chunkMeshGenerator.syncGenerateSyncUploadMesh(chunk, chunkMeshRegistry); 
    }

    processMouseMovement();

    if(updateVisibility > 0){
        chunkMeshRegistry.updateDrawCalls(camera.getPosition(), camera.getFrustum());
        updateVisibility = 0;
    }
    
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

    /*
        Render to shadow map
    */
    glDisable( GL_CULL_FACE );
    suncam.setModelPosition({0,0,0});
    terrainProgram.updateUniforms();
    suncam.prepareForRender();
    chunkMeshRegistry.draw();
    glEnable( GL_CULL_FACE );
    // ====

    gBuffer.bind();
        glViewport(0,0,camera.getScreenWidth(),camera.getScreenHeight());

        block_texture_array->bind(0);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw skybox
        glDisable(GL_CULL_FACE);
        skyboxProgram.use();
        skybox.draw();
        
        // ====

        // Draw terrain
        terrainProgram.updateUniforms();
        chunkMeshRegistry.draw();
    
        // Draw models
        modelProgram.updateUniforms();
        itemPrototypeRegistry.drawItemModels();

        glDisable( GL_CULL_FACE );

        //int start_index = 20;
        //wireframeRenderer.setCubes(start_index);
        //world->drawEntityColliders(wireframeRenderer, start_index);

        wireframeRenderer.draw();
    gBuffer.unbind();

    /*auto& gTextures = gBuffer.getTextures();
    ssao.render(gTextures[0],gTextures[1], fullscreen_quad);
    
    blured_ssao_framebuffer.bind(); 
        ssao.getResultTexture().bind(0);
        blur_program.updateUniforms();

        fullscreen_quad.render();
    blured_ssao_framebuffer.unbind();*/
    
    gBufferProgram.updateUniforms();
    
    glViewport(0, 0, gBuffer.getWidth(), gBuffer.getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gBuffer.bindTextures();
    //blured_ssao_framebuffer.getTextures()[0].bind(3);

    fullscreen_quad.render();

    gBuffer.unbindTextures();
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

void MainScene::enqueueChunkGeneration(glm::ivec3 position){
    if(world->getChunk(position)) return;
    chunk_generation_queue.push(position); 
}

void MainScene::updateLoadedLocations(glm::ivec3 old_location, glm::ivec3 new_location){
    int pregenDistance = renderDistance + 1; 

    for(int x = -pregenDistance; x <= pregenDistance; x++) 
    for(int y = -pregenDistance; y <= pregenDistance; y++) 
    for(int z = -pregenDistance; z <= pregenDistance; z++){
        glm::ivec3 chunkPosition = new_location + glm::ivec3(x,y,z);

        enqueueChunkGeneration(chunkPosition);
    }

    indexer = {};
}

void MainScene::physicsUpdate(){
    double last = glfwGetTime();
    double current = glfwGetTime();
    float deltatime;

    float targetTPS = 120;
    float tickTime = 1.0f / targetTPS;

    while(running){
        current = glfwGetTime();
        deltatime = (float)(current - last);

        threadPool->deployPendingJobs();

        if(deltatime < tickTime) continue;
        last = current;

        glm::ivec3 camWorldPosition = glm::floor(camera.getPosition() / static_cast<float>(CHUNK_SIZE));
        if(glm::distance(glm::vec3(camWorldPosition),glm::vec3(lastCamWorldPosition)) >= 2){ // Crossed chunks
            //std::cout << "New chunk position: " << camWorldPosition.x << " " << camWorldPosition.y << " " << camWorldPosition.z << std::endl;
            updateLoadedLocations(lastCamWorldPosition, camWorldPosition);
            lastCamWorldPosition = camWorldPosition;
        }


        glm::vec3 camDir = glm::normalize(camera.getDirection());
        glm::vec3 horizontalDir = glm::normalize(glm::vec3(camDir.x, 0, camDir.z));
        glm::vec3 leftDir = glm::normalize(glm::cross(camera.getUp(), horizontalDir));

        auto& player = world->getPlayer();
        
        if(player.checkForCollision(world.get(), false)){
            player.setPosition({0,255,0});
            while(
                !player.checkForCollision(world.get(), false, {0,-1,0}) && 
                player.getPosition().y > 0
            ) player.setPosition(player.getPosition() + glm::vec3{0,-1,0});
        }

        bool moving = false;
        if(inputManager.isActive(STRAFE_RIGHT )){
            player.accelerate(-leftDir * camAcceleration, deltatime);
            moving = true;
        }
        if(inputManager.isActive(STRAFE_LEFT  )){
            player.accelerate( leftDir * camAcceleration, deltatime);
            moving = true;
        }
        if(inputManager.isActive(MOVE_BACKWARD)){
            player.accelerate(-horizontalDir * camAcceleration, deltatime);
            moving = true;
        }
        if(inputManager.isActive(MOVE_FORWARD )){
            player.accelerate( horizontalDir * camAcceleration, deltatime);
            moving = true;
        }

        if(!moving) player.decellerate(camAcceleration,deltatime);
        //if(boundKeys[1].isDown) player.accelerate(-camera.getUp() * 0.2f);
        //if(inputManager.isActive(MOVE_UP)) player.accelerate(camera.getUp() * 0.2f);
        if(inputManager.isActive(MOVE_DOWN)) player.accelerate(-camera.getUp() * 2.0f, deltatime);
        if(
            inputManager.isActive(MOVE_UP)
            && player.checkForCollision(world.get(), false, {0,-0.1f,0})
            && player.getVelocity().y == 0
        ) player.accelerate(camera.getUp() * 10.0f, 1.0);

        if(!world->getChunk(camWorldPosition)) continue;

        world->updateEntities(deltatime);

        auto& in_hand_slot = hotbar->getSelectedSlot();
        if(in_hand_slot.hasItem()){
            auto* prototype = in_hand_slot.getItem()->getPrototype();
            if(prototype){
                glm::vec3 item_offset = camera.getDirection() * 0.5f - camera.getLeft() + camera.getRelativeUp() * 0.4f;
                prototype->getModel()->requestDraw(camera.getPosition() + item_offset, {1,1,1}, {0,-camera.getYaw(),camera.getPitch()}, {-0.5,-0.5,0}, {Model::Y,Model::Z,Model::X});
            }
        }
        
        itemPrototypeRegistry.swapModelBuffers();
    }

    threadsStopped++;
}

void UICrosshair::getRenderingInformation(UIRenderBatch& batch){
    auto color = getAttribute(&UIFrame::Style::textColor);

    // Left
    batch.Rectangle(
        transform.x,
        transform.y + transform.height / 2 - thickness / 2,
        transform.width / 2 - part_margin,
        thickness,
        color
    );
    // Right
    batch.Rectangle(
        transform.x + transform.width / 2 + part_margin,
        transform.y + transform.height / 2 - thickness / 2,
        transform.width / 2 - part_margin,
        thickness,
        color
    );

    // Top
    batch.Rectangle(
        transform.x + transform.width / 2 - thickness / 2,
        transform.y,
        thickness,
        transform.height / 2 - part_margin,
        color
    );
    
    //Bottom
    batch.Rectangle(
        transform.x + transform.width  / 2 - thickness / 2,
        transform.y + transform.height / 2 + part_margin,
        thickness,
        transform.height / 2 - part_margin,
        color
    );
}

void UIHotbar::getRenderingInformation(UIRenderBatch& batch){
    ItemInventory::getRenderingInformation(batch);

    batch.BorderedRectangle(
        transform.x + selected_slot * slot_size,
        transform.y,
        slot_size,
        slot_size,
        UIColor{0,0,0,0},
        UIBorderSizes{3,3,3,3},
        UIColor{180,180,180}
    );
}