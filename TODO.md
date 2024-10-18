# TODO

- [ ] Finish making UI
  - [ ] Main Menu
    - [ ] World creation menu
  - [x] Pause Menu
    - [x] Pause Settings
    - [x] Exit
  - [ ] Settings
    - [ ] Graphics Settings
  - [ ] Console
    - [ ] Add relevant normal and debug commands
  - [ ] Refactor UI units (change split each into vertical and horizontal parts: parent width/height)

- [ ] Improve Graphics
  - [x] Fix shadows
  - [ ] Add ambient occlusion
  - [ ] Add support for transparency

- [x] Improve world saving
  - [x] Flexibly save and load chunks
  
- [ ] Improve rendering
  - [x] Batch render chunks
  - [x] Add double buffering to batched chunks
  - [ ] Fix mesh buffer running out of memory
    - [x] Free meshes when out of view if neccesary
  - [x] Free chunk meshes that are far away
  - [ ] Add LOD
    - [ ] Add meshing for different chunk sizes
    - [ ] Make something to approximate distant chunks
  - [ ] Make chunk buffer check for mesh format correctness
  - [ ] Transpose planes to save 1/2 of all memory taken by blocks

- [ ] Add missing features:
  - [ ] Add support for non solid blocks (Stairs and such)
  - [ ] Add support sprite blocks (Like grass and such)
  - [x] Actually make the build height infinite (Render non-zero-y horizontal chunk layers as well)

- [ ] Improve world generation
  - [ ] Add caves
  - [ ] Improve everything
  - [ ] Add biomes
  - [ ] Generate in a spiral from the player outwards
