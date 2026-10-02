#include <iostream>
#include <iomanip>
#include <string>
#include <memory>
#include <chrono>
#include <algorithm>
#define SDL_MAIN_HANDLED
#if defined(__has_include)
  #if __has_include(<SDL2/SDL.h>)
    #include <SDL2/SDL.h>
  #elif __has_include(<SDL.h>)
    #include <SDL.h>
  #else
    #include <SDL2/SDL.h>
  #endif
#else
  #include <SDL2/SDL.h>
#endif

#include "engine.h"
#include "font8x8.h"
#include "gui.h"

int main(int argc, char* argv[]) {
    std::cout << "========================================================\n"
              << "       Conway's Game of Life - Interactive GUI\n"
              << "========================================================\n";

    // 0. Headless CLI Benchmark Mode Check
    if (argc >= 2 && (std::string(argv[1]) == "--benchmark" || std::string(argv[1]) == "-b")) {
        int b_grid = (argc >= 3) ? std::atoi(argv[2]) : 2048;
        int b_iters = (argc >= 4) ? std::atoi(argv[3]) : 100;
        if (b_grid <= 0) b_grid = 2048;
        if (b_iters <= 0) b_iters = 100;

        std::cout << "\n============================================================\n";
        std::cout << "           HPC BENCHMARK SUITE: GAME OF LIFE\n";
        std::cout << "============================================================\n";
        std::cout << "Grid Dimension : " << b_grid << " x " << b_grid << " (" << ((long long)b_grid * b_grid) << " cells)\n";
        std::cout << "Iterations     : " << b_iters << " generations\n\n";

        // [1/2] Benchmark CPU Engine
        std::cout << ">> [1/2] Benchmarking CPU Engine (OpenMP Multi-Core)...\n";
        auto cpu_eng = create_simulation_engine(false, b_grid, b_grid);
        cpu_eng->randomize(20, 12345);
        for (int i = 0; i < 3; ++i) cpu_eng->step(); // Warmup

        auto cpu_t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < b_iters; ++i) {
            cpu_eng->step();
        }
        auto cpu_t1 = std::chrono::high_resolution_clock::now();
        double cpu_ms = std::chrono::duration<double, std::milli>(cpu_t1 - cpu_t0).count();
        double cpu_per_gen = cpu_ms / (double)b_iters;
        double cpu_mc = ((double)b_grid * b_grid * (double)b_iters) / (cpu_ms * 1000.0);

        std::cout << "   - Total Time : " << std::fixed << std::setprecision(2) << cpu_ms << " ms\n";
        std::cout << "   - Avg / Gen  : " << std::fixed << std::setprecision(4) << cpu_per_gen << " ms\n";
        std::cout << "   - Throughput : " << std::fixed << std::setprecision(2) << cpu_mc << " MCells/s\n\n";

        // [2/2] Benchmark GPU Engine
        bool cuda_ok = is_cuda_available();
        if (cuda_ok) {
            std::cout << ">> [2/2] Benchmarking GPU Engine (NVIDIA CUDA)...\n";
            auto gpu_eng = create_simulation_engine(true, b_grid, b_grid);
            gpu_eng->randomize(20, 12345);
            for (int i = 0; i < 3; ++i) gpu_eng->step(); // Warmup

            auto gpu_t0 = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < b_iters; ++i) {
                gpu_eng->step();
            }
            auto gpu_t1 = std::chrono::high_resolution_clock::now();
            double gpu_ms = std::chrono::duration<double, std::milli>(gpu_t1 - gpu_t0).count();
            double gpu_per_gen = gpu_ms / (double)b_iters;
            double gpu_mc = ((double)b_grid * b_grid * (double)b_iters) / (gpu_ms * 1000.0);

            std::cout << "   - Total Time : " << std::fixed << std::setprecision(2) << gpu_ms << " ms\n";
            std::cout << "   - Avg / Gen  : " << std::fixed << std::setprecision(4) << gpu_per_gen << " ms\n";
            std::cout << "   - Throughput : " << std::fixed << std::setprecision(2) << gpu_mc << " MCells/s\n\n";
            std::cout << "------------------------------------------------------------\n";
            std::cout << "⚡ GPU SPEEDUP FACTOR: " << std::fixed << std::setprecision(2) << (cpu_ms / gpu_ms) << "x FASTER\n";
        } else {
            std::cout << ">> [2/2] GPU Engine (CUDA): Not available in this build / hardware.\n";
        }
        std::cout << "============================================================\n\n";
        return 0;
    }

    // 1. Grid Resolution (defaults to 512x512)
    int grid_w = 512;
    int grid_h = 512;
    if (argc >= 3) {
        grid_w = std::atoi(argv[1]);
        grid_h = std::atoi(argv[2]);
    }
    if (grid_w <= 0) grid_w = 512;
    if (grid_h <= 0) grid_h = 512;

    std::cout << "[INFO] Simulation Grid Size: " << grid_w << " x " << grid_h << "\n";

    // 2. Hardware Acceleration Check & Simulation Engine Initialization
    bool has_cuda = is_cuda_available();
    std::cout << "[INFO] CUDA GPU Acceleration: " << (has_cuda ? "Available" : "Not Detected") << "\n";

    std::unique_ptr<SimulationEngine> engine = create_simulation_engine(has_cuda, grid_w, grid_h);
    if (!engine) {
        std::cerr << "[ERROR] Could not initialize simulation engine!\n";
        return 1;
    }

    // Seed initial state
    engine->randomize(20, (unsigned int)time(nullptr));

    // 3. Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "[ERROR] SDL Initialization failed: " << SDL_GetError() << "\n";
        return 1;
    }

    int win_w = 1280;
    int win_h = 768;

    SDL_Window* window = SDL_CreateWindow(
        "Conway's Game of Life [Hardware Accelerated]",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "[ERROR] Window creation failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        // Fallback to software renderer if hardware acceleration is unavailable
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        std::cerr << "[ERROR] Renderer creation failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 4. Initialize Font Renderer
    FontRenderer font;
    if (!font.init(renderer)) {
        std::cerr << "[ERROR] Failed to initialize built-in font atlas!\n";
    }

    // 5. Initialize GUI & Layout
    GUI gui;
    gui.current_grid_w = grid_w;
    gui.current_grid_h = grid_h;
    gui.init(win_w, win_h);
    gui.cuda_capable = has_cuda;
    gui.engine_name = engine->is_gpu() ? "CUDA (GPU)" : "CPU (OpenMP)";

    // Forward declare grid_texture pointer
    SDL_Texture* grid_texture = nullptr;

    // Connect GUI Callbacks
    unsigned long long generation = 0;

    gui.on_change_grid_size = [&](int new_w, int new_h) {
        if (new_w == grid_w && new_h == grid_h) return;

        std::cout << "[INFO] Resizing simulation grid: " << grid_w << "x" << grid_h
                  << " -> " << new_w << "x" << new_h << "\n";

        grid_w = new_w;
        grid_h = new_h;
        gui.current_grid_w = grid_w;
        gui.current_grid_h = grid_h;

        engine->init(grid_w, grid_h);
        engine->randomize(20, (unsigned int)time(nullptr));
        generation = 0;

        if (grid_texture) {
            SDL_DestroyTexture(grid_texture);
        }
        grid_texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            grid_w, grid_h
        );

        gui.fit_to_canvas(grid_w, grid_h);

        long long total_cells = (long long)grid_w * grid_h;
        std::ostringstream ss;
        ss << "Grid resized: " << grid_w << "x" << grid_h << " (";
        if (total_cells >= 1000000) {
            ss << std::fixed << std::setprecision(1) << ((double)total_cells / 1000000.0) << "M cells)! Wheel: zoom, Drag: pan.";
        } else {
            ss << (total_cells / 1000) << "K cells)! Wheel: zoom, Drag: pan.";
        }
        gui.status_msg = ss.str();
    };

    gui.on_step_clicked = [&]() {
        engine->step();
        generation++;
    };

    gui.on_clear_clicked = [&]() {
        engine->clear();
        generation = 0;
    };

    gui.on_randomize_clicked = [&]() {
        engine->randomize(20, (unsigned int)std::chrono::high_resolution_clock::now().time_since_epoch().count());
        generation = 0;
    };

    gui.on_toggle_engine = [&]() {
#ifndef USE_CUDA
        // This binary was compiled WITHOUT CUDA support.
        // To enable GPU acceleration, recompile using: mingw32-make cuda
        gui.status_msg = "[CPU Build] Recompile with 'make cuda' to enable NVIDIA GPU acceleration.";
        gui.engine_name = "CPU (OpenMP)";
        gui.cuda_capable = false;
        return;
#else
        if (!is_cuda_available()) {
            gui.status_msg = "No NVIDIA GPU detected on this machine. Staying on CPU engine.";
            gui.engine_name = "CPU (OpenMP)";
            gui.cuda_capable = false;
            return;
        }

        bool switch_to_gpu = !engine->is_gpu();
        std::cout << "[INFO] Switching engine -> " << (switch_to_gpu ? "CUDA GPU" : "CPU (OpenMP)") << "\n";

        // Backup current grid state so it survives the engine switch
        const unsigned char* old_state = engine->get_grid_state();
        std::vector<unsigned char> state_backup(old_state, old_state + (size_t)grid_w * grid_h);

        // Create new engine and restore state
        engine = create_simulation_engine(switch_to_gpu, grid_w, grid_h);
        engine->set_grid_state(state_backup.data());

        // Rebuild SDL streaming texture (dimensions same, but engine pointer changed)
        if (grid_texture) {
            SDL_DestroyTexture(grid_texture);
            grid_texture = nullptr;
        }
        grid_texture = SDL_CreateTexture(
            renderer, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING, grid_w, grid_h
        );

        gui.engine_name  = engine->is_gpu() ? "CUDA (GPU)" : "CPU (OpenMP)";
        gui.cuda_capable = engine->is_gpu();
        gui.status_msg   = std::string("Engine switched to: ") + engine->get_name();
        std::cout << "[INFO] Switch complete. Engine: " << engine->get_name() << "\n";
#endif
    };

    gui.on_run_benchmark = [&]() {
        const int iters = 100;
        gui.status_msg = "Running benchmark (100 gens) on " + gui.engine_name + "...";

        auto t_start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; ++i) {
            engine->step();
        }
        auto t_end = std::chrono::high_resolution_clock::now();

        double total_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
        double per_gen_ms = total_ms / (double)iters;
        double mcells_per_sec = ((double)grid_w * grid_h * (double)iters) / (total_ms * 1000.0);

        generation += iters;

        gui.has_benchmark_result = true;
        gui.last_benchmark_ms = (float)total_ms;
        gui.last_benchmark_per_gen = (float)per_gen_ms;
        gui.last_benchmark_mcells = (float)mcells_per_sec;
        gui.benchmark_engine = gui.engine_name;
        gui.benchmark_grid_w = grid_w;
        gui.benchmark_grid_h = grid_h;

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << "[Benchmark: " << gui.engine_name << " " << grid_w << "x" << grid_h << " (100 gens)]: "
           << total_ms << " ms (" << per_gen_ms << " ms/gen | " << std::setprecision(1) << mcells_per_sec << " MCells/s)";
        gui.status_msg = ss.str();

        std::cout << "\n=======================================================" << std::endl;
        std::cout << "⏱️  HPC BENCHMARK RESULTS (Game of Life)" << std::endl;
        std::cout << "-------------------------------------------------------" << std::endl;
        std::cout << "Compute Engine : " << gui.engine_name << std::endl;
        std::cout << "Grid Dimension : " << grid_w << " x " << grid_h << " (" << (grid_w * grid_h) << " cells)" << std::endl;
        std::cout << "Iterations     : " << iters << " generations" << std::endl;
        std::cout << "Total Time     : " << std::fixed << std::setprecision(2) << total_ms << " ms" << std::endl;
        std::cout << "Time / Gen     : " << std::fixed << std::setprecision(4) << per_gen_ms << " ms" << std::endl;
        std::cout << "Throughput     : " << std::fixed << std::setprecision(2) << mcells_per_sec << " Million Cells/sec" << std::endl;
        std::cout << "=======================================================\n" << std::endl;
    };

    gui.on_reset_view = [&]() {
        gui.reset_view(grid_w, grid_h);
        gui.status_msg = "View reset to 100% scale.";
    };

    gui.on_fit_canvas = [&]() {
        gui.fit_to_canvas(grid_w, grid_h);
        gui.status_msg = "Fit entire grid inside canvas.";
    };

    // 6. Create SDL Streaming Texture for the Grid
    grid_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        grid_w, grid_h
    );
    if (!grid_texture) {
        std::cerr << "[ERROR] Texture creation failed: " << SDL_GetError() << "\n";
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 7. Simulation & Event Loop
    bool quit = false;
    bool mouse_drawing = false;
    bool mouse_erasing = false;

    auto last_fps_time = std::chrono::steady_clock::now();
    int frame_count = 0;
    float current_fps = 60.0f;

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            }

            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
                win_w = e.window.data1;
                win_h = e.window.data2;
                gui.update_layout(win_w, win_h);
                continue;
            }

            // Let GUI process the event first
            if (gui.handle_event(e)) {
                continue;
            }

            // Keyboard Shortcuts
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_SPACE:
                        gui.is_running = !gui.is_running;
                        gui.status_msg = gui.is_running ? "Simulation running." : "Simulation paused.";
                        break;
                    case SDLK_s:
                        if (!gui.is_running) {
                            engine->step();
                            generation++;
                            gui.status_msg = "Stepped 1 generation.";
                        }
                        break;
                    case SDLK_r:
                        engine->randomize(20, (unsigned int)std::chrono::high_resolution_clock::now().time_since_epoch().count());
                        generation = 0;
                        gui.status_msg = "Grid randomized.";
                        break;
                    case SDLK_c:
                        engine->clear();
                        generation = 0;
                        gui.status_msg = "Grid cleared.";
                        break;
                    case SDLK_EQUALS:
                    case SDLK_PLUS:
                    case SDLK_UP:
                        gui.delay_ms = std::max(0, gui.delay_ms - 4);
                        gui.status_msg = "Speed set to " + std::to_string(gui.delay_ms) + " ms delay.";
                        break;
                    case SDLK_MINUS:
                    case SDLK_DOWN:
                        gui.delay_ms = std::min(200, gui.delay_ms + 4);
                        gui.status_msg = "Speed set to " + std::to_string(gui.delay_ms) + " ms delay.";
                        break;
                    case SDLK_1:
                        if (gui.on_change_grid_size) gui.on_change_grid_size(256, 256);
                        break;
                    case SDLK_2:
                        if (gui.on_change_grid_size) gui.on_change_grid_size(512, 512);
                        break;
                    case SDLK_3:
                        if (gui.on_change_grid_size) gui.on_change_grid_size(1024, 1024);
                        break;
                    case SDLK_4:
                        if (gui.on_change_grid_size) gui.on_change_grid_size(2048, 2048);
                        break;
                    case SDLK_5:
                        if (gui.on_change_grid_size) gui.on_change_grid_size(4096, 4096);
                        break;
                    case SDLK_6:
                        if (gui.on_change_grid_size) gui.on_change_grid_size(1920, 1080);
                        break;
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        quit = true;
                        break;
                    default:
                        break;
                }
            }

            // Mouse Canvas Interaction
            if (gui.mouse_in_canvas) {
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        mouse_drawing = true;
                        int gx, gy;
                        gui.screen_to_grid(e.button.x, e.button.y, grid_w, grid_h, gx, gy);
                        if (gui.current_tool == ToolMode::DRAW) {
                            engine->set_brush(gx, gy, gui.brush_radius, 1);
                        } else if (gui.current_tool == ToolMode::ERASE) {
                            engine->set_brush(gx, gy, gui.brush_radius, 0);
                        } else if (gui.current_tool == ToolMode::STAMP) {
                            engine->spawn_pattern(gx, gy, gui.patterns[gui.selected_pattern_idx]);
                        }
                    } else if (e.button.button == SDL_BUTTON_RIGHT) {
                        mouse_erasing = true;
                        int gx, gy;
                        gui.screen_to_grid(e.button.x, e.button.y, grid_w, grid_h, gx, gy);
                        engine->set_brush(gx, gy, gui.brush_radius, 0);
                    }
                } else if (e.type == SDL_MOUSEBUTTONUP) {
                    if (e.button.button == SDL_BUTTON_LEFT) mouse_drawing = false;
                    if (e.button.button == SDL_BUTTON_RIGHT) mouse_erasing = false;
                } else if (e.type == SDL_MOUSEMOTION) {
                    int gx, gy;
                    gui.screen_to_grid(e.motion.x, e.motion.y, grid_w, grid_h, gx, gy);
                    gui.mouse_grid_x = gx;
                    gui.mouse_grid_y = gy;

                    // Query hover cell state
                    const unsigned char* grid_ptr = engine->get_grid_state();
                    if (grid_ptr && gx >= 0 && gx < grid_w && gy >= 0 && gy < grid_h) {
                        gui.mouse_cell_alive = (grid_ptr[gy * grid_w + gx] != 0);
                    }

                    if (mouse_drawing) {
                        if (gui.current_tool == ToolMode::DRAW) {
                            engine->set_brush(gx, gy, gui.brush_radius, 1);
                        } else if (gui.current_tool == ToolMode::ERASE) {
                            engine->set_brush(gx, gy, gui.brush_radius, 0);
                        }
                    } else if (mouse_erasing) {
                        engine->set_brush(gx, gy, gui.brush_radius, 0);
                    }
                }
            }
        }

        // Advance simulation
        if (gui.is_running) {
            engine->step();
            generation++;
        }

        // Transfer grid pixels to SDL texture
        const unsigned int* pixel_buffer = engine->get_pixels();
        SDL_UpdateTexture(grid_texture, nullptr, pixel_buffer, grid_w * sizeof(unsigned int));

        // Render Canvas & Grid
        SDL_SetRenderDrawColor(renderer, UITheme::BG_DARK.r, UITheme::BG_DARK.g, UITheme::BG_DARK.b, 255);
        SDL_RenderClear(renderer);

        // Clip grid rendering to the canvas area so it doesn't bleed into sidebar or header
        SDL_RenderSetClipRect(renderer, &gui.canvas_rect);

        float cell_pixel_size = (float)gui.canvas_rect.w / (float)grid_w * gui.zoom;
        SDL_Rect grid_dest = {
            gui.canvas_rect.x + (int)gui.pan_x,
            gui.canvas_rect.y + (int)gui.pan_y,
            std::max(1, (int)((float)grid_w * cell_pixel_size)),
            std::max(1, (int)((float)grid_h * cell_pixel_size))
        };
        SDL_RenderCopy(renderer, grid_texture, nullptr, &grid_dest);

        // Reset clip rect before rendering GUI
        SDL_RenderSetClipRect(renderer, nullptr);

        // Calculate Population & FPS
        int population = engine->get_population();

        frame_count++;
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = now - last_fps_time;
        if (elapsed.count() >= 0.5f) {
            current_fps = (float)frame_count / elapsed.count();
            frame_count = 0;
            last_fps_time = now;
        }

        // Draw GUI overlay (Header, Sidebar, Controls, Status Bar)
        gui.draw(renderer, font, generation, population, current_fps, grid_w, grid_h);

        SDL_RenderPresent(renderer);

        if (gui.delay_ms > 0) {
            SDL_Delay(gui.delay_ms);
        }
    }

    // Cleanup
    SDL_DestroyTexture(grid_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "[INFO] Application closed cleanly. Generations computed: " << generation << "\n";
    return 0;
}
