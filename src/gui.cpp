#include "gui.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

bool UIButton::handle_event(const SDL_Event& e) {
    if (e.type == SDL_MOUSEMOTION) {
        int mx = e.motion.x;
        int my = e.motion.y;
        is_hovered = (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h);
    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
        if (e.button.button == SDL_BUTTON_LEFT) {
            int mx = e.button.x;
            int my = e.button.y;
            if (mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h) {
                if (on_click) on_click();
                return true; // Click consumed
            }
        }
    }
    return false;
}

void UIButton::draw(SDL_Renderer* renderer, FontRenderer& font) {
    SDL_Color fill = UITheme::BTN_NORMAL;
    SDL_Color border = UITheme::BTN_BORDER;
    SDL_Color text_col = UITheme::TEXT_NORMAL;

    if (custom_color) {
        fill = override_color;
        border = is_hovered ? UITheme::ACCENT_BLUE : UITheme::BTN_BORDER;
        text_col = { 255, 255, 255, 255 };
    } else if (is_active) {
        fill = has_accent ? accent_color : UITheme::BTN_ACTIVE;
        border = { 112, 228, 255, 255 };
        text_col = UITheme::BTN_ACTIVE_TXT;
    } else if (is_danger) {
        fill = is_hovered ? SDL_Color{ 220, 50, 50, 255 } : UITheme::BTN_NORMAL;
        border = is_hovered ? SDL_Color{ 255, 90, 90, 255 } : UITheme::BTN_BORDER;
        text_col = is_hovered ? SDL_Color{ 255, 255, 255, 255 } : UITheme::TEXT_NORMAL;
    } else if (is_hovered) {
        fill = UITheme::BTN_HOVER;
        border = UITheme::ACCENT_BLUE;
        text_col = { 255, 255, 255, 255 };
    }

    // Button base plate
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(renderer, &rect);

    // Bevel highlight line at top (arcade depth effect)
    if (!is_active) {
        Uint8 hl_r = (Uint8)std::min(255, fill.r + 25);
        Uint8 hl_g = (Uint8)std::min(255, fill.g + 30);
        Uint8 hl_b = (Uint8)std::min(255, fill.b + 35);
        SDL_SetRenderDrawColor(renderer, hl_r, hl_g, hl_b, 255);
        SDL_RenderDrawLine(renderer, rect.x + 1, rect.y + 1, rect.x + rect.w - 2, rect.y + 1);
    }

    // Left accent tag indicator if button has custom accent
    if (has_accent && !is_active) {
        SDL_Rect tag = { rect.x, rect.y, 3, rect.h };
        SDL_SetRenderDrawColor(renderer, accent_color.r, accent_color.g, accent_color.b, 255);
        SDL_RenderFillRect(renderer, &tag);
    }

    // Button outline border
    SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRect(renderer, &rect);

    // Centered text with shadow/bold styling
    int text_y = rect.y + (rect.h - 8) / 2;
    if (is_active) {
        font.draw_text_bold(text, rect.x + (rect.w - font.get_text_width(text, 1)) / 2, text_y, text_col, 1);
    } else if (is_hovered) {
        font.draw_text_shadow(text, rect.x + (rect.w - font.get_text_width(text, 1)) / 2, text_y, text_col, { 0, 0, 0, 255 }, 1);
    } else {
        font.draw_text_centered(text, rect.x + rect.w / 2, text_y, text_col, 1);
    }
}

GUI::GUI() {
    patterns = get_available_patterns();
}

void GUI::init(int win_w, int win_h) {
    update_layout(win_w, win_h);
}

