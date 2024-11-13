#pragma once

#include <game/chunk.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <shared_mutex>
#include <random>

class WorldStream{
    private:
        std::fstream file_stream;
        std::unordered_map<glm::vec3, size_t, Vec3Hash, Vec3Equal> chunkTable = {}; // Chunk locations in the file
        std::shared_mutex mutex;

        struct Header{
            char name[256];
            int seed;

            size_t chunk_table_start;
            size_t chunk_table_size;

            size_t chunk_data_start;
            size_t chunk_data_end;
        };

        Header header;

        ByteArray serializeTableData();
        void saveHeader();
        void loadHeader();

        void loadTable();
        void saveTable();

        size_t moveChunk(size_t from, size_t to);

    public:
        WorldStream(std::string filepath);
        ~WorldStream();
        bool save(Chunk& chunk);
        void load(Chunk* chunk);
        bool hasChunkAt(glm::vec3 position);

        int getSeed() const {return header.seed;};
        std::string getName() const {return std::string(header.name);}
        
        int getChunkCount() {return chunkTable.size();}

        void setName(std::string name) {
            if(name.length() > 256) {
                std::cerr << "Max world name length is 256 chars" << std::endl;
                return;
            }
            std::strcpy(header.name,name.c_str()); // Boo unsafe
            saveHeader();
        }
};