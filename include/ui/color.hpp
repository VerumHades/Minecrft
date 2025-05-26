#pragma once

#include <glm/glm.hpp>
#include <string>
/**
 * @brief Structure used for all colors in the ui, always RGBA
 * 
 */
struct UIColor{
    float r;
    float g;
    float b;
    float a;

    // Integers in ranges 0 - 255
    UIColor(int r, int g, int b, int a = 255){
        this->r = static_cast<float>(r) / 255.0f;
        this->g = static_cast<float>(g) / 255.0f;
        this->b = static_cast<float>(b) / 255.0f;
        this->a = static_cast<float>(a) / 255.0f;
    }

    // Floats in ranges 0.0f - 1.0f
    UIColor(float r, float g, float b, float a = 1.0f): r(r), g(g), b(b), a(a) {}

    // Adds the value to r,g and b components (clamps the color to be always valid after shifting)
    UIColor shifted(float value){
        return UIColor(
            glm::clamp(r + value, .0f, 1.0f),
            glm::clamp(g + value, .0f, 1.0f),
            glm::clamp(b + value, .0f, 1.0f),
            a
        );
    }

    bool operator==(const UIColor& other) const {
        return (
            r == other.r && g == other.g && b == other.b && a == other.a
        );
    }
};


struct UIRegion{
    glm::vec2 min;
    glm::vec2 max;
};

struct UITransform{
    int x;
    int y;
    int width;
    int height;

    UIRegion asRegion(){return {{x,y},{x + width, y + height}}; }
    std::string to_string(){return "Transform(" + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(width) + "," + std::to_string(height) + ")";}
};

struct UISideSizes{
    int top;
    int right;
    int bottom;
    int left;

    UISideSizes(int top, int right, int bottom, int left): top(top), right(right), bottom(bottom), left(left) {}
    UISideSizes(int size): top(size), right(size), bottom(size), left(size) {}

    int operator[](const int& key) const {
        switch (key)
        {
            case 0: return this->top;
            case 1: return this->right;
            case 2: return this->bottom;
            case 3: return this->left;
            default: return this->top;
        }
    }

    // Overload for non-const access
    int& operator[](const int& key) {
        switch (key)
        {
            case 0: return this->top;
            case 1: return this->right;
            case 2: return this->bottom;
            case 3: return this->left;
            default: return this->top;
        }
    }

    int horizontal(){return left + right;}
    int vertical(){return top + bottom;}
};

struct UIBorderColors{
    UIColor top;
    UIColor right;
    UIColor bottom;
    UIColor left;

    UIColor& operator[](int index)
    {
        switch(index){
            case 0: return top;
            case 1: return right;
            case 2: return bottom;
            case 3: return left;
            default: return top;
        }
    }

    UIBorderColors(UIColor top, UIColor right, UIColor bottom, UIColor left): top(top), right(right), bottom(bottom), left(left) {}
    UIBorderColors(UIColor color): top(color), right(color), bottom(color), left(color) {}
};

struct UITextDimensions{
    int width;
    int height;
};

struct UISize{
    int width;
    int height;
};

struct UITranslation{
    int x;
    int y;
};