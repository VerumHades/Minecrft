#include <ui/form.hpp>


UIForm::UIForm(const std::vector<Field>& fields, UIManager& manager): UIFrame(manager){
    for(auto& [name,type]: fields){
        auto row_frame = manager.createElement<UIFrame>();
        row_frame->setLayout(std::make_shared<UIFlexLayout>(UIFlexLayout::HORIZONTAL, false));
        row_frame->setSize({PERCENT, 100}, 30);
        row_frame->setAttribute(&UIFrame::Style::margin, TValue{5});

        auto label = manager.createElement<UILabel>();
        label->setText(name);
        label->setSize({AUTO}, {30});
        row_frame->appendChild(label);

        auto input = manager.createElement<UIInput>();
        input->setSize(150,30);
        row_frame->appendChild(input);
    
        appendChild(row_frame);
    }

    submit_button = manager.createElement<UILabel>();
    submit_button->setAttribute(&UIFrame::Style::borderColor, {UIColor{0,0,0}});
    submit_button->setText("Submit");
    submit_button->setAttribute(&UIFrame::Style::margin, TValue{5});
    submit_button->setSize(100,30);

    appendChild(submit_button);

    setLayout(std::make_shared<UIFlexLayout>(UIFlexLayout::VERTICAL, true));
}