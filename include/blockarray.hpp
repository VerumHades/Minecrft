#pragma once

#include <bitarray.hpp>
#include <game/blocks.hpp>

class SparseBlockArray{
    protected:
        struct Layer{
            BlockID  type;
            Block internal_block;
            BitField3D field;
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
            return solid_field;
        }

        std::vector<BlockID>& getPresentTypes() { return present_types; }
        std::vector<Layer>& getLayers() { return layers; }
        
    private:
        BlockRegistry& blockRegistry;
        Block airBlock = {BLOCK_AIR_INDEX};
        BitField3D solid_field; // Registers solid blocks

        std::vector<Layer> layers;
        std::vector<BlockID> present_types;
        std::unordered_map<BlockID, size_t> type_indexes;
    
    public:
        SparseBlockArray(BlockRegistry& blockRegistry): blockRegistry(blockRegistry){}

        bool isEmpty(){return layers.size() == 0;}

        void setBlock(glm::ivec3 position, Block block){
            auto* block_here = getBlock(position);
            if(block_here != &airBlock){
                getLayer(block_here->id).field.reset(position.x,position.y,position.z);
            }

            if(block.id == BLOCK_AIR_INDEX){
                solid_field.reset(position.x,position.y,position.z);
                return;
            }

            if(!hasLayerOfType(block.id)){
                createLayer(block.id, {});
            }

            auto& layer = getLayer(block.id);
            layer.field.set(position.x,position.y,position.z);

            auto* block_definition = blockRegistry.getBlockPrototypeByIndex(block.id);
            if(!block_definition) return;

            if(!block_definition->transparent) solid_field.set(position.x,position.y,position.z);
            else solid_field.reset(position.x,position.y,position.z);
        }

        /*
            Returns a pointer to a block, if there is no block present returns an air block
        */
        Block* getBlock(glm::ivec3 position){
            for(auto& layer: layers){
                if(!layer.field.get(position.x,position.y,position.z)) continue;

                return &layer.internal_block;
            }

            return &airBlock;
        }
};