#include <game/gamemodes/survival.hpp>

void GameModeSurvival::Initialize() {
    held_item_slot = std::make_shared<ItemSlot>(itemTextureAtlas);

    auto frame = std::make_shared<UIFrame>();
    frame->setPosition(TValue::Center(), TValue::Center());
    frame->setSize(TValue{PERCENT, 50}, TValue{FIT_CONTENT, 0});
    frame->setLayout(std::make_shared<UIFlexLayout>(UIFlexLayout::VERTICAL));

    inventory = std::make_shared<InventoryDisplay>(itemTextureAtlas, held_item_slot);
    inventory->setPosition(TValue::Center(), TValue::Center());

    inventory_crafting = std::make_shared<CraftingDisplay>(itemTextureAtlas, held_item_slot);

    frame->appendChild(inventory_crafting);
    frame->appendChild(inventory);

    hotbar = std::make_shared<UIHotbar>(itemTextureAtlas, held_item_slot);
    hotbar->setPosition(TValue::Center(), TValue::Bottom(20_px));

    health_bar = std::make_shared<HealthBar>(nullptr);
    health_bar->setPosition(TValue::Center(), TValue::Bottom(100_px));
    health_bar->setSize(TValue::Pixels(320), TValue::Pixels(32));

    auto& inventory_layer = getLayerLocal("inventory");
    inventory_layer.addElement(frame);
    inventory_layer.addElement(hotbar);
    inventory_layer.addElement(health_bar);
    inventory_layer.addElement(held_item_slot);

    auto crosshair = std::make_shared<UICrosshair>();
    crosshair->setSize(60_px, 60_px);
    crosshair->setPosition(TValue::Center(), TValue::Center());
    crosshair->setAttribute(&UIFrame::Style::textColor, {255, 255, 255});

    fps_label = std::make_shared<UILabel>();
    fps_label->setPosition(10_px, 10_px);

    auto& base_layer = GetBaseLayer();
    base_layer.addElement(fps_label);
    base_layer.addElement(crosshair);
    base_layer.addElement(hotbar);
    base_layer.addElement(health_bar);
    base_layer.cursorMode = GLFW_CURSOR_DISABLED;

    for (auto& prototype : BlockRegistry::get().prototypes()) {
        if (prototype.id == 0)
            continue; // Dont make air

        if (prototype.name == "crafting") {
            auto interface = std::make_unique<CraftingInterface>(
                state.scene.getWindow()->getLayerPointer(prototype.name + "_interface"), itemTextureAtlas,
                held_item_slot);

            BlockRegistry::get().setPrototypeInterface(prototype.id, std::move(interface));
        }

        ItemRegistry::get().createPrototypeForBlock(&prototype);
    }

    onCursorTargetChange = [this]() {
        auto* block_prototype = BlockRegistry::get().getPrototype(cursor_state.blockUnderCursor->id);
        if (!block_prototype) {
            mining_delay     = 1.0f;
            mining_delay_max = 1.0f;
            return;
        }

        float mining_power = 1;

        auto* slot = hotbar->getSelectedSlot();
        if (slot && slot->getItem() && slot->getItem()->getPrototype()) {
            for (auto& effectiveness : slot->getItem()->getPrototype()->getToolEffectiveness()) {
                if (effectiveness.material_name != block_prototype->material_name)
                    continue;
                mining_power = effectiveness.mining_power;
                break;
            }
        }

        can_break        = mining_power + 1 >= block_prototype->hardness;
        mining_delay_max = static_cast<float>(block_prototype->hardness) / mining_power;
        mining_delay     = mining_delay_max;
    };
}

