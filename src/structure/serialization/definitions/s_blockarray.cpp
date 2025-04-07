#include <structure/serialization/serializer.hpp>
#include <structure/serialization/serializer_extras.hpp>

/*
    BlockArray serialization
*/
#include <blockarray.hpp>

SerializeFunction(SparseBlockArray) {
    array.Append<bool>(this_.isEmpty());
    if(this_.isEmpty()) return true;

    array.Append(this_.solid_field.getCompressed());
    array.Append<size_t>(this_.layers.size());

    for(auto& layer: this_.layers){
        array.Append<BlockID>(layer.type);
        array.Append(layer._field.getCompressed());
    }

    array.Append<size_t>(this_.interactable_blocks.size());
    for(auto& [position, block]: this_.interactable_blocks){
        array.Append<signed char>(position.x);
        array.Append<signed char>(position.y);
        array.Append<signed char>(position.z);

        auto* prototype = BlockRegistry::get().getPrototype(block.id);
        if(prototype) array.Append(prototype->name);
        else array.Append("NO_PROTOTYPE");
        block.metadata->serialize(array);
    }

    return true;
}
SerializeInstatiate(SparseBlockArray)

DeserializeFunction(SparseBlockArray){
    ResolvedOption(is_empty, Read<bool>)
    if(is_empty) return true;

    ResolvedOption(solid_data, ReadVector<uint64_t>)

    BitField3D::decompress(this_.getSolidField().data(), solid_data);

    ResolvedOption(layer_count, Read<size_t>);

    for(size_t i = 0;i < layer_count;i++){
        ResolvedOption(layer_type, Read<BlockID>);
        ResolvedOption(data, ReadVector<uint64_t>);

        BitField3D field{};
        BitField3D::decompress(field.data(), data);
        this_.createLayer(layer_type, field);
    }

    ResolvedOption(interactable_block_count, Read<size_t>);
    for(size_t i = 0;i < interactable_block_count;i++){
        ResolvedOption(x, Read<signed char>)
        ResolvedOption(y, Read<signed char>)
        ResolvedOption(z, Read<signed char>)
        glm::ivec3 position = glm::ivec3{x,y,z};

        ResolvedOption(prototype_name, ReadString);
        auto* prototype = BlockRegistry::get().getPrototype(prototype_name);
        if(prototype) this_.interactable_blocks[position] = Block{prototype->id, prototype->interface->deserialize(array)};
    }

    return true;
}
DeserializeInstatiate(SparseBlockArray);
