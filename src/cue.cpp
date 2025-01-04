
#include "cue.h"

Cue::Cue() : position({0, 0}), length(100), angle(0), power(15.0f), show_guideline(false) {}


float Cue::getAngle() const { return angle; }
float Cue::getPower() const { return power; }
void Cue::setPower(float p) { power = p; }
void Cue::toggle_guideline() { show_guideline = !show_guideline; }

void Cue::update(Position ball_pos, int mouse_x, int mouse_y) {
    angle = std::atan2(mouse_y-ball_pos.y, mouse_x-ball_pos.x);
    position = {ball_pos.x + std::cos(angle)*length, ball_pos.y + std::sin(angle)*length};
}

void Cue::draw(SDL_Renderer* renderer, Position ball_pos) const {
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderDrawLine(renderer, ball_pos.x, ball_pos.y, position.x, position.y);
}


void Cue::draw_guideline(SDL_Renderer* renderer, Position ball_pos, float ball_radius, int table_width, int table_height, const std::list<Ball>& balls) {
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
