#ifndef FONT8X8_H
#define FONT8X8_H

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

// Standard 8x8 monochrome bitmap font for ASCII characters 32 (space) to 126 (~)
// Each character is 8 rows, 1 byte per row (MSB = leftmost pixel).
extern const unsigned char FONT8X8_BASIC[95][8];

class FontRenderer {
private:
    SDL_Texture* font_atlas;
    SDL_Renderer* renderer;

public:
    FontRenderer() : font_atlas(nullptr), renderer(nullptr) {}

    ~FontRenderer() {
        if (font_atlas) {
            SDL_DestroyTexture(font_atlas);
            font_atlas = nullptr;
        }
    }

    bool init(SDL_Renderer* rend);

    void draw_text(const std::string& text, int x, int y, SDL_Color color, int scale = 1);
    void draw_text_centered(const std::string& text, int center_x, int y, SDL_Color color, int scale = 1);
    void draw_text_shadow(const std::string& text, int x, int y, SDL_Color color, SDL_Color shadow_col = {0, 0, 0, 255}, int scale = 1);
    void draw_text_bold(const std::string& text, int x, int y, SDL_Color color, int scale = 1);
    void draw_arcade_banner(const std::string& text, int x, int y, int w, int h, SDL_Color text_col, SDL_Color border_col, SDL_Color moss_col);
    void draw_section_header(const std::string& title, int x, int y, int w, SDL_Color text_col, SDL_Color accent_col);

    int get_text_width(const std::string& text, int scale = 1) const;
    int get_text_height(int scale = 1) const;
};

#endif // FONT8X8_H
