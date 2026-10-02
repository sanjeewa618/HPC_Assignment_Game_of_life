#ifndef GUI_H
#define GUI_H

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
#include <string>
#include <vector>
#include <functional>
#include "font8x8.h"
#include "patterns.h"

namespace UITheme {
    // Canvas & Modern Dark Slate Panels
    const SDL_Color BG_DARK        = {   5,   9,  16, 255 }; // Deep obsidian navy black
    const SDL_Color PANEL_BG       = {   9,  15,  25, 255 }; // Modern dark slate sidebar
    const SDL_Color CARD_BG        = {  13,  20,  32, 255 }; // Card container background (Image 1 style)
    const SDL_Color PANEL_BORDER   = {  24,  38,  58, 255 }; // Crisp modern blue border
    const SDL_Color CARD_BORDER    = {  26,  42,  64, 255 }; // Card container border
    const SDL_Color HEADER_BG      = {   8,  13,  22, 255 }; // Deep modern header plate

    // Button states (Blue Theme)
    const SDL_Color BTN_NORMAL     = {  16,  25,  38, 255 }; // Dark slate button
    const SDL_Color BTN_HOVER      = {  24,  38,  58, 255 }; // Blue glow hover
    const SDL_Color BTN_ACTIVE     = {   0, 212, 255, 255 }; // Electric Neon Cyan/Blue active (Ref Image 2)
    const SDL_Color BTN_ACTIVE_TXT = {   3,  10,  20, 255 }; // High contrast dark text on active
    const SDL_Color BTN_DANGER     = { 220,  50,  50, 255 }; // Crimson danger
    const SDL_Color BTN_BORDER     = {  32,  50,  74, 255 }; // Button edge outline

    // Typography & Accents (Blue theme)
    const SDL_Color TEXT_TITLE     = { 255, 255, 255, 255 }; // Pure white
    const SDL_Color TEXT_NORMAL    = { 220, 235, 245, 255 }; // Crisp cool white text
    const SDL_Color TEXT_MUTED     = { 115, 145, 175, 255 }; // Slate blue muted
    const SDL_Color ACCENT_BLUE    = {   0, 212, 255, 255 }; // Electric Neon Blue (#00D4FF)
    const SDL_Color ACCENT_CYAN    = {  56, 189, 248, 255 }; // Sky Blue (#38BDF8)
    const SDL_Color ACCENT_INDIGO  = { 129, 140, 248, 255 }; // Indigo
    const SDL_Color ACCENT_AMBER   = { 251, 191,  36, 255 }; // Warm Gold
}

enum class ToolMode {
    DRAW,
    ERASE,
    STAMP
};

class UIButton {
public:
    SDL_Rect rect;
    std::string text;
    std::function<void()> on_click;
    bool is_hovered = false;
    bool is_active = false;
    bool is_danger = false;
    bool custom_color = false;
    SDL_Color override_color = {0, 0, 0, 0};
    bool has_accent = false;
    SDL_Color accent_color = {0, 0, 0, 0};

    UIButton(int x, int y, int w, int h, const std::string& label, std::function<void()> callback = nullptr)
        : rect{x, y, w, h}, text(label), on_click(callback) {}

    UIButton(int x, int y, int w, int h, const std::string& label, SDL_Color accent, std::function<void()> callback = nullptr)
        : rect{x, y, w, h}, text(label), on_click(callback), has_accent(true), accent_color(accent) {}

    bool handle_event(const SDL_Event& e);
    void draw(SDL_Renderer* renderer, FontRenderer& font);
};

class GUI {
public:
    int window_width = 1280;
    int window_height = 768;

    // Layout areas
    SDL_Rect header_rect;
    SDL_Rect left_sidebar_rect;
    SDL_Rect right_sidebar_rect;
    SDL_Rect canvas_rect;
    SDL_Rect status_rect;

    // Start Screen state
    bool in_start_screen = true;
    bool in_transition   = false;  // playing enter animation
    Uint32 transition_start_ms = 0;
    static const int TRANSITION_MS = 800;

    // Speed Slider state
    SDL_Rect speed_slider_rect = { 0, 0, 0, 0 };
    bool is_dragging_slider = false;

    // Simulation state references
    bool is_running = true;
    int delay_ms = 16;
    ToolMode current_tool = ToolMode::DRAW;
    int brush_radius = 2; // Default 5x5 as shown in image
    int selected_pattern_idx = 0;
    std::vector<Pattern> patterns;

    // UI Buttons & indices
    std::vector<UIButton> buttons;
    int idx_btn_start = -1;
    int idx_btn_pause = -1;
    int idx_btn_reset = -1;
    int idx_clear = -1;
    int idx_draw = -1;
    int idx_erase = -1;
    int idx_stamp = -1;
    int idx_fill = -1;
    int idx_engine = -1;
    int idx_engine_hdr = -1;  // Switch Engine button in top header bar
    int idx_benchmark = -1;   // ⏱️ Run Benchmark (100 Gens) button
    std::vector<int> idx_brush_sizes;
    std::vector<int> idx_grid_sizes;
    std::vector<int> idx_patterns;
    std::vector<std::pair<int, int>> grid_size_presets = {
        {256, 256},
        {512, 512},
        {1024, 1024},
        {2048, 2048},
        {4096, 4096},
        {1920, 1080}
    };

    // Benchmark Telemetry
    bool has_benchmark_result = false;
    float last_benchmark_ms = 0.0f;
    float last_benchmark_per_gen = 0.0f;
    float last_benchmark_mcells = 0.0f;
    std::string benchmark_engine = "";
    int benchmark_grid_w = 0;
    int benchmark_grid_h = 0;

    // Status message
    std::string status_msg = "Ready. Left-click canvas to draw, drag to paint.";
    int mouse_grid_x = 0;
    int mouse_grid_y = 0;
    bool mouse_in_canvas = false;
    bool mouse_cell_alive = false;

    // Camera / Viewport
    float zoom = 1.0f;
    float pan_x = 0.0f;
    float pan_y = 0.0f;
    bool is_panning = false;
    int last_pan_mx = 0;
    int last_pan_my = 0;

    // Engine info
    std::string engine_name = "CPU (OpenMP)";
    bool cuda_capable = false;

    int current_grid_w = 512;
    int current_grid_h = 512;

    // Callbacks for main loop
    std::function<void()> on_step_clicked;
    std::function<void()> on_clear_clicked;
    std::function<void()> on_randomize_clicked;
    std::function<void()> on_toggle_engine;
    std::function<void()> on_run_benchmark;
    std::function<void()> on_reset_view;
    std::function<void()> on_fit_canvas;
    std::function<void(int new_w, int new_h)> on_change_grid_size;

    GUI();
    void init(int win_w, int win_h);
    void update_layout(int win_w, int win_h);

    void fit_to_canvas(int grid_w, int grid_h);
    void reset_view(int grid_w, int grid_h);

    bool handle_event(const SDL_Event& e);
    void draw(SDL_Renderer* renderer, FontRenderer& font, unsigned long long gen, int population, float fps, int grid_w, int grid_h);

    // Coordinate conversion
    void screen_to_grid(int screen_x, int screen_y, int grid_w, int grid_h, int& grid_x, int& grid_y) const;
    void grid_to_screen(int grid_x, int grid_y, int& screen_x, int& screen_y) const;
};

#endif // GUI_H
