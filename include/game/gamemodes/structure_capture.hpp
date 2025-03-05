#pragma once


#include <ui/elements.hpp>

#include <game/gamemodes/interactable_base.hpp>

#include <ui/elements/selection.hpp>

#include <atomic>

class GameModeStructureCapture: public GameModeInteractable{
    private:
        std::shared_ptr<UILabel> structure_capture_start_label;
        std::shared_ptr<UILabel> structure_capture_end_label;

        glm::ivec3 structureCaptureStart = {0,0,0};
        glm::ivec3 structureCaptureEnd   = {0,0,0};
        bool structureCaptured = false;

        enum StructureMenuMode{
            CAPTURE,
            PLACE
        } structure_menu_mode;

        std::shared_ptr<Structure> selected_structure = nullptr;
        std::shared_ptr<UISelection> structure_selection;

        void UpdateStructureDisplay();
        void UpdateStructureSavingDisplay();
        void RefreshStructureSelection();

    public:
        GameModeStructureCapture(GameModeState& state): GameModeInteractable(state, "structure_capture") {}; 

        void Initialize() override;
        
        void Open() override;
        void Render(double deltatime) override;
        void PhysicsUpdate(double deltatime) override;

        void KeyEvent(int key, int scancode, int action, int mods) override;
        void MouseEvent(int button, int action, int mods) override;
        void MouseMoved(int xoffset, int yoffset) override;
        void MouseScroll(double xoffset, double yoffset) override;

        bool NoClip() override { return true; };
};      
