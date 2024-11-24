#include <game/world/world_stream.hpp>


WorldStream::WorldStream(std::string filepath){
    bool newlyCreated = false;
    
    if(!std::filesystem::exists(filepath)){
        std::ofstream file(filepath, std::ios::binary);
        if(!file.is_open()){
            std::cout << "World stream opening failed, cannot create file: " << filepath << std::endl;
            std::terminate();
        }
        file.close();
        newlyCreated = true;
        std::cout << "Created missing world file." << std::endl;
    }

    file_stream = std::fstream(filepath, std::ios::in | std::ios::out | std::ios::binary);

    if(!file_stream.is_open()){
        std::cout << "Failed to open world file!" << std::endl;
        std::terminate();
    }

    if(newlyCreated){
        std::strcpy(header.name,"World");

        std::random_device rd;
        header.seed = rd(); 
        
        header.chunk_table_start = sizeof(Header);
        header.chunk_data_start = 20 * 1000; // Reserve ahead for about 1000 chunks
        header.chunk_data_end = 20 * 1000;
        header.chunk_table_size = 0;


        saveHeader();
        saveTable();
    } // Create the header table if its a new save file
    else{
        loadHeader();
        loadTable();
    }
}

void WorldStream::saveHeader(){
    file_stream.seekp(0, std::ios::beg); // Move cursor to the file start
    
    bitworks::saveValue(file_stream, header);
}

void WorldStream::loadHeader(){
    file_stream.seekg(0, std::ios::beg); // Move cursor to the file start
    
    header = bitworks::readValue<Header>(file_stream);

    //std::cout << "Table start: " << header.chunk_table_start << " Chunk data start: " << header.chunk_data_start << std::endl;
}

void WorldStream::loadTable(){
    file_stream.seekg(header.chunk_table_start, std::ios::beg); // Move cursor to the tables start
    chunkTable.clear();

    ByteArray tableData;
    if(!tableData.read(file_stream)){
        std::cout << "World file table corrupted, repairing" << std::endl;
        saveTable();
        return;
    }

    size_t size = tableData.read<size_t>();
    //std::cout << "Chunks total: " << size << std::endl;
    for(int i = 0;i < size;i++){
        glm::vec3 position = {
            tableData.read<float>(),
            tableData.read<float>(),
            tableData.read<float>()
        };
        size_t start = tableData.read<size_t>();
        if(start > header.chunk_data_end) {
            std::cout << "Corrupted chunk data, disposing." << std::endl;
            continue; 
        }
        chunkTable[position] = start;
    }
}

ByteArray WorldStream::serializeTableData(){
    ByteArray tableData;
    tableData.append<size_t>(chunkTable.size());

    for(auto& [position, start]: chunkTable){
        tableData.append<float>(position.x);
        tableData.append<float>(position.y);
        tableData.append<float>(position.z);
        tableData.append<size_t>(start);
    }

    return tableData;
}

void WorldStream::saveTable(){
    ByteArray tableData = serializeTableData();
    while(header.chunk_table_start + tableData.getFullSize() >= header.chunk_data_start){
        size_t movedSize = moveChunk(header.chunk_data_start, header.chunk_data_end); // Move the first chunk to the end

        //std::cout << movedSize << " > " << tableData.getFullSize() << std::endl;
        header.chunk_data_start += movedSize;
        header.chunk_data_end += movedSize;
    }
    tableData = serializeTableData();

    file_stream.seekp(header.chunk_table_start, std::ios::beg);
    tableData.write(file_stream);
    saveHeader();
}

size_t WorldStream::moveChunk(size_t from, size_t to){
    file_stream.seekg(from, std::ios::beg); // Move to the position
    
    ByteArray fromData;
    fromData.read(file_stream);

    glm::vec3 position = {
        fromData.read<float>(),
        fromData.read<float>(),
        fromData.read<float>()
    };

    file_stream.seekp(to, std::ios::beg); 
    fromData.write(file_stream);

    chunkTable[position] = to;

    return fromData.getFullSize();
}

