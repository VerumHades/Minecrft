#include "structure/bytearray.hpp"
#include "structure/streams/file_stream.hpp"
#include <game/gamemodes/structure_capture.hpp>

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

void GameModeStructureCapture::Initialize(){
    base_layer_override = "structure_capture";

    structure_capture_start_label = getElementById<UILabel>("start_label");
    structure_capture_end_label   = getElementById<UILabel>("end_label");

    getElementById<UIFrame>("submit")->onClicked = [this](){
        auto [min,max] = pointsToRegion(structureCaptureStart, structureCaptureEnd);
        glm::ivec3 size = (max - min) + glm::ivec3(1,1,1);

        Structure structure = Structure::capture(min, size, state.game_state->GetTerrain());

        const std::string& name = getElementById<UIInput>("structure_name")->getText();

        auto path = Paths::Get(Paths::GAME_STRUCTURES);
        if(path){
            ByteArray array{};
            Serializer::Serialize<Structure>(structure, array);

            FileStream stream{};
            stream.Open(path.value() / (name + ".structure"));

            array.WriteToStream(stream);
        }
        state.scene.setUILayer(GetBaseLayer().name);
        RefreshStructureSelection();
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

        ByteArray array{};

        FileStream stream{};
        stream.Open(structures_path.value() / (name + ".structure"));
        array.LoadFromStream(stream);

        selected_structure = std::make_shared<Structure>(0,0,0);
        Serializer::Deserialize<Structure>(*selected_structure.get(), array);

        structure_name_label->setText("Selected structure:" + name);
        structure_name_label->update();

        this->UpdateStructureDisplay();
    };

    RefreshStructureSelection();

    GetBaseLayer().addElement(structure_selection);
    GetBaseLayer().cursorMode = GLFW_CURSOR_DISABLED;

    GetBaseLayer().onEntered = [this](){
        UICore::get().setFocus(structure_selection);
    };
}

void GameModeStructureCapture::Open(){

}

void GameModeStructureCapture::Render(double deltatime){
    UpdateStructureDisplay();
}

void GameModeStructureCapture::KeyEvent(int key, int scancode, int action, int mods){
    if(IsBaseLayerActive()){
        if(key == GLFW_KEY_B && action == GLFW_PRESS){
            structure_menu_mode = structure_menu_mode == CAPTURE ? PLACE : CAPTURE;
            UpdateStructureDisplay();
        }
        else if(key == GLFW_KEY_N && action == GLFW_PRESS){
            if(state.scene.isActiveLayer("structure_saving")){
                state.scene.setUILayer(GetBaseLayer().name);
                UICore::get().setFocus(structure_selection);
            }
            else{
                state.scene.setUILayer("structure_saving");
                UICore::get().setFocus(getElementById<UIFrame>("structure_name"));
                UpdateStructureSavingDisplay();
            }
        }
    }
}

void GameModeStructureCapture::MouseEvent(int button, int action, int mods){
    if(IsBaseLayerActive()){
        if(structure_menu_mode == CAPTURE){
            if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
                structureCaptured = false;
                structureCaptureStart = cursor_state.blockUnderCursorPosition;
                UpdateStructureDisplay();
            }
            else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS){
                structureCaptureEnd = cursor_state.blockUnderCursorPosition;
                structureCaptured = true;
                UpdateStructureDisplay();
            }
        }
        else{
            if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
                if(selected_structure){
                    auto positions = selected_structure->place(cursor_state.blockUnderCursorPosition, state.game_state->GetTerrain());
                    for(auto& position: positions)
                    state.regenerateChunkMesh(state.game_state->GetTerrain().getChunk(position), {1,1,1});
                }
            }
        }
    }
}

void GameModeStructureCapture::MouseMoved(int mouseX, int mouseY){
    UpdateCursor();
}

void GameModeStructureCapture::MouseScroll(double xoffset, double yoffset){

}

void GameModeStructureCapture::PhysicsUpdate(double deltatime){

}


template <typename T>
static inline std::string vecToString(T vec, const std::string& separator = " "){
    return std::to_string(vec.x) + separator + std::to_string(vec.y) + separator + std::to_string(vec.z);
}

void  GameModeStructureCapture::UpdateStructureDisplay(){
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

        state.wireframe_renderer.setCube(1,glm::vec3(min) - 0.005f, glm::vec3{0.01,0.01,0.01} + glm::vec3(size),{0,0.1,0.1});
    }
    else{
        if(!selected_structure) return;
        state.wireframe_renderer.setCube(
            1,
            glm::vec3(cursor_state.blockUnderCursorPosition) - 0.005f,
            glm::vec3{0.01,0.01,0.01} + glm::vec3(selected_structure->getSize()),
            {0,0.3,0.2}
        );
    }
}

void GameModeStructureCapture::UpdateStructureSavingDisplay(){
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

void GameModeStructureCapture::RefreshStructureSelection(){
    auto structures_path = Paths::Get(Paths::GAME_STRUCTURES);
    if(structures_path){
        structure_selection->clear();
        for (const auto& entry: fs::directory_iterator(structures_path.value())){
            auto& path = entry.path();
            if(!entry.is_regular_file()) continue;

            structure_selection->addOption(path.stem().string());
        }
        UICore::get().setFocus(structure_selection);
        structure_selection->update();
    }
}
