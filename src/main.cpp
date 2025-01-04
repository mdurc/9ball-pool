#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <list>

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

class Ball {
private:
    Position m_pos;
    Velocity m_vel;
    Position m_initial_pos;
    SDL_Color color;

public:
    Ball(Position pos, SDL_Color col)
        : m_pos(pos), m_vel({0, 0}), m_initial_pos(pos), color(col) {}

    Position get_position() const { return m_pos; }
    Velocity get_velocity() const { return m_vel; } // todo: unused
    bool is_moving() const { return m_vel.dx != 0 || m_vel.dy != 0; }

    void reset() {
        m_pos = m_initial_pos;
        m_vel = {0,0};
    }
    
    void draw(SDL_Renderer* renderer) const {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        render_draw_filled_circle(renderer, m_pos.x, m_pos.y, BALL_RADIUS);
    }

    void move() {
        m_pos.x += m_vel.dx;
        m_pos.y += m_vel.dy;
        
        m_vel.dx *= DECELERATION;
        m_vel.dy *= DECELERATION;

        // trunc velocities to near-zero
        if (std::fabs(m_vel.dx) < 0.1f) m_vel.dx = 0;
        if (std::fabs(m_vel.dy) < 0.1f) m_vel.dy = 0;

        // bounce off table edges
        if (m_pos.x - BALL_RADIUS < 0 || m_pos.x + BALL_RADIUS > TABLE_WIDTH) {
            m_vel.dx = -m_vel.dx;
        }
        if (m_pos.y - BALL_RADIUS < 0 || m_pos.y + BALL_RADIUS > TABLE_HEIGHT) {
            m_vel.dy = -m_vel.dy;
        }

        // assert position is within bounds
        m_pos.x = std::clamp(m_pos.x, BALL_RADIUS, (float)TABLE_WIDTH - BALL_RADIUS);
        m_pos.y = std::clamp(m_pos.y, BALL_RADIUS, (float)TABLE_HEIGHT - BALL_RADIUS);
    }

    void apply_force(float angle, float power) {
        m_vel.dx += power * std::cos(angle);
        m_vel.dy += power * std::sin(angle);
    }

    bool check_collision(const Ball& other) {
        int dx = m_pos.x - other.m_pos.x;
        int dy = m_pos.y - other.m_pos.y;
        float dist = std::sqrt(dx*dx + dy*dy);
        return dist <= (2*BALL_RADIUS);
    }

    // move both balls (this, and other)
    void resolve_collision(Ball& other) {
        float dx = other.m_pos.x - m_pos.x;
        float dy = other.m_pos.y - m_pos.y;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist < 0.0001f) return; // div by zero

        // normalize
        dx /= dist;
        dy /= dist;
        
        // relative vel
        float v_relX = m_vel.dx - other.m_vel.dx;
        float v_relY = m_vel.dy - other.m_vel.dy;

        float dot_prod = v_relX*dx + v_relY*dy;

        if (dot_prod > 0) {
            // they are moving towards each other
            m_vel.dx -= dot_prod * dx;
            m_vel.dy -= dot_prod * dy;
            other.m_vel.dx += dot_prod * dx;
            other.m_vel.dy += dot_prod * dy;

            // prevent overlap
            float overlap = (2*BALL_RADIUS-dist)/2.0f;
            m_pos.x -= overlap * dx;
            m_pos.y -= overlap * dy;
            other.m_pos.x += overlap * dx;
            other.m_pos.y += overlap * dy;
        }
    }
};

class Cue {
private:
    Position position;
    int length;
    float angle;
    float power;
    bool show_guideline;

public:
    Cue() : position({0, 0}), length(100), angle(0), power(15.0f), show_guideline(false) {}

    float getAngle() const { return angle; }
    float getPower() const { return power; }
    void setPower(float p) { power = p; }
    void toggle_guideline() { show_guideline = !show_guideline; }

    void update(Position ball_pos, int mouse_x, int mouse_y) {
        angle = std::atan2(mouse_y-ball_pos.y, mouse_x-ball_pos.x);
        position = {ball_pos.x + std::cos(angle)*length, ball_pos.y + std::sin(angle)*length};
    }

    void draw(SDL_Renderer* renderer, Position ball_pos) const {
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderDrawLine(renderer, ball_pos.x, ball_pos.y, position.x, position.y);
    }


