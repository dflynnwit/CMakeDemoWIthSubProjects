// main.cpp

#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <memory>
#include <algorithm>
#include <random>
#include <iostream>
#include <queue>
#include <unordered_map>

// Screen dimensions
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// Global variables
bool debug = true;
bool autonomousMode = false;

// Forward declarations
class InfluenceMap;

// Random number generator
std::default_random_engine generator;
std::uniform_real_distribution<float> distribution(-30.0f, 30.0f);

// Base Unit Class
class Unit {
protected:
    sf::ConvexShape shape;
    sf::RectangleShape healthBar;
    float health;
    bool alive;
    sf::Vector2f position;
    int teamSign; // Positive for friendly, negative for enemy
    sf::Texture* texture;
    sf::Sprite sprite;
    sf::Vector2f targetPosition;
    float speed;
    bool selected;
    float attackRange;
    float attackDamage;
    float projectileSpeed;
    float attackCooldown;      // Time until next attack is allowed
    float attackCooldownTime;  // Total cooldown duration
    bool autonomous;

public:
    Unit(const sf::Vector2f& pos, int team, sf::Texture* tex = nullptr)
            : position(pos), health(100.0f), alive(true), teamSign(team), texture(tex),
              selected(false), speed(100.0f), attackCooldown(0.0f), autonomous(false) {
        if (texture) {
            sprite.setTexture(*texture);
            sprite.setPosition(position);
        } else {
            shape.setPosition(position);
        }
        healthBar.setSize(sf::Vector2f(20, 4));
        healthBar.setFillColor(sf::Color::Green);
        healthBar.setPosition(position + sf::Vector2f(-10, -20));
        targetPosition = position;
        attackRange = 150.0f;
        attackDamage = 10.0f;
        projectileSpeed = 250.0f;
        attackCooldownTime = 1.5f;
    }

    virtual void update(float deltaTime, const std::vector<std::shared_ptr<Unit>>& units,
                        const class InfluenceMap& influenceMap) = 0;

    virtual void draw(sf::RenderWindow& window) {
        if (texture) {
            window.draw(sprite);
        } else {
            if (selected) {
                shape.setOutlineColor(sf::Color::Yellow);
                shape.setOutlineThickness(2.0f);
            } else {
                shape.setOutlineThickness(0.0f);
            }
            window.draw(shape);
        }
        window.draw(healthBar);

        // Draw planned path
        if (debug) {
            sf::VertexArray path(sf::LinesStrip, 2);
            path[0].position = position;
            path[0].color = (teamSign > 0) ? sf::Color::Green : sf::Color::Red;
            path[1].position = targetPosition;
            path[1].color = (teamSign > 0) ? sf::Color::Green : sf::Color::Red;
            window.draw(path);
        }
    }

    virtual void takeDamage(float amount) {
        health -= amount;
        if (health < 0) health = 0;
        healthBar.setSize(sf::Vector2f(20 * (health / 100.0f), 4));
        if (health <= 0) {
            alive = false;
        }
    }

    sf::Vector2f getPosition() const {
        return position;
    }

    bool isAlive() const {
        return alive;
    }

    int getTeamSign() const {
        return teamSign;
    }

    void setTargetPosition(const sf::Vector2f& pos) {
        targetPosition = pos;
    }

    bool containsPoint(const sf::Vector2f& point) const {
        if (texture) {
            return sprite.getGlobalBounds().contains(point);
        } else {
            return shape.getGlobalBounds().contains(point);
        }
    }

    void setSelected(bool sel) {
        selected = sel;
    }

    void setAutonomous(bool autoMode) {
        autonomous = autoMode;
    }

