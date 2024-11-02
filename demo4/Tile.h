#ifndef TILE_H
#define TILE_H

#include <SFML/Graphics.hpp>

class Tile : public sf::Drawable {
public:
    Tile(sf::Sprite sprite, sf::Vector2f position) 
        : m_sprite(sprite) {
        m_sprite.setPosition(position);
    }

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        target.draw(m_sprite, states);
    }

private:
    sf::Sprite m_sprite;
};

#endif

