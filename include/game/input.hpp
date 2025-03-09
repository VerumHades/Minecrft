#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <unordered_map>

enum ControllActions{
    MOVE_FORWARD,
    MOVE_BACKWARD,
    STRAFE_LEFT,
    STRAFE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    SCROLL_ZOOM,
    SPRINT
};

template <typename T>
class KeyInputManager{
    private:
        struct Keybind{
            T action;
            std::string name;
        };

        std::unordered_map<int, Keybind> boundKeys;
        std::unordered_map<T, bool> actionStates;
        std::unordered_map<int, bool> mouseStates;

    public:
        KeyInputManager(){

        }

        void bindKey(int key, T action, std::string name){
            boundKeys[key] = {action,name};
            actionStates[action] = false;
        }

        void unbindKey(int key){
            boundKeys.erase(key);
        }
        
        bool isActive(T action){
            if(actionStates.count(action) == 0) return false;
            return actionStates[action];
        }

        bool isDown(int mouse_key){
            if(!mouseStates.contains(mouse_key)) return false;
            return mouseStates.at(mouse_key);
        }
        
        void keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods){
            if(boundKeys.count(key) == 0) return;
            if(action == GLFW_PRESS) actionStates[boundKeys[key].action] = true;
            else if(action == GLFW_RELEASE) actionStates[boundKeys[key].action] = false;
        }
        void mouseEvent(GLFWwindow* window, int button, int action, int mods){
            mouseStates[button] = action == GLFW_PRESS;
        }

        void reset(){
            for(auto& [action, state]: actionStates)
                state = false;

            for(auto& [action, state]: mouseStates)
                state = false;
        }

        const std::unordered_map<int, Keybind>& getBoundKeys() const {return boundKeys;}
};

std::string getKeyName(int key, int scancode);