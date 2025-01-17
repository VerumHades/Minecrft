#include <ui/backend.hpp>

std::array<glm::vec2, 4> UIRenderBatch::GetRetangleVertices(int x, int y, int width, int height){
    return {
        glm::vec2{x        , y    },
        glm::vec2{x + width, y    },
        glm::vec2{x + width, y + height},
        glm::vec2{x        , y + height}
    };
}

void UIRenderBatch::Rectangle(int x, int y, int width, int height, UIColor fill_color){
    commands.push_back(
        UIRenderCommand{
            GetRetangleVertices(x,y,width,height),
            fill_color,
            {{0,0},{0,0}},
            UIRenderCommand::SOLID
        }
    );
}
void UIRenderBatch::Rectangle(UITransform transform, UIColor fill_color){
    Rectangle(transform.x, transform.y, transform.width, transform.height, fill_color);
}

void UIRenderBatch::BorderedRectangle(int x, int y, int width, int height, UIColor fill_color, UISideSizes border_sizes, UIBorderColors border_colors){
    Rectangle(
        x + border_sizes.left,
        y + border_sizes.top,
        width - border_sizes.left - border_sizes.right,
        height - border_sizes.top - border_sizes.bottom,
        fill_color
    );
    Rectangle(
        x,
        y,
        border_sizes.left,
        height, 
        border_colors.left
    );
    Rectangle(
        x + width - border_sizes.right,
        y,
        border_sizes.right,
        height, 
        border_colors.left
    );
    Rectangle(
        x + border_sizes.left,
        y,
        width - border_sizes.left - border_sizes.right,
        border_sizes.top, 
        border_colors.left
    );
    Rectangle(
        x + border_sizes.left,
        y + height - border_sizes.bottom,
        width - border_sizes.left - border_sizes.right,
        border_sizes.bottom, 
        border_colors.left
    );
}
void UIRenderBatch::BorderedRectangle(UITransform transform, UIColor fill_color, UISideSizes border_sizes, UIBorderColors border_colors){
    BorderedRectangle(transform.x, transform.y, transform.width, transform.height, fill_color, border_sizes, border_colors);
}


void UIRenderBatch::Texture(int x, int y, int width, int height, UIRegion texture_coords, UIColor color_mask){
    commands.push_back({
        GetRetangleVertices(x,y,width,height),
        color_mask,
        texture_coords,
        UIRenderCommand::TEXTURE
    });
}
void UIRenderBatch::TexturePolygon(std::array<glm::vec2, 4> positions, UIRegion texture_coords, UIColor color_mask){
    commands.push_back({
        positions,
        color_mask,
        texture_coords,
        UIRenderCommand::TEXTURE
    });
}

void UIRenderBatch::Texture(UITransform transform, UIRegion texture_coords, UIColor color_mask){
    Texture(transform.x, transform.y, transform.width, transform.height, texture_coords, color_mask);
}
void UIRenderBatch::Text(std::string text, int x, int y, int font_size, UIColor color){
    commands.push_back(UIRenderCommand{
        {
            glm::vec2{x,y},
            glm::vec2{font_size}   
        },
        color,
        {{0,0},{0,0}},
        UIRenderCommand::TEXT,
        text
    });
}