    bool isAutonomous() const {
        return autonomous;
    }

protected:
    void moveTowardsTarget(float deltaTime) {
        // Movement logic towards target position
        sf::Vector2f direction = targetPosition - position;
        float distance = sqrt(direction.x * direction.x + direction.y * direction.y);
        if (distance > 1.0f) {
            direction /= distance;
            sf::Vector2f newPosition = position + direction * speed * deltaTime;

            // Wraparound logic for newPosition
            if (newPosition.x < 0) newPosition.x += SCREEN_WIDTH;
            if (newPosition.x >= SCREEN_WIDTH) newPosition.x -= SCREEN_WIDTH;
            if (newPosition.y < 0) newPosition.y += SCREEN_HEIGHT;
            if (newPosition.y >= SCREEN_HEIGHT) newPosition.y -= SCREEN_HEIGHT;

            position = newPosition;
        }
    }

    void updateGraphics() {
        // Wraparound logic for position
        if (position.x < 0) position.x += SCREEN_WIDTH;
        if (position.x >= SCREEN_WIDTH) position.x -= SCREEN_WIDTH;
        if (position.y < 0) position.y += SCREEN_HEIGHT;
        if (position.y >= SCREEN_HEIGHT) position.y -= SCREEN_HEIGHT;

        if (texture) {
            sprite.setPosition(position);
        } else {
            shape.setPosition(position);
        }
        healthBar.setPosition(position + sf::Vector2f(-10, -20));
    }

    virtual void attack(float deltaTime, const std::vector<std::shared_ptr<Unit>>& units) = 0;
};

// Projectile Class
class Projectile {
public:
    sf::CircleShape shape;
    sf::Vector2f position;
    sf::Vector2f velocity;
    int teamSign;
    float damage;
    bool alive;
    float maxDistance;
    sf::Vector2f startPos;

    Projectile(const sf::Vector2f& pos, const sf::Vector2f& target, int team, float dmg, float speed)
            : position(pos), teamSign(team), damage(dmg), alive(true), maxDistance(200.0f), startPos(pos) {
        sf::Vector2f direction = target - pos;
        float distance = sqrt(direction.x * direction.x + direction.y * direction.y);
        if (distance != 0) {
            direction /= distance;
            velocity = direction * speed;
        } else {
            velocity = sf::Vector2f(0, 0);
        }

        shape.setRadius(3);
        shape.setFillColor((teamSign > 0) ? sf::Color::Cyan : sf::Color::Magenta);
        shape.setPosition(position);
    }

    void update(float deltaTime, const std::vector<std::shared_ptr<Unit>>& units) {
        position += velocity * deltaTime;

        // Wraparound logic for position
        if (position.x < 0) position.x += SCREEN_WIDTH;
        if (position.x >= SCREEN_WIDTH) position.x -= SCREEN_WIDTH;
        if (position.y < 0) position.y += SCREEN_HEIGHT;
        if (position.y >= SCREEN_HEIGHT) position.y -= SCREEN_HEIGHT;

        shape.setPosition(position);

        // Check for collision with units
        for (const auto& unit : units) {
            if (unit->getTeamSign() != teamSign && unit->isAlive()) {
                if (unit->containsPoint(position)) {
                    unit->takeDamage(damage);
                    alive = false;
                    break;
                }
            }
        }

        // Check if projectile has exceeded max distance
        float traveledDistance = calculateDistance(startPos, position);
        if (traveledDistance > maxDistance) {
            alive = false;
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
    }

private:
    float calculateDistance(const sf::Vector2f& pos1, const sf::Vector2f& pos2) {
        sf::Vector2f delta = pos2 - pos1;
        return sqrt(delta.x * delta.x + delta.y * delta.y);
    }
};

// Influence Map Class
class InfluenceMap {
private:
    int width, height;
    int cellSize;
    std::vector<std::vector<float>> mapData;

    sf::Font font;

public:
    InfluenceMap(int w, int h, int cSize)
            : width(w), height(h), cellSize(cSize) {
        mapData.resize(height / cellSize, std::vector<float>(width / cellSize, 0.0f));
        font.loadFromFile("arial.ttf"); // Ensure you have a font file in your directory
    }

