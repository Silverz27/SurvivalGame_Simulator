#pragma once
#include <SFML/Graphics.hpp>

using namespace sf;

class Enemy {
public:
    static constexpr float RADIUS = 16.f;
    static constexpr float WANDER_SPEED = 60.f;
    static constexpr float CHASE_SPEED = 120.f;
    static constexpr float DETECT_RANGE = 200.f;  // pixel radius to start chasing
    static constexpr float WANDER_TIME = 2.5f;   // seconds before picking new direction

    Enemy(Vector2f pos, float power);

    void update(float dt, Vector2f playerPos);
    void draw(RenderWindow& window, const Font& font) const;

    Vector2f getPosition() const { return shape.getPosition(); }
    float getPower() const { return power; }
    bool  isDead()   const { return dead; }
    void  kill() { dead = true; }

private:
    CircleShape shape;
    float power;
    bool dead = false;

    // Wander state
    Vector2f wanderDir;
    float wanderTimer = 0.f;

    void pickNewWanderDir();
    static Color powerToColor(float p);
};