void GameModeSurvival::Open() {
    if (!state.game_state)
        return;
    auto& game_state = *state.game_state;

    inventory->setInventory(&game_state.getPlayerInventory());
    hotbar->setInventory(&game_state.getPlayerHotbar());
    inventory_crafting->setInventories(&game_state.getPlayerCraftingInventory(),
                                       &game_state.getPlayerCraftingResultInventory());
    health_bar->setHealth(&game_state.getPlayerHealth());

    auto& player = game_state.getPlayer();

    player.onTerrainCollision = [this](Entity* self) {
        if (self->getVelocity().y < -15.0f) {
            state.game_state->getPlayerHealth() += self->getVelocity().y / 5;
            update_healthbar = true;
        }
    };

    player.onCollision = [this](Entity* self, Entity* collided_with) {
        if (!collided_with->getData() || collided_with->getData()->type != EntityData::DROPPED_ITEM)
            return;

        const auto& data = dynamic_pointer_cast<DroppedItem>(collided_with->getData());
        // const auto* data = reinterpret_cast<const DroppedItem::Data*>(collided_with->getData());
        state.game_state->giveItemToPlayer(data->getItem());

        this->update_hotbar = true;
    };

    if (player.getPosition().x == 0.0f && player.getPosition().z == 0.0f) {
        int height = state.terrain_manager.getWorldGenerator().getHeightAt({0, 0, 0});
        player.setPosition({0, height + 5, 0});
    }
}

void GameModeSurvival::Render(double deltatime) {
    fps_label->setText(std::to_string(1.0f / deltatime) + "FPS");
    fps_label->update();

    if (update_hotbar) {
        hotbar->update();
        update_hotbar = false;
    }

    if (!state.game_state)
        return;
    auto& game_state = *state.game_state;

    if (update_healthbar) {
        if (game_state.getPlayerHealth() <= 0) {
            auto& player                 = game_state.getPlayer();
            game_state.getPlayerHealth() = 20;

            DropAllInventoryItems(player.getPosition(), &game_state.getPlayerInventory());
            DropAllInventoryItems(player.getPosition(), &game_state.getPlayerHotbar());
            DropAllInventoryItems(player.getPosition(), &game_state.getPlayerCraftingInventory());
            DropAllInventoryItems(player.getPosition(), &game_state.getPlayerCraftingResultInventory());

            int height = state.terrain_manager.getWorldGenerator().getHeightAt({0, 0, 0});
            game_state.getPlayer().setPosition({0, height + 5, 0});

            hotbar->update();
        }

        health_bar->update();
        update_healthbar = false;
    }

    if (!cursor_state.blockUnderCursor || cursor_state.blockUnderCursor->id == BLOCK_AIR_INDEX)
        return;

    if (mining)
        state.cube_renderer.setCube(0, cursor_state.blockUnderCursorPosition,
                                    5 - static_cast<int>(5.0f * (mining_delay / mining_delay_max)));
    else
        state.cube_renderer.removeCube(0);

    if (IsBaseLayerActive()) {
        if (state.input_manager.isDown(GLFW_MOUSE_BUTTON_LEFT) && can_break) {

            if (mining_delay <= 0.0f) {
                BreakBlockUnderCursor();
                mining_delay = mining_delay_max;
                state.cube_renderer.removeCube(0);
                UpdateCursor();
            } else
                mining_delay -= deltatime;

            mining = true;
        } else
            mining = false;
    }
}

void GameModeSurvival::DropItem(const glm::vec3& position, ItemRef item) {
    if (!state.game_state)
        return;

    Entity entity = DroppedItem::create(position, item);
    entity.accelerate({static_cast<float>(std::rand() % 200) / 100.0f - 1.0f, 0.6f,
                       static_cast<float>(std::rand() % 200) / 100.0f - 1.0f},
                      1.0f);
    state.game_state->addEntity(entity);
}

void GameModeSurvival::DropAllInventoryItems(const glm::vec3& position, LogicalItemInventory* inventory) {
    if (!inventory)
        return;
    for (auto& slot : inventory->getItemSlots()) {
        if (slot.getItem()) {
            DropItem(position, slot.getItem());
            slot.clear();
        }
    }
}

