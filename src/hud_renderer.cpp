#include "hud_renderer.hpp"
#include <cmath>
#include <cstdio>

namespace {
constexpr double DEG_TO_RAD = 3.14159265358979323846 / 180.0;
}

HudRenderer::HudRenderer(int width, int height) : width_(width), height_(height) {}

HudRenderer::~HudRenderer() {
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_)   SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool HudRenderer::init(const char* title) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;
    window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width_, height_, SDL_WINDOW_SHOWN);
    if (!window_) return false;
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    return renderer_ != nullptr;
}

void HudRenderer::drawRotatedLine(double cx, double cy, double x1, double y1,
                                  double x2, double y2, double bank_rad) {
    const double c = std::cos(bank_rad);
    const double s = std::sin(bank_rad);
    const int rx1 = static_cast<int>(cx + (x1 * c - y1 * s));
    const int ry1 = static_cast<int>(cy + (x1 * s + y1 * c));
    const int rx2 = static_cast<int>(cx + (x2 * c - y2 * s));
    const int ry2 = static_cast<int>(cy + (x2 * s + y2 * c));
    SDL_RenderDrawLine(renderer_, rx1, ry1, rx2, ry2);
}

void HudRenderer::drawChar(char c, int x, int y, int s) {
    // 7-segment style vector stroke font for avionics HUD readouts
    auto seg = [&](int x1, int y1, int x2, int y2) {
        SDL_RenderDrawLine(renderer_, x + x1 * s, y + y1 * s, x + x2 * s, y + y2 * s);
    };
    switch (c) {
        case '0': seg(0,0,2,0); seg(2,0,2,4); seg(2,4,0,4); seg(0,4,0,0); break;
        case '1': seg(1,0,1,4); break;
        case '2': seg(0,0,2,0); seg(2,0,2,2); seg(2,2,0,2); seg(0,2,0,4); seg(0,4,2,4); break;
        case '3': seg(0,0,2,0); seg(2,0,2,4); seg(0,2,2,2); seg(0,4,2,4); break;
        case '4': seg(0,0,0,2); seg(0,2,2,2); seg(2,0,2,4); break;
        case '5': seg(2,0,0,0); seg(0,0,0,2); seg(0,2,2,2); seg(2,2,2,4); seg(2,4,0,4); break;
        case '6': seg(2,0,0,0); seg(0,0,0,4); seg(0,4,2,4); seg(2,4,2,2); seg(2,2,0,2); break;
        case '7': seg(0,0,2,0); seg(2,0,2,4); break;
        case '8': seg(0,0,2,0); seg(2,0,2,4); seg(2,4,0,4); seg(0,4,0,0); seg(0,2,2,2); break;
        case '9': seg(2,2,0,2); seg(0,2,0,0); seg(0,0,2,0); seg(2,0,2,4); seg(0,4,2,4); break;
        case '-': seg(0,2,2,2); break;
        case '.': seg(1,3,1,4); break;
        case ':': seg(1,1,1,1); seg(1,3,1,3); break;
        case 'A': seg(0,4,0,0); seg(0,0,2,0); seg(2,0,2,4); seg(0,2,2,2); break;
        case 'B': seg(0,0,0,4); seg(0,0,2,0); seg(2,0,2,4); seg(0,2,2,2); seg(0,4,2,4); break;
        case 'C': seg(2,0,0,0); seg(0,0,0,4); seg(0,4,2,4); break;
        case 'D': seg(0,0,0,4); seg(0,0,2,1); seg(2,1,2,3); seg(2,3,0,4); break;
        case 'E': seg(2,0,0,0); seg(0,0,0,4); seg(0,2,2,2); seg(0,4,2,4); break;
        case 'F': seg(2,0,0,0); seg(0,0,0,4); seg(0,2,2,2); break;
        case 'G': seg(2,0,0,0); seg(0,0,0,4); seg(0,4,2,4); seg(2,4,2,2); seg(1,2,2,2); break;
        case 'H': seg(0,0,0,4); seg(2,0,2,4); seg(0,2,2,2); break;
        case 'I': seg(1,0,1,4); seg(0,0,2,0); seg(0,4,2,4); break;
        case 'K': seg(0,0,0,4); seg(0,2,2,0); seg(0,2,2,4); break;
        case 'L': seg(0,0,0,4); seg(0,4,2,4); break;
        case 'M': seg(0,4,0,0); seg(0,0,1,2); seg(1,2,2,0); seg(2,0,2,4); break;
        case 'N': seg(0,4,0,0); seg(0,0,2,4); seg(2,4,2,0); break;
        case 'P': seg(0,4,0,0); seg(0,0,2,0); seg(2,0,2,2); seg(2,2,0,2); break;
        case 'R': seg(0,4,0,0); seg(0,0,2,0); seg(2,0,2,2); seg(2,2,0,2); seg(1,2,2,4); break;
        case 'S': seg(2,0,0,0); seg(0,0,0,2); seg(0,2,2,2); seg(2,2,2,4); seg(2,4,0,4); break;
        case 'T': seg(0,0,2,0); seg(1,0,1,4); break;
        case 'U': seg(0,0,0,4); seg(0,4,2,4); seg(2,4,2,0); break;
        case 'V': seg(0,0,1,4); seg(2,0,1,4); break;
        default: break;
    }
}

