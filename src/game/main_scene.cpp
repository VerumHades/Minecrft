#include <game/main_scene.hpp>


template <typename T>
static inline void swapToOrder(T& a, T& b){
    if(a > b){
        T temp = a;
        a = b;
        b = temp;
    }
}

static inline std::tuple<glm::ivec3, glm::ivec3> pointsToRegion(glm::ivec3 min, glm::ivec3 max){
    swapToOrder(min.x,max.x);
    swapToOrder(min.y,max.y);
    swapToOrder(min.z,max.z);
    
    return {min,max};
}

void MainScene::initialize(){
    fpsLock = false;
 
    this->setUILayer("settings");
    addElement(getElementById<UIScrollableFrame>("settings_scrollable"));

    UICore::get().loadWindowFromXML(*getWindow(), "resources/templates/game.xml");
    
    this->getUILayer("default").cursorMode = GLFW_CURSOR_DISABLED;
    //this->getUILayer("chat").eventLocks = {true, true, true, true};
    //this->getUILayer("menu").eventLocks = {true, true, true, true};
    //this->getUILayer("settings").eventLocks = {true, true, true, true};
    this->getUILayer("structure_capture").cursorMode = GLFW_CURSOR_DISABLED;

    this->setUILayer("default");

    fps_label = std::make_shared<UILabel>();
    fps_label->setPosition(10_px,10_px);
    addElement(fps_label);

    auto crosshair = std::make_shared<UICrosshair>();
    crosshair->setSize(60_px,60_px);
    crosshair->setPosition(TValue::Center(), TValue::Center());
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

    ItemRegistry::get().addPrototype(ItemPrototype("diamond","resources/textures/diamond.png"));
    ItemRegistry::get().addPrototype(ItemPrototype("crazed","resources/textures/crazed.png"));

    held_item_slot = std::make_shared<ItemSlot>(itemTextureAtlas);

    inventory = std::make_shared<InventoryDisplay>(itemTextureAtlas, held_item_slot);
    inventory->setPosition(
        {OPERATION_MINUS, {PERCENT,50}, {MY_PERCENT,50}},
        {OPERATION_MINUS, {PERCENT,50}, {MY_PERCENT,50}}
    );

    hotbar = std::make_shared<UIHotbar>(itemTextureAtlas, held_item_slot);
    hotbar->setPosition(
        TValue::Center(),
        TValue::Bottom(20_px)
    );

    structure_capture_start_label = getElementById<UILabel>("start_label");
    structure_capture_end_label   = getElementById<UILabel>("end_label");
    
    getElementById<UIFrame>("submit")->onClicked = [this](){
        auto [min,max] = pointsToRegion(structureCaptureStart, structureCaptureEnd);
        glm::ivec3 size = (max - min) + glm::ivec3(1,1,1);

        Structure structure = Structure::capture(min, size, game_state->getTerrain());

        const std::string& name = getElementById<UIInput>("structure_name")->getText();

        auto path = Paths::Get(Paths::GAME_STRUCTURES);
        if(path) structure.serialize().saveToFile(path.value() / fs::path(name + ".structure"));

        setUILayer("structure_capture"); 

        if(!this->structure_selection->hasOption(name)) this->structure_selection->addOption(name);
    };

    structure_selection = std::make_shared<UISelection>();
    structure_selection->setPosition(10_px,10_px);
    structure_selection->setSize(200_px,300_px);
    structure_selection->setAttribute(&UIFrame::Style::backgroundColor, {10,10,10});
    structure_selection->setAttribute(&UIFrame::Style::textColor, {220,220,220});
    structure_selection->setAttribute(&UIFrame::Style::fontSize, 16_px);

    structure_selection->onSelected = [this](const std::string& name){
        auto structures_path = Paths::Get(Paths::GAME_STRUCTURES);

        auto structure_name_label = getElementById<UILabel>("selected_structure");
        
        if(!structures_path){
            structure_name_label->setText("Error: Structure file missing.");
            structure_name_label->update();
            return;
        }

        auto bt = ByteArray::FromFile(structures_path.value() / name);

        structure_name_label->setText("Selected structure:" + name);
        structure_name_label->update();

        selected_structure = std::make_shared<Structure>(Structure::deserialize(bt));
        this->updateStructureDisplay();
    };

    auto structures_path = Paths::Get(Paths::GAME_STRUCTURES);
    if(structures_path){
        for (const auto& entry: fs::directory_iterator(structures_path.value())){
            auto& path = entry.path();
            if(!entry.is_regular_file()) continue;

            structure_selection->addOption(path.stem().string());
        }
    }

    setUILayer("structure_capture");
    addElement(structure_selection);
    setUILayer("inventory");
    addElement(inventory);
    addElement(hotbar);
    addElement(held_item_slot);
    setUILayer("default");
    addElement(hotbar);
    setUILayer("generation");

    generation_progress = std::make_shared<UILoading>();
    generation_progress->setPosition(TValue::Center(), TValue::Center());
    generation_progress->setSize(TValue::Pixels(600), TValue::Pixels(200));
    addElement(generation_progress);

    setUILayer("default");

    skyboxProgram.use();
    skybox.load(skyboxPaths);
    
    terrainProgram.use();
    
    block_texture_array = BlockRegistry::get().load();

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

    camera.setModelPosition({0,0,0});
    suncam.setCaptureSize(renderDistance * 2 * CHUNK_SIZE);

    //threadPool = std::make_unique<ThreadPool>(12);
    //world->load("saves/worldsave.bin");

    inputManager.bindKey(GLFW_KEY_W    , MOVE_FORWARD , "Move forward ");
    inputManager.bindKey(GLFW_KEY_S    , MOVE_BACKWARD, "Move backward");
    inputManager.bindKey(GLFW_KEY_A    , STRAFE_LEFT  , "Strafe left");
    inputManager.bindKey(GLFW_KEY_D    , STRAFE_RIGHT , "Strafe right");
    inputManager.bindKey(GLFW_KEY_SPACE, MOVE_UP      , "Jump");
    inputManager.bindKey(GLFW_KEY_LEFT_CONTROL, MOVE_DOWN, "Move down when flying");
    inputManager.bindKey(GLFW_KEY_LEFT_SHIFT, SCROLL_ZOOM, "Hold to zoom");

    auto settingsFrame = getElementById<UIFrame>("keybind_container");

    for(auto& [key,action]: inputManager.getBoundKeys()){
        std::string kename = getKeyName(key,0);

        auto frame = std::make_shared<UIFrame>();
        frame->setIdentifiers({"controlls_member"});

        auto name = std::make_shared<UILabel>();
        name->setText(action.name);
        name->setHoverable(false);
        name->setIdentifiers({"controlls_member_name"});
        
        auto keyname = std::make_shared<UILabel>();
        keyname->setText(kename);
        keyname->setFocusable(true);
        keyname->setIdentifiers({"controlls_member_keyname"});

        keyname->onKeyEvent = [this, key, action, keyname](int new_key, int /*scancode*/, int /*action*/, int /*mods*/){
            inputManager.unbindKey(key);
            inputManager.bindKey(new_key, action.action, action.name);

            std::string new_name = getKeyName(new_key,0);
            keyname->setText(new_name);
        };

        frame->appendChild(name);
        frame->appendChild(keyname);
        settingsFrame->appendChild(frame);
    }

    getElementById<UIFrame>("settings_scrollable")->calculateTransforms();

    auto mouse_settings = getElementById<UIFrame>("mouse_sensitivity_container");

    auto sensitivity_slider = std::make_shared<UISlider>();
    sensitivity_slider->setValuePointer(&sensitivity);
    sensitivity_slider->setSize({PERCENT,60},{PERCENT,100});
    sensitivity_slider->setMin(1);
    sensitivity_slider->setMax(100);
    sensitivity_slider->setDisplayValue(true);
    sensitivity_slider->setIdentifiers({"mouse_sensitivity_slider"});
    
    mouse_settings->appendChild(sensitivity_slider);

    for(auto& prototype: BlockRegistry::get().prototypes()){
        if(prototype.id == 0) continue; // Dont make air
        
        if(prototype.name == "crafting"){
            auto interface = std::make_unique<CraftingInterface>(prototype.name + "_interface", itemTextureAtlas, held_item_slot);
            getWindow()->addExternalLayer(interface->getLayer());

            BlockRegistry::get().setPrototypeInterface(prototype.id, std::move(interface));
        }

        ItemRegistry::get().createPrototypeForBlock(&prototype);
    }

    CraftingRecipeRegistry::get().addRecipe(CraftingRecipe({CraftingRecipe::RecipeItemRequirement{"block_iron", 5},{},{},{},{},{},{},{},{}}, "crazed",64));
    CraftingRecipeRegistry::get().addRecipe(CraftingRecipe(
        {CraftingRecipe::RecipeItemRequirement{"crazed", 1},{"crazed", 1},{"crazed", 1},{"crazed", 1},{"crazed", 1},{"crazed", 1},{"crazed", 1},{"crazed", 1},{"crazed", 1}}
        , "block_crazed",1));
}

