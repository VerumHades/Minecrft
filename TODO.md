# TODO

- [ ] Finish making UI
  - [ ] Main Menu
    - [ ] World creation menu
  - [ ] Pause Menu
    - [ ] Pause Settings
    - [ ] Exit
  - [ ] Settings
    - [ ] Graphics Settings
  - [ ] Console
    - [ ] Add relevant normal and debug commands

- [ ] Improve Graphics
  - [x] Fix shadows
  - [ ] Add ambient occlusion
  - [ ] Add support for transparency

- [ ] Improve rendering
  - [x] Batch render chunks
  - [x] Add double buffering to batched chunks
  - [ ] Fix mesh buffer running out of memory
    - [ ] Free meshes when out of view if neccesary
  - [ ] Free chunk meshes that are far away

- [ ] Add missing features:
  - [ ] Add support for non solid blocks (Stairs and such)
  - [ ] Add support sprite blocks (Like grass and such)
  - [x] Actually make the build height infinite (Render non-zero-y horizontal chunk layers as well)

- [ ] Improve world generation
  - [ ] Add caves
  - [ ] Improve everything
  - [ ] Add biomes