void HudRenderer::drawText(const std::string& text, int x, int y, int scale) {
    int cursor = x;
    for (char c : text) {
        drawChar(c, cursor, y, scale);
        cursor += 3 * scale + scale;
    }
}

void HudRenderer::render(const HudState& state, bool sim_mode) {
    // Dark cockpit glass background
    SDL_SetRenderDrawColor(renderer_, 4, 14, 8, 255);
    SDL_RenderClear(renderer_);

    // Avionics phosphor green symbology
    SDL_SetRenderDrawColor(renderer_, 0, 255, 100, 255);

    const double cx = width_ * 0.5;
    const double cy = height_ * 0.5;
    const double bank_rad = -state.bank_deg * DEG_TO_RAD;
    const double px_per_deg = 6.5;
    const double pitch_offset = state.pitch_deg * px_per_deg;

    // 1. Artificial Horizon Line
    drawRotatedLine(cx, cy, -220, pitch_offset, -40, pitch_offset, bank_rad);
    drawRotatedLine(cx, cy,  40,  pitch_offset,  220, pitch_offset, bank_rad);

    // 2. Pitch Ladder (-30 to +30 deg in 5-deg increments)
    for (int deg = -30; deg <= 30; deg += 5) {
        if (deg == 0) continue;
        const double y_rung = pitch_offset - (deg * px_per_deg);
        if (std::abs(y_rung) > 190.0) continue;

        if (deg > 0) {
            drawRotatedLine(cx, cy, -75, y_rung, -30, y_rung, bank_rad);
            drawRotatedLine(cx, cy, -75, y_rung, -75, y_rung + 10, bank_rad);
            drawRotatedLine(cx, cy,  30, y_rung,  75, y_rung, bank_rad);
            drawRotatedLine(cx, cy,  75, y_rung,  75, y_rung + 10, bank_rad);
        } else {
            // Dashed negative pitch rungs
            drawRotatedLine(cx, cy, -75, y_rung, -55, y_rung, bank_rad);
            drawRotatedLine(cx, cy, -45, y_rung, -30, y_rung, bank_rad);
            drawRotatedLine(cx, cy, -75, y_rung, -75, y_rung - 10, bank_rad);
            drawRotatedLine(cx, cy,  30, y_rung,  45, y_rung, bank_rad);
            drawRotatedLine(cx, cy,  55, y_rung,  75, y_rung, bank_rad);
            drawRotatedLine(cx, cy,  75, y_rung,  75, y_rung - 10, bank_rad);
        }
    }

    // 3. Fixed Center Aircraft Bore-Sight / Waterline (-v-)
    const int icx = static_cast<int>(cx);
    const int icy = static_cast<int>(cy);
    SDL_RenderDrawLine(renderer_, icx - 35, icy, icx - 12, icy);
    SDL_RenderDrawLine(renderer_, icx - 12, icy, icx - 6,  icy + 8);
    SDL_RenderDrawLine(renderer_, icx - 6,  icy + 8, icx,  icy);
    SDL_RenderDrawLine(renderer_, icx,      icy, icx + 6,  icy + 8);
    SDL_RenderDrawLine(renderer_, icx + 6,  icy + 8, icx + 12, icy);
    SDL_RenderDrawLine(renderer_, icx + 12, icy, icx + 35, icy);

    // 4. Top Heading Tape Box
    SDL_Rect hdg_box = {icx - 160, 20, 320, 36};
    SDL_RenderDrawRect(renderer_, &hdg_box);
    SDL_RenderDrawLine(renderer_, icx, 56, icx - 6, 66);
    SDL_RenderDrawLine(renderer_, icx, 56, icx + 6, 66);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "HDG %03d", static_cast<int>(state.sim_heading_deg) % 360);
    drawText(buf, icx - 42, 30, 3);

    // 5. Right-Side Altitude Tape
    SDL_Rect alt_box = {width_ - 135, 120, 110, 360};
    SDL_RenderDrawRect(renderer_, &alt_box);
    for (int i = -4; i <= 4; ++i) {
        int ty = icy + i * 38;
        SDL_RenderDrawLine(renderer_, width_ - 135, ty, width_ - 120, ty);
    }
    std::snprintf(buf, sizeof(buf), "ALT %05d", static_cast<int>(state.sim_altitude_ft));
    drawText(buf, width_ - 130, 92, 3);

    // 6. Left-Side IMU Telemetry Readout Box
    SDL_Rect imu_box = {25, 120, 165, 110};
    SDL_RenderDrawRect(renderer_, &imu_box);
    std::snprintf(buf, sizeof(buf), "PIT %+.1f", state.pitch_deg);
    drawText(buf, 35, 138, 3);
    std::snprintf(buf, sizeof(buf), "BNK %+.1f", state.bank_deg);
    drawText(buf, 35, 178, 3);

    // 7. Bottom Status Bar
    drawText(sim_mode ? "SRC: SIL SIMULATION" : "SRC: FPGA UART LIVE", 25, height_ - 35, 3);
    drawText("ICE40UP5K VHDL HUD", width_ - 245, height_ - 35, 3);

    SDL_RenderPresent(renderer_);
}