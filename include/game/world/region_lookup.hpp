#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <vec_hash.hpp>
#include <memory>
#include <unordered_set>
#include <shared_mutex>

/*
    Structure for looking up regions in 'infinite' space, 

    Thread safe.
*/
template <typename T>
class RegionRegistry{
    public:
        struct Region{
            glm::ivec3 min;
            glm::ivec3 max;
            T value;

            Region(const glm::ivec3& min, const glm::ivec3& max, const T& value): 
                min(min), max(max), value(value) {}
        };
    private:
        std::shared_mutex mutex;

        const float subregion_size = 16;
        std::unordered_map<glm::ivec3, std::vector<std::shared_ptr<Region>>, IVec3Hash, IVec3Equal> regions;

    public:
        bool add(const glm::ivec3& position, const glm::ivec3& size, const T& value){
            if(instersects(position, size)) return false;

            glm::ivec3 min = position;
            glm::ivec3 max = position + size;

            auto region = std::make_shared<Region>(min, max, value);

            std::array<glm::vec3, 8> points = {
                min,
                {min.x,max.y,min.z},
                {min.x,max.y,max.z},
                {min.x,min.y,max.z},
                {max.x,min.y,min.z},
                {max.x,max.y,min.z},
                max,
                {max.x,min.y,max.z}
            };

            std::unordered_set<glm::ivec3, IVec3Hash, IVec3Equal> already_checked;

            std::unique_lock lock(mutex);

            for(auto& point: points){
                glm::ivec3 subregion_position = glm::floor(glm::vec3(point) / subregion_size);
                if(already_checked.contains(subregion_position)) continue;
                already_checked.emplace(subregion_position);

                regions[subregion_position].push_back(region);
            }

            return true;
        }
        bool instersects(const glm::ivec3& position, const glm::ivec3& size){
            glm::ivec3 min = position;
            glm::ivec3 max = position + size;

            std::array<glm::vec3, 8> points = {
                min,
                {min.x,max.y,min.z},
                {min.x,max.y,max.z},
                {min.x,min.y,max.z},
                {max.x,min.y,min.z},
                {max.x,max.y,min.z},
                max,
                {max.x,min.y,max.z}
            };

            std::unordered_set<glm::ivec3, IVec3Hash, IVec3Equal> already_checked;
            std::shared_lock lock(mutex);

            for(auto& point: points){
                glm::ivec3 subregion_position = glm::floor(glm::vec3(point) / subregion_size);
                
                if(already_checked.contains(subregion_position)) continue;
                already_checked.emplace(subregion_position);
                if(!regions.contains(subregion_position)) continue;

                for(auto& region: regions.at(subregion_position)){
                    if(
                        max.x >= region->min.x && min.x < region->max.x &&
                        max.y >= region->min.y && min.y < region->max.y &&
                        max.z >= region->min.z && min.z < region->max.z
                    ){
                        return true;
                    }
                }
            }

            return false;
        }
        

        Region* get(const glm::ivec3& position){
            glm::ivec3 subregion_position = glm::floor(glm::vec3(position) / subregion_size);
            
            std::shared_lock lock(mutex);
            if(!regions.contains(subregion_position)) return nullptr;

            for(auto& region: regions.at(subregion_position)){
                if(
                    position.x >= region->min.x && position.x < region->max.x &&
                    position.y >= region->min.y && position.y < region->max.y &&
                    position.z >= region->min.z && position.z < region->max.z
                ){
                    return region.get();
                }
            }

            return nullptr;
        }
};