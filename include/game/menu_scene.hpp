#pragma once

#pragma once

#include <path_config.hpp>
#include <scene.hpp>

#include <game/main_scene.hpp>

#include <cctype> // Needed for std::isdigit

/**
 * @brief A scene for the main menu
 * 
 */
class MenuScene : public Scene {
  private:
    int preview_zoom = 5;

  public:
    void initialize() override;
};