/*
    Saves a chunk mask in this format:

    int type
    size_t compressed_24bit_count, data...
    size_t compressed_24bit_count_rotated, data...

static inline void saveMask(ByteArray& out, DynamicChunkMask& mask, int type){
    out.append<int>(type);
    out.append(mask.getSegments().compress().data);
}

/*
    Saved the chunk:
    float x,y,z
    size_t layerCount
    saved masks using the above function..

bool WorldStream::save(Chunk& chunk){
    if(hasChunkAt(chunk.getWorldPosition())) return false;
    ByteArray out;

    out.append(chunk.getWorldPosition().x);
    out.append(chunk.getWorldPosition().y);
    out.append(chunk.getWorldPosition().z);
    
    // Get the 64 bit masks if they exist otherwise fail
    if(!chunk.isMainGroupOfSize(64)) return false;
    auto& group = chunk.getMainGroup();

    size_t layer_count = group->getMasks().size();
    out.append(layer_count);

    saveMask(out, group->getSolidField(), -1);
    for(auto& [key, mask]: group->getMasks()) saveMask(out, mask, static_cast<int>(mask.getBlock().type));

    file_stream.seekp(header.chunk_data_end, std::ios::beg);
    out.write(file_stream);

    chunkTable[chunk.getWorldPosition()] = header.chunk_data_end;
    header.chunk_data_end += out.getFullSize();

    saveHeader();
    saveTable();
    return true;
}

void WorldStream::load(Chunk* chunk){
    if(!hasChunkAt(chunk->getWorldPosition())){
        return;
    }

    //std::cout << "Loadeding: " << chunk->getWorldPosition().x << " " << chunk->getWorldPosition().y << " " << chunk->getWorldPosition().z << std::endl;
    //std::cout << "Located at: " << chunkTable[chunk->getWorldPosition()] << std::endl;
    file_stream.seekg(chunkTable[chunk->getWorldPosition()], std::ios::beg);
    ByteArray source;
    
    if(!source.read(file_stream)){
        std::cout << "Corrupted chunk:" << chunk->getWorldPosition().x << " " << chunk->getWorldPosition().y << " " << chunk->getWorldPosition().z << std::endl;
        return;
    }
    
    //std::cout << "Loaded: " << chunk->getWorldPosition().x << " " << chunk->getWorldPosition().y << " " << chunk->getWorldPosition().z << std::endl;

    //std::cout << source << std::endl;

    glm::vec3 position = {
        source.read<float>(),
        source.read<float>(),
        source.read<float>()
    };
    size_t layerCount = source.read<size_t>();

    //std::cout << "Found: " << position.x << " " << position.y << " " << position.z << std::endl;
    //std::cout << "With total layers: " << layerCount << std::endl;

    source.read<int>();

    std::unique_ptr<DynamicChunkContents> outputDataGroup = std::make_unique<DynamicChunkContents>(64);

    DynamicChunkMask solidMask = DynamicChunkMask(64, {source.vread<compressed_24bit>(), 64 * 64});
    outputDataGroup->setSolidMask(solidMask);

    for(int layerIndex = 0; layerIndex < layerCount; layerIndex++){
        int type = source.read<int>();
        DynamicChunkMask mask =  DynamicChunkMask(64, {source.vread<compressed_24bit>(), 64 * 64});
        outputDataGroup->setMask(static_cast<BlockID>(type),mask);
    }

    chunk->setMainGroup(std::move(outputDataGroup));
    
    //std::cout << "Loaded: " << position.x << " " << position.y << " " << position.z << std::endl;
}
*/
bool WorldStream::hasChunkAt(glm::vec3 position){
    return chunkTable.count(position) != 0;
}

WorldStream::~WorldStream(){
    saveHeader();
    saveTable();
}