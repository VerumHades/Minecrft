#pragma once

#include <bitarray.hpp>
#include <game/blocks.hpp>

class SparseBlockArray{
    protected:
        struct Layer{
            BlockType  type;
            Block internal_block;
            BitField3D field;
        };

        /*
            Returns whether a layer is present
        */
        bool hasLayerOfType(BlockType type){
            return type_indexes[static_cast<size_t>(type)] != -1ULL;
        }

        /*
            Creates a layer, if layer already exists doesnt create another
        */
        bool createLayer(BlockType type, const BitField3D& field){
            if(hasLayerOfType(type)) return false;
            type_indexes[static_cast<size_t>(type)] = layers.size();
            layers.push_back({type,{type},field});
            present_types.push_back(type);
            return true;
        }

        /*
            Returns a layer of type if its present, otherwise crashes.
        */
        Layer& getLayer(BlockType type){
            if(!hasLayerOfType(type)) throw std::runtime_error("Layer has no layer of that type. Use 'hasLayerOfType' to check for layers.");

            return layers[type_indexes[static_cast<size_t>(type)]];
        }

        BitField3D& getSolidField(){
            return solid_field;
        }

        std::vector<BlockType>& getPresentTypes() { return present_types; }
        std::vector<Layer>& getLayers() { return layers; }
    private:
        Block airBlock = {BlockType::Air};
        BitField3D solid_field; // Registers solid blocks

        std::vector<Layer> layers;
        std::vector<BlockType> present_types;
        std::array<size_t, static_cast<size_t>(BlockType::BLOCK_TYPES_TOTAL)> type_indexes;
    
    public:
        SparseBlockArray(){
            type_indexes.fill(-1ULL); // Initialize every block type as not present
        }

        bool isEmpty(){return layers.size() == 0;}

        void setBlock(uint x, uint y, uint z, Block block){
            auto* block_here = getBlock(x,y,z);
            if(block_here != &airBlock){
                getLayer(block_here->type).field.reset(x,y,z);
            }

            if(block.type == BlockType::Air){
                solid_field.reset(x,y,z);
                std::cout << "Air!" << std::endl;
                return;
            }

            if(!hasLayerOfType(block.type)){
                createLayer(block.type, {});
            }

            auto& layer = getLayer(block.type);
            layer.field.set(x,y,z);

            auto& block_definition = getBlockDefinition(&block);
            
            if(block_definition.solid) solid_field.set(x,y,z);
            else solid_field.reset(x,y,z);
        }

        /*
            Returns a pointer to a block, if there is no block present returns an air block
        */
        Block* getBlock(uint x, uint y, uint z){
            for(auto& layer: layers){
                if(!layer.field.get(x,y,z)) continue;

                return &layer.internal_block;
            }

            return &airBlock;
        }
};