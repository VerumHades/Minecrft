#include <ui/elements/selection.hpp>

UISelection::UISelection(){
    onKeyEvent = [this](int key, int scancode, int action, int mods){
        if(action == GLFW_PRESS && key == GLFW_KEY_ENTER && onSelected) onSelected(options[selected]);
    };

    setFocusable(true);
}

bool UISelection::hasOption(const std::string& option){
    return std::find(options.begin(), options.end(), option) != options.end();
}

void UISelection::addOption(const std::string& option){
    options.push_back(option);
}
void UISelection::selectNext(){
    selected = (selected + 1) % options.size();
}
void UISelection::selectPrevious(){
    selected = (selected - 1 + options.size()) % options.size();
}

void UISelection::getRenderingInformation(UIRenderBatch& batch) {
    UIFrame::getRenderingInformation(batch);

    if(options.size() == 0) return;

    auto font_size = getValueInPixels(getAttribute(&Style::fontSize), true);
    auto text_size = UICore::get().getBackend().getTextDimensions(options[0], font_size);

    int line_height = text_size.height + 10;
    int origin_x = transform.x + 10;
    int origin_y = transform.y + transform.height / 2 - line_height / 2;
    int text_origin_y = transform.y + transform.height / 2 - text_size.height / 2;

    int i = 0;

    batch.Rectangle(transform.x, origin_y, transform.width, line_height, getAttribute(&Style::backgroundColor).shifted(0.1));
    for(const auto& option: options){
        int offset = ((i++) - selected) * line_height; 
        batch.Text(option, origin_x, text_origin_y - offset, font_size, getAttribute(&Style::textColor));
    }
}