void GUI::update_layout(int win_w, int win_h) {
    window_width = win_w;
    window_height = win_h;

    const int header_h = 62;
    const int left_sidebar_w = 310;
    const int right_sidebar_w = 236;

    header_rect        = { 0, 0, win_w, header_h };
    left_sidebar_rect  = { 0, header_h, left_sidebar_w, win_h - header_h };
    right_sidebar_rect = { win_w - right_sidebar_w, header_h, right_sidebar_w, win_h - header_h };
    canvas_rect        = { left_sidebar_w, header_h, win_w - left_sidebar_w - right_sidebar_w, win_h - header_h };

    buttons.clear();
    idx_grid_sizes.clear();
    idx_brush_sizes.clear();
    idx_patterns.clear();

    // =========================================================================
    // HEADER BAR: Switch Engine button (right of Grid Size HUD card)
    // =========================================================================
    // HEADER BAR: Switch Engine button (right of Grid Size HUD card)
    // HUD cards start at x=410, widths: 108, 120, 90, 115
    // =========================================================================
    {
        const int hud_gaps = 6;
        int hx_btn = 410 + (108+hud_gaps) + (120+hud_gaps) + (90+hud_gaps) + (115+hud_gaps);
        int hdr_btn_w = (win_w - 236 - hx_btn - 8); // fill remaining header before right sidebar
        if (hdr_btn_w > 60) hdr_btn_w = std::min(hdr_btn_w, 120);
        buttons.emplace_back(hx_btn, 11, hdr_btn_w, 42, "Switch Engine", [this]() {
            if (on_toggle_engine) on_toggle_engine();
        });
        idx_engine_hdr = (int)buttons.size() - 1;
        buttons.back().has_accent = true;
        buttons.back().accent_color = UITheme::ACCENT_BLUE;
    }

    // =========================================================================
    // LEFT SIDEBAR CARDS & BUTTONS (With Generous Spacing & No Overlap)
    // =========================================================================
    int lx = 8;
    int lw = left_sidebar_w - 16;
    int l_inner_x = lx + 8;
    int l_inner_w = lw - 16;

    // --- Card 1: Simulation Controls (Start, Pause, Reset) ---
    int card1_y = header_h + 8;
    int btn_w3 = (l_inner_w - 12) / 3;
    // Shift the 3 control buttons rightward so they sit comfortably away from the card icon
    int ctrl_offset_x = 14;

    // Start button
    buttons.emplace_back(l_inner_x + ctrl_offset_x, card1_y + 24, btn_w3, 26, "  Start", [this]() {
        is_running = true;
        status_msg = "Simulation running.";
    });
    idx_btn_start = (int)buttons.size() - 1;

    // Pause button
    buttons.emplace_back(l_inner_x + ctrl_offset_x + btn_w3 + 6, card1_y + 24, btn_w3, 26, "  Pause", [this]() {
        is_running = false;
        status_msg = "Simulation paused.";
    });
    idx_btn_pause = (int)buttons.size() - 1;

    // Reset button
    buttons.emplace_back(l_inner_x + ctrl_offset_x + (btn_w3 + 6) * 2, card1_y + 24, btn_w3, 26, "  Reset", [this]() {
        if (on_randomize_clicked) on_randomize_clicked();
        status_msg = "Grid randomized & reset.";
    });
    idx_btn_reset = (int)buttons.size() - 1;

    // --- Card 2: Step & Speed ---
    int card2_y = card1_y + 64;
    // Row 1: Step (S), Random (R), Clear (C)
    buttons.emplace_back(l_inner_x, card2_y + 24, btn_w3, 24, "Step (S)", [this]() {
        if (on_step_clicked) on_step_clicked();
        status_msg = "Stepped 1 generation.";
    });
    buttons.emplace_back(l_inner_x + btn_w3 + 6, card2_y + 24, btn_w3, 24, "Random (R)", [this]() {
        if (on_randomize_clicked) on_randomize_clicked();
        status_msg = "Grid randomized (20% alive).";
    });
    buttons.emplace_back(l_inner_x + (btn_w3 + 6) * 2, card2_y + 24, btn_w3, 24, "Clear (C)", [this]() {
        if (on_clear_clicked) on_clear_clicked();
        status_msg = "Grid cleared.";
    });
    buttons.back().is_danger = true;
    idx_clear = (int)buttons.size() - 1;

    // Speed Slider bounds (positioned below the label so text is never covered)
    speed_slider_rect = { l_inner_x, card2_y + 68, lw - 76, 16 };

    // Row 2: Slow <<, Fast >>
    int btn_w2 = (l_inner_w - 8) / 2;
    buttons.emplace_back(l_inner_x, card2_y + 92, btn_w2, 22, "<< Slow", [this]() {
        delay_ms = std::min(200, delay_ms + 10);
        status_msg = "Delay set to " + std::to_string(delay_ms) + " ms.";
    });
    buttons.emplace_back(l_inner_x + btn_w2 + 8, card2_y + 92, btn_w2, 22, "Fast >>", [this]() {
        delay_ms = std::max(0, delay_ms - 10);
        status_msg = "Delay set to " + std::to_string(delay_ms) + " ms.";
    });

    // --- Card 3: Grid Settings (6 Resolution Presets in 3x2 Grid) ---
    int card3_y = card2_y + 124;
    const std::string grid_names[] = {
        "256x256",
        "512x512",
        "1024x1024",
        "2048x2048",
        "4096x4096",
        "1920x1080"
    };

    for (size_t i = 0; i < grid_size_presets.size() && i < 6; ++i) {
        int col = (int)(i % 3);
        int row = (int)(i / 3);
        int gx = l_inner_x + col * (btn_w3 + 6);
        int gy = card3_y + 36 + row * 26;
        int w = grid_size_presets[i].first;
        int h = grid_size_presets[i].second;

        buttons.emplace_back(gx, gy, btn_w3, 22, grid_names[i], [this, w, h]() {
            if (on_change_grid_size) on_change_grid_size(w, h);
        });
        idx_grid_sizes.push_back((int)buttons.size() - 1);
    }

    // --- Card 4: Drawing Tools & Brush Matrix ---
    int card4_y = card3_y + 98;
    int btn_w4 = (l_inner_w - 18) / 4;
    
    // Tools: Pen, Erase, Fill, Brush
    buttons.emplace_back(l_inner_x, card4_y + 22, btn_w4, 22, "Pen", [this]() {
        current_tool = ToolMode::DRAW;
        status_msg = "Tool: Pen (Draw living cells).";
    });
    idx_draw = (int)buttons.size() - 1;

    buttons.emplace_back(l_inner_x + (btn_w4 + 6), card4_y + 22, btn_w4, 22, "Erase", [this]() {
        current_tool = ToolMode::ERASE;
        status_msg = "Tool: Erase cells.";
    });
    idx_erase = (int)buttons.size() - 1;

    buttons.emplace_back(l_inner_x + (btn_w4 + 6) * 2, card4_y + 22, btn_w4, 22, "Fill", [this]() {
        if (on_randomize_clicked) on_randomize_clicked();
        status_msg = "Canvas populated with random cells.";
    });
    idx_fill = (int)buttons.size() - 1;

    buttons.emplace_back(l_inner_x + (btn_w4 + 6) * 3, card4_y + 22, btn_w4, 22, "Brush", [this]() {
        current_tool = ToolMode::DRAW;
        brush_radius = 2;
        status_msg = "Tool: Brush (Radius 5x5).";
    });

    // Brush Sizes: 1x1, 5x5, 15x15
    buttons.emplace_back(l_inner_x, card4_y + 66, btn_w3, 20, "1x1", [this]() {
        brush_radius = 0;
        status_msg = "Brush size: 1x1.";
    });
    idx_brush_sizes.push_back((int)buttons.size() - 1);

    buttons.emplace_back(l_inner_x + btn_w3 + 6, card4_y + 66, btn_w3, 20, "5x5", [this]() {
        brush_radius = 2;
        status_msg = "Brush size: 5x5.";
    });
    idx_brush_sizes.push_back((int)buttons.size() - 1);

    buttons.emplace_back(l_inner_x + (btn_w3 + 6) * 2, card4_y + 66, btn_w3, 20, "15x15", [this]() {
        brush_radius = 7;
        status_msg = "Brush size: 15x15.";
    });
    idx_brush_sizes.push_back((int)buttons.size() - 1);

    // --- Card 5: HPC Performance Benchmark ---
    int card5_y = card4_y + 98;
    buttons.emplace_back(l_inner_x, card5_y + 24, l_inner_w, 24, "   Benchmark (100 Gens)", [this]() {
        if (on_run_benchmark) on_run_benchmark();
    });
    idx_benchmark = (int)buttons.size() - 1;
    buttons.back().has_accent = true;
    buttons.back().accent_color = UITheme::ACCENT_AMBER;

    // --- Engine Switch: Full-width button pinned to bottom of left sidebar ---
    // Positioned 36px from the bottom of the sidebar
    int eng_bottom_y = win_h - 36;
    buttons.emplace_back(lx, eng_bottom_y, lw, 28, "  Switch Compute Engine", [this]() {
        if (on_toggle_engine) on_toggle_engine();
    });
    idx_engine = (int)buttons.size() - 1;
    // Style it with the accent color to make it stand out
    buttons.back().has_accent = true;
    buttons.back().accent_color = UITheme::ACCENT_BLUE;

    // =========================================================================
    // RIGHT SIDEBAR: PATTERN LIBRARY & STATS
    // =========================================================================
    int rx = win_w - right_sidebar_w + 8;
    int rw = right_sidebar_w - 16;
    int r_inner_x = rx + 8;
    int r_inner_w = rw - 16;

    int r_card1_y = header_h + 8;
    for (size_t i = 0; i < patterns.size(); ++i) {
        int py = r_card1_y + 24 + (int)i * 28;
        buttons.emplace_back(r_inner_x, py, r_inner_w, 24, patterns[i].name, [this, i]() {
            current_tool = ToolMode::STAMP;
            selected_pattern_idx = (int)i;
            status_msg = "Selected Pattern: " + patterns[i].name + ". Click canvas to place!";
        });
        idx_patterns.push_back((int)buttons.size() - 1);
    }
}

void GUI::fit_to_canvas(int grid_w, int grid_h) {
    if (grid_w <= 0 || grid_h <= 0 || canvas_rect.w <= 0 || canvas_rect.h <= 0) return;
    float zoom_w = 1.0f;
    float zoom_h = ((float)canvas_rect.h / (float)canvas_rect.w) * ((float)grid_w / (float)grid_h);
    zoom = std::min(zoom_w, zoom_h);

    float rendered_w = (float)canvas_rect.w * zoom;
    float rendered_h = (float)grid_h * ((float)canvas_rect.w / (float)grid_w) * zoom;
    pan_x = ((float)canvas_rect.w - rendered_w) / 2.0f;
    pan_y = ((float)canvas_rect.h - rendered_h) / 2.0f;
}

