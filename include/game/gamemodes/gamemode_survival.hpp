#pragma once

#include <game/gamemodes/gamemode.hpp>
#include <ui/elements.hpp>
#include <game/items/item.hpp>
#include <game/items/item_renderer.hpp>

class GameModeSurvival: public GameMode{
    private:
        struct CursorState{
            glm::ivec3 blockUnderCursorPosition = {0,0,0};
            glm::vec3 blockUnderCursorEmpty = {0,0,0}; // Block before the selected block, air where a block will be placed
            Block* blockUnderCursor = nullptr;
        } cursor_state;

        void updateCursor(GameModeState& state);

        std::shared_ptr<ItemSlot> held_item_slot;
        std::shared_ptr<InventoryDisplay> inventory;
        std::shared_ptr<UIHotbar> hotbar;

        ItemTextureAtlas itemTextureAtlas{};

    public:
        GameModeSurvival();

        void KeyEvent(GameModeState& state, int key, int scancode, int action, int mods) override;
        void MouseEvent(GameModeState& state, int button, int action, int mods) override;
        void MouseMoved(GameModeState& state, int xoffset, int yoffset) override;
};

class UICrosshair: public UIFrame{
    private:
        int part_margin = 5;
        int thickness = 5;
    public:
        UICrosshair(){setFocusable(false);setHoverable(false);}
        void getRenderingInformation(UIRenderBatch& batch) override;
};

class UIHotbar: public InventoryDisplay{
    private:
        int selected_slot = 0;
        const int slots_total = 9;

    public:
        UIHotbar(ItemTextureAtlas& textureAtlas, std::shared_ptr<ItemSlot> held_item_ptr):
        InventoryDisplay(textureAtlas, held_item_ptr){}

        void getRenderingInformation(UIRenderBatch& batch) override;
        void selectSlot(int slot){
            selected_slot = (slot + slots_total) % slots_total;
        }
        int getSelectedSlotNumber(){return selected_slot;}

        LogicalItemSlot* getSelectedSlot() {
            if(!inventory) return nullptr;
            return inventory->getSlot(selected_slot,0);
        };
};