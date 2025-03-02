#include <game/gamemodes/gamemode.hpp>

GameMode::GameMode(const std::string& name): name(name){

}

std::string GameMode::getLocalLayerName(const std::string& name){
    return name + "_" + this->name + "_gamemode_interface";
}

bool GameMode::isActiveLayerLocal(const std::string& name, GameModeState& state){
    return state.isActiveLayer(getLocalLayerName(name));
}

UILayer& GameMode::getLayerLocal(const std::string& name){
    if(!ui_layers.contains(name)){
        ui_layers.at(name) = std::make_shared<UILayer>();
        ui_layers.at(name)->name = getLocalLayerName(name);
    }

    return *ui_layers.at(name);
}