    void update(const std::vector<std::shared_ptr<Unit>>& units) {
        // Reset map data
        for (auto& row : mapData) {
            std::fill(row.begin(), row.end(), 0.0f);
        }

        // Update influence based on units
        for (const auto& unit : units) {
            applyInfluence(unit);
        }
    }

    void draw(sf::RenderWindow& window) {
        if (!debug) return;

        sf::Text text;
        text.setFont(font);
        text.setCharacterSize(10);
        text.setFillColor(sf::Color::White);

        for (int y = 0; y < mapData.size(); ++y) {
            for (int x = 0; x < mapData[0].size(); ++x) {
                sf::RectangleShape cell(sf::Vector2f(cellSize, cellSize));
                cell.setPosition(x * cellSize, y * cellSize);
                cell.setOutlineThickness(1.0f);
                cell.setOutlineColor(sf::Color(50, 50, 50));

                float value = mapData[y][x];
                if (value != 0) {
                    if (value > 0) {
                        cell.setFillColor(sf::Color(0, 0, 255, std::min(50 + std::abs(value) * 20, 255.0f)));
                    } else if (value < 0) {
                        cell.setFillColor(sf::Color(255, 0, 0, std::min(50 + std::abs(value) * 20, 255.0f)));
                    }
                } else {
                    cell.setFillColor(sf::Color(0, 0, 0, 0));
                }
                window.draw(cell);

                // Draw influence value
                if (debug) {
                    text.setString(std::to_string(static_cast<int>(value)));
                    text.setPosition(x * cellSize + 2, y * cellSize + 2);
                    window.draw(text);
                }
            }
        }
    }

    float getInfluenceAtPosition(const sf::Vector2f& position) const {
        int x = static_cast<int>(position.x / cellSize);
        int y = static_cast<int>(position.y / cellSize);

        if (x < 0 || x >= mapData[0].size() || y < 0 || y >= mapData.size())
            return 0.0f;

        return mapData[y][x];
    }

private:
    void applyInfluence(const std::shared_ptr<Unit>& unit) {
        int influenceRadius = 3; // Number of cells around the unit to spread influence
        sf::Vector2f unitPos = unit->getPosition();
        int unitX = static_cast<int>(unitPos.x / cellSize);
        int unitY = static_cast<int>(unitPos.y / cellSize);

        for (int y = -influenceRadius; y <= influenceRadius; ++y) {
            for (int x = -influenceRadius; x <= influenceRadius; ++x) {
                int cellX = unitX + x;
                int cellY = unitY + y;

                if (cellX < 0 || cellX >= mapData[0].size() || cellY < 0 || cellY >= mapData.size())
                    continue;

                float distance = sqrt(x * x + y * y);
                if (distance <= influenceRadius) {
                    // Influence decreases with distance
                    float influenceValue = unit->getTeamSign() * (1.0f / (1.0f + distance));
                    mapData[cellY][cellX] += influenceValue;
                }
            }
        }
    }
};

// Friendly Unit Class
class FriendlyUnit : public Unit {
private:
    std::vector<Projectile> projectiles;

public:
    FriendlyUnit(const sf::Vector2f& pos, sf::Texture* tex = nullptr)
            : Unit(pos, 1, tex) {
        if (!texture) {
            shape.setPointCount(3);
            shape.setPoint(0, sf::Vector2f(0, -10));
            shape.setPoint(1, sf::Vector2f(10, 10));
            shape.setPoint(2, sf::Vector2f(-10, 10));
            shape.setFillColor(sf::Color::Blue);
            shape.setPosition(position);
        }
    }

