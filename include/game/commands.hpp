#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <memory>

#include <ui/manager.hpp>

enum CommandArgumentType {
    INT,
    STRING,
    NONE
};

struct CommandArgument{
    CommandArgumentType type = NONE;
    std::string stringValue;
    int intValue;

    CommandArgument(int i) {type = INT; intValue = i;}
    CommandArgument(std::string  value) {type = STRING; stringValue = value;}
};

using CommandFunction = std::function<void(std::vector<CommandArgument>)>;
class Command{
    private:
        std::vector<CommandArgumentType> argumentTypes;
    public:
        Command(std::vector<CommandArgumentType> argumentTypes, CommandFunction function): argumentTypes(argumentTypes), execute(function) {}
        CommandFunction execute;
        const std::vector<CommandArgumentType>& getArgumentTypes(){return argumentTypes;}
}; 

class CommandProcessor{
    private:
        std::unordered_map<std::string, size_t> commandsIDS;
        std::vector<std::unique_ptr<Command>> commands;

        CommandArgument processArgument(std::string raw);

    public:
        void addCommand(std::vector<std::string> names, std::unique_ptr<Command> command);
        void processCommand(std::string);

        const std::unordered_map<std::string, size_t>& getCommandIDs() {return commandsIDS;};
};

class UICommandInput: public UIInput{
    private:
        CommandProcessor* commandProcessor;
    public:
        UICommandInput(CommandProcessor* commandProcessor, TValue x, TValue y, TValue width, TValue height);
        std::vector<UIRenderInfo> getRenderingInformation(UIManager& manager) override;

};