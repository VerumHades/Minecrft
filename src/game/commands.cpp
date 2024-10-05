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


UICommandInput::UICommandInput(TValue x, TValue y, TValue width, TValue height, glm::vec3 color): UIInput(x,y,width,height,color){
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

std::vector<UIRenderInfo> UICommandInput::getRenderingInformation(UIManager& manager){
    glm::vec2 textDimensions = manager.getMainFont().getTextDimensions(text);
    int sw = manager.getScreenWidth();
    int sh = manager.getScreenHeight();

    int rx = getValueInPixels(x, true , sw);
    int ry = getValueInPixels(y, false, sh);
    int w = getValueInPixels(width, true , sw);
    int h = getValueInPixels(height, false, sh);

    int txpadding = getValueInPixels(padding, true , sw);
    int typadding = getValueInPixels(padding, false, sh);

    int tx = rx + txpadding;
    int ty = ry + (h / 2) - textDimensions.y / 2;
    
    std::vector<UIRenderInfo> out = UIFrame::getRenderingInformation(manager);
    std::vector<UIRenderInfo> temp = manager.buildTextRenderingInformation(text,tx,ty,1,{1,1,1});
    out.push_back({
        TValue(PIXELS,tx + textDimensions.x),
        TValue(PIXELS,ty    ),
        TValue(PIXELS,2     ),
        TValue(PIXELS, textDimensions.y + 3),
        {1,1,1}
    });
    out.insert(out.end(), temp.begin(), temp.end());

    return out;
}