#include <structure/serialization/serializer.hpp>
#include <structure/serialization/serializer_extras.hpp>

/*
    BlockArray serialization
*/
#include <blockarray.hpp>

SerializeFunction(SparseBlockArray) {
    array.append<bool>(this_.isEmpty());
    if(this_.isEmpty()) return true;

    array.append(this_.solid_field.getCompressed());
    array.append<size_t>(this_.layers.size());

    for(auto& layer: this_.layers){
        array.append<BlockID>(layer.type);
        array.append(layer._field.getCompressed());
    }

    array.append<size_t>(this_.interactable_blocks.size());
    for(auto& [position, block]: this_.interactable_blocks){
        array.append<signed char>(position.x);
        array.append<signed char>(position.y);
        array.append<signed char>(position.z);

        auto* prototype = BlockRegistry::get().getPrototype(block.id);
        if(prototype) array.append(prototype->name);
        else array.append("NO_PROTOTYPE");
        block.metadata->serialize(array);
    }

    return true;
}
SerializeInstatiate(SparseBlockArray)

DeserializeFunction(SparseBlockArray){
    ResolvedOption(is_empty, read<bool>)
    if(is_empty) return true;

    ResolvedOption(solid_data, vread<uint64_t>)

    BitField3D::decompress(this_.getSolidField().data(), solid_data);

    ResolvedOption(layer_count, read<size_t>);

    for(size_t i = 0;i < layer_count;i++){
        ResolvedOption(layer_type, read<BlockID>);
        ResolvedOption(data, vread<uint64_t>);

        BitField3D field{};
        BitField3D::decompress(field.data(), data);
        this_.createLayer(layer_type, field);
    }

    ResolvedOption(interactable_block_count, read<size_t>);
    for(size_t i = 0;i < interactable_block_count;i++){
        ResolvedOption(x, read<signed char>)
        ResolvedOption(y, read<signed char>)
        ResolvedOption(z, read<signed char>)
        glm::ivec3 position = glm::ivec3{x,y,z};

        ResolvedOption(prototype_name, sread);
        auto* prototype = BlockRegistry::get().getPrototype(prototype_name);
        if(prototype) this_.interactable_blocks[position] = Block{prototype->id, prototype->interface->deserialize(array)};
    }

    return true;
}
DeserializeInstatiate(SparseBlockArray);
