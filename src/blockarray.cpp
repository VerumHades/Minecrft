#include <blockarray.hpp>

void SparseBlockArray::setBlock(glm::ivec3 position, const Block& block, bool dont_check){
    altered = true;

    if(!dont_check){
        auto* block_here = getBlock(position);
        if(block_here != &airBlock){
            getLayer(block_here->id).field().reset(position.x,position.y,position.z);
        }
    }

    if(block.id == BLOCK_AIR_INDEX){
        if(interactable_blocks.contains(position)) interactable_blocks.erase(position);
        solid_field.get()->reset(position.x,position.y,position.z);
        return;
    }

    if(!hasLayerOfType(block.id)){
        createLayer(block.id, {});
    }

    auto& layer = getLayer(block.id);
    layer.field().set(position.x,position.y,position.z);

    auto* block_definition = BlockRegistry::get().getPrototype(block.id);
    if(!block_definition) return;

    if(!block_definition->transparent) solid_field.get()->set(position.x,position.y,position.z);
    else solid_field.get()->reset(position.x,position.y,position.z);

    if(interactable_blocks.contains(position)) interactable_blocks.erase(position);
    if(block_definition->interface) interactable_blocks.emplace(position,Block{block.id, block_definition->interface->createMetadata()});
}

void SparseBlockArray::fill(const Block& block){
    altered = true;

    layers.clear();

    if(block.id == BLOCK_AIR_INDEX){
        solid_field.get()->fill(0);
        return;
    }

    if(!hasLayerOfType(block.id)){
        createLayer(block.id, {});
    }

    auto& layer = getLayer(block.id);
    layer.field().fill(1);

    auto* block_definition = BlockRegistry::get().getPrototype(block.id);
    if(!block_definition) return;

    solid_field.get()->fill(!block_definition->transparent);
}

Block* SparseBlockArray::getBlock(glm::ivec3 position){
    if(interactable_blocks.contains(position)) return &interactable_blocks.at(position);

    for(auto& layer: layers){
        if(!layer.field().get(position.x,position.y,position.z)) continue;

        return &layer.internal_block;
    }

    return &airBlock;
}