#include <game/gamemodes/gamemode_survival.hpp>

GameModeSurvival::GameModeSurvival(): GameMode("survival"){
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

    getLayerLocal("inventory").addElement(inventory);
    getLayerLocal("inventory").addElement(hotbar);
    getLayerLocal("inventory").addElement(held_item_slot);


    auto crosshair = std::make_shared<UICrosshair>();
    crosshair->setSize(60_px,60_px);
    crosshair->setPosition(TValue::Center(), TValue::Center());
    crosshair->setAttribute(&UIFrame::Style::textColor, {255,255,255});

    GetBaseLayer().addElement(crosshair);
    GetBaseLayer().addElement(hotbar);
}

void GameModeSurvival::Initialize(GameModeState& state){
    for(auto& prototype: BlockRegistry::get().prototypes()){
        if(prototype.id == 0) continue; // Dont make air
        
        if(prototype.name == "crafting"){
            auto interface = std::make_unique<CraftingInterface>(prototype.name + "_interface", itemTextureAtlas, held_item_slot);
            state.addLayer(interface->getLayer());

            BlockRegistry::get().setPrototypeInterface(prototype.id, std::move(interface));
        }

        ItemRegistry::get().createPrototypeForBlock(&prototype);
    }

    CraftingRecipeRegistry::get().addRecipe(CraftingRecipe({CraftingRecipe::RecipeItemRequirement{"block_iron", 5},{},{},{},{},{},{},{},{}}, "crazed",64));
    CraftingRecipeRegistry::get().addRecipe(CraftingRecipe(
        {CraftingRecipe::RecipeItemRequirement{"crazed", 1},{"crazed", 1},{"crazed", 1},{"crazed", 1},{"crazed", 1},{"crazed", 1},{"crazed", 1},{"crazed", 1},{"crazed", 1}}
        , "block_crazed",1));
}

void GameModeSurvival::Open(GameModeState& state){
    if(!state.game_state) return;
    auto& game_state = *state.game_state;

    inventory->setInventory(&game_state.getPlayerInventory());
    hotbar->setInventory(&game_state.getPlayerHotbar());

    auto& player = game_state.getPlayer();

    player.onCollision = [this, state](Entity* self, Entity* collided_with){
        if(!collided_with->getData() || collided_with->getData()->type != EntityData::DROPPED_ITEM) return;
        
        const auto& data = dynamic_pointer_cast<DroppedItem>(collided_with->getData());
        //const auto* data = reinterpret_cast<const DroppedItem::Data*>(collided_with->getData());
        state.game_state->giveItemToPlayer(data->getItem());

        this->update_hotbar = true;
    };

    if(player.getPosition().x == 0.0f && player.getPosition().z == 0.0f){
        int height = state.terrain_manager.getWorldGenerator().getHeightAt({0,0,0});
        player.setPosition({0,height + 5,0});
    }
}

void GameModeSurvival::Render(GameModeState& state){
    if(update_hotbar){
        hotbar->update();
        update_hotbar = false;
    }
}