void GameModeSurvival::BreakBlockUnderCursor() {
    if (!state.game_state)
        return;
    auto& game_state = *state.game_state;

    if (!cursor_state.blockUnderCursor || cursor_state.blockUnderCursor->id == BLOCK_AIR_INDEX)
        return;

    auto* block_prototype = BlockRegistry::get().getPrototype(cursor_state.blockUnderCursor->id);
    if (!block_prototype)
        return;
    auto* item_prototype = ItemRegistry::get().getPrototype("block_" + block_prototype->name);
    if (!item_prototype)
        return;

    DropItem(glm::vec3(cursor_state.blockUnderCursorPosition) + glm::vec3(0.5, 0.5, 0.5), Item::Create(item_prototype));

    // auto& selected_slot = hotbar->getSelectedSlot();
    // inventory->addItem();

    game_state.getTerrain().setBlock(cursor_state.blockUnderCursorPosition, {BLOCK_AIR_INDEX});

    auto chunk = game_state.getTerrain().getChunkFromBlockPosition(cursor_state.blockUnderCursorPosition);
    if (!chunk)
        return;
    state.regenerateChunkMesh(
        chunk, game_state.getTerrain().getGetChunkRelativeBlockPosition(cursor_state.blockUnderCursorPosition));
}

void GameModeSurvival::PlaceBlock() {
    if (!state.game_state)
        return;
    auto& game_state = *state.game_state;

    if (!cursor_state.blockUnderCursor || cursor_state.blockUnderCursor->id == BLOCK_AIR_INDEX)
        return;

    auto* block_prototype = BlockRegistry::get().getPrototype(cursor_state.blockUnderCursor->id);

    if (block_prototype && block_prototype->interface) {
        block_prototype->interface->open(cursor_state.blockUnderCursor->metadata, state.game_state);
        state.scene.setUILayer(block_prototype->interface->getName());
        return;
    }

    auto& player = game_state.getPlayer();

    glm::ivec3 blockPosition = glm::floor(cursor_state.blockUnderCursorEmpty);

    auto* selected_slot = hotbar->getSelectedSlot();
    if (!selected_slot || !selected_slot->hasItem())
        return;

    auto* prototype = selected_slot->getItem()->getPrototype();
    if (!prototype || !prototype->isBlock())
        return;

    game_state.getTerrain().setBlock(blockPosition, {prototype->getBlockID()});
    if (game_state.entityCollision(player, player.getVelocity()) || game_state.entityCollision(player)) {
        game_state.getTerrain().setBlock(blockPosition, {BLOCK_AIR_INDEX});
        return;
    }

    selected_slot->decreaseQuantity(1);
    hotbar->update();

    auto* chunk = game_state.getTerrain().getChunkFromBlockPosition(blockPosition);
    if (!chunk)
        return;
    state.regenerateChunkMesh(chunk, game_state.getTerrain().getGetChunkRelativeBlockPosition(blockPosition));
}

void GameModeSurvival::KeyEvent(int key, int scancode, int action, int mods) {
    if (!state.game_state)
        return;
    auto& game_state = *state.game_state;

    if (IsBaseLayerActive()) {
        if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
            auto* slot = hotbar->getSelectedSlot();
            if (!slot || !slot->hasItem())
                return;

            auto* item_prototype = slot->getItem()->getPrototype();
            if (!item_prototype)
                return;

            auto entity = DroppedItem::create(state.camera.getPosition() + state.camera.getDirection() * 0.5f,
                                              Item::Create(item_prototype));
            entity.accelerate(state.camera.getDirection(), 1.0f);
            game_state.addEntity(entity);

            slot->decreaseQuantity(1);
            hotbar->update();
        } else if ((key == GLFW_KEY_TAB || key == GLFW_KEY_E) && action == GLFW_PRESS)
            setLayerLocal("inventory");
    } else if (isActiveLayerLocal("inventory") && (key == GLFW_KEY_TAB || key == GLFW_KEY_E) && action == GLFW_PRESS)
        state.scene.setUILayer(GetBaseLayer().name);
}
void GameModeSurvival::MouseEvent(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && IsBaseLayerActive()) {
        PlaceBlock();
        UpdateCursor();
    }
}

void GameModeSurvival::MouseMoved(int mouseX, int mouseY) {
    if (held_item_slot) {
        held_item_slot->setPosition(TValue::Pixels(mouseX), TValue::Pixels(mouseY));
        held_item_slot->calculateTransforms();
        held_item_slot->update();
    }

    UpdateCursor();
}

void GameModeSurvival::MouseScroll(double xoffset, double yoffset) {
    int scroll = abs(yoffset) / yoffset;
    hotbar->selectSlot(hotbar->getSelectedSlotNumber() - scroll);
    hotbar->update();

    onCursorTargetChange();
}