    void update(float deltaTime, const std::vector<std::shared_ptr<Unit>>& units,
                const InfluenceMap& influenceMap) override {
        if (autonomous || selected) {
            moveTowardsTarget(deltaTime);
        }
        attack(deltaTime, units);
        updateGraphics();

        // Update projectiles
        for (auto& projectile : projectiles) {
            projectile.update(deltaTime, units);
        }
        // Remove dead projectiles
        projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                         [](const Projectile& p) { return !p.alive; }),
                          projectiles.end());
    }

    void draw(sf::RenderWindow& window) override {
        Unit::draw(window);
        // Draw projectiles
        for (auto& projectile : projectiles) {
            projectile.draw(window);
        }
    }

protected:
    void attack(float deltaTime, const std::vector<std::shared_ptr<Unit>>& units) override {
        // Update attack cooldown
        if (attackCooldown > 0.0f) {
            attackCooldown -= deltaTime;
            return;
        }

        // Find nearest enemy within attack range
        float minDistance = attackRange;
        std::shared_ptr<Unit> nearestEnemy = nullptr;

        for (const auto& unit : units) {
            if (unit->getTeamSign() != teamSign && unit->isAlive()) {
                float dist = calculateDistance(position, unit->getPosition());
                if (dist < minDistance) {
                    minDistance = dist;
                    nearestEnemy = unit;
                }
            }
        }

        // Attack if enemy found
        if (nearestEnemy) {
            // Fire projectile
            projectiles.emplace_back(position, nearestEnemy->getPosition(), teamSign, attackDamage, projectileSpeed);
            attackCooldown = attackCooldownTime; // Reset cooldown
        }
    }

    float calculateDistance(const sf::Vector2f& pos1, const sf::Vector2f& pos2) {
        sf::Vector2f delta = pos2 - pos1;
        return sqrt(delta.x * delta.x + delta.y * delta.y);
    }
};

// Enemy Unit Class
class EnemyUnit : public Unit {
private:
    enum State { Idle, Attack, Evade } state;
    std::vector<Projectile> projectiles;

public:
    EnemyUnit(const sf::Vector2f& pos, sf::Texture* tex = nullptr)
            : Unit(pos, -1, tex), state(Idle) {
        speed = 80.0f;
        attackDamage = 8.0f;
        attackCooldownTime = 2.0f;
        if (!texture) {
            shape.setPointCount(4);
            shape.setPoint(0, sf::Vector2f(-10, -10));
            shape.setPoint(1, sf::Vector2f(10, -10));
            shape.setPoint(2, sf::Vector2f(10, 10));
            shape.setPoint(3, sf::Vector2f(-10, 10));
            shape.setFillColor(sf::Color::Red);
            shape.setPosition(position);
        }
    }

    void update(float deltaTime, const std::vector<std::shared_ptr<Unit>>& units,
                const InfluenceMap& influenceMap) override {
        // Use influence map to decide movement
        if (autonomous) {
            makeDecision(units, influenceMap);
        }

        switch (state) {
            case Idle:
                // Idle behavior
                break;
            case Attack:
                attack(deltaTime, units);
                moveTowardsTarget(deltaTime);
                break;
            case Evade:
                evade(deltaTime);
                break;
        }
        updateGraphics();

        // Update projectiles
        for (auto& projectile : projectiles) {
            projectile.update(deltaTime, units);
        }
        // Remove dead projectiles
        projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                         [](const Projectile& p) { return !p.alive; }),
                          projectiles.end());
    }

    void draw(sf::RenderWindow& window) override {
        Unit::draw(window);
        // Draw projectiles
        for (auto& projectile : projectiles) {
            projectile.draw(window);
        }
    }

