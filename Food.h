#pragma once
#include <SFML/Graphics.hpp>

using namespace sf;

struct Food {
    static constexpr float RADIUS = 8.f;
    static constexpr float NUTRITION = 30.f;

    sf::CircleShape shape;
    bool eaten = false;

    Food(sf::Vector2f pos) {
        shape.setRadius(RADIUS);
        shape.setOrigin({ RADIUS, RADIUS });
        shape.setPosition(pos);
        shape.setFillColor(sf::Color(80, 220, 120));
        shape.setOutlineColor(sf::Color(200, 255, 200, 160));
        shape.setOutlineThickness(1.f);
    }

    sf::Vector2f getPosition() const { return shape.getPosition(); }
};