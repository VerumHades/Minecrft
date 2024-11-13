#include <ui/font.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h> // Include stb_image_write for image saving

void saveRedComponentTexture(GLuint textureID, int width, int height, const char* filename) {
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Create a buffer to store the texture data (only the red component)
    unsigned char* redData = new unsigned char[width * height]; // 1 byte per pixel since it's only red

    // Read the texture data (only the red component)
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, redData);

    // Use stb_image_write to save the red component as a PNG
    // Note: You can save it in other formats like BMP, TGA, etc.
    stbi_write_png(filename, width, height, 1, redData, width);

    // Clean up
    delete[] redData;

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);
}


Font::Font(std::string filepath, int size){
    if (FT_Init_FreeType(&ft)) {
        std::cout << "Could not init FreeType Library" << std::endl;
        return;
    }
    if (FT_New_Face(ft, filepath.c_str(), 0, &face)) {
        std::cout << "Failed to load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, size); // Set font size to 48 pixels
    createAtlas();
}

void Font::createAtlas(){
    int atlasWidth = 512;  // Chosen size for the atlas, adjust based on your needs
    int atlasHeight = 512;
    
    atlas = std::make_unique<GLTexture>();
    atlas->bind(0);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    CHECK_GL_ERROR();

    // Texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    CHECK_GL_ERROR();

    int xOffset = 0;
    int yOffset = 0;
    int rowHeight = 0;

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "Failed to load Glyph" << std::endl;
            continue;
        }

        // Check if we need to move to the next row
        if (xOffset + face->glyph->bitmap.width >= atlasWidth) {
            xOffset = 0;
            yOffset += rowHeight;
            rowHeight = 0;
        }

        // Update row height to fit this glyph
        rowHeight = std::max(rowHeight, static_cast<int>(face->glyph->bitmap.rows));

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        // Upload glyph bitmap to the atlas texture
        glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset,
                        face->glyph->bitmap.width, face->glyph->bitmap.rows,
                        GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

        CHECK_GL_ERROR();
        // Store texture coordinates and other metrics for later rendering
        Character ch = {
            atlas->getID(),
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x,
            glm::vec2(xOffset / (float)atlasWidth, yOffset / (float)atlasHeight),
            glm::vec2((xOffset + face->glyph->bitmap.width) / (float)atlasWidth, 
                    (yOffset + face->glyph->bitmap.rows) / (float)atlasHeight)
        };
        characters.insert(std::pair<char, Character>(c, ch));

        xOffset += face->glyph->bitmap.width;
    }

    //saveRedComponentTexture(atlas->getID(), atlasWidth, atlasHeight, "temp.png");
}

void FontManager::initialize(){
    program = std::make_unique<ShaderProgram>();
    program->initialize();
    program->addShader("shaders/graphical/ui/text.vs", GL_VERTEX_SHADER);
    program->addShader("shaders/graphical/ui/text.fs", GL_FRAGMENT_SHADER);
    program->compile();
    program->use();

    textColor.attach(*program);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    CHECK_GL_ERROR();

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    CHECK_GL_ERROR();

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    CHECK_GL_ERROR();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    CHECK_GL_ERROR();
}

FontManager::~FontManager(){
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

void FontManager::renderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color, Font& font) {
    textColor = color;
    // Activate corresponding render state
    program->updateUniforms();
    CHECK_GL_ERROR();

    font.getAtlas()->bind(0);

    CHECK_GL_ERROR();

    glBindVertexArray(VAO);

    CHECK_GL_ERROR();
    // Iterate through each character in the text
    for (auto c = text.begin(); c != text.end(); c++) {
        Character ch = font.getCharacters()[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - ch.Bearing.y * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        // Update VBO for each character with the glyph's quad and texture coordinates
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   ch.TexCoordsMin.x, ch.TexCoordsMax.y },
            { xpos,     ypos,       ch.TexCoordsMin.x, ch.TexCoordsMin.y },
            { xpos + w, ypos,       ch.TexCoordsMax.x, ch.TexCoordsMin.y },

            { xpos,     ypos + h,   ch.TexCoordsMin.x, ch.TexCoordsMax.y },
            { xpos + w, ypos,       ch.TexCoordsMax.x, ch.TexCoordsMin.y },
            { xpos + w, ypos + h,   ch.TexCoordsMax.x, ch.TexCoordsMax.y }
        };

        // Render glyph quad
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance to next glyph
        x += (ch.Advance >> 6) * scale;  // Bitshift by 6 to get the value in pixels
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

glm::vec2 Font::getTextDimensions(std::string text){
    glm::vec2 out = {0,0};

    int x = 0;
    for (auto c = text.begin(); c != text.end(); c++) {
        Character ch = characters[*c];

        GLfloat xpos = ch.Bearing.x;
        GLfloat ypos = (ch.Size.y - ch.Bearing.y);

        GLfloat w = ch.Size.x;
        GLfloat h = ch.Size.y;

        out.y = std::max(out.y, h);

        // Advance to next glyph
        x += (ch.Advance >> 6);  // Bitshift by 6 to get the value in pixels
    }

    out.x = x;

    return out;
}