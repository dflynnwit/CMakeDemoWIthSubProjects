#include <SFML/Graphics.hpp>
#include "Tile.h"
#include "TileFactory.h"
#include <vector>

int main() {
    // Set up the SFML window
    sf::RenderWindow window(sf::VideoMode(800, 600), "Flyweight Pattern Example");

    // Load the sprite sheet texture
    sf::Texture tileTexture;
    if (!tileTexture.loadFromFile("resources/tiles.png")) {
        return -1;
    }

    // Create the TileFactory
    TileFactory tileFactory(tileTexture);

    // Set up a grid of tiles
    const int rows = 10;
    const int cols = 10;
    std::vector<Tile> tiles;

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            // Randomly choose a tile type (0 = grass, 1 = water, 2 = wall, 3 = tree)
            int tileType = rand() % 4;
            tiles.emplace_back(tileFactory.getTile(tileType), sf::Vector2f(x * 64, y * 64));
        }
    }

    // Main game loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();

        // Draw all tiles
        for (const auto& tile : tiles) {
            window.draw(tile);
        }

        window.display();
    }

    return 0;
}