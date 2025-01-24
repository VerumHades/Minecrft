#pragma once

#include <scene.hpp>

#include <ui/core.hpp>
#include <ui/form.hpp>

class TestScene: public Scene{
    private:
        bool stage = 0;

    public:
        TestScene(){}
        void initialize() override;
        void open(GLFWwindow* window) override;
};