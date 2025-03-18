#include <game/game_state.hpp>

namespace fs = fs;

GameState::GameState(const std::string& path, int worldSeed): save_structure(path){
    Entity player = Entity(glm::vec3(0,30,0), glm::vec3(0.6, 1.8, 0.6));
    player.addTag("player");
    entities.push_back(player);
    
    world_stream  = save_structure.save<WorldStream>("");
    player_stream = save_structure.save<FileStream>("player", "",
        [this](FileStream*){
            this->savePlayer();
        },
        [this](FileStream* stream){
            this->loadPlayer();
        }
    );

    entity_stream = save_structure.save<FileStream>("entities", "",
        [this](FileStream*){
            this->saveEntities();
        },
        [this](FileStream*){
            this->loadEntities();
        }
    );

    save_structure.open();

    if(worldSeed != -1) world_stream->setSeed(worldSeed);
}

void GameState::savePlayer(){
    ByteArray array{};

    player_inventory.serialize(array);
    player_hotbar.serialize(array);
    
    player_stream->go_to(0);
    array.write(player_stream->stream());
}
void GameState::loadPlayer(){
    player_stream->go_to(0);
    ByteArray array = ByteArray::FromStream(player_stream->stream());

    player_inventory = LogicalItemInventory::deserialize(array);
    player_hotbar = LogicalItemInventory::deserialize(array);
}

void GameState::saveEntities(){
    ByteArray array{};

    array.append<size_t>(entities.size());
    for(auto& entity: entities) entity.serialize(array);

    entity_stream->go_to(0);
    array.write(entity_stream->stream());
}

void GameState::loadEntities(){
    entity_stream->go_to(0);
    entities.clear();
    ByteArray array = ByteArray::FromStream(entity_stream->stream());

    size_t count = array.read<size_t>();
    for(size_t i = 0;i < count;i++){
        entities.emplace_back(Entity::deserialize(array));
    }
}

void GameState::giveItemToPlayer(ItemRef item){
    if(!player_hotbar.addItem(item))
            player_inventory.addItem(item);
}

static int rotation = 0;
static float position = 0;
static float position_mult = 1;

void GameState::drawEntity(Entity& entity){
    if(!entity.getModel()) return;
    
    entity.getModel()->requestDraw(entity.getPosition() + glm::vec3{0,position,0}, {0.3,0.3,0.3}, {0,rotation,0}, entity.getModel()->getRotationCenterOffset());
}

bool GameState::entityCollision(Entity& checked_entity, const glm::vec3& offset){
    if(checked_entity.shouldGetDestroyed()) return false;

    auto& checked_collider = checked_entity.getCollider();
    auto position = checked_entity.getPosition() + offset;

    if(terrain.collision(position, &checked_collider)){
        if(checked_entity.onTerrainCollision) checked_entity.onTerrainCollision(&checked_entity);
        return true;
    }

    for(auto& entity: entities){
        if(entity.shouldGetDestroyed()) continue;
        if(&entity == &checked_entity) continue;

        auto& entity_collider = entity.getCollider();

        float distance = glm::distance(entity_collider.center, checked_collider.center);
        if(distance > checked_collider.bounding_sphere_radius + entity_collider.bounding_sphere_radius) continue; // Definitly dont collide
        
        if(entity_collider.collidesWith(&checked_collider, entity.getPosition(), position)){
            if(checked_entity.onCollision) checked_entity.onCollision(&checked_entity, &entity);
            if(entity.onCollision) entity.onCollision(&entity, &checked_entity);
            
            if(entity.isSolid()) return true;
        }
    }

    return false;
}

void GameState::loadChunk(const glm::ivec3& position){
    auto* chunk = terrain.getChunk(position);
    if(chunk) return;

    if(world_stream && world_stream->hasChunkAt(position)){
        chunk = terrain.createEmptyChunk(position);
        world_stream->load(chunk);
        return;
    }
}

void GameState::unloadChunk(const glm::ivec3& position){
    auto* chunk = terrain.getChunk(position);
    if(!chunk) return;

    if(world_stream) world_stream->save(*chunk);
    terrain.removeChunk(position);
}


void GameState::unload(){
    //for(auto& [position, chunk]: terrain.chunks) unloadChunk(position);

    world_stream->bulkSave(terrain.chunks);
    terrain.chunks.clear();
    savePlayer();
    saveEntities();
}

void GameState::updateEntity(Entity& entity, float deltatime){
    if(entity.data && entity.data->do_update) entity.data->update(this);

    if(entity.hasGravity) entity.accelerate(glm::vec3(0,-(15.0 + entity.friction * 2),0), deltatime);
    
    if(
        entity.velocity.x != 0 || 
        entity.velocity.y != 0 || 
        entity.velocity.z != 0
    ) entity.on_ground = false;

    glm::vec3& vel = entity.velocity;

    float relative_friction = entity.friction * deltatime;

    float len = glm::length(vel);
    if(len > relative_friction) vel = glm::normalize(vel) * (len - relative_friction);
    else vel = glm::vec3(0);

    if(vel.x != 0 && entityCollision(entity, {vel.x * deltatime, 0, 0})) vel.x = 0;
    if(vel.y != 0 && entityCollision(entity, {0, vel.y * deltatime, 0})) vel.y = 0;
    if(vel.z != 0 && entityCollision(entity, {0, 0, vel.z * deltatime})) vel.z = 0;

    entity.position += vel * deltatime;
}

void GameState::updateEntities(float deltatime){
    int index = 0;

    std::list<Entity>::iterator i = entities.begin();
    while (i != entities.end())
    {
        updateEntity(*i, deltatime);
        if(i->shouldGetDestroyed()){
            i = entities.erase(i);
            continue;
        }

        drawEntity(*i);
        ++i;
    }

    const float position_addition = 0.002;

    position += position_mult;
    if(position <= 0.1) position_mult = position_addition;
    if(position >= 0.4) position_mult = -position_addition;

    rotation = (rotation + 1) % 360;
}

/*void GameState::save(){
    for(auto& [position,chunk]: chunks){
        if(!chunk->wasAltered()) continue;
        chunk->resetAlteredFlag();
        stream->save(*chunk);
    }
}*/