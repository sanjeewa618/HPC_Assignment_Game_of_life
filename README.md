# High Performance Computing (HPC): Parallel Conway's Game of Life

A high-performance, real-time 2D Cellular Automaton simulation engine developed in **C++14**, featuring dual compute backends (**OpenMP Multi-Core CPU** and **NVIDIA CUDA GPU**) paired with an interactive **SDL2** graphical user interface.

Designed to scale seamlessly from small configurations up to massive **$4096 \times 4096$ (16.8 Million Cells)** grids while maintaining interactive 60+ FPS performance.

---

## ⚡ Quick Start (Run Locally)

### 🪟 On Windows (via MSYS2 MINGW64 Terminal)
```bash
# 1. Install dependencies (one-time setup if needed)
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-SDL2 make

# 2. Navigate to project directory
cd "/c/Users/DELL/Documents/5th semester/HPC/HPC_Assignment_Game_of_life"

# 3A. CPU (OpenMP Multi-Core) build & run
mingw32-make clean
mingw32-make cpu
./bin/game_of_life.exe

# 3B. CUDA + NVIDIA GPU build (auto-detects nvcc)
mingw32-make clean
mingw32-make cuda
./bin/game_of_life.exe
```

---

### 🐧 On Linux / WSL (Ubuntu)
```bash
# 1. Install dependencies (one-time setup)
sudo apt update && sudo apt install build-essential libsdl2-dev make

# 2. Build & Run
make clean
make cpu
./bin/game_of_life
```

---

## 📌 Table of Contents

