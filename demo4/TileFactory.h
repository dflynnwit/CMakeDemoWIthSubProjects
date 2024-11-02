#ifndef TILEFACTORY_H
#define TILEFACTORY_H

#include <SFML/Graphics.hpp>
#include <unordered_map>

class TileFactory {
public:
    TileFactory(sf::Texture& texture) : m_texture(texture) {
        // Preload sprites for each tile type
        m_sprites[0] = createSprite(0, 0);     // Grass
        m_sprites[1] = createSprite(64, 0);    // Water
        m_sprites[2] = createSprite(128, 0);   // Wall
        m_sprites[3] = createSprite(192, 0);   // Tree
    }

    sf::Sprite getTile(int type) {
        return m_sprites[type];
    }

private:
    sf::Sprite createSprite(int x, int y) {
        sf::Sprite sprite;
        sprite.setTexture(m_texture);
        sprite.setTextureRect(sf::IntRect(x, y, 64, 64));
        return sprite;
    }

    sf::Texture& m_texture;
    std::unordered_map<int, sf::Sprite> m_sprites;
};

#endif

