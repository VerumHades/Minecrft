#include <ui/font.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h> // Include stb_image_write for image saving

void saveRedComponentTexture(GLuint textureID, int width, int height, const char* filename) {
    // Bind the texture
    GL_CALL( glBindTexture(GL_TEXTURE_2D, textureID));

    // Create a buffer to store the texture data (only the red component)
    unsigned char* redData = new unsigned char[width * height]; // 1 byte per pixel since it's only red

    // Read the texture data (only the red component)
    GL_CALL( glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, redData));

    // Use stb_image_write to save the red component as a PNG
    // Note: You can save it in other formats like BMP, TGA, etc.
    stbi_write_png(filename, width, height, 1, redData, width);

    // Clean up
    delete[] redData;

    // Unbind the texture
    GL_CALL( glBindTexture(GL_TEXTURE_2D, 0));
}


Font::Font(std::string filepath, int size): size(size){
    if (FT_Init_FreeType(&ft)) {
        LogError("Could not init FreeType Library");
        throw std::runtime_error("");
    }
    if (FT_New_Face(ft, filepath.c_str(), 0, &face)) {
        LogError("Failed to load font '{}'", filepath);
        throw std::runtime_error("");
    }

    FT_Set_Pixel_Sizes(face, 0, size); // Set font size to 48 pixels
    createAtlas();
}

void Font::createAtlas(){
    uint atlasWidth = 512;  // Chosen size for the atlas, adjust based on your needs
    uint atlasHeight = 512;
    
    atlas = std::make_unique<GLTexture2D>();
    atlas->bind(0);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    //CHECK_GL_ERROR();

    // Texture options
    GL_CALL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    //CHECK_GL_ERROR();

    int xOffset = 0;
    int yOffset = 0;
    int rowHeight = 0;

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            LogError("Failed to load Glyph for '{}'", c);
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

        if(!face->glyph->bitmap.buffer) LogWarning("No pixel buffer for glyph: {}", c);
        else {
            GL_CALL( glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
            // Upload glyph bitmap to the atlas texture
            glTexSubImage2D(GL_TEXTURE_2D, 0, xOffset, yOffset,
                            face->glyph->bitmap.width, face->glyph->bitmap.rows,
                            GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
        }

        //CHECK_GL_ERROR();
        // Store texture coordinates and other metrics for later rendering
        Character ch = {
            atlas->getID(),
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<uint>(face->glyph->advance.x),
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
    program.use();

    GL_CALL( glGenVertexArrays(1, &VAO));
    GL_CALL( glGenBuffers(1, &VBO));
    
    //CHECK_GL_ERROR();

    GL_CALL( glBindVertexArray(VAO));
    GL_CALL( glBindBuffer(GL_ARRAY_BUFFER, VBO));
    GL_CALL( glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW));

    //CHECK_GL_ERROR();

    GL_CALL( glEnableVertexAttribArray(0));
    GL_CALL( glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0));

    //CHECK_GL_ERROR();

    GL_CALL( glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CALL( glBindVertexArray(0));

    //CHECK_GL_ERROR();
}

FontManager::~FontManager(){
    GL_CALL( glDeleteBuffers(1, &VBO));
    GL_CALL( glDeleteVertexArrays(1, &VAO));
}

void FontManager::renderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color, Font& font) {
    textColor = color;
    // Activate corresponding render state
    program.updateUniforms();
    //CHECK_GL_ERROR();

    font.getAtlas()->bind(0);

    //CHECK_GL_ERROR();

    GL_CALL( glBindVertexArray(VAO));

    //CHECK_GL_ERROR();
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
        GL_CALL( glBindBuffer(GL_ARRAY_BUFFER, VBO));
        GL_CALL( glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices));
        GL_CALL( glDrawArrays(GL_TRIANGLES, 0, 6));

        // Advance to next glyph
        x += (ch.Advance >> 6) * scale;  // Bitshift by 6 to get the value in pixels
    }

    GL_CALL( glBindVertexArray(0));
    GL_CALL( glBindTexture(GL_TEXTURE_2D, 0));
}

glm::vec2 Font::getTextDimensions(std::string text, int size){
    if(size == -1) size = this->size;
    glm::vec2 out = {0,0};

    int x = 0;
    for (auto c = text.begin(); c != text.end(); c++) {
        Character ch = characters[*c];

        //GLfloat xpos = ch.Bearing.x;
        //GLfloat ypos = (ch.Size.y - ch.Bearing.y);

        //GLfloat w = ch.Size.x;
        GLfloat h = ch.Size.y;

        out.y = std::max(out.y, h);

        // Advance to next glyph
        x += (ch.Advance >> 6);  // Bitshift by 6 to get the value in pixels
    }

    out.x = x;

    float scale = static_cast<float>(size) / static_cast<float>(this->size);

    return out * scale;
}