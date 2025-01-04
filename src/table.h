#ifndef TABLE_H
#define TABLE_H

#include "utility.h"
#include "ball.h"
#include "cue.h"

#include <string>
#include "SDL2/SDL_ttf.h"
#include <vector>

class Table {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;

    bool is_running;
    Ball cue_ball;
    Cue cue;
    std::list<Ball> balls;
    std::vector<Position> pockets;
    int score;

public:
    Table();
    ~Table();

    void initialize_balls();
    void reset_balls();
    void initialize_pockets();
    void initialize_SDL();
    void process_input();
    void check_collisions();
    void check_pockets();
    void render_text(const std::string& str, int x, int y);
    void render();
    bool is_ball_in_pocket(const Position& ball_pos);
    void update();
    void run();
};

#endif
