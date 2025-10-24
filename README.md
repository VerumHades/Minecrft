
# Minecraft Clone (C++ / OpenGL)

> Built from scratch in C++ using modern OpenGL — **no game engine**.

## Project Overview

This is a high-performance Minecraft-style voxel game, designed to push the limits of rendering speed and modular game design. It is inspired by the original Minecraft's core mechanics and block-based world, but rewritten from the ground up to achieve exceptional performance and flexibility.

The project implements several advanced graphics techniques and systems to ensure optimal speed and quality — reaching **1000+ FPS** on modern hardware.

---

## Features

### Rendering Techniques

* **LOD (Level of Detail):** Optimizes distant chunks with simplified geometry.
* **Instanced Rendering:** Efficiently draws thousands of blocks with minimal overhead.
* **Indirect Rendering:** Batches draw calls for reduced CPU-GPU sync.
* **Binary Greedy Meshing:** Reduces face count by merging adjacent blocks into larger faces.
* **Binary Ambient Lighting:** Simulates soft light without the cost of full dynamic lighting.

### World Mechanics

* Block placing and destruction
* World saving and loading
* Procedural terrain generation with seeds

### Crafting & Inventory

* Fully configurable crafting system
* Inventory UI and item management
* Tool effectivity on different materials changes

---

## Performance

Designed with performance in mind:

* Written entirely in **C++**
* Uses **OpenGL** directly (no intermediary game engine)
* Optimized data structures for mesh generation and rendering


## Running the game

### Installer

### Windows
Steps:
- Download installer [here](https://github.com/VerumHades/MinecrftRelease/releases/latest) 
- Install
- Play the game!

### Build Instructions

> **Note:** Make sure you have a modern C++ compiler and OpenGL development libraries installed. Download cbuild [here](https://github.com/VerumHades/cbuild)


```bash
git clone https://github.com/VerumHades/Minecrft
cd Minecrft
cbuild -ra majnkraft # To build and run
```

### Dependencies

All of the dependencies are already included in the cmake file using the FetchContent utility.

* C++20 or later
* OpenGL 4.5+
* GLFW
* GLM
* stb\_image