void MainScene::resize(GLFWwindow* window, int width, int height){
    camera.resizeScreen(width, height, camFOV);
    gBuffer = GBuffer(width,height);
    
    terrainProgram.updateUniforms();
    skyboxProgram.updateUniforms();
}

void MainScene::mouseMove(GLFWwindow* window, int mouseX, int mouseY){
    //if(isActiveLayer("inventory")){
    if(held_item_slot){
        held_item_slot->setPosition(
            TValue::Pixels(mouseX),
            TValue::Pixels(mouseY)
        );
        held_item_slot->calculateTransforms();
        held_item_slot->update();
    }
    //}
    if(isActiveLayer("default") || isActiveLayer("structure_capture")){
        this->mouseX = mouseX;
        this->mouseY = mouseY;
        mouseMoved = true;
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

    RaycastResult hit = game_state->getTerrain().raycast(camPosition,camDirection,10);
 
    blockUnderCursor = game_state->getTerrain().getBlock(hit.position);
    blockUnderCursorPosition = hit.position;
    blockUnderCursorEmpty = hit.lastPosition;

    if(!blockUnderCursor || blockUnderCursor->id == BLOCK_AIR_INDEX) wireframeRenderer.removeCube(0);
    else wireframeRenderer.setCube(0,glm::vec3(hit.position) - 0.005f, {1.01,01.01,1.01},{0,0,0});
    
    if(isActiveLayer("structure_capture")){
        if(!structureCaptured) structureCaptureEnd = blockUnderCursorPosition;
        updateStructureDisplay();
    }
    //wireframeRenderer.setCube(1,glm::vec3(hit.lastPosition) - 0.005f, {1.01,01.01,1.01},{1.0,0,0});
}

void MainScene::mouseEvent(GLFWwindow* window, int button, int action, int mods){
    if(!blockUnderCursor || blockUnderCursor->id == BLOCK_AIR_INDEX) return;

    auto& player = game_state->getPlayer();

    if(isActiveLayer("default")){
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS ){  
            auto* block_prototype = BlockRegistry::get().getPrototype(blockUnderCursor->id);
            if(!block_prototype) return;
            auto* item_prototype = ItemRegistry::get().getPrototype("block_" + block_prototype->name);
            if(!item_prototype) return;
            
            Entity entity = DroppedItem::create(glm::vec3(blockUnderCursorPosition) + glm::vec3(0.5,0.5,0.5), ItemRegistry::get().createItem(item_prototype));
            entity.accelerate({
                static_cast<float>(std::rand() % 200) / 100.0f - 1.0f,
                0.6f,
                static_cast<float>(std::rand() % 200) / 100.0f - 1.0f
            }, 1.0f);
            game_state->addEntity(entity);
            //auto& selected_slot = hotbar->getSelectedSlot();
            //inventory->addItem();

            game_state->getTerrain().setBlock(blockUnderCursorPosition, {BLOCK_AIR_INDEX});

            auto chunk = game_state->getTerrain().getChunkFromBlockPosition(blockUnderCursorPosition);
            if(!chunk) return;
            regenerateChunkMesh(chunk, game_state->getTerrain().getGetChunkRelativeBlockPosition(blockUnderCursorPosition));
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS){
            auto* block_prototype = BlockRegistry::get().getPrototype(blockUnderCursor->id);
            if(block_prototype && block_prototype->interface){
                block_prototype->interface->open(blockUnderCursor->metadata, game_state.get());
                setUILayer(block_prototype->interface->getName());
                return;
            }



            glm::ivec3 blockPosition = glm::floor(blockUnderCursorEmpty);

            auto* selected_slot = hotbar->getSelectedSlot();
            if(!selected_slot || !selected_slot->hasItem()) return;

            auto* prototype = selected_slot->getItem()->getPrototype();
            if(!prototype || !prototype->isBlock()) return;

            game_state->getTerrain().setBlock(blockPosition, {prototype->getBlockID()});
            if(
                game_state->entityCollision(player, player.getVelocity()) ||
                game_state->entityCollision(player)
            ){
                game_state->getTerrain().setBlock(blockPosition, {BLOCK_AIR_INDEX});
                return;
            }

            selected_slot->decreaseQuantity(1);
            hotbar->update();

            auto* chunk = game_state->getTerrain().getChunkFromBlockPosition(blockPosition);
            if(!chunk) return;
            regenerateChunkMesh(chunk, game_state->getTerrain().getGetChunkRelativeBlockPosition(blockPosition));
        }
    }
    else if(isActiveLayer("structure_capture")){
        if(structure_menu_mode == CAPTURE){
            if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
                structureCaptured = false;
                structureCaptureStart = blockUnderCursorPosition;
                updateStructureDisplay();
            }
            else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS){
                structureCaptureEnd = blockUnderCursorPosition;
                structureCaptured = true;
                updateStructureDisplay();
            }
        }
        else{
            if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
                if(selected_structure){
                    auto positions = selected_structure->place(blockUnderCursorPosition, game_state->getTerrain());
                    for(auto& position: positions)
                        regenerateChunkMesh(game_state->getTerrain().getChunk(position), {1,1,1});
                }
            }
        }
    }

    updateCursor();
}

