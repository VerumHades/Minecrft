#include <game/items/item_renderer.hpp>


ItemTextureAtlas::StoredTexture ItemTextureAtlas::storeImage(const Image& image){
    int width = image.getWidth();
    int height = image.getHeight();
    auto* image_data = image.getData();
    
    const int layerIndex = 0;
    int textures_per_row = atlas_size / single_texture_size;

    int x = (textures_stored_total % textures_per_row) * single_texture_size;
    int y = (textures_stored_total / textures_per_row) * single_texture_size;

    // Upload the image data to the specified layer
    glTexSubImage3D(
        GL_TEXTURE_2D_ARRAY, // Target
        0,                  // Mipmap level
        x, y, layerIndex,   // x, y, and z (layer) offsets
        single_texture_size, single_texture_size, 1,   // Width, height, depth (only one layer here)
        GL_RGBA,            // Format of the pixel data
        GL_UNSIGNED_BYTE,   // Data type of the pixel data
        image_data          // Pointer to the image data
    );

    textures_stored_total++;
    //image.save(std::to_string(x) + "_" + std::to_string(y) + "saved_temp.png");

    return {
        {
            glm::vec2{x,y} / static_cast<float>(atlas_size) ,
            glm::vec2{x + width, y + height} / static_cast<float>(atlas_size) ,
        },
        1
    };
}

ItemTextureAtlas::TextureSet* ItemTextureAtlas::getPrototypeTextureSet(ItemPrototype* prototype){
    if(!prototype) return nullptr;


    if(!stored_textures.contains(prototype)){
        this->texture_array->bind(0);

        TextureSet set = {};

        for(int i = 0;i < (prototype->display_type == ItemPrototype::SIMPLE ? 1 : 3);i++){
            auto image = Image::LoadWithSize(prototype->texture_paths[i], single_texture_size, single_texture_size);
           
            set.textures[i] = storeImage(image);
        }
        
        stored_textures[prototype] = set;
    }

    return &stored_textures[prototype];
}

void ItemTextureAtlas::RenderItemIntoSlot(UIRenderBatch& batch, ItemPrototype* prototype, UITransform transform){

    //std::cout << prototype << std::endl;
    
    auto* texture_info = getPrototypeTextureSet(prototype);
    if(!texture_info) return;
    
    if(prototype->display_type == ItemPrototype::SIMPLE)
        batch.Texture(transform, texture_info->textures[0].uvs);
    else{
        int x = transform.x;
        int y = transform.y;
        int center_x = transform.x + transform.width / 2;
        int center_y = transform.y + transform.height / 2;
        int right_x = transform.x + transform.width;
        int bottom_y = transform.y + transform.height;

        glm::vec2 center = {center_x, center_y};
        glm::vec2 quarter_right = {right_x , y + transform.height / 4};
        glm::vec2 quarter_left  = {x       , y + transform.height / 4};

        batch.TexturePolygon(
            {
                glm::vec2{center_x, y       },
                quarter_right,
                center,
                quarter_left
            },
            texture_info->textures[0].uvs
        );

        batch.TexturePolygon(
            {
                center,
                quarter_right,
                quarter_right + glm::vec2{0, transform.height / 2},
                glm::vec2{center_x, bottom_y}
            },
            texture_info->textures[1].uvs,
            {100,100,100}
        );

        batch.TexturePolygon(
            {
                quarter_left,
                center,
                glm::vec2{center_x, bottom_y},
                quarter_left + glm::vec2{0, transform.height / 2}
            },
            texture_info->textures[2].uvs,
            {150,150,150}
        );
    }
}

void ItemSlot::getRenderingInformation(UIRenderBatch& batch){
   /*yeet(
        UIRenderInfo::Rectangle(
            transform,
            getAttribute(&UIFrame::Style::backgroundColor)
        ),
        clipRegion
    );*/

    if(!item_slot.hasItem()) return;
    auto* item = item_slot.getItem();
    auto* prototype = item->getPrototype();
    if(!prototype) return;

    textureAtlas.RenderItemIntoSlot(batch,prototype,transform);

    int slot_x = transform.x + slot_padding;
    int slot_y = transform.y + slot_padding;
    int slot_width  = slot_size - slot_padding * 2;
    int slot_height = slot_width;

    std::string quantity_number = std::to_string(item->getQuantity());
    UITextDimensions textDimensions = UICore::get().getBackend().getTextDimensions(quantity_number, quantity_number_font_size);
    batch.Text(
        quantity_number, 
        slot_x + slot_width  - textDimensions.width,
        slot_y + slot_height - textDimensions.height,
        quantity_number_font_size,
        getAttribute(&UIFrame::Style::textColor)
    );
}