protected:
    void attack(float deltaTime, const std::vector<std::shared_ptr<Unit>>& units) override {
        // Update attack cooldown
        if (attackCooldown > 0.0f) {
            attackCooldown -= deltaTime;
            return;
        }

        // Find nearest enemy within attack range
        float minDistance = attackRange;
        std::shared_ptr<Unit> nearestEnemy = nullptr;

        for (const auto& unit : units) {
            if (unit->getTeamSign() != teamSign && unit->isAlive()) {
                float dist = calculateDistance(position, unit->getPosition());
                if (dist < minDistance) {
                    minDistance = dist;
                    nearestEnemy = unit;
                }
            }
        }

        if (nearestEnemy) {
            targetPosition = nearestEnemy->getPosition();
            // Fire projectile
            projectiles.emplace_back(position, nearestEnemy->getPosition(), teamSign, attackDamage, projectileSpeed);
            attackCooldown = attackCooldownTime; // Reset cooldown
        } else {
            state = Idle;
        }
    }

    void makeDecision(const std::vector<std::shared_ptr<Unit>>& units, const InfluenceMap& influenceMap) {
        // Get the influence value at the current position
        float currentInfluence = influenceMap.getInfluenceAtPosition(position);

        if (currentInfluence > -0.5f) {
            // High player influence, evade
            state = Evade;
        } else if (currentInfluence < -1.0f) {
            // Player influence is weak, attack
            state = Attack;
            findTarget(units);
        } else {
            state = Idle;
        }
    }

    void findTarget(const std::vector<std::shared_ptr<Unit>>& units) {
        // Simple AI to find the closest friendly unit
        float minDistance = std::numeric_limits<float>::max();
        std::shared_ptr<Unit> closestUnit = nullptr;

        for (const auto& unit : units) {
            if (unit->getTeamSign() != teamSign && unit->isAlive()) {
                float dist = calculateDistance(position, unit->getPosition());
                if (dist < minDistance) {
                    minDistance = dist;
                    closestUnit = unit;
                }
            }
        }

        if (closestUnit) {
            targetPosition = closestUnit->getPosition();
        }
    }

    void evade(float deltaTime) {
        // Move away from the area of high influence
        sf::Vector2f direction = position - targetPosition;
        float distance = sqrt(direction.x * direction.x + direction.y * direction.y);
        if (distance != 0) {
            direction /= distance;
            position += direction * speed * deltaTime;
        }
    }

    float calculateDistance(const sf::Vector2f& pos1, const sf::Vector2f& pos2) {
        sf::Vector2f delta = pos2 - pos1;
        return sqrt(delta.x * delta.x + delta.y * delta.y);
    }
};

// GUI Class for Adding Units and Controls
class GUI {
private:
    sf::Font font;
    sf::Text addFriendlyText;
    sf::Text addEnemyText;
    sf::Text toggleAutoText;
    sf::RectangleShape friendlyButton;
    sf::RectangleShape enemyButton;
    sf::RectangleShape toggleAutoButton;

public:
    GUI() {
        font.loadFromFile("arial.ttf");

        // Friendly Button
        friendlyButton.setSize(sf::Vector2f(100, 30));
        friendlyButton.setFillColor(sf::Color::Blue);
        friendlyButton.setPosition(10, 10);

        addFriendlyText.setFont(font);
        addFriendlyText.setCharacterSize(14);
        addFriendlyText.setFillColor(sf::Color::White);
        addFriendlyText.setString("Add Friendly");
        addFriendlyText.setPosition(15, 15);

        // Enemy Button
        enemyButton.setSize(sf::Vector2f(100, 30));
        enemyButton.setFillColor(sf::Color::Red);
        enemyButton.setPosition(120, 10);

        addEnemyText.setFont(font);
        addEnemyText.setCharacterSize(14);
        addEnemyText.setFillColor(sf::Color::White);
        addEnemyText.setString("Add Enemy");
        addEnemyText.setPosition(125, 15);

        // Toggle Autonomous Button
        toggleAutoButton.setSize(sf::Vector2f(140, 30));
        toggleAutoButton.setFillColor(sf::Color::Green);
        toggleAutoButton.setPosition(230, 10);

        toggleAutoText.setFont(font);
        toggleAutoText.setCharacterSize(14);
        toggleAutoText.setFillColor(sf::Color::White);
        toggleAutoText.setString("Toggle Autonomous");
        toggleAutoText.setPosition(235, 15);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(friendlyButton);
        window.draw(addFriendlyText);
        window.draw(enemyButton);
        window.draw(addEnemyText);
        window.draw(toggleAutoButton);
        window.draw(toggleAutoText);
    }

