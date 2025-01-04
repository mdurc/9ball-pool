
#ifndef BALL_H
#define BALL_H

#include "utility.h"

class Ball {
private:
    Position m_pos;
    Velocity m_vel;
    Position m_initial_pos;
    SDL_Color color;

public:
    Ball(Position pos, SDL_Color col);

    Position get_position() const;
    Velocity get_velocity() const;
    bool is_moving() const;
    void reset();
    void draw(SDL_Renderer* renderer) const;
    void move();
    void apply_force(float angle, float power);
    bool check_collision(const Ball& other);
    void resolve_collision(Ball& other);
};

#endif