InventoryDisplay::InventoryDisplay(ItemTextureAtlas& textureAtlas, std::shared_ptr<ItemSlot> held_item_ptr): 
    textureAtlas(textureAtlas), held_item_ptr(held_item_ptr)
{
    dedicated_texture_array = textureAtlas.getTextureArray();

    setAttribute(&UIFrame::Style::backgroundColor, {20,20,20,200});
    setAttribute(&UIFrame::Style::borderWidth, {3_px,3_px,3_px,3_px});
    setAttribute(&UIFrame::Style::borderColor, {UIColor{0,0,0},{0,0,0},{0,0,0},{0,0,0}});

    onMouseEvent = [this](int button, int action, int mods){
        if(!this->hover) return;
        if(!this->held_item_ptr) return;
        if(!this->inventory) return;

        auto mousePosition = UICore::get().getMousePosition();
        int relative_x = mousePosition.x - transform.x;
        int relative_y = mousePosition.y - transform.y;

        int slot_x = relative_x / this->slot_size;
        int slot_y = relative_y / this->slot_size;

        auto* slot = this->inventory->getSlot(slot_x, slot_y);
        if(!slot) return;

        auto& item_slot = *slot;
        auto& hand_slot = this->held_item_ptr->getSlot();

        if(button == GLFW_MOUSE_BUTTON_LEFT){
            if(action == GLFW_PRESS && hand_slot.hasItem()){
                hand_slot.moveItemTo(item_slot);
            }
            else if(action == GLFW_PRESS && !hand_slot.hasItem()){
                hand_slot.takeItemFrom(item_slot);
            }
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT){
            if(action == GLFW_PRESS && hand_slot.hasItem()){
                hand_slot.moveItemTo(item_slot, 1);
            }
            else if(action == GLFW_PRESS && !hand_slot.hasItem()){
                hand_slot.takeItemFrom(item_slot, 1);
            }
        }
        
        this->held_item_ptr->update();
        update();
    };
}
    
void InventoryDisplay::getRenderingInformation(UIRenderBatch& batch){
    int line_width = borderSizes.top;
    UIColor line_color = getAttribute(&UIFrame::Style::borderColor).top;

    batch.BorderedRectangle(
        transform.x - borderSizes.left + line_width / 2,
        transform.y - borderSizes.top  + line_width / 2,
        transform.width  - line_width / 2,
        transform.height - line_width / 2,
        getAttribute(&UIFrame::Style::backgroundColor),
        borderSizes,
        getAttribute(&UIFrame::Style::borderColor)
    );

    if(!this->inventory) return;
    for(int y = 1;y < inventory->getHeight(); y++){
        batch.Rectangle(
            transform.x,
            transform.y + y * slot_size - line_width / 2,
            transform.width,
            line_width,
            line_color
        );
    }

    for(int x = 1;x < inventory->getWidth(); x++){
        batch.Rectangle(
            transform.x + x * slot_size - line_width / 2,
            transform.y,
            line_width,
            transform.height - line_width,
            line_color
        );
    }

    for(int y = 0;y < inventory->getHeight();y++)
    for(int x = 0;x < inventory->getWidth();x++){
        auto* slot = inventory->getSlot(x,y);

        auto* item = slot->getItem();
        if(!item) continue;
        auto* prototype = item->getPrototype();
        if(!prototype) continue;


        int slot_x = transform.x + x * slot_size + slot_padding;
        int slot_y = transform.y + y * slot_size + slot_padding;
        int slot_width  = slot_size - slot_padding * 2;
        int slot_height = slot_width;

        textureAtlas.RenderItemIntoSlot(batch,prototype,{
            slot_x,
            slot_y,
            slot_width,
            slot_height,
        });

        std::string quantity_number = std::to_string(item->getQuantity());
        UITextDimensions textDimensions = UICore::get().getBackend().getTextDimensions(quantity_number, quantity_number_font_size);
        batch.Text( 
            quantity_number, 
            slot_x + slot_width  - textDimensions.width,
            slot_y + slot_height - textDimensions.height,
            quantity_number_font_size,
            getAttribute(&UIFrame::Style::textColor)
        );
    }
}

void InventoryDisplay::setInventory(LogicalItemInventory* new_inventory){
    inventory = new_inventory;
    if(!inventory) return;
    setSize(TValue::Pixels(inventory->getWidth() * slot_size), TValue::Pixels(inventory->getHeight() * slot_size));
}