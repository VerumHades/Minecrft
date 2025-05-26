#pragma once

#include <bitarray.hpp>
#include <game/blocks.hpp>

#include <structure/serialization/serializer.hpp>
#include <structure/bytearray.hpp>
#include <structure/bitworks.hpp>

#include <vec_hash.hpp>

/**
 * @brief An array of fields of different types, only recording existing blocks, nothing represents empty space
 * 
 */
class SparseBlockArray {
  protected:
    struct Layer {
        BlockID type;
        Block internal_block;
        CompressedBitField3D _field;

        BitField3D& field() {
            return *_field.get();
        };
    };

    /**
     * @brief Returns whether a layer is present
     * 
     * @param type 
     * @return true 
     * @return false 
     */
    bool hasLayerOfType(BlockID type) {
        return type_indexes.contains(type);
    }

    /**
     * @brief Creates a layer, if layer already exists doesnt create another
     * 
     * @param type 
     * @param field 
     * @return true if success
     * @return false if layer already exists
     */
    bool createLayer(BlockID type, const BitField3D& field) {
        if (hasLayerOfType(type))
            return false;
        type_indexes[type] = layers.size();
        layers.push_back({type, Block{type}, field});
        present_types.push_back(type);
        return true;
    }

    /**
     * @brief Returns a layer of type, if it doesnt exist creates it
     * 
     * @param type 
     * @return Layer& 
     */
    Layer& getLayer(BlockID type) {
        if (!hasLayerOfType(type))
            createLayer(type, {});

        return layers[type_indexes[type]];
    }

    BitField3D& getSolidField() {
        return *solid_field.get();
    }

    std::vector<BlockID>& getPresentTypes() {
        return present_types;
    }
    std::vector<Layer>& getLayers() {
        return layers;
    }

  private:
    Block airBlock = {BLOCK_AIR_INDEX};
    CompressedBitField3D solid_field; // Registers solid blocks

    bool altered = false;

    std::vector<Layer> layers;
    std::vector<BlockID> present_types;
    std::unordered_map<BlockID, size_t> type_indexes;
    std::unordered_map<glm::ivec3, Block, IVec3Hash, IVec3Equal> interactable_blocks{};

    friend class Serializer;

  public:
    SparseBlockArray() {}

    bool isEmpty() {
        return layers.size() == 0;
    }

    /**
     * @brief Fills the entire chunk with one kind of block
     * 
     * @param block 
     */
    void fill(const Block& block);

    /**
     * @brief Set the Block object
     * 
     * @param position 
     * @param block 
     * @param dont_check if true ignores already existing block if there is one (will be faster)
     */
    void setBlock(glm::ivec3 position, const Block& block, bool dont_check = false);
    
    /**
     * @brief Returns a block at position
     * 
     * @param position 
     * @return Block* a pointer to a block, if there is no block present returns an air block
     */
    Block* getBlock(glm::ivec3 position);
};
