#include <ui/backend.hpp>

void UIRenderBatch::Rectangle(UITransform transform, UIColor fill_color){
    commands.push_back(
        UIRenderCommand{
            {transform.x,transform.y},
            {transform.width,transform.height},
            fill_color,
            {0,0,0,0},
            UIRenderCommand::SOLID
        }
    );
}

void UIRenderBatch::BorderedRectangle(UITransform transform, UIColor fill_color, UIBorderSizes border_sizes, UIBorderColors border_colors){
    Rectangle(transform, fill_color);
    Rectangle({
            0,
            0,
            border_sizes.left,
            transform.height
    }, border_colors.left);
    Rectangle({
            transform.width - border_sizes.right,
            0,
            border_sizes.right,
            transform.height
    }, border_colors.left);
    Rectangle({
            border_sizes.left,
            0,
            transform.width - border_sizes.left - border_sizes.right,
            border_sizes.top
    }, border_colors.left);

    Rectangle({
            border_sizes.left,
            transform.height - border_sizes.bottom,
            transform.width - border_sizes.left - border_sizes.right,
            border_sizes.bottom
    }, border_colors.left);
}
void UIRenderBatch::Texture(UITransform transform, UITransform texture_coords){
    commands.push_back({
        {transform.x,transform.y},
        {transform.width,transform.height},
        UIColor{0,0,0,0},
        texture_coords,
        UIRenderCommand::TEXTURE
    });
}
void UIRenderBatch::Text(int x, int y, std::string text, int font_size, UIColor color){
    commands.push_back(UIRenderCommand{
        {x,y},
        {font_size,0},
        color,
        {0,0,0,0},
        UIRenderCommand::TEXT,
        text
    });
}