template <typename T>
static inline std::string vecToString(T vec, const std::string& separator = " "){
    return std::to_string(vec.x) + separator + std::to_string(vec.y) + separator + std::to_string(vec.z);
}

void MainScene::updateStructureDisplay(){
    auto mode_label = getElementById<UILabel>("mode");
    mode_label->setText(
        "Current mode: " + std::string(structure_menu_mode == CAPTURE ? "Capture" : "Place")
    );
    structure_capture_start_label->setText(
        "Capture start: " + vecToString(structureCaptureStart)
    );
    structure_capture_end_label->setText(
        "Capture end: " + vecToString(structureCaptureEnd)
    );
    structure_capture_start_label->update();
    structure_capture_end_label->update();
    mode_label->update();

    if(structure_menu_mode == CAPTURE){
        auto [min,max] = pointsToRegion(structureCaptureStart, structureCaptureEnd);
        glm::ivec3 size = (max - min) + glm::ivec3(1,1,1);

        wireframeRenderer.setCube(1,glm::vec3(min) - 0.005f, glm::vec3{0.01,0.01,0.01} + glm::vec3(size),{0,0.1,0.1});
    }
    else{
        if(!selected_structure) return;
        wireframeRenderer.setCube(
            1,
            glm::vec3(blockUnderCursorPosition) - 0.005f, 
            glm::vec3{0.01,0.01,0.01} + glm::vec3(selected_structure->getSize()),
            {0,0.3,0.2}
        );
    }
}

