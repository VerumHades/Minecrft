#include <game/input.hpp>


std::string getKeyName(int key, int scancode) {
    const char* name = glfwGetKeyName(key, scancode);
    if (name) return std::string(name);  

    static const std::unordered_map<int, std::string> specialKeys = {
        {GLFW_KEY_SPACE, "Space"},
        {GLFW_KEY_ENTER, "Enter"},
        {GLFW_KEY_TAB, "Tab"},
        {GLFW_KEY_BACKSPACE, "Backspace"},
        {GLFW_KEY_ESCAPE, "Escape"},
        {GLFW_KEY_INSERT, "Insert"},
        {GLFW_KEY_DELETE, "Delete"},
        {GLFW_KEY_RIGHT, "Right Arrow"},
        {GLFW_KEY_LEFT, "Left Arrow"},
        {GLFW_KEY_DOWN, "Down Arrow"},
        {GLFW_KEY_UP, "Up Arrow"},
        {GLFW_KEY_PAGE_UP, "Page Up"},
        {GLFW_KEY_PAGE_DOWN, "Page Down"},
        {GLFW_KEY_HOME, "Home"},
        {GLFW_KEY_END, "End"},
        {GLFW_KEY_CAPS_LOCK, "Caps Lock"},
        {GLFW_KEY_SCROLL_LOCK, "Scroll Lock"},
        {GLFW_KEY_NUM_LOCK, "Num Lock"},
        {GLFW_KEY_PRINT_SCREEN, "Print Screen"},
        {GLFW_KEY_PAUSE, "Pause"},
        {GLFW_KEY_F1, "F1"},
        {GLFW_KEY_F2, "F2"},
        {GLFW_KEY_F3, "F3"},
        {GLFW_KEY_F4, "F4"},
        {GLFW_KEY_F5, "F5"},
        {GLFW_KEY_F6, "F6"},
        {GLFW_KEY_F7, "F7"},
        {GLFW_KEY_F8, "F8"},
        {GLFW_KEY_F9, "F9"},
        {GLFW_KEY_F10, "F10"},
        {GLFW_KEY_F11, "F11"},
        {GLFW_KEY_F12, "F12"},
        {GLFW_KEY_LEFT_SHIFT, "Left Shift"},
        {GLFW_KEY_LEFT_CONTROL, "Left Control"},
        {GLFW_KEY_LEFT_ALT, "Left Alt"},
        {GLFW_KEY_LEFT_SUPER, "Left Super"},
        {GLFW_KEY_RIGHT_SHIFT, "Right Shift"},
        {GLFW_KEY_RIGHT_CONTROL, "Right Control"},
        {GLFW_KEY_RIGHT_ALT, "Right Alt"},
        {GLFW_KEY_RIGHT_SUPER, "Right Super"},
        {GLFW_KEY_MENU, "Menu"}
    };

    auto it = specialKeys.find(key);
    if (it != specialKeys.end()) {
        return it->second;
    }
    return "Unknown";
}