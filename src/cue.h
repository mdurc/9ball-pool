#ifndef CUE_H
#define CUE_H

#include "utility.h"

#include <SDL2/SDL.h>
#include <list>
#include "ball.h"

class Cue {
private:
    Position position;
    int length;
    float angle;
    float power;
    bool show_guideline;

public:
    Cue();
    float getAngle() const;
    float getPower() const;
    void setPower(float p);
    void toggle_guideline();
    void update(Position ball_pos, int mouse_x, int mouse_y);
    void draw(SDL_Renderer* renderer, Position ball_pos) const;
    void draw_guideline(SDL_Renderer *renderer, Position ball_pos,
                        float ball_radius, int table_width, int table_height,
                        const std::list<Ball> &balls);
};

#endif