void MainScene::updateStructureSavingDisplay(){
    auto [min,max] = pointsToRegion(structureCaptureStart, structureCaptureEnd);
    glm::ivec3 size = max - min;

    auto save_size_label = getElementById<UILabel>("save_size");
    auto blocks_total_label = getElementById<UILabel>("blocks_total");

    save_size_label->setText(
        "Size: " + vecToString(size, "x")
    );

    blocks_total_label->setText(
        "Blocks total: " + std::to_string(size.x * size.y * size.z)
    );

    save_size_label->update();
    blocks_total_label->update();
}

void MainScene::scrollEvent(GLFWwindow* window, double xoffset, double yoffset){
    if(!isActiveLayer("default")) return;

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
    if(isActiveLayer("default")){
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
            auto* slot = hotbar->getSelectedSlot();
            if(!slot || !slot->hasItem()) return;
            
            auto* item_prototype = slot->getItem()->getPrototype();
            if(!item_prototype) return;
            
            auto entity = DroppedItem::create(camera.getPosition() + camera.getDirection() * 0.5f, ItemRegistry::get().createItem(item_prototype));
            entity.accelerate(camera.getDirection(),1.0f);
            game_state->addEntity(entity);

            slot->decreaseQuantity(1);
            hotbar->update();
        }
    }
    else if(isActiveLayer("structure_capture")){
        inputManager.keyEvent(window,key,scancode,action,mods);

        if(key == GLFW_KEY_UP && action == GLFW_PRESS){
            structure_selection->selectNext();
            structure_selection->update();
        }
        else if(key == GLFW_KEY_DOWN && action == GLFW_PRESS){
            structure_selection->selectPrevious();
            structure_selection->update();
        }

        if(key == GLFW_KEY_M && action == GLFW_PRESS){
            structure_menu_mode = structure_menu_mode == CAPTURE ? PLACE : CAPTURE;
            updateStructureDisplay();
        }
    }

    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS && !isActiveLayer("generation")){
        if(!isActiveLayer("default")) 
            this->setUILayer("default");
        else
            this->setUILayer("menu");
    }
    else if(key == GLFW_KEY_TAB && action == GLFW_PRESS){
        if(!isActiveLayer("inventory")) setUILayer("inventory");
        else setUILayer("default");
    }
    else if(key == GLFW_KEY_N && action == GLFW_PRESS){
        if(!isActiveLayer("structure_capture") && !isActiveLayer("structure_saving")){
            this->setUILayer("structure_capture");
            UICore::get().setFocus(structure_selection);
        }
        else if(!isActiveLayer("structure_saving")){
            this->setUILayer("structure_saving");
            UICore::get().setFocus(getElementById<UIFrame>("structure_name"));
            updateStructureSavingDisplay();
        }
    }
    else if(key == GLFW_KEY_B && action == GLFW_PRESS){
        indexer = {};
    }
}

