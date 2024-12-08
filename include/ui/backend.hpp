#pragma once

#include <glm/glm.hpp>
#include <rendering/texture.hpp>
#include <ui/color.hpp>

struct UIRenderCommand{
    glm::vec2 position;
    glm::vec2 size;
    
    UIColor color;
    UIRegion uvs;
    
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
    UIRegion clipRegion;

    BindableTexture* texture = nullptr;

    void Rectangle(int x, int y, int width, int height, UIColor fill_color);
    void Rectangle(UITransform transform, UIColor fill_color);

    void BorderedRectangle(int x, int y, int width, int height, UIColor fill_color, UIBorderSizes border_sizes, UIBorderColors border_colors);
    void BorderedRectangle(UITransform transform, UIColor fill_color, UIBorderSizes border_sizes, UIBorderColors border_colors);

    void Texture(int x, int y, int width, int height, UIRegion texture_coords);
    void Texture(UITransform transform, UIRegion texture_coords);

    void Text(std::string text, int x, int y, int font_size, UIColor color);
};


class UIBackend{   
    public:
        struct Batch{
            UIRegion clipRegion = {{0,0},{0,0}};

            BindableTexture* texture = nullptr;
            
            size_t vertex_start = 0;
            size_t index_start = 0;
            size_t vertex_size = 0;
            size_t index_size = 0;
        };
    protected:
        std::list<Batch> batches = {};
    public:
        virtual std::list<Batch>::iterator addRenderBatch(UIRenderBatch& batch) = 0;
        virtual void setupRender() = 0;
        virtual void cleanupRender() = 0;
        virtual void renderBatch(std::list<Batch>::iterator batch_iter) = 0;
        virtual void removeBatch(std::list<Batch>::iterator batch_iter) = 0;
        virtual void resizeVieport(int width, int height) = 0;
        virtual UITextDimensions getTextDimensions(std::string text, int size) = 0;
};