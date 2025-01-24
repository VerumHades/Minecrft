#include <game/game_state.hpp>

GameState::GameState(const std::string& filename): world_stream(filename){
    entities.push_back(Entity(glm::vec3(0,60,0), glm::vec3(0.6, 1.8, 0.6)));
}
static int rotation = 0;
static float position = 0;
static float position_mult = 1;

void GameState::drawEntity(Entity& entity){
    if(!entity.getModel()) return;
    
    entity.getModel()->requestDraw(entity.getPosition() + glm::vec3{0,position,0}, {0.3,0.3,0.3}, {0,rotation,0}, entity.getModel()->getRotationCenterOffset());
}

void GameState::loadChunk(const glm::ivec3& position){
    auto* chunk = terrain.getChunk(position);
    if(chunk) return;

    chunk = terrain.createEmptyChunk(position);
    world_generator.generateTerrainChunkAccelerated(chunk, position);
}

bool GameState::entityCollision(Entity& checked_entity, const glm::vec3& offset){
    auto& checked_collider = checked_entity.getCollider();
    auto position = checked_entity.getPosition() + offset;

    if(terrain.collision(position, &checked_collider)) return true;

    for(auto& entity: entities){
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

void GameState::unloadChunk(const glm::ivec3& position){
    terrain.removeChunk(position);
}

void GameState::updateEntity(Entity& entity, float deltatime){
    if(entity.hasGravity) entity.accelerate(glm::vec3(0,-(0.98 + entity.friction * 2),0), deltatime);
    
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

void GameState::drawEntityColliders(WireframeCubeRenderer& renderer, size_t start_index){
    /*for(auto& [region_position,entities]: entity_regions){
        renderer.setCube(
            start_index++,
            region_position * entity_region_size, 
            glm::vec3{entity_region_size},
            {1.0,0.0,1.0}
        );
        for(auto& entity: entities){
            auto& collider = entity->getCollider();
            renderer.setCube(
                start_index++,
                glm::vec3{collider.x, collider.y, collider.z} + entity->getPosition(), 
                glm::vec3{collider.width, collider.height, collider.depth},
                {1.0,1.0,0}
            );
        }
    }*/

    /*for(auto& region_position: getPlayer().getRegionPositions()){
        renderer.setCube(
            start_index++,
            region_position * entity_region_size, 
            glm::vec3{entity_region_size},
            {1.0,0.0,1.0}
        );
    }*/
}