void MainScene::open(GLFWwindow* window){
    game_state = nullptr;
    running = true;
    indexer = {};
    
    game_state = std::make_unique<GameState>(worldPath);

    for(auto& prototype: BlockRegistry::get().prototypes()){
        if(prototype.id == BLOCK_AIR_INDEX) continue; // Dont make air

        auto* ptype = ItemRegistry::get().createPrototypeForBlock(&prototype);
        ItemID id = ItemRegistry::get().createItem(ptype);
        ItemRegistry::get().getItem(id)->setQuantity(256);
        game_state->getPlayerHotbar().addItem(id);
    }

    inventory->setInventory(&game_state->getPlayerInventory());
    hotbar->setInventory(&game_state->getPlayerHotbar());

    auto& player = game_state->getPlayer();

    player.onCollision = [this](Entity* self, Entity* collided_with){
        if(!collided_with->getData() || collided_with->getData()->type != EntityData::DROPPED_ITEM) return;
        
        const auto& data = dynamic_pointer_cast<DroppedItem>(collided_with->getData());
        //const auto* data = reinterpret_cast<const DroppedItem::Data*>(collided_with->getData());
        this->game_state->giveItemToPlayer(data->getItem());

        this->update_hotbar = true;
    };

    terrain_manager.setGameState(game_state.get());

    if(player.getPosition().x == 0.0f && player.getPosition().z == 0.0f){
        int height = terrain_manager.getWorldGenerator().getHeightAt({0,0,0});
        player.setPosition({0,height + 5,0});
    }
    //Entity e = Entity(player.getPosition() + glm::vec3{5,0,5}, glm::vec3(1,1,1));
    //e.setModel(std::make_shared<GenericModel>("resources/models/130/scene.gltf"));
    //game_state->addEntity(e);g

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

    while(threadsStopped < 1){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } 

    terrain_manager.unloadAll();
    terrain_manager.setGameState(nullptr);

    game_state->unload();
    game_state = nullptr;
}

