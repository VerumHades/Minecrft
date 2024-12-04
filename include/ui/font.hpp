#ifndef FONT_H
#define FONT_H

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <string>
#include <iostream>
#include <memory>
#include <algorithm>

#include <rendering/texture.hpp>
#include <rendering/shaders.hpp>

struct Character {
    unsigned int TextureID;  // This will be the ID of the atlas texture
    glm::ivec2 Size;   // Size of the glyph
    glm::ivec2 Bearing; // Offset from baseline to left/top of the glyph
    unsigned int Advance;    // Offset to advance to next glyph
    glm::vec2 TexCoordsMin; // Lower-left corner of the glyph in the atlas
    glm::vec2 TexCoordsMax; // Upper-right corner of the glyph in the atlas
};

class Font{
    private:
        FT_Library ft;
        FT_Face face;
        int size;

        std::unique_ptr<GLTexture> atlas;
        std::unordered_map<char, Character> characters;
        void createAtlas();

    public:
        Font(std::string filepath, int size);

        glm::vec2 getTextDimensions(std::string, int size);

        GLTexture* getAtlas(){return atlas.get();}
        std::unordered_map<char, Character>& getCharacters() {return characters;}

        int getSize(){return size;}
};

class FontManager{
    private:
        ShaderProgram program = ShaderProgram("shaders/graphical/ui/text.vs","shaders/graphical/ui/text.fs");
        Uniform<glm::vec3> textColor = Uniform<glm::vec3>("textColor");

        unsigned int VAO, VBO;

    public:
        void renderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color, Font& font);
        void initialize();

        ~FontManager();

        ShaderProgram& getProgram(){return program;}
};

#endif