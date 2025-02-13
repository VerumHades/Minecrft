#include <game/world/world_stream.hpp>


WorldStream::WorldStream(const std::string& path): FileStream("world", path){
    set_callbacks(
        [this](FileStream* stream){
            std::strcpy(header.name,"Terrain");

            std::random_device rd;
            header.seed = rd(); 
            
            header.chunk_table_start = sizeof(Header);
            header.chunk_data_start = 20 * 1000; // Reserve ahead for about 1000 chunks
            header.chunk_data_end = 20 * 1000;
            header.chunk_table_size = 0;
    
    
            saveHeader();
            saveTable();
        },
        [this](FileStream* stream){
            loadHeader();
            loadTable();
        }
    );
}

void WorldStream::saveHeader(){
    stream().seekp(0, std::ios::beg); // Move cursor to the file start
    
    bitworks::saveValue(stream(), header);
}

void WorldStream::loadHeader(){
    stream().seekg(0, std::ios::beg); // Move cursor to the file start
    
    header = bitworks::readValue<Header>(stream());

    //std::cout << "Table start: " << header.chunk_table_start << " Chunk data start: " << header.chunk_data_start << std::endl;
}

void WorldStream::loadTable(){
    stream().seekg(header.chunk_table_start, std::ios::beg); // Move cursor to the tables start
    chunkTable.clear();

    ByteArray tableData;
    if(!tableData.read(stream())){
        std::cout << "Terrain file table corrupted, repairing" << std::endl;
        saveTable();
        return;
    }

    size_t size = tableData.read<size_t>();
    //std::cout << "Chunks total: " << size << std::endl;
    for(int i = 0;i < size;i++){
        auto row = tableData.read<TableRow>();
        chunkTable[glm::ivec3(row.position.x,row.position.y,row.position.z)] = row;
    }
}

ByteArray WorldStream::serializeTableData(){
    ByteArray tableData;
    tableData.append<size_t>(chunkTable.size());

    for(auto& [position, row]: chunkTable) tableData.append<TableRow>(row);

    return tableData;
}



void WorldStream::saveTable(){
    ByteArray tableData = serializeTableData();
    while(header.chunk_table_start + tableData.getFullSize() >= header.chunk_data_start){
        size_t movedSize = moveChunk(header.chunk_data_start, header.chunk_data_end); // Move the first chunk to the end

        //std::cout << movedSize << " > " << tableData.getFullSize() << std::endl;
        header.chunk_data_start = findFirstChunk();
        header.chunk_data_end += movedSize;
    }
    tableData = serializeTableData();

    stream().seekp(header.chunk_table_start, std::ios::beg);
    tableData.write(stream());
    saveHeader();
}

size_t WorldStream::findFirstChunk(){
    size_t position = std::numeric_limits<size_t>::max();
    
    for(auto& [geo_position, file_position]: chunkTable){
        position = std::min(position,file_position.in_file_start);
    }

    return position;
}

size_t WorldStream::moveChunk(size_t from, size_t to){
    stream().seekg(from, std::ios::beg); // Move to the position
    
    ByteArray fromData;
    fromData.read(stream());

    auto loaded_chunk = Chunk::deserialize(fromData);

    stream().seekp(to, std::ios::beg); 
    fromData.write(stream());

    chunkTable[loaded_chunk.getWorldPosition()].in_file_start = to;

    if(from == header.chunk_data_start) 
        header.chunk_data_start += chunkTable[loaded_chunk.getWorldPosition()].serialized_size;

    return fromData.getFullSize();
}

bool WorldStream::save(Chunk& chunk){
    auto position = chunk.getWorldPosition();
    
    ByteArray out = chunk.serialize();

    if(hasChunkAt(position)){
        auto& old_registered_value = chunkTable[chunk.getWorldPosition()];
        auto new_size = out.getFullSize();

        if(out.getFullSize() < old_registered_value.serialized_size){
            stream().seekp(old_registered_value.in_file_start, std::ios::beg);
            out.write(stream());
            
            old_registered_value.serialized_size = new_size;
            return true;
        }
    }

    stream().seekp(header.chunk_data_end, std::ios::beg);
    out.write(stream());

    chunkTable[chunk.getWorldPosition()] = {
        {
            position.x,
            position.y,
            position.z
        },
        header.chunk_data_end,
        out.getFullSize()
    };

    header.chunk_data_end += out.getFullSize();

    saveHeader();
    saveTable();
    return true;
}

void WorldStream::load(Chunk* chunk){
    if(!hasChunkAt(chunk->getWorldPosition())) return;
    //std::cout << "Loadeding: " << chunk->getWorldPosition().x << " " << chunk->getWorldPosition().y << " " << chunk->getWorldPosition().z << std::endl;
    //std::cout << "Located at: " << chunkTable[chunk->getWorldPosition()] << std::endl;
    stream().seekg(chunkTable[chunk->getWorldPosition()].in_file_start, std::ios::beg);
    ByteArray source;
    
    if(!source.read(stream())){
        std::cout << "Corrupted chunk:" << chunk->getWorldPosition().x << " " << chunk->getWorldPosition().y << " " << chunk->getWorldPosition().z << std::endl;
        return;
    }
    
    //std::cout << "Loaded: " << chunk->getWorldPosition().x << " " << chunk->getWorldPosition().y << " " << chunk->getWorldPosition().z << std::endl;

    //std::cout << source << std::endl;

    *chunk = Chunk::deserialize(source);
    
    //std::cout << "Loaded: " << position.x << " " << position.y << " " << position.z << std::endl;
}

bool WorldStream::hasChunkAt(glm::vec3 position){
    return chunkTable.count(position) != 0;
}

WorldStream::~WorldStream(){
    saveHeader();
    saveTable();
}