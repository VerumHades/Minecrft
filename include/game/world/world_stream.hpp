#pragma once

#include <game/chunk.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <shared_mutex>
#include <random>
#include <game/save_structure.hpp>
#include <limits>

class WorldStream: public FileStream{
    private:
        std::shared_mutex mutex;

        struct Header{
            char name[256];
            int seed;

            size_t chunk_table_start;
            size_t chunk_table_size;

            size_t chunk_data_start;
            size_t chunk_data_end;
        };

        struct TableRow{
            struct{
                int x;
                int y;
                int z;
            } position;
            size_t in_file_start;
            size_t serialized_size;
        };

        std::unordered_map<glm::ivec3, TableRow, IVec3Hash, IVec3Equal> chunkTable = {}; // Chunk locations in the file

        Header header;

        ByteArray serializeTableData();
        void saveHeader();
        void loadHeader();

        void loadTable();
        void saveTable();

        size_t moveChunk(size_t from, size_t to);

        size_t findFirstChunk();
    public:
        WorldStream(const std::string& path);
        ~WorldStream();
        //bool save(Chunk& chunk);
        //void load(Chunk* chunk);
        bool hasChunkAt(glm::vec3 position);

        int getSeed() const {return header.seed;};
        void setSeed(int value) {header.seed = value; saveHeader();}
        std::string getName() const {return std::string(header.name);}
        
        int getChunkCount() {return chunkTable.size();}

        void bulkSave(std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, IVec3Hash, IVec3Equal>& chunks);

        bool save(Chunk& chunk);
        void load(Chunk* chunk);

        void setName(const std::string& name) {
            if(name.length() > 256) {
                LogError("Max world name length is 256 chars");
                return;
            }
            std::strcpy(header.name,name.c_str()); // Boo unsafe
            saveHeader();
        }
};