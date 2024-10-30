#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <unordered_map>

template <typename T>
class KeyInputManager{
    private:
        struct Keybind{
            T action;
            std::string name;
        };

        std::unordered_map<int, Keybind> boundKeys;
        std::unordered_map<T, bool> actionStates;

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
        
        void keyEvent(GLFWwindow* window, int key, int scancode, int action, int mods){
            if(boundKeys.count(key) == 0) return;
            actionStates[boundKeys[key].action] = (action == GLFW_PRESS);
        }

        const std::unordered_map<int, Keybind>& getBoundKeys() const {return boundKeys;}
};

std::string getKeyName(int key, int scancode);