
#include "ball.h"
#include <algorithm>

Ball::Ball(Position pos, SDL_Color col) : m_pos(pos), m_vel({0, 0}), m_initial_pos(pos), color(col) {}

Position Ball::get_position() const { return m_pos; }
Velocity Ball::get_velocity() const { return m_vel; }
bool Ball::is_moving() const { return m_vel.dx != 0 || m_vel.dy != 0; }

void Ball::reset() {
    m_pos = m_initial_pos;
    m_vel = {0,0};
}

void Ball::draw(SDL_Renderer* renderer) const {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    render_draw_filled_circle(renderer, m_pos.x, m_pos.y, BALL_RADIUS);
}

void Ball::move() {
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

void Ball::apply_force(float angle, float power) {
    m_vel.dx += power * std::cos(angle);
    m_vel.dy += power * std::sin(angle);
}

bool Ball::check_collision(const Ball& other) {
    int dx = m_pos.x - other.m_pos.x;
    int dy = m_pos.y - other.m_pos.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    return dist <= (2*BALL_RADIUS);
}

// move both balls (this, and other)
void Ball::resolve_collision(Ball& other) {
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
