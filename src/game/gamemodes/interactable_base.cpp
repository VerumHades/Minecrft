#include <game/gamemodes/interactable_base.hpp>

void GameModeInteractable::UpdateCursor(){
    if(!state.game_state) return;
    auto& game_state = *state.game_state;
    
    const glm::vec3& camDirection = state.camera.getDirection();
    const glm::vec3& camPosition = state.camera.getPosition();

    RaycastResult hit = game_state.getTerrain().raycast(camPosition,camDirection,10);
 
    cursor_state.blockUnderCursor = game_state.getTerrain().getBlock(hit.position);
    cursor_state.blockUnderCursorPosition = hit.position;
    cursor_state.blockUnderCursorEmpty = hit.lastPosition;

    if(!cursor_state.blockUnderCursor || cursor_state.blockUnderCursor->id == BLOCK_AIR_INDEX) state.wireframe_renderer.removeCube(0);
    else state.wireframe_renderer.setCube(0,glm::vec3(hit.position) - 0.005f, {1.01,01.01,1.01},{0,0,0});
}
