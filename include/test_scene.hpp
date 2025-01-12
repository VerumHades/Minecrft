#pragma once

#include <scene.hpp>

#include <ui/manager.hpp>
#include <ui/form.hpp>

class TestScene: public Scene{
    private:
        bool stage = 0;

    public:
        void initialize() override;
        void open(GLFWwindow* window) override;
};

class TestWidget: public UIFrame{
    public:
        TestWidget(UIManager& manager): UIFrame(manager){}
        void getRenderingInformation(UIRenderBatch& batch) override;
};