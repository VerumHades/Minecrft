#include <game/main_scene.hpp>

void MainScene::initialize() {
    fpsLock = false;

    gBuffer = std::make_shared<GBuffer>(1920, 1080);

    this->setUILayer("settings");
    addElement(getElementById<UIScrollableFrame>("settings_scrollable"));

    UICore::get().loadWindowFromXML(*getWindow(), "resources/templates/game.xml");

    // this->getUILayer("chat").eventLocks = {true, true, true, true};
    // this->getUILayer("menu").eventLocks = {true, true, true, true};
    // this->getUILayer("settings").eventLocks = {true, true, true, true};

    std::vector<std::string> freeCubeTextures = {"resources/textures/block_breaking/level0.png", "resources/textures/block_breaking/level1.png",
                                                 "resources/textures/block_breaking/level2.png", "resources/textures/block_breaking/level3.png",
                                                 "resources/textures/block_breaking/level4.png", "resources/textures/block_breaking/level5.png",
                                                 "resources/textures/block_cursor.png"};
    cubeRenderer.loadTextures(freeCubeTextures);

    std::array<std::string, 6> skyboxPaths = {"resources/skybox/stars/right.png",  "resources/skybox/stars/left.png",  "resources/skybox/stars/top.png",
                                              "resources/skybox/stars/bottom.png", "resources/skybox/stars/front.png", "resources/skybox/stars/back.png"};

    // ItemRegistry::get().addPrototype(ItemPrototype("diamond","resources/textures/diamond.png"));
    // ItemRegistry::get().addPrototype(ItemPrototype("crazed","resources/textures/crazed.png"));

    fogDensity = 0.003f;

    setUILayer("generation");
    generation_progress = std::make_shared<UILoading>();
    generation_progress->setPosition(TValue::Center(), TValue::Center());
    generation_progress->setSize(TValue::Pixels(600), TValue::Pixels(200));
    addElement(generation_progress);

    skyboxProgram.use();
    skybox.load(skyboxPaths);

    block_texture_array = BlockRegistry::get().load();

    modelProgram.setSamplerSlot("textureIn", 0);

    gBufferProgram.setSamplerSlot("gPosition", 0);
    gBufferProgram.setSamplerSlot("gNormal", 1);
    gBufferProgram.setSamplerSlot("gAlbedoSpec", 2);
    gBufferProgram.setSamplerSlot("AmbientOcclusion", 3);

    // blur_program.setSamplerSlot("input", 0);

    camera.setPosition(0.0f, 160.0f, 0.0f);

    sunDirUniform.setValue({
        cos(glm::radians(sunAngle)), // X position (cosine component)
        sin(glm::radians(sunAngle)), // Y position (sine component for vertical angle)
        0                            // Z position (cosine component)
    });

    skyboxProgram.updateUniforms();

    camera.setModelPosition({0, 0, 0});
    suncam.setCaptureSize(renderDistance * 2 * CHUNK_SIZE);

    // threadPool = std::make_unique<ThreadPool>(12);
    // world->load("saves/worldsave.bin");

    inputManager.bindKey(GLFW_KEY_W, MOVE_FORWARD, "Move forward ");
    inputManager.bindKey(GLFW_KEY_S, MOVE_BACKWARD, "Move backward");
    inputManager.bindKey(GLFW_KEY_A, STRAFE_LEFT, "Strafe left");
    inputManager.bindKey(GLFW_KEY_D, STRAFE_RIGHT, "Strafe right");
    inputManager.bindKey(GLFW_KEY_SPACE, MOVE_UP, "Jump");
    inputManager.bindKey(GLFW_KEY_LEFT_CONTROL, MOVE_DOWN, "Move down when flying");
    inputManager.bindKey(GLFW_KEY_LEFT_SHIFT, SPRINT, "Sprint");
    inputManager.bindKey(GLFW_KEY_C, SCROLL_ZOOM, "Hold to zoom");

    auto settingsFrame = getElementById<UIFrame>("keybind_container");

    for (auto& [key, action] : inputManager.getBoundKeys()) {
        std::string kename = getKeyName(key, 0);

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

        keyname->onKeyEvent = [this, key, action, keyname](int new_key, int /*scancode*/, int /*action*/, int /*mods*/) {
            inputManager.unbindKey(key);
            inputManager.bindKey(new_key, action.action, action.name);

            std::string new_name = getKeyName(new_key, 0);
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
    sensitivity_slider->setSize({PERCENT, 60}, {PERCENT, 100});
    sensitivity_slider->setMin(1);
    sensitivity_slider->setMax(100);
    sensitivity_slider->setDisplayValue(true);
    sensitivity_slider->setIdentifiers({"mouse_sensitivity_slider"});

    mouse_settings->appendChild(sensitivity_slider);

    auto graphics_settings = getElementById<UIFrame>("render_distance_container");

    auto render_distance_slider = std::make_shared<UISlider>();
    render_distance_slider->setValuePointer(&renderDistance);
    render_distance_slider->setSize({PERCENT, 60}, {PERCENT, 100});
    render_distance_slider->setMin(4);
    render_distance_slider->setMax(64);
    render_distance_slider->setDisplayValue(true);
    render_distance_slider->setIdentifiers({"render_distance_slider"});
    render_distance_slider->onMove = [this]() { update_render_distance = true; };

    graphics_settings->appendChild(render_distance_slider);

    AddGameMode<GameModeSurvival>();
    AddGameMode<GameModeStructureCapture>();
    selected_game_mode = 1;
}

void MainScene::resize(GLFWwindow* window, int width, int height) {
    camera.resizeScreen(width, height, camFOV);
    gBuffer = std::make_shared<GBuffer>(width, height);

    skyboxProgram.updateUniforms();
}

void MainScene::mouseMove(GLFWwindow* window, int mouseX, int mouseY) {
    // if(isActiveLayer("inventory")){
    HandleGamemodeEvent(&GameMode::MouseMoved, mouseX, mouseY);

    this->mouseX = mouseX;
    this->mouseY = mouseY;

    float xoffset = (float)mouseX - lastMouseX;
    float yoffset = lastMouseY - (float)mouseY; // Reversed since y-coordinates go from bottom to top
    lastMouseX    = (int)mouseX;
    lastMouseY    = (int)mouseY;

    if (getCurrentUILayer().cursorMode != GLFW_CURSOR_DISABLED)
        return;

    float real_sensitivity = static_cast<float>(sensitivity) / 100.0f;

    xoffset *= real_sensitivity;
    yoffset *= real_sensitivity * 2;

    float camYaw   = camera.getYaw() + xoffset;
    float camPitch = camera.getPitch() + yoffset;

    camYaw = std::fmod((camYaw + xoffset), (GLfloat)360.0f);

    // Constrain the pitch to avoid gimbal lock
    if (camPitch > 89.0f)
        camPitch = 89.0f;
    if (camPitch < -89.0f)
        camPitch = -89.0f;

    if (abs(lastCamPitch - camPitch) > chunkVisibilityUpdateThreshold) {
        updateVisibility = 1;
        lastCamPitch     = camPitch;
    }
    if (abs(lastCamYaw - camYaw) > chunkVisibilityUpdateThreshold) {
        updateVisibility = 1;
        lastCamYaw       = camYaw;
    }

    camera.setRotation(camPitch, camYaw);

    skyboxProgram.updateUniforms();
}

void MainScene::mouseEvent(GLFWwindow* window, int button, int action, int mods) {
    HandleGamemodeEvent(&GameMode::MouseEvent, button, action, mods);
    inputManager.mouseEvent(window, button, action, mods);
}

void MainScene::scrollEvent(GLFWwindow* window, double xoffset, double yoffset) {
    HandleGamemodeEvent(&GameMode::MouseScroll, xoffset, yoffset);

    if (getCurrentUILayer().cursorMode != GLFW_CURSOR_DISABLED)
        return;

    if (inputManager.isActive(SCROLL_ZOOM)) {
        camFOV -= (float)yoffset * 5.0f;

        if (camFOV < minFOV)
            camFOV = minFOV;
        else if (camFOV > maxFOV)
            camFOV = maxFOV;

        camera.adjustFOV(camFOV);
    }
}

void MainScene::keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods) {
    HandleGamemodeEvent(&GameMode::KeyEvent, key, scancode, action, mods);

    if (getCurrentUILayer().cursorMode == GLFW_CURSOR_DISABLED) {
        if (key == GLFW_KEY_M && action == GLFW_PRESS) {
            lineMode = !lineMode;
            if (!lineMode)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            else
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if (key == GLFW_KEY_K && action == GLFW_PRESS) {
            updateVisibility = 1;
        }

        inputManager.keyEvent(window, key, scancode, action, mods);
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (!isActiveLayer(GetCurrentBaseLayer().name))
            ResetToBaseLayer();
        else
            this->setUILayer("menu");
    } else if (key == GLFW_KEY_G && action == GLFW_PRESS) {
        SetGameMode((selected_game_mode + 1) % game_modes.size());
    }
}

void MainScene::open(GLFWwindow* window) {
    game_state = nullptr;
    running    = true;
    indexer    = {};

    game_state = std::make_unique<GameState>(worldPath);

    /*for(auto& prototype: BlockRegistry::get().prototypes()){
        if(prototype.id == BLOCK_AIR_INDEX) continue; // Dont make air

        auto* ptype = ItemRegistry::get().createPrototypeForBlock(&prototype);
        ItemRef item = Item::Create(ptype);
        item->setQuantity(256);
        game_state->getPlayerHotbar().addItem(item);
    }*/

    gamemodeState.game_state = game_state.get();
    HandleGamemodeEvent(&GameMode::Open);

    terrain_manager.setGameState(game_state.get());

    // Entity e = Entity(player.getPosition() + glm::vec3{5,0,5}, glm::vec3(1,1,1));
    // e.setModel(std::make_shared<GenericModel>("resources/models/130/scene.gltf"));
    // game_state->addEntity(e);g

    // std::thread physicsThread(std::bind(&MainScene::pregenUpdate, this));
    std::thread physicsThread(std::bind(&MainScene::physicsUpdate, this));

    physicsThread.detach();

    SetGameMode(0);
}

void MainScene::close(GLFWwindow* window) {
    threadsStopped = 0;
    running        = false;

    while (threadsStopped < 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    terrain_manager.unloadAll();
    terrain_manager.setGameState(nullptr);

    gamemodeState.game_state = nullptr;

    game_state->unload();
    game_state = nullptr;
}

void MainScene::render() {
    // ScopeTimer timer("Rendered scene");
    current   = glfwGetTime();
    deltatime = (float)(current - last);
    last      = current;

    HandleGamemodeEvent(&GameMode::Render, deltatime);

    if (terrain_manager.uploadPendingMeshes())
        updateVisibility = 1;

    interpolation_time = (current - last_tick_time) / tickTime;

    GL_CALL(glEnable(GL_DEPTH_TEST));
    GL_CALL(glDisable(GL_CULL_FACE));
    // GL_CALL(glEnable(GL_BLEND));

    camera.setPosition(glm::mix(lastCamPosition, camPosition, interpolation_time.getValue()));

    // cubeRenderer.setCube(0, game_state->getPlayer().getPosition() + camera.getDirection() * 2.0f, 0);

    if (updateVisibility > 0) {
        terrain_manager.getMeshRegistry().updateDrawCalls(camera.getPosition(), camera.getFrustum());
        updateVisibility = 0;
    }

    /*const int chunk_bound_distance = 4;

    int counter = 10;
    for(int x = -chunk_bound_distance; x <= chunk_bound_distance; x++)
    for(int z = -chunk_bound_distance; z <= chunk_bound_distance; z++)
    for(int y = -chunk_bound_distance; y <= chunk_bound_distance; y++) {
        glm::ivec3 position = (glm::ivec3{x,y,z} + glm::ivec3{glm::floor(game_state->getPlayer().getPosition() /
    (float)CHUNK_SIZE)})  * CHUNK_SIZE; wireframeRenderer.setCube(counter++, position, glm::vec3{CHUNK_SIZE},
    glm::vec3{0.1,0.5,0.5});
    }*/

    sunDirUniform.setValue({
        cos(glm::radians(sunAngle)), // X position (cosine component)
        sin(glm::radians(sunAngle)), // Y position (sine component for vertical angle)
        0                            // Z position (cosine component)
    });

    gBuffer->bind();
    GL_CALL(glViewport(0, 0, camera.getScreenWidth(), camera.getScreenHeight()));

    if (lineMode)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    block_texture_array->bind(0);

    GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // Draw skybox
    skyboxProgram.updateUniforms();
    skybox.draw();
    // ====

    // Draw terrain
    terrain_manager.getMeshRegistry().draw();

    // Draw models
    modelProgram.updateUniforms();
    Model::DrawAll();

    wireframeRenderer.draw();
    cubeRenderer.draw();

    GL_CALL(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    gBuffer->unbind();

    // timer.timestamp("Rendered to gBuffer->");

    gBufferProgram.updateUniforms();

    GL_CALL(glViewport(0, 0, gBuffer->getWidth(), gBuffer->getHeight()));
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    gBuffer->bindTextures();

    fullscreen_quad.render();

    gBuffer->unbindTextures();
    // timer.timestamp("Rendered to screen.");
}

void MainScene::updateLoadedLocations(glm::ivec3 old_location, glm::ivec3 new_location) {
    terrain_manager.loadRegion(new_location, renderDistance);
}

void MainScene::physicsUpdate() {
    last_tick_time = glfwGetTime();
    double current = glfwGetTime();
    float deltatime;

    lastCamWorldPosition = {-1000000,-1000000,-100000};
    //double last_sun_move_time = glfwGetTime();

    while (running) {
        current   = glfwGetTime();
        deltatime = (float)(current - last_tick_time);

        /*if(current - last_sun_move_time > 0.01){
            sunAngle = sunAngle + 0.1;
            if(sunAngle > 360.0f) sunAngle = 0;
            std::cout << sunAngle << std::endl;
            last_sun_move_time = glfwGetTime();
        }*/

        // threadPool->deployPendingJobs();

        if (deltatime < tickTime)
            continue;

        last_tick_time = current;

        glm::ivec3 camWorldPosition = glm::floor(camera.getPosition() / static_cast<float>(CHUNK_SIZE));
        if (glm::distance(glm::vec2(camWorldPosition.x, camWorldPosition.z), glm::vec2(lastCamWorldPosition.x, lastCamWorldPosition.z)) >= 2 ||
            update_render_distance) { // Crossed chunks

            updateLoadedLocations(lastCamWorldPosition, camWorldPosition);
            lastCamWorldPosition   = camWorldPosition;
            update_render_distance = false;
        }

        if (!game_state->GetTerrain().getChunk(camWorldPosition))
            continue;

        glm::vec3 camDir        = glm::normalize(camera.getDirection());
        glm::vec3 horizontalDir = glm::normalize(glm::vec3(camDir.x, 0, camDir.z));
        glm::vec3 leftDir       = glm::normalize(glm::cross(camera.getUp(), horizontalDir));

        auto& player = game_state->getPlayer();

        lastCamPosition = camPosition;
        camPosition     = player.getPosition() + camOffset;

        if (!CurrentGameMode() || (!CurrentGameMode()->NoClip())) {
            float speed = camAcceleration;

            player.setGravity(true);
            bool moving = false;
            if (inputManager.isActive(STRAFE_RIGHT)) {
                player.accelerate(-leftDir * speed, deltatime);
                moving = true;
            }
            if (inputManager.isActive(STRAFE_LEFT)) {
                player.accelerate(leftDir * speed, deltatime);
                moving = true;
            }
            if (inputManager.isActive(MOVE_BACKWARD)) {
                player.accelerate(-horizontalDir * speed, deltatime);
                moving = true;
            }
            if (inputManager.isActive(MOVE_FORWARD)) {
                player.accelerate(horizontalDir * speed, deltatime);
                moving = true;
            }

            if (!moving)
                player.decellerate(speed, deltatime);
            // if(boundKeys[1].isDown) player.accelerate(-camera.getUp() * 0.2f);
            // if(inputManager.isActive(MOVE_UP)) player.accelerate(camera.getUp() * 0.2f);
            if (inputManager.isActive(MOVE_DOWN))
                player.accelerate(-camera.getUp() * 2.0f, deltatime);
            if (inputManager.isActive(MOVE_UP) && game_state->entityCollision(player, {0, -0.1f, 0}) && player.getVelocity().y == 0)
                player.accelerate(camera.getUp() * 10.0f, 1.0);
        } else {
            player.setGravity(false);
            if (inputManager.isActive(STRAFE_RIGHT))
                player.setPosition(player.getPosition() + -leftDir * camAcceleration * deltatime);
            if (inputManager.isActive(STRAFE_LEFT))
                player.setPosition(player.getPosition() + leftDir * camAcceleration * deltatime);
            if (inputManager.isActive(MOVE_BACKWARD))
                player.setPosition(player.getPosition() + -horizontalDir * camAcceleration * deltatime);
            if (inputManager.isActive(MOVE_FORWARD))
                player.setPosition(player.getPosition() + horizontalDir * camAcceleration * deltatime);

            if (inputManager.isActive(MOVE_DOWN))
                player.setPosition(player.getPosition() + -camera.getUp() * camAcceleration * deltatime);
            if (inputManager.isActive(MOVE_UP))
                player.setPosition(player.getPosition() + camera.getUp() * camAcceleration * deltatime);
        }

        HandleGamemodeEvent(&GameMode::PhysicsUpdate, deltatime);

        game_state->updateEntities(deltatime);
    }

    threadsStopped++;
}

void UILoading::getRenderingInformation(UIRenderBatch& batch) {
    if (!values)
        return;

    const int margin = 10;
    const int width  = (transform.width - values->size() * margin) / values->size();

    int i = 0;
    for (auto& value : *values) {
        if (value > max_values[i])
            max_values[i] = value;

        float max = max_values[i] != 0 ? max_values[i] : 1;

        batch.Rectangle(transform.x + width * i + margin * (i + 1), transform.y, width, ((float)value / max) * (float)transform.height, UIColor{200, 100, 0});
        i++;
    }
}
