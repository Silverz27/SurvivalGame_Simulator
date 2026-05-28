#include "Enemy.h"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <string>

using namespace sf;
using namespace std;

// Helpers
static float randF(float lo, float hi) {
    return lo + (hi - lo) * (std::rand() / float(RAND_MAX));
}

Color Enemy::powerToColor(float p) {
    // Low power -> green, medium -> orange, high -> red
    if (p < 40.f)  return Color(80, 200, 80);
    if (p < 70.f)  return Color(240, 160, 40);
    return              Color(220, 50, 50);
}

// Constructor
Enemy::Enemy(Vector2f pos, float power) : power(power) {
    shape.setRadius(RADIUS);
    shape.setOrigin({ RADIUS, RADIUS });
    shape.setPosition(pos);
    shape.setFillColor(powerToColor(power));
    shape.setOutlineColor(Color(255, 255, 255, 120));
    shape.setOutlineThickness(1.5f);
    pickNewWanderDir();
}

// Update
void Enemy::update(float dt, Vector2f playerPos) {
    if (dead) return;

    Vector2f pos = shape.getPosition();
    Vector2f diff = playerPos - pos;
    float dist = sqrt(diff.x * diff.x + diff.y * diff.y);

    Vector2f moveDir;
    float speed;

    if (dist < DETECT_RANGE) {
        // Chase player
        moveDir = diff / dist;
        speed = CHASE_SPEED;
    }
    else {
        // Wander
        wanderTimer -= dt;
        if (wanderTimer <= 0.f) pickNewWanderDir();
        moveDir = wanderDir;
        speed = WANDER_SPEED;
    }

    shape.move(moveDir * speed * dt);

    // Bounce off world edges
    Vector2f p = shape.getPosition();
    if (p.x < RADIUS || p.x > 1920.f - RADIUS) {
        wanderDir.x = -wanderDir.x;
        p.x = clamp(p.x, RADIUS, 1920.f - RADIUS);
    }
    if (p.y < RADIUS || p.y > 1080.f - RADIUS) {
        wanderDir.y = -wanderDir.y;
        p.y = clamp(p.y, RADIUS, 1080.f - RADIUS);
    }
    shape.setPosition(p);
}

// Draw
void Enemy::draw(RenderWindow& window, const Font& font) const {
    if (dead) return;
    window.draw(shape);

    // Power label above the circle
    Text label(font, std::to_string(static_cast<int>(power)), 13);
    label.setFillColor(Color::White);
    label.setStyle(Text::Bold);
    FloatRect bounds = label.getLocalBounds();
    Vector2f pos = shape.getPosition();
    label.setPosition({ pos.x - bounds.size.x / 2.f,
                        pos.y - RADIUS - 18.f });
    window.draw(label);
}

// Private
void Enemy::pickNewWanderDir() {
    float angle = randF(0.f, 2.f * 3.14159f);
    wanderDir = { cos(angle), sin(angle) };
    wanderTimer = randF(1.5f, WANDER_TIME);
}