void MainScene::render(){
    //ScopeTimer timer("Rendered scene");
    current = glfwGetTime();
    deltatime = (float)(current - last);
    last = current;

    fps_label->setText(std::to_string(1.0f / deltatime) + "FPS");
    fps_label->update();

    if(terrain_manager.isGenerating()){
        if(!isActiveLayer("generation")) setUILayer("generation");
        //std::string value = "";
        //for(auto& segment: terrain_manager.getGenerationCountsLeft()){
        //    value += " " + std::to_string(segment);
        //}
        generation_progress->setValues(&terrain_manager.getGenerationCountsLeft());
        //generation_progress_label->setText(value);
        generation_progress->calculateTransforms();
        generation_progress->update();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return;
    }
    else if(isActiveLayer("generation")){
        generation_progress->setValues(nullptr);
        setUILayer("default");
    }
    //timer.timestamp("Updated fps label");
    if(terrain_manager.uploadPendingMeshes()) updateVisibility = 1;
    //timer.timestamp("Uploaded meshes.");

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glm::vec3 camPosition = game_state->getPlayer().getPosition() + camOffset;

    camera.setPosition(camPosition);

    if(update_hotbar){
        hotbar->update();
        update_hotbar = false;
    } 

    //timer.timestamp("Updated ui");
    processMouseMovement();

    if(updateVisibility > 0){
        terrain_manager.getMeshRegistry().updateDrawCalls(camera.getPosition(), camera.getFrustum());
        updateVisibility = 0;
    }

    /*const int chunk_bound_distance = 4;
    const int chunk_bound_diameter = (chunk_bound_distance + 1) * 2;

    int counter = 10;
    for(int x = -chunk_bound_distance; x <= chunk_bound_distance; x++) 
    for(int z = -chunk_bound_distance; z <= chunk_bound_distance; z++)
    for(int y = -chunk_bound_distance; y <= chunk_bound_distance; y++) {
        glm::ivec3 position = (glm::ivec3{x,y,z} + glm::ivec3{glm::floor(game_state->getPlayer().getPosition() / (float)CHUNK_SIZE)})  * CHUNK_SIZE;
        wireframeRenderer.setCube(counter++, position, glm::vec3{CHUNK_SIZE}, glm::vec3{0.1,0.5,0.5});
    }*/
    //timer.timestamp("Updated visibility");

    //timer.timestamp("Rendered to shadow map.");

    gBuffer.bind();
        glViewport(0,0,camera.getScreenWidth(),camera.getScreenHeight());

        block_texture_array->bind(0);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw skybox
        skyboxProgram.updateUniforms();
        skybox.draw();
        // ====

        // Draw terrain
        terrainProgram.updateUniforms();
        terrain_manager.getMeshRegistry().draw();

        //interpolation_time = (current - last_tick_time) / tickTime;
        // Draw models
        modelProgram.updateUniforms();
        Model::DrawAll();
        //ItemRegistry::get().drawItemModels();



        //int start_index = 20;
        //wireframeRenderer.setCubes(start_index);
        //world->drawEntityColliders(wireframeRenderer, start_index);

        wireframeRenderer.draw();
    gBuffer.unbind();

    //timer.timestamp("Rendered to gBuffer.");

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
    //timer.timestamp("Rendered to screen.");
}

