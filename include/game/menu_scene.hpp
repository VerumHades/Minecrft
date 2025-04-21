#pragma once

#pragma once

#include <path_config.hpp>
#include <scene.hpp>

#include <game/main_scene.hpp>

#include <cctype> // Needed for std::isdigit

class MenuScene : public Scene {
  private:
    int preview_zoom = 5;

  public:
    void initialize() override;
};
