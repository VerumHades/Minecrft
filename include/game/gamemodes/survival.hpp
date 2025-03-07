#pragma once

#include <ui/elements.hpp>

#include <game/gamemodes/interactable_base.hpp>

#include <game/items/item.hpp>
#include <game/items/item_renderer.hpp>
#include <game/items/crafting.hpp>

#include <atomic>

class UIHotbar;

class GameModeSurvival: public GameModeInteractable{
    private:
        std::shared_ptr<ItemSlot> held_item_slot;
        std::shared_ptr<InventoryDisplay> inventory;
        std::shared_ptr<UIHotbar> hotbar;

        std::shared_ptr<UILabel> fps_label;

        std::atomic<bool> update_hotbar = false;
        
        bool mining = false;
        bool can_break = false;
        float mining_delay = 1.0f;
        float mining_delay_max = 1.0f;

        ItemTextureAtlas itemTextureAtlas{};

        void BreakBlockUnderCursor();
        void PlaceBlock();

    public:
        GameModeSurvival(GameModeState& state): GameModeInteractable(state, "survival"){

        }

        void Initialize() override;
        
        void Open() override;
        void Render(double deltatime) override;
        void PhysicsUpdate(double deltatime) override;

        void KeyEvent(int key, int scancode, int action, int mods) override;
        void MouseEvent(int button, int action, int mods) override;
        void MouseMoved(int xoffset, int yoffset) override;
        void MouseScroll(double xoffset, double yoffset) override;
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