void GameModeSurvival::PhysicsUpdate(double deltatime) {
    auto* in_hand_slot = hotbar->getSelectedSlot();

    if (in_hand_slot && in_hand_slot->hasItem()) {
        auto* prototype = in_hand_slot->getItem()->getPrototype();
        if (prototype && prototype->getModel() &&
            (!in_hand_model_instance || !in_hand_model_instance->IsOfModel(*prototype->getModel()))) {
            in_hand_model_instance = prototype->getModel()->NewInstance();
            in_hand_model_instance->MoveRotationOffset({-0.5, -0.5, 0});
            in_hand_model_instance->Scale({1.0,1.0,1.0});
            // prototype->getModel()->requestDraw(state.camera.getPosition() + item_offset, {1, 1, 1}, rot,
            //                                    {-0.5, -0.5, 0});
        }

        if (in_hand_model_instance) {
            glm::vec3 item_offset =
                state.camera.getDirection() * 0.5f - state.camera.getLeft() + state.camera.getRelativeUp() * 0.4f;

            glm::mat4 look = glm::lookAt(glm::vec3(0), state.camera.getDirection(), glm::vec3(0, 1, 0));
            glm::quat rot  = glm::quat_cast(glm::inverse(look));

            in_hand_model_instance->MoveTo(state.camera.getPosition() + item_offset);
            in_hand_model_instance->Rotate(rot);
        }
    } else
        in_hand_model_instance = nullptr;
}

void UICrosshair::getRenderingInformation(UIRenderBatch& batch) {
    auto color = getAttribute(&UIFrame::Style::textColor);

    // Left
    batch.Rectangle(transform.x, transform.y + transform.height / 2 - thickness / 2, transform.width / 2 - part_margin,
                    thickness, color);
    // Right
    batch.Rectangle(transform.x + transform.width / 2 + part_margin, transform.y + transform.height / 2 - thickness / 2,
                    transform.width / 2 - part_margin, thickness, color);

    // Top
    batch.Rectangle(transform.x + transform.width / 2 - thickness / 2, transform.y, thickness,
                    transform.height / 2 - part_margin, color);

    // Bottom
    batch.Rectangle(transform.x + transform.width / 2 - thickness / 2, transform.y + transform.height / 2 + part_margin,
                    thickness, transform.height / 2 - part_margin, color);
}

void UIHotbar::getRenderingInformation(UIRenderBatch& batch) {
    InventoryDisplay::getRenderingInformation(batch);

    batch.BorderedRectangle(transform.x + selected_slot * slot_size, transform.y, slot_size, slot_size,
                            UIColor{0, 0, 0, 0}, UISideSizes{3, 3, 3, 3}, UIColor{180, 180, 180});
}

HealthBar::HealthBar(int* health) : health(health) {
    dedicated_texture_array = std::make_shared<GLTextureArray>();
    dedicated_texture_array->setup(32 * 3, 32, 1);

    dedicated_texture_array->putImage(0, 0, 0, Image::LoadWithSize("resources/textures/ui/heart_full.png", 32, 32));
    dedicated_texture_array->putImage(32, 0, 0,
                                      Image::LoadWithSize("resources/textures/ui/heart_half_empty.png", 32, 32));
    dedicated_texture_array->putImage(64, 0, 0, Image::LoadWithSize("resources/textures/ui/heart_empty.png", 32, 32));
}

void HealthBar::getRenderingInformation(UIRenderBatch& batch) {
    int hearts_total       = max_health / 2;
    int single_heart_width = transform.width / hearts_total;

    const float texture_step = 1.0f / 3.0f;

    int hp = 10;
    if (health)
        hp = *health;

    for (int i = 0; i < hearts_total; i++) {
        int texture_index = 2 - ((hp > 0) + (hp > 1));
        hp -= 2;

        batch.Texture(transform.x + single_heart_width * i, transform.y, single_heart_width, transform.height,
                      UIRegion{{texture_step * static_cast<float>(texture_index), 0},
                               {texture_step * static_cast<float>(texture_index) + texture_step, 1}});
    }
}
