#include <game/commands.hpp>

void CommandProcessor::addCommand(std::vector<std::string> names, std::unique_ptr<Command> command){
    for(auto& name: names){
        commandsIDS[name] = commands.size();
    }

    commands.push_back(std::move(command));
}

bool isInt(const std::string s){
    return s.find_first_not_of("0123456789") == std::string::npos;
}

CommandArgument CommandProcessor::processArgument(std::string raw){
    if(isInt(raw)) return {std::stoi(raw)};
    else return {raw};
}

void CommandProcessor::processCommand(std::string raw){
    std::string commandName;

    size_t commandNameEnd = raw.find(" ");
    if(commandNameEnd == std::string::npos){
        commandName = raw;
        raw = "";
    }
    else {
        commandName = raw.substr(0, commandNameEnd);
        raw = raw.substr(commandNameEnd + 1, raw.size());
    }

    if(commandsIDS.count(commandName) == 0) {
        std::cout << "Command not recognized: " << commandName << std::endl;
        return;
    }

    Command& command = *commands[commandsIDS[commandName]];

    std::vector<std::string> raw_args;
    size_t start = 0;
    size_t end = raw.find(" ");

    while (end != std::string::npos) {
        raw_args.push_back(raw.substr(start, end - start));
        start = end + 1;  
        end = raw.find(" ", start);  
    }
    raw_args.push_back(raw.substr(start));

    if(raw_args.size() != command.getArgumentTypes().size()) {
        std::cout << "Invalid arguments for command: " << commandName << std::endl;
        return;
    }

    std::vector<CommandArgument> arguments = {};
    for(int i = 0;i < command.getArgumentTypes().size();i++){
        CommandArgument argument = processArgument(raw_args[i]);

        if(argument.type == command.getArgumentTypes()[i]){
            arguments.push_back(argument);
            continue;
        }

        std::cout << "Invalid arguments for command: " << commandName << std::endl;
        return;
    }

    if(command.execute) command.execute(arguments);
}

/*
UICommandInput::UICommandInput(UIManager& manager): UIInput(manager) {
    onKeyTyped = [this](GLFWwindow* window, unsigned int codepoint){
        char typedChar = static_cast<char>(codepoint);

        if (typedChar >= 32 && typedChar <= 126) {
            this->text += typedChar;
        }
    };

    onKeyEvent = [this](GLFWwindow* window, int key, int scancode, int action, int mods){
        if(key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS){
            this->text = this->text.substr(0, this->text.size() - 1);
        }

        if(key == GLFW_KEY_ENTER && action == GLFW_PRESS){
            if(this->onSubmit) this->onSubmit(this->text);
        }
    };
}

void UICommandInput::getRenderingInformation(UIRenderBatch& batch){
    UITextDimensions textDimensions = manager.getBackend()->getTextDimensions(text, font_size);
    int sw = manager.getScreenWidth();
    int sh = manager.getScreenHeight();

    int suggestions_padding = 10;
    
    int tx = transform.x + suggestions_padding;
    int ty = transform.y + (transform.height / 2) - textDimensions.y / 2;
    
    UIFrame::getRenderingInformation(yeet);
    
    std::vector<std::string> suggestions;
    for(auto& [key,value]: commandProcessor->getCommandIDs()){
        if(key.starts_with(text)) suggestions.push_back(key);
    }

    int cursorW = 2;
    int cursorH = transform.height / 2;
    
    yeet(UIRenderInfo::Rectangle(
        transform.x + textDimensions.x    ,
        transform.y + transform.height / 2 - cursorH  / 2,
        cursorW                  ,
        cursorH                  ,
        {1,1,1,1}
    ),clipRegion);

    int suggestions_height = 0;
    int suggestions_width = 0;
    for(auto& s: suggestions){
        glm::vec2 dm = manager.getMainFont().getTextDimensions(s, font_size);
        suggestions_height += dm.y + suggestions_padding;
        suggestions_width = std::max(static_cast<int>(dm.x), suggestions_width);
    }

    suggestions_width += suggestions_padding * 2;
    
    yeet(UIRenderInfo::Rectangle(
        transform.x                     ,
        transform.y - suggestions_height,
        suggestions_width      ,
        suggestions_height     ,
        {0,0,0,1}
    ),clipRegion);

    int currentY = 0;
    for(auto& s: suggestions){
        manager.buildTextRenderingInformation(yeet,clipRegion,s,transform.x + suggestions_padding,transform.y - suggestions_height + currentY + suggestions_padding,1,{0.5f,1.0f,1.0f,});
        glm::vec2 dm = manager.getMainFont().getTextDimensions(s, font_size);
        currentY += dm.y + suggestions_padding;
    }

    manager.buildTextRenderingInformation(yeet,clipRegion,text,tx,ty,1,{1,1,1,1});
}*/