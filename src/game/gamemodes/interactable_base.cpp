#include <game/gamemodes/interactable_base.hpp>

void GameModeInteractable::UpdateCursor(){
    if(!state.game_state) return;
    auto& game_state = *state.game_state;

    const glm::vec3& camDirection = state.camera.getDirection();
    const glm::vec3& camPosition = state.camera.getPosition();

    RaycastResult hit = game_state.GetTerrain().raycast(camPosition,camDirection,10);

    auto last = cursor_state.blockUnderCursorPosition;

    cursor_state.blockUnderCursor = game_state.GetTerrain().getBlock(hit.position);
    cursor_state.blockUnderCursorPosition = hit.position;
    cursor_state.blockUnderCursorEmpty = hit.lastPosition;

    if(last != cursor_state.blockUnderCursorPosition && onCursorTargetChange && cursor_state.blockUnderCursor) onCursorTargetChange();

    if(!cursor_state.blockUnderCursor || cursor_state.blockUnderCursor->id == BLOCK_AIR_INDEX) state.cube_renderer.removeCube(1);
    else state.cube_renderer.setCube(1,glm::vec3(hit.position) - 0.005f, 6);
}

void GameModeInteractable::PhysicsUpdate(double deltatime){
    if(!state.game_state) return;
    auto& game_state = *state.game_state;


    auto player_position = game_state.getPlayer().getPosition();
    if(glm::distance(player_position, lastPlayerPosition) >= 1){
        pending_cursor_update = true;
        lastPlayerPosition = player_position;
    }
}

void GameModeInteractable::Render(double deltatime){
    if(pending_cursor_update){
        UpdateCursor();
        pending_cursor_update = false;
    }
}