void MainScene::updateLoadedLocations(glm::ivec3 old_location, glm::ivec3 new_location){
    terrain_manager.loadRegion(new_location, renderDistance);
}

void MainScene::physicsUpdate(){
    last_tick_time = glfwGetTime();
    double current = glfwGetTime();
    float deltatime;

    updateLoadedLocations({},glm::ivec3{game_state->getPlayer().getPosition() / (float)CHUNK_SIZE});

    while(running){
        current = glfwGetTime();
        deltatime = (float)(current - last_tick_time);

        //threadPool->deployPendingJobs();

        if(deltatime < tickTime) continue;
        last_tick_time = current;

        glm::ivec3 camWorldPosition = glm::floor(camera.getPosition() / static_cast<float>(CHUNK_SIZE));
        if(glm::distance(glm::vec3(camWorldPosition),glm::vec3(lastCamWorldPosition)) >= 2){ // Crossed chunks
            //std::cout << "New chunk position: " << camWorldPosition.x << " " << camWorldPosition.y << " " << camWorldPosition.z << std::endl;
            updateLoadedLocations(lastCamWorldPosition, camWorldPosition);
            lastCamWorldPosition = camWorldPosition;
        }


        glm::vec3 camDir = glm::normalize(camera.getDirection());
        glm::vec3 horizontalDir = glm::normalize(glm::vec3(camDir.x, 0, camDir.z));
        glm::vec3 leftDir = glm::normalize(glm::cross(camera.getUp(), horizontalDir));

        auto& player = game_state->getPlayer();

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
            && game_state->entityCollision(player, {0,-0.1f,0})
            && player.getVelocity().y == 0
        ) player.accelerate(camera.getUp() * 10.0f, 1.0);

        if(!game_state->getTerrain().getChunk(camWorldPosition)) continue;

        game_state->updateEntities(deltatime);

        if(isActiveLayer("default")){
            auto* in_hand_slot = hotbar->getSelectedSlot();
            if(in_hand_slot && in_hand_slot->hasItem()){
                auto* prototype = in_hand_slot->getItem()->getPrototype();
                if(prototype && prototype->getModel()){
                    glm::vec3 item_offset = camera.getDirection() * 0.5f - camera.getLeft() + camera.getRelativeUp() * 0.4f;
                    prototype->getModel()->requestDraw(camera.getPosition() + item_offset, {1,1,1}, {0,-camera.getYaw(),camera.getPitch()}, {-0.5,-0.5,0}, {Model::Y,Model::Z,Model::X});
                }
            }
        }
        
        Model::SwapAll();
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
    InventoryDisplay::getRenderingInformation(batch);

    batch.BorderedRectangle(
        transform.x + selected_slot * slot_size,
        transform.y,
        slot_size,
        slot_size,
        UIColor{0,0,0,0},
        UISideSizes{3,3,3,3},
        UIColor{180,180,180}
    );
}

void UILoading::getRenderingInformation(UIRenderBatch& batch){
    if(!values) return;
    
    const int margin = 10; 
    const int width = (transform.width - values->size() * margin) / values->size();

    int i = 0;
    for(auto& value: *values){
        if(value > max_values[i]) max_values[i] = value;

        float max = max_values[i] != 0 ? max_values[i] : 1;

        batch.Rectangle(
            transform.x + width * i + margin * (i + 1), 
            transform.y, 
            width, 
            ((float)value / max) * (float)transform.height, 
            UIColor{200,100,0}
        );
        i++;
    }
}