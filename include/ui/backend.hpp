#pragma once

#include <glm/glm.hpp>
#include <rendering/texture.hpp>

struct UIRenderCommand{
    glm::vec2 position;
    glm::vec3 uv_or_color;
    bool is_solid = false;

    const static int vertex_size = 6;
};


struct UIRenderBatch{
    std::vector<UIRenderCommand> commands;
    glm::vec2 clip_min;
    glm::vec2 clip_max;

    BindableTexture* texture = nullptr;

    size_t vertexCount(){
        return 4 * UIRenderCommand::vertex_size;
    }
    size_t indexCount(){
        return 6;
    }
};


class UIBackend{    
    public:
        virtual void addRenderBatch(UIRenderBatch& batch) = 0;
        virtual void render() = 0;
};