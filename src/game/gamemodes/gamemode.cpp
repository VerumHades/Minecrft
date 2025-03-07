#include <game/gamemodes/gamemode.hpp>

GameMode::GameMode(GameModeState& state, const std::string& name): state(state), name(name) {

}

std::string GameMode::getLocalLayerName(const std::string& name){
    return name + "_" + this->name + "_gamemode_interface";
}

bool GameMode::isActiveLayerLocal(const std::string& name){
    return state.scene.isActiveLayer(getLocalLayerName(name));
}

void GameMode::setLayerLocal(const std::string& name){
    state.scene.setUILayer(getLocalLayerName(name));
}
UILayer& GameMode::getLayerLocal(const std::string& name){
    return state.scene.getUILayer(getLocalLayerName(name));
}
bool GameMode::IsBaseLayerActive(){ 
    if(!base_layer_override.empty()) return state.scene.isActiveLayer(base_layer_override);
    return isActiveLayerLocal("base"); 
}

UILayer& GameMode::GetBaseLayer(){
    if(!base_layer_override.empty()) return state.scene.getUILayer(base_layer_override);
    return getLayerLocal("base");
}