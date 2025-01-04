#include "table.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include <vector>
#include <iostream>

Table::Table()
    : is_running(true),
      cue_ball({100, 200}, {255, 255, 255, 255}),
      score(0) {
    initialize_balls();
    initialize_pockets();
    initialize_SDL();
}

Table::~Table() {
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Table::initialize_balls() {
    balls.clear();
    balls.emplace_back(Position{400, 200}, SDL_Color{255, 255, 0, 255});
    balls.emplace_back(Position{420, 190}, SDL_Color{0, 0, 255, 255});
    balls.emplace_back(Position{420, 210}, SDL_Color{255, 0, 0, 255});
    balls.emplace_back(Position{440, 180}, SDL_Color{255, 165, 0, 255});
    balls.emplace_back(Position{440, 200}, SDL_Color{0, 128, 0, 255});
    balls.emplace_back(Position{440, 220}, SDL_Color{128, 0, 128, 255});
    balls.emplace_back(Position{460, 210}, SDL_Color{255, 20, 147, 255});
    balls.emplace_back(Position{460, 190}, SDL_Color{0, 128, 128, 255});
    balls.emplace_back(Position{480, 200}, SDL_Color{128, 0, 0, 255});
}

void Table::reset_balls() {
    initialize_balls();
    cue_ball.reset();
}

void Table::initialize_pockets() {
    const int offset = 20;

    pockets = {
        {offset, offset},
        {TABLE_WIDTH - offset, offset},
        {offset, TABLE_HEIGHT - offset},
        {TABLE_WIDTH - offset, TABLE_HEIGHT - offset},
        {(int)(TABLE_WIDTH/2), offset},
        {(int)(TABLE_WIDTH/2), TABLE_HEIGHT - offset}
    };
}

void Table::initialize_SDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        std::cerr << "SDL Initialization Error: " << SDL_GetError() << "\n";
        exit(EXIT_FAILURE);
    }

    if (TTF_Init() == -1){
        std::cerr << "SDL_ttf Initialization Error: " << TTF_GetError() << "\n";
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    font = TTF_OpenFont("/Users/mdurcan/Library/Fonts/FSEX302.ttf", 24);
    if (!font){
        std::cerr << "Font Loading Error: " << TTF_GetError() << "\n";
        TTF_Quit();
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    window = SDL_CreateWindow("9 Ball Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, TABLE_WIDTH, TABLE_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window){
        std::cerr << "Window Creation Error: " << SDL_GetError() << "\n";
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer){
        std::cerr << "Renderer Creation Error: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
}

void Table::process_input() {
    const float power_step = 1.0f;
    const float min_power = 5.0f;
    const float max_power = FPS/3.0f;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            is_running = false;
        }

        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            if (!cue_ball.is_moving()) {
                int mouse_x, mouse_y;
                SDL_GetMouseState(&mouse_x, &mouse_y);
                float angle = cue.getAngle();
                cue_ball.apply_force(angle, cue.getPower());
            }
        }

        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_g:
                    cue.toggle_guideline();
                    break;
                case SDLK_UP:
                    cue.setPower(std::min(cue.getPower() + power_step, max_power));
                    break;
                case SDLK_DOWN:
                    cue.setPower(std::max(cue.getPower() - power_step, min_power));
                    break;
            }
        }
    }
}

void Table::check_collisions() {
    // check collision between cueball and other balls
    for (Ball& ball : balls) {
        if (cue_ball.check_collision(ball)) {
            cue_ball.resolve_collision(ball);
        }
    }

    // now non cueballs against other non cueballs
    std::list<Ball>::iterator i,j;
    for (i=balls.begin(); i!=balls.end(); ++i){
        j=i;
        ++j; // one ahead of iterator i
        for (;j!=balls.end(); ++j){
            if (i->check_collision(*j)){
                i->resolve_collision(*j); // handles the movement of both balls
            }
        }
    }
}

void Table::check_pockets() {
    std::list<Ball>::iterator i = balls.begin();
    while (i!=balls.end()){
        if (is_ball_in_pocket(i->get_position())){
            i = balls.erase(i);
            ++score;
        }else{
            ++i;
        }
    }
}

void Table::render_text(const std::string& str, int x, int y) {
    SDL_Color text_color = {255, 255, 255, 255}; // white

    SDL_Surface* text_surface = TTF_RenderText_Solid(font, str.c_str(), text_color);
    if (!text_surface) {
        std::cerr << "Text Rendering Error: " << TTF_GetError() << "\n";
        return;
    }

    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    SDL_Rect text_rect = {x, y, text_surface->w, text_surface->h};

    SDL_FreeSurface(text_surface);

    SDL_RenderCopy(renderer, text_texture, nullptr, &text_rect);
    SDL_DestroyTexture(text_texture);
}

void Table::render() {
    SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255); // green
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black
    for (const Position& pocket : pockets) {
        render_draw_filled_circle(renderer, pocket.x, pocket.y, POCKET_RADIUS);
    }

    cue_ball.draw(renderer);
    for (const Ball& ball : balls) {
        ball.draw(renderer);
    }

    cue.draw(renderer, cue_ball.get_position());
    cue.draw_guideline(renderer, cue_ball.get_position(), BALL_RADIUS, TABLE_WIDTH, TABLE_HEIGHT, balls);

    render_text("Score: "+std::to_string(score), 40, 20);
    render_text("Power: "+std::to_string((int)cue.getPower()), TABLE_WIDTH-150, 20);
    render_text("[G] to toggle guideline", 100, TABLE_HEIGHT-35);

    SDL_RenderPresent(renderer);
}

bool Table::is_ball_in_pocket(const Position& ball_pos) {
    float dx, dy, dist;
    for (const Position& pocket : pockets) {
        dx = ball_pos.x - pocket.x;
        dy = ball_pos.y - pocket.y;
        dist = std::sqrt(dx*dx + dy*dy);
        if (dist <= POCKET_RADIUS) {
            return true;
        }
    }
    return false;
}

void Table::update() {
    cue_ball.move();
    for (Ball& ball : balls) {
        ball.move();
    }

    // if cueball is in the pocket (scratch), then penalize
    if (is_ball_in_pocket(cue_ball.get_position())) {
        score -= 5;
        cue_ball.reset();
    }

    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    cue.update(cue_ball.get_position(), mouse_x, mouse_y);

    check_collisions();
    check_pockets();

    // reset non-cue balls only when all are pocketed
    if (balls.empty()) {
        reset_balls();
    }
}

void Table::run() {
    while (is_running) {
        process_input();
        update();
        render();
        SDL_Delay(1000 / FPS);
    }
}
