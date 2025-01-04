#include "utility.h"

void render_draw_filled_circle(SDL_Renderer* renderer, int center_x, int center_y, int radius) {
    int x = 0;
    int y = radius;
    int decision = 1 - radius;

    while (x <= y) {
        SDL_RenderDrawLine(renderer, center_x - x, center_y + y, center_x + x, center_y + y);
        SDL_RenderDrawLine(renderer, center_x - x, center_y - y, center_x + x, center_y - y);
        SDL_RenderDrawLine(renderer, center_x - y, center_y + x, center_x + y, center_y + x);
        SDL_RenderDrawLine(renderer, center_x - y, center_y - x, center_x + y, center_y - x);

        ++x;
        if (decision < 0) {
            decision += 2*x + 1;
        } else {
            --y;
            decision += 2*(x-y) + 1;
        }
    }
}