void GUI::reset_view(int grid_w, int grid_h) {
    zoom = 1.0f;
    pan_x = 0.0f;
    pan_y = 0.0f;
}

bool GUI::handle_event(const SDL_Event& e) {
    if (in_start_screen) {
        // Must specifically be a Left Mouse Click to enter the game
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            // Trigger neon scanline transition instead of instant jump
            in_start_screen     = false;
            in_transition       = true;
            transition_start_ms = SDL_GetTicks();
            status_msg = "Simulation started. Left-click to draw, Drag to pan.";
            return true;
        }
        return true; // Ignore keyboard presses and other events during start screen
    }

    // 1. Check all UI Buttons in sidebar
    for (auto& btn : buttons) {
        if (btn.handle_event(e)) {
            return true;
        }
    }

    // 1.5. Interactive Speed Slider Drag & Click
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mx = e.button.x;
        int my = e.button.y;
        if (mx >= speed_slider_rect.x - 4 && mx <= speed_slider_rect.x + speed_slider_rect.w + 4 &&
            my >= speed_slider_rect.y - 6 && my <= speed_slider_rect.y + speed_slider_rect.h + 6) {
            is_dragging_slider = true;
            float ratio = (float)(mx - speed_slider_rect.x) / (float)speed_slider_rect.w;
            ratio = std::max(0.0f, std::min(1.0f, ratio));
            delay_ms = (int)((1.0f - ratio) * 100.0f);
            if (delay_ms <= 3) delay_ms = 0;
            status_msg = (delay_ms == 0) ? "Max speed unlocked (0ms delay)." : ("Execution delay: " + std::to_string(delay_ms) + " ms.");
            return true;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        if (is_dragging_slider) {
            is_dragging_slider = false;
            return true;
        }
    } else if (e.type == SDL_MOUSEMOTION) {
        if (is_dragging_slider) {
            int mx = e.motion.x;
            float ratio = (float)(mx - speed_slider_rect.x) / (float)speed_slider_rect.w;
            ratio = std::max(0.0f, std::min(1.0f, ratio));
            delay_ms = (int)((1.0f - ratio) * 100.0f);
            if (delay_ms <= 3) delay_ms = 0;
            status_msg = (delay_ms == 0) ? "Max speed unlocked (0ms delay)." : ("Execution delay: " + std::to_string(delay_ms) + " ms.");
            return true;
        }
    }

    // 2. Track mouse position relative to canvas
    if (e.type == SDL_MOUSEMOTION) {
        int mx = e.motion.x;
        int my = e.motion.y;
        mouse_in_canvas = (mx >= canvas_rect.x && mx < canvas_rect.x + canvas_rect.w &&
                           my >= canvas_rect.y && my < canvas_rect.y + canvas_rect.h);

        if (is_panning) {
            pan_x += (mx - last_pan_mx);
            pan_y += (my - last_pan_my);
            last_pan_mx = mx;
            last_pan_my = my;
            return true;
        }
    }

    // 3. Middle click to Pan
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_MIDDLE) {
        if (mouse_in_canvas) {
            is_panning = true;
            last_pan_mx = e.button.x;
            last_pan_my = e.button.y;
            return true;
        }
    } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_MIDDLE) {
        is_panning = false;
    }

    // 4. Mouse wheel zoom
    if (e.type == SDL_MOUSEWHEEL && mouse_in_canvas) {
        float old_zoom = zoom;
        if (e.wheel.y > 0) {
            zoom = std::min(32.0f, zoom * 1.20f);
        } else if (e.wheel.y < 0) {
            zoom = std::max(0.1f, zoom / 1.20f);
        }

        // Adjust pan to zoom towards mouse position
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        float mouse_canvas_x = (float)(mx - canvas_rect.x);
        float mouse_canvas_y = (float)(my - canvas_rect.y);

        pan_x = mouse_canvas_x - (mouse_canvas_x - pan_x) * (zoom / old_zoom);
        pan_y = mouse_canvas_y - (mouse_canvas_y - pan_y) * (zoom / old_zoom);
        status_msg = "Zoom: " + std::to_string((int)(zoom * 100.0f)) + "%";
        return true;
    }

    return false;
}

void GUI::screen_to_grid(int screen_x, int screen_y, int grid_w, int grid_h, int& grid_x, int& grid_y) const {
    float local_x = (float)(screen_x - canvas_rect.x) - pan_x;
    float local_y = (float)(screen_y - canvas_rect.y) - pan_y;

    float cell_size = (float)canvas_rect.w / (float)grid_w * zoom;
    if (cell_size <= 0.001f) cell_size = 1.0f;

    grid_x = (int)(local_x / cell_size);
    grid_y = (int)(local_y / cell_size);
}

void GUI::grid_to_screen(int grid_x, int grid_y, int& screen_x, int& screen_y) const {
    float cell_size = (float)canvas_rect.w / (float)current_grid_w * zoom;
    screen_x = canvas_rect.x + (int)(pan_x + (float)grid_x * cell_size);
    screen_y = canvas_rect.y + (int)(pan_y + (float)grid_y * cell_size);
}

