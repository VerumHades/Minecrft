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

- [ ] Add missing features:
  - [ ] Add support for non solid blocks (Stairs and such)
  - [ ] Add support sprite blocks (Like grass and such)
  - [x] Actually make the build height infinite (Render non-zero-y horizontal chunk layers as well)

- [ ] Improve world generation
  - [ ] Add caves
  - [ ] Improve everything
  - [ ] Add biomes
