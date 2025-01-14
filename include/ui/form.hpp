#pragma once

#include <ui/core.hpp>

class UIForm: public UIFrame{
    public:
        enum FieldType{
            TEXT,
            INT,
            FLOAT
        };

        struct Field{
            std::string name = "";
            FieldType type = TEXT;
        };

        struct FieldValue{
            FieldType type = TEXT;
            union{
                int int_value;
                float float_value;
            };
            std::string text_value;
        };

    private:
        std::shared_ptr<UILabel> submit_button;
        
    public:
        UIForm(const std::vector<Field>& fields);        
};