void GUI::draw(SDL_Renderer* renderer, FontRenderer& font, unsigned long long gen, int population, float fps, int grid_w, int grid_h) {
    if (in_start_screen) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 5, 10, 18, 240);
        SDL_Rect full_screen = { 0, 0, window_width, window_height };
        SDL_RenderFillRect(renderer, &full_screen);

        // 3x Scale Animated Title
        const std::string title = "CONWAY'S GAME OF LIFE";
        int scale = 3;
        int char_w = 8 * scale;
        int total_w = (int)title.length() * char_w;
        int start_x = (window_width - total_w) / 2;
        int base_y = (window_height / 2) - 110;

        Uint32 ticks = SDL_GetTicks();
        float t = (float)ticks / 300.0f;

        // Draw animated backdrop box behind the 3x title
        int box_pad_x = 24;
        int box_pad_y = 16;
        int box_w = total_w + box_pad_x * 2;
        int box_h = 24 + box_pad_y * 2;
        int box_x = (window_width - box_w) / 2;
        int box_y = base_y - box_pad_y;

        // Animated neon border color cycle
        Uint8 border_r = (Uint8)(128 + 127 * sin(t * 1.5f));
        Uint8 border_g = (Uint8)(128 + 127 * sin(t * 1.5f + 2.0f));
        Uint8 border_b = (Uint8)(128 + 127 * sin(t * 1.5f + 4.0f));

        SDL_Rect title_box = { box_x, box_y, box_w, box_h };
        SDL_SetRenderDrawColor(renderer, 10, 16, 26, 250);
        SDL_RenderFillRect(renderer, &title_box);
        SDL_SetRenderDrawColor(renderer, border_r, border_g, border_b, 255);
        SDL_RenderDrawRect(renderer, &title_box);
        SDL_Rect title_box_inner = { box_x + 2, box_y + 2, box_w - 4, box_h - 4 };
        SDL_RenderDrawRect(renderer, &title_box_inner);

        // Animated wave per character with scale = 3
        for (size_t i = 0; i < title.length(); ++i) {
            char ch = title[i];
            if (ch == ' ') continue;
            std::string s(1, ch);

            float wave_offset = sin(t * 3.0f + (float)i * 0.45f) * 5.0f;
            int cx = start_x + (int)i * char_w;
            int cy = base_y + (int)wave_offset;

            // Neon glowing color wave
            Uint8 cr = (Uint8)(150 + 105 * sin(t * 2.0f + (float)i * 0.3f));
            Uint8 cg = (Uint8)(200 + 55 * cos(t * 2.0f + (float)i * 0.3f));
            Uint8 cb = (Uint8)(255);
            SDL_Color col = { cr, cg, cb, 255 };

            // Shadow
            font.draw_text(s, cx + 2, cy + 2, { 0, 0, 0, 255 }, scale);
            // Main glowing character
            font.draw_text(s, cx, cy, col, scale);
        }

        std::string hw_tag = cuda_capable ? "CUDA GPU HARDWARE ACCELERATED" : "OPENMP MULTI-CORE CPU ENGINE";
        font.draw_text_centered(hw_tag, window_width / 2, base_y + 64, UITheme::ACCENT_CYAN, 1);

        // Animated Retro Blinking "- CLICK TO PLAY -" Box
        bool blink = (ticks / 400) % 2 == 0;
        int btn_y = base_y + 115;
        int btn_w = 260;
        int btn_h = 44;
        int btn_x = (window_width - btn_w) / 2;
        SDL_Rect play_box = { btn_x, btn_y, btn_w, btn_h };

        if (blink) {
            SDL_SetRenderDrawColor(renderer, 20, 32, 50, 255);
            SDL_RenderFillRect(renderer, &play_box);
            SDL_SetRenderDrawColor(renderer, 0, 212, 255, 255);
            SDL_RenderDrawRect(renderer, &play_box);
            SDL_Rect inner_play = { btn_x + 2, btn_y + 2, btn_w - 4, btn_h - 4 };
            SDL_RenderDrawRect(renderer, &inner_play);
            font.draw_text_centered("- CLICK TO PLAY -", window_width / 2, btn_y + 14, { 255, 255, 255, 255 }, 2);
        } else {
            SDL_SetRenderDrawColor(renderer, 10, 16, 24, 255);
            SDL_RenderFillRect(renderer, &play_box);
            SDL_SetRenderDrawColor(renderer, 80, 100, 130, 255);
            SDL_RenderDrawRect(renderer, &play_box);
            font.draw_text_centered("- CLICK TO PLAY -", window_width / 2, btn_y + 14, { 140, 160, 190, 255 }, 2);
        }

        font.draw_text_centered("[ CLICK MOUSE TO ENTER GAME ]", window_width / 2, btn_y + 70, UITheme::TEXT_MUTED, 1);
        return;
    }

    // --- Update button active/highlight states ---
    if (idx_btn_start >= 0 && idx_btn_start < (int)buttons.size()) {
        buttons[idx_btn_start].is_active = is_running;
    }
    if (idx_btn_pause >= 0 && idx_btn_pause < (int)buttons.size()) {
        buttons[idx_btn_pause].is_active = !is_running;
    }
    if (idx_draw >= 0 && idx_draw < (int)buttons.size()) {
        buttons[idx_draw].is_active = (current_tool == ToolMode::DRAW);
    }
    if (idx_erase >= 0 && idx_erase < (int)buttons.size()) {
        buttons[idx_erase].is_active = (current_tool == ToolMode::ERASE);
    }
    if (idx_stamp >= 0 && idx_stamp < (int)buttons.size()) {
        buttons[idx_stamp].is_active = (current_tool == ToolMode::STAMP);
    }
    for (size_t i = 0; i < idx_brush_sizes.size(); ++i) {
        int idx = idx_brush_sizes[i];
        if (idx >= 0 && idx < (int)buttons.size()) {
            bool active = false;
            if (i == 0 && brush_radius == 0) active = true;
            if (i == 1 && brush_radius == 2) active = true;
            if (i == 2 && brush_radius == 7) active = true;
            buttons[idx].is_active = active;
        }
    }
    for (size_t i = 0; i < idx_grid_sizes.size() && i < grid_size_presets.size(); ++i) {
        int idx = idx_grid_sizes[i];
        if (idx >= 0 && idx < (int)buttons.size()) {
            buttons[idx].is_active = (current_grid_w == grid_size_presets[i].first &&
                                      current_grid_h == grid_size_presets[i].second);
        }
    }
    for (size_t i = 0; i < idx_patterns.size(); ++i) {
        int idx = idx_patterns[i];
        if (idx >= 0 && idx < (int)buttons.size()) {
            buttons[idx].is_active = (current_tool == ToolMode::STAMP && selected_pattern_idx == (int)i);
        }
    }

    // --- Helper to draw sleek modern cards ---
    auto draw_modern_card = [&](const SDL_Rect& r, const std::string& title, const std::string& icon_sym, const SDL_Color& icon_col) {
        SDL_SetRenderDrawColor(renderer, UITheme::CARD_BG.r, UITheme::CARD_BG.g, UITheme::CARD_BG.b, 255);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, UITheme::CARD_BORDER.r, UITheme::CARD_BORDER.g, UITheme::CARD_BORDER.b, 255);
        SDL_RenderDrawRect(renderer, &r);

        if (!title.empty()) {
            int icon_w = font.get_text_width(icon_sym, 1);
            font.draw_text(icon_sym, r.x + 8, r.y + 7, icon_col, 1);
            font.draw_text_bold(title, r.x + 12 + icon_w, r.y + 7, UITheme::TEXT_TITLE, 1);
        }
    };

    // =========================================================================
    // 1. DRAW TOP HEADER BAR
    // =========================================================================
    SDL_SetRenderDrawColor(renderer, UITheme::HEADER_BG.r, UITheme::HEADER_BG.g, UITheme::HEADER_BG.b, 255);
    SDL_RenderFillRect(renderer, &header_rect);
    SDL_SetRenderDrawColor(renderer, UITheme::PANEL_BORDER.r, UITheme::PANEL_BORDER.g, UITheme::PANEL_BORDER.b, 255);
    SDL_RenderDrawLine(renderer, header_rect.x, header_rect.h - 1, header_rect.w, header_rect.h - 1);

    // Left Logo: 3x3 Matrix Grid of Dots (with Center Purple Dot - Matching Reference Screenshot)
    int logo_x = 12, logo_y = 12;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            SDL_Rect dot = { logo_x + c * 8, logo_y + r * 8, 6, 6 };
            if (r == 1 && c == 1) {
                SDL_SetRenderDrawColor(renderer, 180, 110, 255, 255); // Center Purple Dot
            } else {
                SDL_SetRenderDrawColor(renderer, UITheme::ACCENT_BLUE.r, UITheme::ACCENT_BLUE.g, UITheme::ACCENT_BLUE.b, 255);
            }
            SDL_RenderFillRect(renderer, &dot);
        }
    }

    // Prominent 3x Larger Header Title & Two-Tone Subtitle
    font.draw_text_bold("Conway's Game of Life", 48, 10, { 255, 255, 255, 255 }, 2);
    font.draw_text("Cellular Automaton ", 48, 34, { 145, 175, 205, 255 }, 1);
    int sub_prefix_w = font.get_text_width("Cellular Automaton ", 1);
    font.draw_text_bold("Simulation", 48 + sub_prefix_w, 34, UITheme::ACCENT_BLUE, 1);

    // --- Pixel-Art Helper Icons for HUD Cards ---
    auto draw_cpu_icon = [&](int ix, int iy, SDL_Color col) {
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
        SDL_Rect chip = { ix + 3, iy + 3, 13, 13 };
        SDL_RenderDrawRect(renderer, &chip);
        SDL_Rect inner = { ix + 6, iy + 6, 7, 7 };
        SDL_RenderFillRect(renderer, &inner);
        // Top/bottom/left/right microchip pins
        SDL_RenderDrawLine(renderer, ix + 6, iy, ix + 6, iy + 2);
        SDL_RenderDrawLine(renderer, ix + 12, iy, ix + 12, iy + 2);
        SDL_RenderDrawLine(renderer, ix + 6, iy + 16, ix + 6, iy + 18);
        SDL_RenderDrawLine(renderer, ix + 12, iy + 16, ix + 12, iy + 18);
        SDL_RenderDrawLine(renderer, ix, iy + 6, ix + 2, iy + 6);
        SDL_RenderDrawLine(renderer, ix, iy + 12, ix + 2, iy + 12);
        SDL_RenderDrawLine(renderer, ix + 16, iy + 6, ix + 18, iy + 6);
        SDL_RenderDrawLine(renderer, ix + 16, iy + 12, ix + 18, iy + 12);
    };

    auto draw_play_icon = [&](int ix, int iy, SDL_Color col) {
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
        for (int i = 0; i < 11; ++i) {
            SDL_RenderDrawLine(renderer, ix + 4 + i, iy + 3 + i / 2, ix + 4 + i, iy + 15 - (i + 1) / 2);
        }
    };

    auto draw_users_icon = [&](int ix, int iy, SDL_Color col) {
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
        // Center Person
        SDL_Rect c_head = { ix + 8, iy + 2, 4, 4 };
        SDL_RenderFillRect(renderer, &c_head);
        SDL_Rect c_body = { ix + 6, iy + 7, 8, 9 };
        SDL_RenderFillRect(renderer, &c_body);
        // Left Person
        SDL_Rect l_head = { ix + 2, iy + 4, 3, 3 };
        SDL_RenderFillRect(renderer, &l_head);
        SDL_Rect l_body = { ix + 1, iy + 8, 4, 8 };
        SDL_RenderFillRect(renderer, &l_body);
        // Right Person
        SDL_Rect r_head = { ix + 14, iy + 4, 3, 3 };
        SDL_RenderFillRect(renderer, &r_head);
        SDL_Rect r_body = { ix + 14, iy + 8, 4, 8 };
        SDL_RenderFillRect(renderer, &r_body);
    };

    auto draw_gauge_icon = [&](int ix, int iy, SDL_Color col) {
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
        // Semi-circle gauge outline
        SDL_RenderDrawLine(renderer, ix + 3, iy + 14, ix + 3, iy + 7);
        SDL_RenderDrawLine(renderer, ix + 3, iy + 7, ix + 7, iy + 3);
        SDL_RenderDrawLine(renderer, ix + 7, iy + 3, ix + 13, iy + 3);
        SDL_RenderDrawLine(renderer, ix + 13, iy + 3, ix + 17, iy + 7);
        SDL_RenderDrawLine(renderer, ix + 17, iy + 7, ix + 17, iy + 14);
        // Needle
        SDL_Rect pivot = { ix + 8, iy + 11, 4, 4 };
        SDL_RenderFillRect(renderer, &pivot);
        SDL_RenderDrawLine(renderer, ix + 10, iy + 11, ix + 14, iy + 6);
    };

    auto draw_grid_icon = [&](int ix, int iy, SDL_Color col) {
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                SDL_Rect cell = { ix + 3 + c * 5, iy + 3 + r * 5, 3, 3 };
                SDL_RenderFillRect(renderer, &cell);
            }
        }
    };

    // --- 4 Top HUD Cards + Engine Switch ---
    int hx = 410;
    const int hud_y = 12;
    const int hud_h = 40;
    const int hgap  = 6;

    auto draw_hud_card = [&](int x, int w, std::function<void(int, int, SDL_Color)> draw_icon, const std::string& label, const std::string& val, const SDL_Color& col) {
        SDL_Rect r = { x, hud_y, w, hud_h };
        SDL_SetRenderDrawColor(renderer, UITheme::CARD_BG.r, UITheme::CARD_BG.g, UITheme::CARD_BG.b, 255);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, UITheme::CARD_BORDER.r, UITheme::CARD_BORDER.g, UITheme::CARD_BORDER.b, 255);
        SDL_RenderDrawRect(renderer, &r);

        draw_icon(x + 8, hud_y + 10, col);
        font.draw_text(label, x + 32, hud_y + 6, UITheme::TEXT_MUTED, 1);
        font.draw_text_bold(val, x + 32, hud_y + 20, col, 1);
    };

    // 1. Generation Card
    draw_hud_card(hx, 108, draw_play_icon, "Generation", std::to_string(gen), UITheme::TEXT_TITLE);
    hx += 108 + hgap;

    // 2. Population Card
    draw_hud_card(hx, 120, draw_users_icon, "Population", std::to_string(population), UITheme::TEXT_TITLE);
    hx += 120 + hgap;

    // 3. FPS Card
    std::ostringstream ss_fps;
    ss_fps << std::fixed << std::setprecision(1) << fps;
    draw_hud_card(hx, 90, draw_gauge_icon, "FPS", ss_fps.str(), UITheme::ACCENT_BLUE);
    hx += 90 + hgap;

    // 4. Grid Size Card
    std::ostringstream ss_g;
    ss_g << grid_w << " x " << grid_h;
    draw_hud_card(hx, 115, draw_grid_icon, "Grid Size", ss_g.str(), UITheme::TEXT_TITLE);
    hx += 115 + hgap;

    // 5. Switch Engine HUD button (right of Grid Size, drawn as a special card)
    {
        int sw_w = (right_sidebar_rect.x - hx - 8);
        sw_w = std::max(60, std::min(sw_w, 120));
        bool eng_is_gpu = cuda_capable;
        SDL_Color sw_fill = eng_is_gpu ? SDL_Color{0, 40, 60, 255} : SDL_Color{13, 20, 32, 255};
        SDL_Color sw_border = eng_is_gpu ? SDL_Color{0, 200, 120, 255} : UITheme::ACCENT_BLUE;
        SDL_Rect sw_r = { hx, hud_y, sw_w, hud_h };
        SDL_SetRenderDrawColor(renderer, sw_fill.r, sw_fill.g, sw_fill.b, 255);
        SDL_RenderFillRect(renderer, &sw_r);
        SDL_SetRenderDrawColor(renderer, sw_border.r, sw_border.g, sw_border.b, 255);
        SDL_RenderDrawRect(renderer, &sw_r);
        // Left accent bar
        SDL_Rect tag = { hx, hud_y, 3, hud_h };
        SDL_RenderFillRect(renderer, &tag);
        // Icon: small CPU chip pixels
        draw_cpu_icon(hx + 8, hud_y + 10, sw_border);
        // Label
        font.draw_text("Switch", hx + 32, hud_y + 6,  UITheme::TEXT_MUTED, 1);
        font.draw_text_bold("Engine", hx + 32, hud_y + 20, sw_border, 1);
    }

    // =========================================================================
    // 2. DRAW LEFT SIDEBAR & CARDS (Well-Spaced & Clean)
    // =========================================================================
    SDL_SetRenderDrawColor(renderer, UITheme::PANEL_BG.r, UITheme::PANEL_BG.g, UITheme::PANEL_BG.b, 255);
    SDL_RenderFillRect(renderer, &left_sidebar_rect);
    SDL_SetRenderDrawColor(renderer, UITheme::PANEL_BORDER.r, UITheme::PANEL_BORDER.g, UITheme::PANEL_BORDER.b, 255);
    SDL_RenderDrawLine(renderer, left_sidebar_rect.x + left_sidebar_rect.w - 1, left_sidebar_rect.y, left_sidebar_rect.x + left_sidebar_rect.w - 1, left_sidebar_rect.y + left_sidebar_rect.h);

    int lx = 8;
    int lw = left_sidebar_rect.w - 16;
    int card_header_h = header_rect.h;

    // Card 1: Simulation Controls
    int c1_y = card_header_h + 8;
    draw_modern_card({ lx, c1_y, lw, 56 }, "Simulation Controls", ">", UITheme::ACCENT_BLUE);

    // Card 2: Step & Speed
    int c2_y = c1_y + 64;
    draw_modern_card({ lx, c2_y, lw, 122 }, "Step & Speed", "o", UITheme::ACCENT_CYAN);
    font.draw_text("Execution Speed (Delay)", lx + 8, c2_y + 52, UITheme::TEXT_MUTED, 1);
    
    // Speed Slider visualization (Interactive Click & Drag)
    int slider_x = speed_slider_rect.x;
    int slider_y = speed_slider_rect.y + 4;
    int slider_w = speed_slider_rect.w;
    int slider_h = 4;
    
    // Background track
    SDL_Rect s_bg = { slider_x, slider_y, slider_w, slider_h };
    SDL_SetRenderDrawColor(renderer, 24, 38, 58, 255);
    SDL_RenderFillRect(renderer, &s_bg);
    SDL_SetRenderDrawColor(renderer, UITheme::CARD_BORDER.r, UITheme::CARD_BORDER.g, UITheme::CARD_BORDER.b, 255);
    SDL_RenderDrawRect(renderer, &s_bg);
    
    // Active Neon Blue filled progress
    float speed_ratio = 1.0f - std::min(1.0f, (float)delay_ms / 100.0f);
    int fill_w = (int)((float)slider_w * speed_ratio);
    SDL_Rect s_fill = { slider_x, slider_y, fill_w, slider_h };
    SDL_SetRenderDrawColor(renderer, UITheme::ACCENT_BLUE.r, UITheme::ACCENT_BLUE.g, UITheme::ACCENT_BLUE.b, 255);
    SDL_RenderFillRect(renderer, &s_fill);
    
    // Slider Knob (White capsule matching user screenshot)
    int knob_w = 6;
    int knob_h = 12;
    int knob_x = slider_x + fill_w - knob_w / 2;
    knob_x = std::max(slider_x, std::min(slider_x + slider_w - knob_w, knob_x));
    SDL_Rect knob = { knob_x, slider_y - (knob_h - slider_h) / 2, knob_w, knob_h };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &knob);
    if (is_dragging_slider) {
        SDL_SetRenderDrawColor(renderer, UITheme::ACCENT_BLUE.r, UITheme::ACCENT_BLUE.g, UITheme::ACCENT_BLUE.b, 255);
        SDL_RenderDrawRect(renderer, &knob);
    }

    // Speed badge
    std::string speed_badge = (delay_ms == 0) ? "MAX" : (delay_ms <= 16 ? "60 FPS" : (std::to_string(1000 / std::max(1, delay_ms)) + " FPS"));
    font.draw_text_bold(speed_badge, slider_x + slider_w + 10, slider_y - 3, UITheme::ACCENT_BLUE, 1);

    // Card 3: Grid Settings
    int c3_y = c2_y + 124;
    draw_modern_card({ lx, c3_y, lw, 94 }, "Grid Settings", "#", UITheme::ACCENT_BLUE);
    font.draw_text("Grid Resolution Presets", lx + 8, c3_y + 22, UITheme::TEXT_MUTED, 1);

    // Card 4: Drawing Tools & Brush
    int c4_y = c3_y + 98;
    draw_modern_card({ lx, c4_y, lw, 94 }, "Drawing Tools & Brush", "/", UITheme::ACCENT_CYAN);
    font.draw_text("Brush Size", lx + 8, c4_y + 50, UITheme::TEXT_MUTED, 1);

    // Card 5: HPC Performance Benchmark
    int c5_y = c4_y + 98;
    int c5_h = has_benchmark_result ? 142 : 56;
    draw_modern_card({ lx, c5_y, lw, c5_h }, "HPC Benchmark (100g)", "~", UITheme::ACCENT_AMBER);
    
    if (has_benchmark_result) {
        std::ostringstream ss_t, ss_p, ss_c;
        ss_t << std::fixed << std::setprecision(2) << last_benchmark_ms << " ms";
        ss_p << std::fixed << std::setprecision(3) << last_benchmark_per_gen << " ms";
        ss_c << std::fixed << std::setprecision(1) << last_benchmark_mcells << " MC/s";

        // Inner sleek stat container
        SDL_Rect sbox = { lx + 6, c5_y + 52, lw - 12, 82 };
        SDL_SetRenderDrawColor(renderer, 7, 12, 20, 255);
        SDL_RenderFillRect(renderer, &sbox);
        SDL_SetRenderDrawColor(renderer, 32, 52, 78, 255);
        SDL_RenderDrawRect(renderer, &sbox);
        
        // Cyan accent strip at top of stat box
        SDL_SetRenderDrawColor(renderer, UITheme::ACCENT_BLUE.r, UITheme::ACCENT_BLUE.g, UITheme::ACCENT_BLUE.b, 200);
        SDL_RenderDrawLine(renderer, sbox.x + 1, sbox.y + 1, sbox.x + sbox.w - 2, sbox.y + 1);

        // Header label inside box
        font.draw_text("COMPUTE TIME (100 GENS)", sbox.x + 8, sbox.y + 6, UITheme::TEXT_MUTED, 1);

        // Prominent Scale 2 Large execution time display
        font.draw_text(ss_t.str(), sbox.x + 8, sbox.y + 20, UITheme::ACCENT_BLUE, 2);

        // Thin separator
        SDL_SetRenderDrawColor(renderer, 24, 38, 56, 255);
        SDL_RenderDrawLine(renderer, sbox.x + 6, sbox.y + 44, sbox.x + sbox.w - 7, sbox.y + 44);

        // Sub-stats (Time/Gen and Throughput) with high-contrast text
        font.draw_text("Avg/Gen :", sbox.x + 8, sbox.y + 50, UITheme::TEXT_MUTED, 1);
        int p_w = font.get_text_width(ss_p.str(), 1);
        font.draw_text(ss_p.str(), sbox.x + sbox.w - p_w - 8, sbox.y + 50, { 255, 255, 255, 255 }, 1);

        font.draw_text("Rate    :", sbox.x + 8, sbox.y + 65, UITheme::TEXT_MUTED, 1);
        int c_w = font.get_text_width(ss_c.str(), 1);
        font.draw_text(ss_c.str(), sbox.x + sbox.w - c_w - 8, sbox.y + 65, UITheme::ACCENT_AMBER, 1);
    }

    // --- Engine Switch Bar: Full-width strip pinned to the very bottom of left sidebar ---
    int eng_bar_y = window_height - 36;
    SDL_Rect eng_bar_bg = { left_sidebar_rect.x, eng_bar_y, left_sidebar_rect.w, 36 };
    SDL_SetRenderDrawColor(renderer, 8, 18, 32, 255);
    SDL_RenderFillRect(renderer, &eng_bar_bg);
    // Top separator line
    SDL_SetRenderDrawColor(renderer, UITheme::ACCENT_BLUE.r, UITheme::ACCENT_BLUE.g, UITheme::ACCENT_BLUE.b, 180);
    SDL_RenderDrawLine(renderer, eng_bar_bg.x, eng_bar_y, eng_bar_bg.x + eng_bar_bg.w, eng_bar_y);
    // Engine state indicator dot
    SDL_Color eng_dot_col = cuda_capable ? SDL_Color{0, 255, 120, 255} : SDL_Color{100, 140, 200, 255};
    SDL_Rect eng_dot = { left_sidebar_rect.x + 10, eng_bar_y + 13, 6, 6 };
    SDL_SetRenderDrawColor(renderer, eng_dot_col.r, eng_dot_col.g, eng_dot_col.b, 255);
    SDL_RenderFillRect(renderer, &eng_dot);
    // Current engine label next to dot
    font.draw_text(engine_name, left_sidebar_rect.x + 22, eng_bar_y + 12, eng_dot_col, 1);

    // =========================================================================
    // 3. DRAW RIGHT SIDEBAR & CARDS
    // =========================================================================
    SDL_SetRenderDrawColor(renderer, UITheme::PANEL_BG.r, UITheme::PANEL_BG.g, UITheme::PANEL_BG.b, 255);
    SDL_RenderFillRect(renderer, &right_sidebar_rect);
    SDL_SetRenderDrawColor(renderer, UITheme::PANEL_BORDER.r, UITheme::PANEL_BORDER.g, UITheme::PANEL_BORDER.b, 255);
    SDL_RenderDrawLine(renderer, right_sidebar_rect.x, right_sidebar_rect.y, right_sidebar_rect.x, right_sidebar_rect.y + right_sidebar_rect.h);

    int rx = right_sidebar_rect.x + 8;
    int rw = right_sidebar_rect.w - 16;

    // Card 1: Pattern Library (7 patterns)
    int rc1_y = card_header_h + 8;
    draw_modern_card({ rx, rc1_y, rw, 226 }, "Pattern Library", "#", UITheme::ACCENT_BLUE);

    // Card 2: Simulation Info
    int rc2_y = rc1_y + 232;
    draw_modern_card({ rx, rc2_y, rw, 100 }, "Simulation Info", "i", UITheme::ACCENT_CYAN);
    
    int info_y = rc2_y + 24;
    auto draw_info_row = [&](const std::string& key, const std::string& val) {
        font.draw_text(key, rx + 8, info_y, UITheme::TEXT_MUTED, 1);
        int val_w = font.get_text_width(val, 1);
        font.draw_text(val, rx + rw - val_w - 8, info_y, UITheme::TEXT_TITLE, 1);
        info_y += 14;
    };
    draw_info_row("Generation", std::to_string(gen));
    draw_info_row("Population", std::to_string(population));
    draw_info_row("Grid Size", ss_g.str());
    draw_info_row("Speed", ss_fps.str() + " FPS");
    draw_info_row("Engine", engine_name);

    // Card 3: Quick Shortcuts
    int rc3_y = rc2_y + 106;
    draw_modern_card({ rx, rc3_y, rw, 114 }, "Quick Shortcuts", "~", UITheme::ACCENT_AMBER);
    
    int sc_y = rc3_y + 24;
    auto draw_sc_row = [&](const std::string& key, const std::string& desc) {
        font.draw_text(key, rx + 8, sc_y, UITheme::ACCENT_BLUE, 1);
        font.draw_text(desc, rx + 64, sc_y, UITheme::TEXT_NORMAL, 1);
        sc_y += 14;
    };
    draw_sc_row("[Space]", "Play / Pause");
    draw_sc_row("[S]", "Step 1 Gen");
    draw_sc_row("[R]", "Randomize");
    draw_sc_row("[C]", "Clear Grid");
    draw_sc_row("[1-6]", "Grid Presets");
    draw_sc_row("Mouse", "Draw / Erase");

    // =========================================================================
    // 4. DRAW ALL BUTTONS
    // =========================================================================
    for (auto& btn : buttons) {
        btn.draw(renderer, font);
    }

    // --- Pixel-art icon overlays for Start, Pause, Reset buttons ---
    // These are drawn AFTER the button base so they sit on top of the button face.
    auto draw_btn_play_icon = [&](const SDL_Rect& r, bool active) {
        SDL_Color c = active ? UITheme::BTN_ACTIVE_TXT : UITheme::ACCENT_BLUE;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
        int bx = r.x + 5;
        int by = r.y + (r.h - 10) / 2;
        // Solid play triangle (4 scan lines)
        for (int i = 0; i < 5; ++i) {
            SDL_RenderDrawLine(renderer, bx + i, by + i, bx + i, by + 9 - i);
        }
    };

    auto draw_btn_pause_icon = [&](const SDL_Rect& r, bool active) {
        SDL_Color c = active ? UITheme::BTN_ACTIVE_TXT : UITheme::ACCENT_BLUE;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
        int bx = r.x + 5;
        int by = r.y + (r.h - 10) / 2;
        SDL_Rect bar1 = { bx, by, 3, 10 };
        SDL_Rect bar2 = { bx + 5, by, 3, 10 };
        SDL_RenderFillRect(renderer, &bar1);
        SDL_RenderFillRect(renderer, &bar2);
    };

    auto draw_btn_reset_icon = [&](const SDL_Rect& r, bool active) {
        SDL_Color c = active ? UITheme::BTN_ACTIVE_TXT : UITheme::ACCENT_BLUE;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
        int bx = r.x + 5;
        int by = r.y + (r.h - 11) / 2;
        // Circular arrow: arc approximation using line segments
        SDL_RenderDrawLine(renderer, bx + 1, by + 2, bx + 1, by + 7);   // left
        SDL_RenderDrawLine(renderer, bx + 1, by + 2, bx + 4, by);       // top-left
        SDL_RenderDrawLine(renderer, bx + 4, by,     bx + 8, by);       // top
        SDL_RenderDrawLine(renderer, bx + 8, by,     bx + 10, by + 2);  // top-right
        SDL_RenderDrawLine(renderer, bx + 10, by + 2, bx + 10, by + 6); // right
        SDL_RenderDrawLine(renderer, bx + 10, by + 6, bx + 7, by + 9);  // bottom-right
        SDL_RenderDrawLine(renderer, bx + 7, by + 9, bx + 3, by + 9);   // bottom
        // Arrow head pointing left-down
        SDL_RenderDrawLine(renderer, bx + 1, by + 7, bx + 4, by + 7);   // head top
        SDL_RenderDrawLine(renderer, bx + 1, by + 7, bx + 1, by + 11);  // head stem
        SDL_RenderDrawLine(renderer, bx - 2, by + 8, bx + 2, by + 8);   // arrowhead
    };

    auto draw_btn_timer_icon = [&](const SDL_Rect& r) {
        SDL_SetRenderDrawColor(renderer, 255, 180, 50, 255);
        int bx = r.x + 6;
        int by = r.y + (r.h - 11) / 2;
        // Stopwatch body
        SDL_RenderDrawLine(renderer, bx + 2, by + 2, bx + 6, by + 2);
        SDL_RenderDrawLine(renderer, bx + 2, by + 8, bx + 6, by + 8);
        SDL_RenderDrawLine(renderer, bx, by + 4, bx, by + 6);
        SDL_RenderDrawLine(renderer, bx + 8, by + 4, bx + 8, by + 6);
        // Top stem
        SDL_RenderDrawLine(renderer, bx + 3, by, bx + 5, by);
        SDL_RenderDrawLine(renderer, bx + 4, by, bx + 4, by + 2);
        // Hands
        SDL_RenderDrawLine(renderer, bx + 4, by + 5, bx + 4, by + 3);
        SDL_RenderDrawLine(renderer, bx + 4, by + 5, bx + 6, by + 5);
    };

    if (idx_btn_start >= 0 && idx_btn_start < (int)buttons.size()) {
        draw_btn_play_icon(buttons[idx_btn_start].rect, buttons[idx_btn_start].is_active);
    }
    if (idx_btn_pause >= 0 && idx_btn_pause < (int)buttons.size()) {
        draw_btn_pause_icon(buttons[idx_btn_pause].rect, buttons[idx_btn_pause].is_active);
    }
    if (idx_btn_reset >= 0 && idx_btn_reset < (int)buttons.size()) {
        draw_btn_reset_icon(buttons[idx_btn_reset].rect, false);
    }
    if (idx_benchmark >= 0 && idx_benchmark < (int)buttons.size()) {
        draw_btn_timer_icon(buttons[idx_benchmark].rect);
    }

    // =========================================================================
    // 5. DRAW CANVAS BORDER & STAMP PREVIEW
    // =========================================================================
    SDL_SetRenderDrawColor(renderer, UITheme::PANEL_BORDER.r, UITheme::PANEL_BORDER.g, UITheme::PANEL_BORDER.b, 255);
    SDL_RenderDrawRect(renderer, &canvas_rect);

    if (mouse_in_canvas && current_tool == ToolMode::STAMP && selected_pattern_idx >= 0 && selected_pattern_idx < (int)patterns.size()) {
        const auto& pat = patterns[selected_pattern_idx];
        SDL_SetRenderDrawColor(renderer, UITheme::ACCENT_BLUE.r, UITheme::ACCENT_BLUE.g, UITheme::ACCENT_BLUE.b, 180);
        float cell_pixel_size = (float)canvas_rect.w / (float)grid_w * zoom;
        for (const auto& pt : pat.points) {
            int px = canvas_rect.x + (int)(pan_x + (float)(mouse_grid_x + pt.dx) * cell_pixel_size);
            int py = canvas_rect.y + (int)(pan_y + (float)(mouse_grid_y + pt.dy) * cell_pixel_size);
            SDL_Rect r = { px, py, std::max(1, (int)cell_pixel_size), std::max(1, (int)cell_pixel_size) };
            SDL_RenderFillRect(renderer, &r);
        }
    }

    // =========================================================================
    // 6. TRANSITION ANIMATION (Neon Scanline Reveal on Click-to-Play)
    // =========================================================================
    if (in_transition) {
        Uint32 elapsed = SDL_GetTicks() - transition_start_ms;
        if (elapsed >= (Uint32)TRANSITION_MS) {
            in_transition = false;
        } else {
            float t = (float)elapsed / (float)TRANSITION_MS; // 0.0 -> 1.0
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_Rect full = { 0, 0, window_width, window_height };

            // --- Phase 1: Initial bright neon flash (0 - 120ms) ---
            if (elapsed < 120) {
                float ft = (float)elapsed / 120.0f;
                Uint8 fa = (Uint8)(210 * (1.0f - ft * ft)); // quadratic fade
                SDL_SetRenderDrawColor(renderer, 0, 220, 255, fa);
                SDL_RenderFillRect(renderer, &full);
            }

            // --- Phase 2: Neon scanline wipes top->bottom (starts at t=0.08) ---
            float sweep_t = std::max(0.0f, (t - 0.08f) / 0.92f);
            // Use ease-in-out cubic for smooth acceleration
            float ease = sweep_t < 0.5f
                ? 4.0f * sweep_t * sweep_t * sweep_t
                : 1.0f - (-2.0f * sweep_t + 2.0f) * (-2.0f * sweep_t + 2.0f) * (-2.0f * sweep_t + 2.0f) / 2.0f;
            int sweep_y = (int)(ease * (float)window_height);

            // Dark curtain covers everything below the sweep line
            if (sweep_y < window_height - 2) {
                SDL_SetRenderDrawColor(renderer, 5, 10, 18, 248);
                SDL_Rect curtain = { 0, sweep_y + 3, window_width, window_height - sweep_y - 3 };
                SDL_RenderFillRect(renderer, &curtain);
            }

            // Bright neon cyan sweep line with glow effect
            if (sweep_y > 0 && sweep_y < window_height) {
                // Glow trail lines below sweep
                SDL_SetRenderDrawColor(renderer, 0, 212, 255, 30);
                SDL_RenderDrawLine(renderer, 0, sweep_y + 3, window_width, sweep_y + 3);
                SDL_SetRenderDrawColor(renderer, 0, 212, 255, 60);
                SDL_RenderDrawLine(renderer, 0, sweep_y + 2, window_width, sweep_y + 2);
                SDL_SetRenderDrawColor(renderer, 0, 212, 255, 120);
                SDL_RenderDrawLine(renderer, 0, sweep_y + 1, window_width, sweep_y + 1);
                // Core bright line
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawLine(renderer, 0, sweep_y,     window_width, sweep_y);
                // Glow trail lines above sweep
                SDL_SetRenderDrawColor(renderer, 180, 240, 255, 200);
                SDL_RenderDrawLine(renderer, 0, sweep_y - 1, window_width, sweep_y - 1);
                SDL_SetRenderDrawColor(renderer, 100, 220, 255, 100);
                SDL_RenderDrawLine(renderer, 0, sweep_y - 2, window_width, sweep_y - 2);
                SDL_SetRenderDrawColor(renderer, 0, 180, 255, 40);
                SDL_RenderDrawLine(renderer, 0, sweep_y - 3, window_width, sweep_y - 3);
            }

            // --- Phase 3: Subtle vignette fade-out after full reveal (t > 0.85) ---
            if (t > 0.85f) {
                float vt = (t - 0.85f) / 0.15f;
                Uint8 va = (Uint8)(80 * (1.0f - vt));
                SDL_SetRenderDrawColor(renderer, 0, 20, 40, va);
                SDL_RenderFillRect(renderer, &full);
            }
        }
    }
}
