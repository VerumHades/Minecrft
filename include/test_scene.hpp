#pragma once

#include <scene.hpp>

class TestScene: public Scene{
    private:
        bool stage = 0;

    public:
        void initialize();
};

class TestWidget: public UIFrame{
    public:
        TestWidget(UIManager& manager): UIFrame(manager){}
        void getRenderingInformation(UIRenderBatch& batch) override;
};