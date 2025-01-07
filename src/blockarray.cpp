#include <blockarray.hpp>

void SparseBlockArray::setBlock(glm::ivec3 position, Block block){
    auto* block_here = getBlock(position);
    if(block_here != &airBlock){
        getLayer(block_here->id).field().reset(position.x,position.y,position.z);
    }

    if(block.id == BLOCK_AIR_INDEX){
        solid_field.get()->reset(position.x,position.y,position.z);
        return;
    }

    if(!hasLayerOfType(block.id)){
        createLayer(block.id, {});
    }

    auto& layer = getLayer(block.id);
    layer.field().set(position.x,position.y,position.z);

    auto* block_definition = global_block_registry.getBlockPrototypeByIndex(block.id);
    if(!block_definition) return;

    if(!block_definition->transparent) solid_field.get()->set(position.x,position.y,position.z);
    else solid_field.get()->reset(position.x,position.y,position.z);
}

/*
    Returns a pointer to a block, if there is no block present returns an air block
*/
Block* SparseBlockArray::getBlock(glm::ivec3 position){
    for(auto& layer: layers){
        if(!layer.field().get(position.x,position.y,position.z)) continue;

        return &layer.internal_block;
    }

    return &airBlock;
}

ByteArray SparseBlockArray::serialize(){
    ByteArray output{};

    output.append(BitField3D::compress(getSolidField().data()));
    output.append<size_t>(layers.size());

    for(auto& layer: layers){
        output.append<BlockID>(layer.type);
        output.append(BitField3D::compress(layer.field().data()));
    }

    return output;
}

SparseBlockArray SparseBlockArray::deserialize(ByteArray& array){
    SparseBlockArray output{};

    auto solid_data = array.vread<uint64_t>();
    BitField3D::decompress(output.getSolidField().data(), solid_data);

    size_t layer_count = array.read<size_t>();

    for(int i = 0;i < layer_count;i++){
        BlockID layer_type = array.read<BlockID>();
        auto data = array.vread<uint64_t>();

        BitField3D field{};
        BitField3D::decompress(field.data(), data);
        output.createLayer(layer_type, field);
    }

    return output;
}