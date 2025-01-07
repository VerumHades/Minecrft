#pragma once

#include <bitarray.hpp>
#include <game/blocks.hpp>
#include <rendering/bitworks.hpp>

class SparseBlockArray{
    protected:
        struct Layer{
            BlockID  type;
            Block internal_block;
            CompressedBitField3D _field;
            
            BitField3D& field() {return *_field.get();};
        };

        /*
            Returns whether a layer is present
        */
        bool hasLayerOfType(BlockID type){
            return type_indexes.contains(type);
        }

        /*
            Creates a layer, if layer already exists doesnt create another
        */
        bool createLayer(BlockID type, const BitField3D& field){
            if(hasLayerOfType(type)) return false;
            type_indexes[type] = layers.size();
            layers.push_back({type,{type},field});
            present_types.push_back(type);
            return true;
        }

        /*
            Returns a layer of type if its present, otherwise crashes.
        */
        Layer& getLayer(BlockID type){
            if(!hasLayerOfType(type)) throw std::runtime_error("Layer has no layer of that type. Use 'hasLayerOfType' to check for layers.");

            return layers[type_indexes[type]];
        }

        BitField3D& getSolidField(){
            return *solid_field.get();
        }

        std::vector<BlockID>& getPresentTypes() { return present_types; }
        std::vector<Layer>& getLayers() { return layers; }
        
    private:
        Block airBlock = {BLOCK_AIR_INDEX};
        CompressedBitField3D solid_field; // Registers solid blocks

        std::vector<Layer> layers;
        std::vector<BlockID> present_types;
        std::unordered_map<BlockID, size_t> type_indexes;
    
    public:
        SparseBlockArray(){}

        bool isEmpty(){return layers.size() == 0;}

        void setBlock(glm::ivec3 position, Block block);
        /*
            Returns a pointer to a block, if there is no block present returns an air block
        */
        Block* getBlock(glm::ivec3 position);

        virtual ByteArray serialize();
        static SparseBlockArray deserialize(ByteArray& array);
};