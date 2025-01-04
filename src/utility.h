#ifndef UTILITY_H
#define UTILITY_H

#include <SDL2/SDL.h>

constexpr int TABLE_WIDTH = 800;
constexpr int TABLE_HEIGHT = 400;
constexpr float BALL_RADIUS = 10.0f;
constexpr float DECELERATION = 0.98f;
constexpr int FPS = 60;
constexpr float POCKET_RADIUS = 15.0f;
constexpr float FLOAT_MAX = std::numeric_limits<float>::max();

struct Position {
    float x, y;
};

struct Velocity {
    float dx, dy;
};


// used to draw balls and pockets
void render_draw_filled_circle(SDL_Renderer* renderer, int center_x, int center_y, int radius);

#endif
