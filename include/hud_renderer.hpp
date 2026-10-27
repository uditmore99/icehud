#ifndef HUD_RENDERER_HPP
#define HUD_RENDERER_HPP

#include "telemetry_parser.hpp"
#include <SDL.h>
#include <string>

class HudRenderer {
public:
    HudRenderer(int width, int height);
    ~HudRenderer();

    bool init(const char* title);
    void render(const HudState& state, bool sim_mode);

private:
    int width_;
    int height_;
    SDL_Window*   window_   = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    void drawChar(char c, int x, int y, int scale);
    void drawText(const std::string& text, int x, int y, int scale);
    void drawRotatedLine(double cx, double cy, double x1, double y1,
                         double x2, double y2, double bank_rad);
};

#endif // HUD_RENDERER_HPP