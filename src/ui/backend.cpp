#include <ui/backend.hpp>

void UIRenderBatch::Rectangle(int x, int y, int width, int height, UIColor fill_color){
    commands.push_back(
        UIRenderCommand{
            {x,y},
            {width,height},
            fill_color,
            {{0,0},{0,0}},
            UIRenderCommand::SOLID
        }
    );
}
void UIRenderBatch::Rectangle(UITransform transform, UIColor fill_color){
    Rectangle(transform.x, transform.y, transform.width, transform.height, fill_color);
}

void UIRenderBatch::BorderedRectangle(int x, int y, int width, int height, UIColor fill_color, UIBorderSizes border_sizes, UIBorderColors border_colors){
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
void UIRenderBatch::BorderedRectangle(UITransform transform, UIColor fill_color, UIBorderSizes border_sizes, UIBorderColors border_colors){
    BorderedRectangle(transform.x, transform.y, transform.width, transform.height, fill_color, border_sizes, border_colors);
}


void UIRenderBatch::Texture(int x, int y, int width, int height, UIRegion texture_coords){
    commands.push_back({
        {x,x},
        {width,height},
        UIColor{0,0,0,0},
        texture_coords,
        UIRenderCommand::TEXTURE
    });
}
void UIRenderBatch::Texture(UITransform transform, UIRegion texture_coords){
    Texture(transform.x, transform.y, transform.width, transform.height, texture_coords);
}
void UIRenderBatch::Text(std::string text, int x, int y, int font_size, UIColor color){
    commands.push_back(UIRenderCommand{
        {x,y},
        {font_size,0},
        color,
        {{0,0},{0,0}},
        UIRenderCommand::TEXT,
        text
    });
}