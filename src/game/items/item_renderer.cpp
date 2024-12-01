#include <game/items/item_renderer.hpp>


ItemTextureAtlas::StoredTexture* ItemTextureAtlas::getPrototypeTexture(ItemPrototype* prototype){
    if(!prototype) return nullptr;

    if(!stored_textures.contains(prototype)){
        this->texture_array->bind(0);

        int width, height, original_channels;
        unsigned char* image_data;
        // Load the image with 4 channels (RGBA)
        image_data = stbi_load(prototype->texture_path.c_str(), &width, &height, &original_channels, 4);

        if (!image_data) {
            std::cout << "Failed to load image: " << stbi_failure_reason() << std::endl;
            return nullptr;
        }


        if(width != single_texture_size || height != single_texture_size){
            std::cerr << "Item prototype texture has invalid size: " << width << "x" << height << " and not " << single_texture_size << "x" << single_texture_size << std::endl;
            stbi_image_free(image_data);
            return nullptr;
        }

        const int layerIndex = 0;
        int textures_per_row = atlas_size / single_texture_size;

        int x = (textures_stored_total % textures_per_row) * single_texture_size;
        int y = (textures_stored_total / textures_per_row) * single_texture_size;
        // Upload the image data to the specified layer
        glTexSubImage3D(
            GL_TEXTURE_2D_ARRAY, // Target
            0,                  // Mipmap level
            x, y, layerIndex,   // x, y, and z (layer) offsets
            width, height, 1,   // Width, height, depth (only one layer here)
            GL_RGBA,            // Format of the pixel data
            GL_UNSIGNED_BYTE,   // Data type of the pixel data
            image_data          // Pointer to the image data
        );

        stored_textures[prototype] = {
            glm::vec2{x,y} / static_cast<float>(atlas_size) ,
            glm::vec2{x + width, y + height} / static_cast<float>(atlas_size) ,
            1
        };

        textures_stored_total++;

        //CHECK_GL_ERROR();;

        stbi_image_free(image_data);
    }

    return &stored_textures[prototype];
}

void ItemSlot::getRenderingInformation(RenderYeetFunction& yeet){
    yeet(
        UIRenderInfo::Rectangle(
            transform,
            getAttribute(&UIFrame::Style::backgroundColor)
        ),
        clipRegion
    );

    if(!item.has_value()) return;
    auto* prototype = item.value().getPrototype();
    std::cout << "Looking for item prototype" << std::endl;
    if(!prototype) return;

    auto* texture_info = textureAtlas.getPrototypeTexture(prototype);

    std::cout << "Rendering item" << std::endl;
    yeet(
        UIRenderInfo::Texture(
            transform.x,transform.y,transform.width,transform.height,
            {
                {texture_info->uv_min.x, texture_info->uv_min.y},
                {texture_info->uv_max.x, texture_info->uv_min.y},
                {texture_info->uv_max.x, texture_info->uv_max.y},
                {texture_info->uv_min.x, texture_info->uv_max.y}
            },
            texture_info->index
        ),
        clipRegion
    );
}