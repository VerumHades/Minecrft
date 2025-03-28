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

void SparseBlockArray::serialize(ByteArray& output_array) {
    output_array.append(solid_field.getCompressed());
    output_array.append<size_t>(layers.size());

    for(auto& layer: layers){
        output_array.append<BlockID>(layer.type);
        output_array.append(layer._field.getCompressed());
    }

    output_array.append<size_t>(interactable_blocks.size());
    for(auto& [position, block]: interactable_blocks){
        output_array.append<signed char>(position.x);
        output_array.append<signed char>(position.y);
        output_array.append<signed char>(position.z);

        auto* prototype = BlockRegistry::get().getPrototype(block.id);
        if(prototype) output_array.append(prototype->name);
        else output_array.append("NO_PROTOTYPE");
        block.metadata->serialize(output_array);
    }
}
bool SparseBlockArray::deserialize(SparseBlockArray& output, ByteArray& array){
    ResolvedOption(solid_data, false, array, vread<uint64_t>)

    BitField3D::decompress(output.getSolidField().data(), solid_data);

    ResolvedOption(layer_count, false, array, read<size_t>);

    for(size_t i = 0;i < layer_count;i++){
        ResolvedOption(layer_type, false, array, read<BlockID>);
        ResolvedOption(data, false, array, vread<uint64_t>);

        BitField3D field{};
        BitField3D::decompress(field.data(), data);
        output.createLayer(layer_type, field);
    }

    ResolvedOption(interactable_block_count, false, array, read<size_t>);
    for(size_t i = 0;i < interactable_block_count;i++){
        glm::ivec3 position = {
            array.read<signed char>(),
            array.read<signed char>(),
            array.read<signed char>()
        };
        
        ResolvedOption(prototype_name, false, array, sread);
        auto* prototype = BlockRegistry::get().getPrototype(prototype_name);
        if(prototype) output.interactable_blocks[position] = Block{prototype->id, prototype->interface->deserialize(array)};
    }
}