    bool isFriendlyButtonPressed(const sf::Vector2f& mousePos) {
        return friendlyButton.getGlobalBounds().contains(mousePos);
    }

    bool isEnemyButtonPressed(const sf::Vector2f& mousePos) {
        return enemyButton.getGlobalBounds().contains(mousePos);
    }

    bool isToggleAutoButtonPressed(const sf::Vector2f& mousePos) {
        return toggleAutoButton.getGlobalBounds().contains(mousePos);
    }
};

// Main Function
int main() {
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "RTS Influence Map Demo");

    // Load font for influence map (ensure "arial.ttf" is in the working directory)
    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        std::cout << "Error loading font\n";
        return -1;
    }

    // Load textures if needed (optional)
    // sf::Texture friendlyTexture;
    // friendlyTexture.loadFromFile("friendly.png");
    // sf::Texture enemyTexture;
    // enemyTexture.loadFromFile("enemy.png");

    // Create units
    std::vector<std::shared_ptr<Unit>> units;

    // Create influence map
    InfluenceMap influenceMap(SCREEN_WIDTH, SCREEN_HEIGHT, 40);

    // Create GUI
    GUI gui;

    sf::Clock clock;

    // Variables for unit selection and movement
    bool isDragging = false;
    std::shared_ptr<Unit> selectedUnit = nullptr;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        // Event handling
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            // Toggle debug mode
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::D)
                debug = !debug;

            // Mouse events for unit selection and movement
            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

                // Check GUI buttons
                if (gui.isFriendlyButtonPressed(mousePos)) {
                    // Spawn friendly unit near left middle
                    float randomOffsetY = distribution(generator);
                    units.push_back(std::make_shared<FriendlyUnit>(
                            sf::Vector2f(100, SCREEN_HEIGHT / 2 + randomOffsetY)));
                } else if (gui.isEnemyButtonPressed(mousePos)) {
                    // Spawn enemy unit near right middle
                    float randomOffsetY = distribution(generator);
                    units.push_back(std::make_shared<EnemyUnit>(
                            sf::Vector2f(SCREEN_WIDTH - 100, SCREEN_HEIGHT / 2 + randomOffsetY)));
                } else if (gui.isToggleAutoButtonPressed(mousePos)) {
                    autonomousMode = !autonomousMode;
                    for (auto& unit : units) {
                        unit->setAutonomous(autonomousMode);
                    }
                } else if (event.mouseButton.button == sf::Mouse::Left) {
                    // Check if a unit is under the mouse
                    for (auto& unit : units) {
                        if (unit->containsPoint(mousePos)) {
                            selectedUnit = unit;
                            unit->setSelected(true);
                            isDragging = true;
                            break;
                        }
                    }
                }
            }
            if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left && isDragging) {
                    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    if (selectedUnit) {
                        selectedUnit->setTargetPosition(mousePos);
                        selectedUnit->setSelected(false);
                        selectedUnit = nullptr;
                    }
                    isDragging = false;
                }
            }
        }

        // Update influence map
        influenceMap.update(units);

        // Update units
        for (auto& unit : units) {
            unit->update(deltaTime, units, influenceMap);
        }

        // Remove dead units
        units.erase(std::remove_if(units.begin(), units.end(),
                                   [](const std::shared_ptr<Unit>& unit) { return !unit->isAlive(); }),
                    units.end());

        // Rendering
        window.clear();

        // Draw influence map if debugging
        influenceMap.draw(window);

        // Draw units
        for (auto& unit : units) {
            unit->draw(window);
        }

        // Draw GUI
        gui.draw(window);

        window.display();
    }

    return 0;
}
