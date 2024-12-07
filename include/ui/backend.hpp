#pragma once

#include <glm/glm.hpp>
#include <rendering/texture.hpp>
#include <ui/color.hpp>

struct UIRenderCommand{
    glm::vec2 position;
    glm::vec2 size;
    
    UIColor color;
    UITransform uvs;
    
    enum{
        SOLID,
        GLYPH,
        TEXTURE,
        TEXT
    } type;
    std::string text = "";
};


struct UIRenderBatch{
    std::vector<UIRenderCommand> commands;
    glm::vec2 clip_min;
    glm::vec2 clip_max;

    BindableTexture* texture = nullptr;

    void Rectangle(UITransform transform, UIColor fill_color);
    void BorderedRectangle(UITransform transform, UIColor fill_color, UIBorderSizes border_sizes, UIBorderColors border_colors);
    void Texture(UITransform transform, UITransform texture_coords);
    void Text(int x, int y, std::string text, int font_size, UIColor color);
};


class UIBackend{   
    public:
        struct Batch{
            glm::vec2 clip_min;
            glm::vec2 clip_max;

            BindableTexture* texture = nullptr;
            
            size_t vertex_start;
            size_t index_start;
            size_t vertex_size;
            size_t index_size;
        };
    protected:
        std::list<Batch> batches;
    public:
        virtual std::list<Batch>::iterator addRenderBatch(UIRenderBatch& batch) = 0;
        virtual void setupRender() = 0;
        virtual void cleanupRender() = 0;
        virtual void renderBatch(std::list<Batch>::iterator batch_iter) = 0;
        virtual void removeBatch(std::list<Batch>::iterator batch_iter) = 0;
        virtual void resizeVieport(int width, int height) = 0;
};