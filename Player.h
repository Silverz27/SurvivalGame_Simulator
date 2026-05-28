#pragma once
#include <iostream>
#include <SFML/Graphics.hpp>

using namespace sf;
using namespace std;

class Player {
public:
    static constexpr float RADIUS = 18.f;
    static constexpr float SPEED = 200.f;
    static constexpr float MAX_HP = 100.f;
    static constexpr float MAX_HUNGER = 100.f;
    static constexpr float HUNGER_RATE = 3.f;   // per second
    static constexpr float HP_DRAIN = 5.f;   // per second when starving

    Player(Vector2f pos);

    void handleInput(float dt);
    void update(float dt);
    void draw(RenderWindow& window) const;

    Vector2f getPosition() const { return shape.getPosition(); }
    float getHP() const { return hp; }
    float getHunger() const { return hunger; }
    float getPower() const { return power; }
    bool  isDead() const { return hp <= 0.f; }

    void eatFood(float nutrition);   // called by Game on pickup
    void gainPower(float amount);
    void takeDamage(float amount);
    void move(Vector2f delta);

    CircleShape getPlayer() { return shape; }

private:
    CircleShape shape;
    float hp;
    float hunger;
    float power;

    void drawHUD(RenderWindow& window) const;
    static RectangleShape makeBar(Vector2f pos, Vector2f size, float ratio, Color fill, Color bg);
};