void GameModeSurvival::KeyEvent(GameModeState& state, int key, int scancode, int action, int mods){
    if(!state.game_state) return;
    auto& game_state = *state.game_state;
    
    if(isActiveLayerLocal("base", state)){
        if(key == GLFW_KEY_Q && action == GLFW_PRESS){
            auto* slot = hotbar->getSelectedSlot();
            if(!slot || !slot->hasItem()) return;
            
            auto* item_prototype = slot->getItem()->getPrototype();
            if(!item_prototype) return;
            
            auto entity = DroppedItem::create(state.camera.getPosition() + state.camera.getDirection() * 0.5f, ItemRegistry::get().createItem(item_prototype));
            entity.accelerate(state.camera.getDirection(),1.0f);
            game_state.addEntity(entity);

            slot->decreaseQuantity(1);
            hotbar->update();
        }
    }
}
void GameModeSurvival::MouseEvent(GameModeState& state, int button, int action, int mods){
    if(!state.game_state) return;
    auto& game_state = *state.game_state;
    
    if(!cursor_state.blockUnderCursor || cursor_state.blockUnderCursor->id == BLOCK_AIR_INDEX) return;

    auto& player = game_state.getPlayer();

    if(isActiveLayerLocal("base", state)){
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS ){  
            auto* block_prototype = BlockRegistry::get().getPrototype(cursor_state.blockUnderCursor->id);
            if(!block_prototype) return;
            auto* item_prototype = ItemRegistry::get().getPrototype("block_" + block_prototype->name);
            if(!item_prototype) return;
            
            Entity entity = DroppedItem::create(glm::vec3(cursor_state.blockUnderCursorPosition) + glm::vec3(0.5,0.5,0.5), ItemRegistry::get().createItem(item_prototype));
            entity.accelerate({
                static_cast<float>(std::rand() % 200) / 100.0f - 1.0f,
                0.6f,
                static_cast<float>(std::rand() % 200) / 100.0f - 1.0f
            }, 1.0f);
            game_state.addEntity(entity);
            //auto& selected_slot = hotbar->getSelectedSlot();
            //inventory->addItem();

            game_state.getTerrain().setBlock(cursor_state.blockUnderCursorPosition, {BLOCK_AIR_INDEX});

            auto chunk = game_state.getTerrain().getChunkFromBlockPosition(cursor_state.blockUnderCursorPosition);
            if(!chunk) return;
            state.regenerateChunkMesh(chunk, game_state.getTerrain().getGetChunkRelativeBlockPosition(cursor_state.blockUnderCursorPosition));
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS){
            auto* block_prototype = BlockRegistry::get().getPrototype(cursor_state.blockUnderCursor->id);
            if(block_prototype && block_prototype->interface){
                block_prototype->interface->open(cursor_state.blockUnderCursor->metadata, &game_state);
                state.setUILayer(block_prototype->interface->getName());
                return;
            }

            glm::ivec3 blockPosition = glm::floor(cursor_state.blockUnderCursorEmpty);

            auto* selected_slot = hotbar->getSelectedSlot();
            if(!selected_slot || !selected_slot->hasItem()) return;

            auto* prototype = selected_slot->getItem()->getPrototype();
            if(!prototype || !prototype->isBlock()) return;

            game_state.getTerrain().setBlock(blockPosition, {prototype->getBlockID()});
            if(
                game_state.entityCollision(player, player.getVelocity()) ||
                game_state.entityCollision(player)
            ){
                game_state.getTerrain().setBlock(blockPosition, {BLOCK_AIR_INDEX});
                return;
            }

            selected_slot->decreaseQuantity(1);
            hotbar->update();

            auto* chunk = game_state.getTerrain().getChunkFromBlockPosition(blockPosition);
            if(!chunk) return;
            state.regenerateChunkMesh(chunk, game_state.getTerrain().getGetChunkRelativeBlockPosition(blockPosition));
        }
    }
   
    updateCursor(state);
}

void GameModeSurvival::MouseMoved(GameModeState& state, int mouseX, int mouseY){
    if(held_item_slot){
        held_item_slot->setPosition(
            TValue::Pixels(mouseX),
            TValue::Pixels(mouseY)
        );
        held_item_slot->calculateTransforms();
        held_item_slot->update();
    }
    
    updateCursor(state);
}

void GameModeSurvival::MouseScroll(GameModeState& state, double xoffset, double yoffset){
    int scroll = abs(yoffset) / yoffset;
    hotbar->selectSlot(hotbar->getSelectedSlotNumber() - scroll);
    hotbar->update();
}

void GameModeSurvival::updateCursor(GameModeState& state){
    if(!state.game_state) return;
    auto& game_state = *state.game_state;
    
    const glm::vec3& camDirection = state.camera.getDirection();
    const glm::vec3& camPosition = state.camera.getPosition();

    RaycastResult hit = game_state.getTerrain().raycast(camPosition,camDirection,10);
 
    cursor_state.blockUnderCursor = game_state.getTerrain().getBlock(hit.position);
    cursor_state.blockUnderCursorPosition = hit.position;
    cursor_state.blockUnderCursorEmpty = hit.lastPosition;

    if(!cursor_state.blockUnderCursor || cursor_state.blockUnderCursor->id == BLOCK_AIR_INDEX) state.wireframe_renderer.removeCube(0);
    else state.wireframe_renderer.setCube(0,glm::vec3(hit.position) - 0.005f, {1.01,01.01,1.01},{0,0,0});
    
    //wireframeRenderer.setCube(1,glm::vec3(hit.lastPosition) - 0.005f, {1.01,01.01,1.01},{1.0,0,0});
}

void GameModeSurvival::PhysicsUpdate(GameModeState& state){
    auto* in_hand_slot = hotbar->getSelectedSlot();
    if(in_hand_slot && in_hand_slot->hasItem()){
        auto* prototype = in_hand_slot->getItem()->getPrototype();
        if(prototype && prototype->getModel()){
            glm::vec3 item_offset = state.camera.getDirection() * 0.5f - state.camera.getLeft() + state.camera.getRelativeUp() * 0.4f;
            prototype->getModel()->requestDraw(state.camera.getPosition() + item_offset, {1,1,1}, {0,-state.camera.getYaw(),state.camera.getPitch()}, {-0.5,-0.5,0}, {Model::Y,Model::Z,Model::X});
        }
    }
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
