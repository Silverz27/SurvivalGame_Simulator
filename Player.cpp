#include "Player.h"
#include <SFML/Graphics.hpp>
#include <algorithm>

using namespace std;
using namespace sf;

Player::Player(Vector2f pos) : hp(MAX_HP), hunger(MAX_HUNGER), power(10.f) {
    shape.setRadius(RADIUS);
    shape.setOrigin({ RADIUS, RADIUS });
    shape.setPosition(pos);
    shape.setFillColor(Color(100, 180, 255));
    shape.setOutlineColor(Color::White);
    shape.setOutlineThickness(2.f);
}

void Player::handleInput(float dt) {
    Vector2f dir(0.f, 0.f);
    if (Keyboard::isKeyPressed(Keyboard::Key::W) ||
        Keyboard::isKeyPressed(Keyboard::Key::Up))    dir.y -= 1.f;
    if (Keyboard::isKeyPressed(Keyboard::Key::S) ||
        Keyboard::isKeyPressed(Keyboard::Key::Down))  dir.y += 1.f;
    if (Keyboard::isKeyPressed(Keyboard::Key::A) ||
        Keyboard::isKeyPressed(Keyboard::Key::Left))  dir.x -= 1.f;
    if (Keyboard::isKeyPressed(Keyboard::Key::D) ||
        Keyboard::isKeyPressed(Keyboard::Key::Right)) dir.x += 1.f;

    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 0.f) dir /= len;

    shape.move(dir * SPEED * dt);
}

void Player::move(sf::Vector2f delta)
{
    shape.setPosition(shape.getPosition() + delta);
}

void Player::update(float dt) {
    // Hunger drains over time
    hunger = max(0.f, hunger - HUNGER_RATE * dt);

    // Starving -> lose HP
    if (hunger <= 0.f)
        hp = max(0.f, hp - HP_DRAIN * dt);

    // Clamp position to  world bounds (match window)
    Vector2f p = shape.getPosition();
    p.x = clamp(p.x, RADIUS, 1920.f - RADIUS);
    p.y = clamp(p.y, RADIUS, 1080.f - RADIUS);
    shape.setPosition(p);
}

void Player::draw(RenderWindow& window) const {
    window.draw(shape);
    drawHUD(window);
}

void Player::eatFood(float nutrition) {
    hunger = min(MAX_HUNGER, hunger + nutrition);
}

void Player::gainPower(float amount) {
    power += amount;
}

void Player::takeDamage(float amount) {
    hp = max(0.f, hp - amount);
}

// HUD bars drawn just above the player
RectangleShape Player::makeBar(Vector2f pos, Vector2f size, float ratio, Color fill, Color bg) {
    // Background
    RectangleShape bar({ size.x * ratio, size.y });
    bar.setPosition(pos);
    bar.setFillColor(fill);
    return bar;
}

void Player::drawHUD(RenderWindow& window) const {
    const Vector2f center = shape.getPosition();
    const float barW = 60.f, barH = 7.f, gap = 10.f;
    const float startX = center.x - barW / 2.f;
    float y = center.y - RADIUS - gap - barH * 3.f - 6.f;

    auto drawBar = [&](float ratio, Color fill, float yOff) {
        // bg
        RectangleShape bg({ barW, barH });
        bg.setPosition({ startX, y + yOff });
        bg.setFillColor(Color(50, 50, 50, 180));
        window.draw(bg);
        // fill
        RectangleShape bar({ barW * ratio, barH });
        bar.setPosition({ startX, y + yOff });
        bar.setFillColor(fill);
        window.draw(bar);
        };

    drawBar(hp / MAX_HP, Color(220, 60, 60), 0.f);          // red  HP
    drawBar(hunger / MAX_HUNGER, Color(240, 180, 40), barH + 2.f);   // yellow hunger
    drawBar(min(power / 100.f, 1.f), Color(80, 200, 120), (barH + 2.f) * 2.f); // green power

    // Power number label
    Font font;
}