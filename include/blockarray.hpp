#pragma once

#include <bitarray.hpp>
#include <game/blocks.hpp>
#include <rendering/bitworks.hpp>
#include <vec_hash.hpp>
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
            layers.push_back({type,Block{type},field});
            present_types.push_back(type);
            return true;
        }

        /*
            Returns a layer of type if its present, otherwise crashes.
        */
        Layer& getLayer(BlockID type){
            if(!hasLayerOfType(type)) *(int*)nullptr = 1; ; // Crashing is easier to find than an exception

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

        bool altered = false;

        std::vector<Layer> layers;
        std::vector<BlockID> present_types;
        std::unordered_map<BlockID, size_t> type_indexes;
        std::unordered_map<glm::ivec3, Block, IVec3Hash, IVec3Equal> interactable_blocks{};
    
    public:
        SparseBlockArray(){}

        bool isEmpty(){return layers.size() == 0;}

        /*
            Fills the entire chunk with one kind of block
        */
        void fill(const Block& block);

        /*
            dont_check => if true ignores already existing block if there is one (will be faster)
        */
        void setBlock(glm::ivec3 position, const Block& block, bool dont_check = false);
        /*
            Returns a pointer to a block, if there is no block present returns an air block
        */
        Block* getBlock(glm::ivec3 position);

        void resetAlteredFlag() { altered = false; }
        bool wasAltered() {return altered;}

        virtual ByteArray serialize();
        virtual void serialize(ByteArray& output_array);
        
        static SparseBlockArray deserialize(ByteArray& array);
};