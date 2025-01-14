#include <ui/elements/selection.hpp>

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
    auto text_size = ui_core.getBackend().getTextDimensions(options[0], font_size);

    int line_height = text_size.height + 10;
    int origin_x = transform.x;
    int origin_y = transform.y + transform.height / 2 - line_height / 2;
    int text_origin_y = transform.y + transform.height / 2 - text_size.height / 2;

    int i = 0;

    batch.Rectangle(origin_x, origin_y, transform.width, line_height, getAttribute(&Style::backgroundColor).shifted(0.1));
    for(const auto& option: options){
        int offset = ((i++) - selected) * line_height; 
        batch.Text(option, origin_x, text_origin_y - offset, font_size, getAttribute(&Style::textColor));
    }
}