#pragma once

#include <ui/core.hpp>

class UISelection: public UIFrame{
    private:
        std::vector<std::string> options{};

        int selected = 0;

        void getRenderingInformation(UIRenderBatch& batch) override;

    public:
        UISelection();
        std::function<void(const std::string&)> onSelected;

        void addOption(const std::string& option);
        bool hasOption(const std::string& option);
        void selectNext();
        void selectPrevious();

        void clear();
};