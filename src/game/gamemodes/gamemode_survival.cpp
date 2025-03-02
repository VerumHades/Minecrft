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

    getLayerLocal("base").addElement(crosshair);
    getLayerLocal("base").addElement(hotbar);
}

void GameModeSurvival::KeyEvent(GameModeState& state, int key, int scancode, int action, int mods){
    if(isActiveLayerLocal("base", state)){
        if(key == GLFW_KEY_Q && action == GLFW_PRESS){
            auto* slot = hotbar->getSelectedSlot();
            if(!slot || !slot->hasItem()) return;
            
            auto* item_prototype = slot->getItem()->getPrototype();
            if(!item_prototype) return;
            
            auto entity = DroppedItem::create(state.camera.getPosition() + state.camera.getDirection() * 0.5f, ItemRegistry::get().createItem(item_prototype));
            entity.accelerate(state.camera.getDirection(),1.0f);
            state.game_state.addEntity(entity);

            slot->decreaseQuantity(1);
            hotbar->update();
        }
    }
}
void GameModeSurvival::MouseEvent(GameModeState& state, int button, int action, int mods){
    if(!cursor_state.blockUnderCursor || cursor_state.blockUnderCursor->id == BLOCK_AIR_INDEX) return;

    auto& player = state.game_state.getPlayer();

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
            state.game_state.addEntity(entity);
            //auto& selected_slot = hotbar->getSelectedSlot();
            //inventory->addItem();

            state.game_state.getTerrain().setBlock(cursor_state.blockUnderCursorPosition, {BLOCK_AIR_INDEX});

            auto chunk = state.game_state.getTerrain().getChunkFromBlockPosition(cursor_state.blockUnderCursorPosition);
            if(!chunk) return;
            regenerateChunkMesh(chunk, state.game_state.getTerrain().getGetChunkRelativeBlockPosition(cursor_state.blockUnderCursorPosition));
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS){
            auto* block_prototype = BlockRegistry::get().getPrototype(cursor_state.blockUnderCursor->id);
            if(block_prototype && block_prototype->interface){
                block_prototype->interface->open(cursor_state.blockUnderCursor->metadata, game_state.get());
                state.setUILayer(block_prototype->interface->getName());
                return;
            }

            glm::ivec3 blockPosition = glm::floor(cursor_state.blockUnderCursorEmpty);

            auto* selected_slot = hotbar->getSelectedSlot();
            if(!selected_slot || !selected_slot->hasItem()) return;

            auto* prototype = selected_slot->getItem()->getPrototype();
            if(!prototype || !prototype->isBlock()) return;

            state.game_state.getTerrain().setBlock(blockPosition, {prototype->getBlockID()});
            if(
                state.game_state.entityCollision(player, player.getVelocity()) ||
                state.game_state.entityCollision(player)
            ){
                state.game_state.getTerrain().setBlock(blockPosition, {BLOCK_AIR_INDEX});
                return;
            }

            selected_slot->decreaseQuantity(1);
            hotbar->update();

            auto* chunk = state.game_state.getTerrain().getChunkFromBlockPosition(blockPosition);
            if(!chunk) return;
            regenerateChunkMesh(chunk, state.game_state.getTerrain().getGetChunkRelativeBlockPosition(blockPosition));
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

void GameModeSurvival::updateCursor(GameModeState& state){
    const glm::vec3& camDirection = state.camera.getDirection();
    const glm::vec3& camPosition = state.camera.getPosition();

    RaycastResult hit = state.game_state.getTerrain().raycast(camPosition,camDirection,10);
 
    cursor_state.blockUnderCursor = state.game_state.getTerrain().getBlock(hit.position);
    cursor_state.blockUnderCursorPosition = hit.position;
    cursor_state.blockUnderCursorEmpty = hit.lastPosition;

    if(!cursor_state.blockUnderCursor || cursor_state.blockUnderCursor->id == BLOCK_AIR_INDEX) state.wireframe_renderer.removeCube(0);
    else state.wireframe_renderer.setCube(0,glm::vec3(hit.position) - 0.005f, {1.01,01.01,1.01},{0,0,0});
    
    //wireframeRenderer.setCube(1,glm::vec3(hit.lastPosition) - 0.005f, {1.01,01.01,1.01},{1.0,0,0});
}