- [Quick Start (Run Locally)](#-quick-start-run-locally)
- [Overview & Problem Statement](#-overview--problem-statement)
- [System Architecture & HPC Design](#-system-architecture--hpc-design)
  - [1. Multi-Core CPU Engine (OpenMP)](#1-multi-core-cpu-engine-openmp)
  - [2. Massive Parallelism on GPU (CUDA)](#2-massive-parallelism-on-gpu-cuda)
  - [3. Memory Management & Texture Pipeline](#3-memory-management--texture-pipeline)
- [Key Capabilities & Features](#-key-capabilities--features)
- [Project Directory Structure](#-project-directory-structure)
- [Prerequisites & Dependencies](#-prerequisites--dependencies)
- [Build & Run Instructions](#-build--run-instructions)
- [User Interface & Keybindings](#-user-interface--keybindings)
- [Pattern Presets Reference](#-pattern-presets-reference)

---

## 🔬 Overview & Problem Statement

**Conway's Game of Life** is a zero-player cellular automaton governed by simple local rules on a 2D orthogonal grid:

1. **Underpopulation:** Any live cell with fewer than 2 live neighbors dies.
2. **Survival:** Any live cell with 2 or 3 live neighbors lives on to the next generation.
3. **Overpopulation:** Any live cell with more than 3 live neighbors dies.
4. **Reproduction:** Any dead cell with exactly 3 live neighbors becomes a live cell.

### Computational Challenge
Each generation requires evaluating a 2D Moore neighborhood ($3 \times 3$ stencil) for every cell with **toroidal (periodic) boundary wrapping**. For an $N \times M$ grid:
- **Time Complexity:** $\mathcal{O}(N \times M)$ evaluations per generation step.
- At $4096 \times 4096$, calculating a single generation requires **16,777,216 stencil evaluations** ($\sim 134 \times 10^6$ memory lookups).

This project implements parallel stencil computation and reduction techniques across modern multi-core CPUs and SIMT GPU architectures to achieve real-time simulation speeds.

---

## ⚡ System Architecture & HPC Design

The simulation uses an object-oriented polymorphic design where the core computation is abstracted behind the `SimulationEngine` interface.

```
                  +-----------------------------------+
                  |         Main Application          |
                  |     (SDL2 Event Loop & GUI)       |
                  +-----------------+-----------------+
                                    |
                       [SimulationEngine Interface]
                                    |
          +-------------------------+-------------------------+
          |                                                   |
+---------v-------------------+             +-----------------v-------------------+
|       CPUEngine (OpenMP)    |             |         CUDAEngine (NVCC)           |
+-----------------------------+             +-------------------------------------+
| - Static 1D Row Partition   |             | - 2D Grid/Block Decomposition       |
| - Boundary Separation       |             | - Parallel Tree Reduction (Shared)  |
| - Vectorized Inner Loops    |             | - Device ARGB Pixel Blitting        |
| - OpenMP Reduction Clauses  |             | - Pinned Host Memory Transfers      |
+-----------------------------+             +-------------------------------------+
```

### 1. Multi-Core CPU Engine (OpenMP)
- **1D Domain Decomposition:** Work is partitioned across CPU threads by rows using `#pragma omp parallel for schedule(static)`.
- **Zero-Modulo Fast Inner Loop:** Periodic boundary wrapping (modulo operations) is extracted away from the inner compute loop:
  - Separate passes for the leftmost column ($x = 0$) and rightmost column ($x = W - 1$).
  - Interior columns ($1 \le x < W - 1$) execute a pure branch-free contiguous memory stencil traversal.
- **SIMD Vectorization:** Inner loops are decorated with `#pragma GCC ivdep` to encourage vectorization across SSE/AVX registers.
- **Parallel Reduction:** Live cell population counting is calculated concurrently via OpenMP `reduction(+:alive_count)` without lock contention.
- **Thread-Local Random Initialization:** Concurrent linear congruential pseudorandom generation per thread.

### 2. Massive Parallelism on GPU (CUDA)
- **2D Block Decomposition:** Threads are structured into 2D thread blocks ($16 \times 16 = 256$ threads/block) mapped directly to coordinates $(x, y)$.
- **Loop Unrolling:** Moore neighborhood offsets are unrolled with `#pragma unroll` to minimize branch latency.
- **Shared Memory Reduction:** Population tracking uses a tree-based parallel reduction in CUDA shared memory (`__shared__ int s_data[256]`) across thread blocks.
- **On-Device Pixel Generation:** Cell states are transformed into 32-bit ARGB screen pixels directly inside GPU VRAM via `cuda_generate_pixels_kernel`, minimizing host-device payload.
- **GPU-Side Brush & Randomizer:** In-place grid edits and fast integer hash randomizations execute entirely on the GPU without host staging.

### 3. Memory Management & Texture Pipeline
- **Pinned Host Memory:** Host memory buffers leverage `cudaMallocHost` for page-locked transfers, maximizing PCIe bus throughput.
- **Double Buffering:** Alternating `current_state` and `next_state` buffers prevent data hazards without dynamic reallocations.
- **Seamless Engine Switching:** Grid states can be dynamically migrated between CPU and GPU memory at runtime without interrupting the simulation.

---

## 🌟 Key Capabilities & Features

| Feature | Details |
| :--- | :--- |
| **Dual Compute Backends** | Automatic hardware detection for CUDA GPU acceleration; falls back to OpenMP CPU. |
| **Dynamic Resizing** | Live grid dimension scaling ($256^2$ to $4096^2$) from GUI or shortcuts on-the-fly. |
| **Interactive Viewport** | Smooth mouse-centered zoom (10% – 3200%), panning, and 1:1 view reset. |
| **Built-in Drawing Suite** | Interactive Pen, Eraser, configurable brush radii (1px, 3px, 5px). |
| **Pattern Stamp Library** | Real-time translucent placement preview with pre-configured Conway patterns. |
| **Self-Contained GUI** | Integrated dark-mode UI with embedded 8x8 bitmap font rendering (zero external font dependencies). |
| **Live Telemetry** | Real-time HUD showing Engine, FPS, Generation index, Population count, and Cursor coordinates. |

---

## 📁 Project Directory Structure

```
.
├── include/                  # Modular C++ Header files
│   ├── engine.h              # Abstract SimulationEngine interface & CPU engine definition
│   ├── engine_cuda.h         # CUDA engine and GPU kernel declarations
│   ├── font8x8.h             # Embedded bitmap font atlas & SDL2 font blitter
│   ├── gui.h                 # User interface components, layout, and camera math
│   └── patterns.h            # Predefined pattern matrix definitions
├── src/                      # Source implementations
│   ├── engine_cpu.cpp        # OpenMP parallelized CPU implementation
│   ├── engine_cuda.cu        # CUDA GPU kernels and host-device memory transfers
│   ├── engine_factory.cpp    # Engine instantiation & hardware capability detection
│   ├── font8x8.cpp           # Bitmap font lookup tables
│   ├── gui.cpp               # UI widget rendering, buttons, and camera controls
│   └── main.cpp              # Application entry point, event pump, and main loop
├── assets/                   # Application icons and metadata
├── Makefile                  # Build system with auto-hardware detection
├── install.sh                # Desktop environment integration script
└── uninstall.sh              # Application removal script
```

---

## 🛠️ Prerequisites & Dependencies

### Required Packages
- **C++ Compiler:** `g++` (C++14 standard support)
- **OpenMP Library:** Built into GCC/Clang (`-fopenmp`)
- **Graphics Library:** `SDL2` development libraries (`libsdl2-dev`)
- **Build Tool:** `make`

### Optional (For CUDA GPU Acceleration)
- **NVIDIA CUDA Toolkit:** `nvcc` compiler and CUDA Runtime (`libcudart`)
- **GPU:** NVIDIA Compute Capability 3.5+

### Installation Commands (Ubuntu / Debian Linux)
```bash
sudo apt update
sudo apt install build-essential libsdl2-dev
```
*(If using an NVIDIA GPU with CUDA, install the `nvidia-cuda-toolkit`)*

---

## 🚀 Build & Run Instructions

### 1. Automatic Compilation (Recommended)
The build system automatically detects whether `nvcc` is present on the system PATH and compiles the highest performance target available:

```bash
# Build binary
make

# Run simulation
./game_of_life
```

You can pass custom grid dimensions via command-line arguments:
```bash
./game_of_life 1920 1080
```

---

### 2. Explicit Compilation Targets

To manually choose the compute backend:

- **Force Multi-Threaded CPU Engine (OpenMP):**
  - *Linux / WSL:*
    ```bash
    make cpu
    ./game_of_life
    ```
  - *Windows (MSYS2 MINGW64):*
    ```bash
    mingw32-make cpu
    ./bin/game_of_life.exe
    ```

- **Force CUDA GPU Accelerated Engine:**
  - *Linux / WSL:*
    ```bash
    make cuda
    ./game_of_life
    ```
  - *Windows (MSYS2 MINGW64):*
    ```bash
    mingw32-make cuda
    ./bin/game_of_life.exe
    ```

---

### 3. Automated HPC Benchmark Mode (Headless / CLI)

To measure exact compute execution time across 100+ iterations without opening the graphical window (ideal for research tables and speedup graphs):

```bash
# Syntax: ./bin/game_of_life.exe --benchmark [Grid_Width] [Iterations]
./bin/game_of_life.exe --benchmark 2048 100
```

---

### 4. Desktop Installation (Optional)
To register the application in your desktop menu with an icon shortcut:

```bash
make install
```
To remove the desktop installation:
```bash
make uninstall
```

---

## 🎮 User Interface & Keybindings

### Mouse Controls
| Interaction | Function |
| :--- | :--- |
| **Left Click / Drag** | Paint alive cells (Pen mode) / Place selected stamp pattern |
| **Right Click / Drag** | Erase cells (Kill alive cells) |
| **Middle Click / Drag** | Pan canvas viewport across large grids |
| **Mouse Wheel** | Zoom in / Zoom out centered on cursor position |

### Keyboard Shortcuts
| Shortcut | Action |
| :--- | :--- |
| `[Space]` | Play / Pause simulation |
| `[S]` | Single-step 1 generation (while paused) |
| `[R]` | Randomize grid (20% alive density) |
| `[C]` | Clear grid (reset population to 0) |
| `[+]` / `[=]` / `[Up]` | Increase simulation step speed |
| `[-]` / `[Down]` | Decrease simulation step speed |
| `[1]` – `[6]` | Quick grid presets (`256x256`, `512x512`, `1024x1024`, `2048x2048`, `4096x4096`, `1920x1080`) |
| `[Esc]` / `[Q]` | Exit application |

---

## 🧬 Pattern Presets Reference

The application includes interactive stamp templates for classic Game of Life structures:

| Category | Pattern | Description |
| :--- | :--- | :--- |
| **Spaceships** | `Glider` | Compact 5-cell structure that translates diagonally across the grid. |
| **Guns** | `Gosper Glider Gun` | Periodic oscillator that indefinitely generates and launches gliders. |
| **Oscillators** | `Pulsar` | Period-3 symmetric oscillator. |
| **Oscillators** | `Pentadecathlon` | Period-15 linear oscillator. |
| **Methuselahs** | `Acorn` | 7-cell seed requiring 5,206 generations to stabilize into gliders and still lifes. |
| **Methuselahs** | `Diehard` | 7-cell pattern that completely disappears after 130 generations. |