    void draw_guideline(SDL_Renderer* renderer, Position ball_pos, float ball_radius, int table_width, int table_height, const std::list<Ball>& balls) {
        if (!show_guideline) return;

        // main aiming guide (default and permanent), yellow, coming out of the ball
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderDrawLine(renderer, ball_pos.x, ball_pos.y, position.x, position.y);

        // normalize direction of the guideline
        float dx = position.x - ball_pos.x;
        float dy = position.y - ball_pos.y;
        float length = std::sqrt(dx*dx + dy*dy);
        if (length != 0) {
            dx /= length;
            dy /= length;
        }

        float nearest_collision_dist = FLOAT_MAX;
        const Ball* target_ball = nullptr;
        Position collision_point = {0, 0};
        Position cue_bounce = {0, 0};
        Position hit_ball_end = {0, 0};

        for (const Ball& ball : balls) {
            Position target_pos = ball.get_position();
            float rel_x = target_pos.x - ball_pos.x;
            float rel_y = target_pos.y - ball_pos.y;
            float projection = (rel_x*dx + rel_y*dy);
            Position projected_point = {ball_pos.x + projection * dx, ball_pos.y + projection * dy};

            float dist_to_proj = std::sqrt(std::pow(projected_point.x - target_pos.x, 2) + std::pow(projected_point.y - target_pos.y, 2));

            if (dist_to_proj <= ball_radius && projection > 0) {
                // collision
                float overlap_dist = std::sqrt(std::pow(ball_radius, 2) - std::pow(dist_to_proj, 2));
                float collision_dist = projection - overlap_dist;

                if (collision_dist < nearest_collision_dist) {
                    // now update the ball
                    nearest_collision_dist = collision_dist;
                    target_ball = &ball;

                    collision_point = {ball_pos.x + dx*collision_dist, ball_pos.y + dy*collision_dist};

                    // overalp correction
                    float dist = std::sqrt(std::pow(target_pos.x - collision_point.x, 2) + std::pow(target_pos.y - collision_point.y, 2));
                    if (dist < 2 * ball_radius) {
                        float overlap = (2 * ball_radius - dist) / 2.0f;
                        collision_point.x -= overlap * dx;
                        collision_point.y -= overlap * dy;
                    }

                    // reflection of projected trajectory
                    float nx = (target_pos.x - collision_point.x) / (2*ball_radius);
                    float ny = (target_pos.y - collision_point.y) / (2*ball_radius);

                    float dot = dx*nx + dy*ny;
                    float cue_dx = dx - 2*dot*nx;
                    float cue_dy = dy - 2*dot*ny;
                    cue_bounce = {collision_point.x + cue_dx*200, collision_point.y + cue_dy*200};

                    float hit_dx = nx*(dx*nx + dy*ny);
                    float hit_dy = ny*(dx*nx + dy*ny);
                    hit_ball_end = {target_pos.x + hit_dx*200, target_pos.y + hit_dy*200};
                }
            }
        }

        if (target_ball) {
            // draw collision path if we found a target ball
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // yellow
            SDL_RenderDrawLine(renderer, ball_pos.x, ball_pos.y, collision_point.x, collision_point.y);

            // cue ball new bounce
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // green
            SDL_RenderDrawLine(renderer, collision_point.x, collision_point.y, cue_bounce.x, cue_bounce.y);

            // target ball trajectory
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // blue
            SDL_RenderDrawLine(renderer, target_ball->get_position().x, target_ball->get_position().y, hit_ball_end.x, hit_ball_end.y);
        } else {
            // clue ball is projected to bounce off of the wall
            float tMin = FLOAT_MAX,
                  tTop = FLOAT_MAX,
                  tBottom = FLOAT_MAX,
                  tLeft = FLOAT_MAX,
                  tRight = FLOAT_MAX;

            Position bounce_point;

            if (dy != 0) {
                tTop = (ball_radius - ball_pos.y) / dy;
                tBottom = (table_height - ball_radius - ball_pos.y) / dy;
            }

            if (dx != 0) {
                tLeft = (ball_radius - ball_pos.x) / dx;
                tRight = (table_width - ball_radius - ball_pos.x) / dx;
            }

            if (tTop > 0) tMin = std::min(tMin, tTop);
            if (tBottom > 0) tMin = std::min(tMin, tBottom);
            if (tLeft > 0) tMin = std::min(tMin, tLeft);
            if (tRight > 0) tMin = std::min(tMin, tRight);

            if (tMin < FLOAT_MAX) {
                bounce_point = {ball_pos.x + dx * tMin, ball_pos.y + dy * tMin};

                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // green
                SDL_RenderDrawLine(renderer, ball_pos.x, ball_pos.y, bounce_point.x, bounce_point.y);

                if (tMin == tTop || tMin == tBottom) dy = -dy;
                if (tMin == tLeft || tMin == tRight) dx = -dx;

                Position reflect_point = {bounce_point.x + dx * 100, bounce_point.y + dy * 100};
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red for the reflection
                SDL_RenderDrawLine(renderer, bounce_point.x, bounce_point.y, reflect_point.x, reflect_point.y);
            }
        }
    }
};

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
    Table()
        : is_running(true),
          cue_ball({100, 200}, {255, 255, 255, 255}),
          score(0) {
        initialize_balls();
        initialize_pockets();
        initialize_SDL();
    }

    ~Table() {
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void initialize_balls() {
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

    void reset_balls() {
        initialize_balls();
        cue_ball.reset();
    }

    void initialize_pockets() {
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

    void initialize_SDL() {
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

    void process_input() {
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

    void check_collisions() {
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

    void check_pockets() {
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

    void render_text(const std::string& str, int x, int y) {
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

    void render() {
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

    bool is_ball_in_pocket(const Position& ball_pos) {
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

    void update() {
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

    void run() {
        while (is_running) {
            process_input();
            update();
            render();
            SDL_Delay(1000 / FPS);
        }
    }
};

int main() {
    Table table;
    table.run();
    return 0;
}
