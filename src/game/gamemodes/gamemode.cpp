#include <game/gamemodes/gamemode.hpp>

GameMode::GameMode(const std::string& name){
    ui_layer = std::make_shared<UILayer>();
    ui_layer->name = name + "_gamemode_interface";
}