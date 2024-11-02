// main.cpp

#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <memory>
#include <algorithm>
#include <random>
#include <iostream>
#include <queue>

// Global debug flag
bool debug = true;

// Screen dimensions
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// Forward declarations
class InfluenceMap;

// Terrain Obstacle Class
class Obstacle {
public:
    sf::RectangleShape shape;

    Obstacle(const sf::Vector2f& position, const sf::Vector2f& size) {
        shape.setPosition(position);
        shape.setSize(size);
        shape.setFillColor(sf::Color(100, 100, 100));
    }

    bool intersects(const sf::FloatRect& rect) const {
        return shape.getGlobalBounds().intersects(rect);
    }
};

// Base Unit Class
class Unit {
protected:
    sf::ConvexShape shape;
    sf::RectangleShape healthBar;
    float health;
    bool alive;
    sf::Vector2f position;
    int teamSign; // Positive for friendly, negative for enemy
    bool useImage;
    sf::Texture texture;
    sf::Sprite sprite;
    sf::Vector2f targetPosition;
    float speed;
    bool selected;
    float attackRange;
    float attackDamage;
    float projectileSpeed;
    bool isAttacking;
    std::shared_ptr<Unit> attackTarget;
    float attackCooldown;      // Time until next attack is allowed
    float attackCooldownTime;  // Total cooldown duration

public:
    Unit(const sf::Vector2f& pos, int team, bool useImg = false, const std::string& imgPath = "")
            : position(pos), health(100.0f), alive(true), teamSign(team), useImage(useImg), selected(false),
              speed(100.0f), isAttacking(false), attackTarget(nullptr), attackCooldown(0.0f), attackCooldownTime(1.0f) {
        if (useImage && !imgPath.empty()) {
            texture.loadFromFile(imgPath);
            sprite.setTexture(texture);
            sprite.setPosition(position);
        } else {
            shape.setPosition(position);
        }
        healthBar.setSize(sf::Vector2f(20, 4));
        healthBar.setFillColor(sf::Color::Green);
        healthBar.setPosition(position + sf::Vector2f(-10, -20));
        targetPosition = position;
        attackRange = 150.0f;      // Reduced attack range
        attackDamage = 10.0f;      // Reduced attack damage
        projectileSpeed = 250.0f;  // Reduced projectile speed
        attackCooldownTime = 1.5f; // Increased cooldown duration
    }

    virtual void update(float deltaTime, const std::vector<std::shared_ptr<Unit>>& units,
                        const std::vector<Obstacle>& obstacles, const class InfluenceMap& influenceMap) = 0;

    virtual void draw(sf::RenderWindow& window) {
        if (useImage) {
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
        if (useImage) {
            return sprite.getGlobalBounds().contains(point);
        } else {
            return shape.getGlobalBounds().contains(point);
        }
    }

    void setSelected(bool sel) {
        selected = sel;
    }

protected:
    void moveTowardsTarget(float deltaTime, const std::vector<Obstacle>& obstacles) {
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

            // Check for collisions with obstacles
            sf::FloatRect futureRect(newPosition.x - 10, newPosition.y - 10, 20, 20);
            bool collision = false;
            for (const auto& obstacle : obstacles) {
                if (obstacle.intersects(futureRect)) {
                    collision = true;
                    break;
                }
            }
            if (!collision) {
                position = newPosition;
            }
        }
    }

    void updateGraphics() {
        // Wraparound logic for position
        if (position.x < 0) position.x += SCREEN_WIDTH;
        if (position.x >= SCREEN_WIDTH) position.x -= SCREEN_WIDTH;
        if (position.y < 0) position.y += SCREEN_HEIGHT;
        if (position.y >= SCREEN_HEIGHT) position.y -= SCREEN_HEIGHT;

        if (useImage) {
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
    float maxDistance;     // Maximum distance projectile can travel
    sf::Vector2f startPos; // Starting position of the projectile

    Projectile(const sf::Vector2f& pos, const sf::Vector2f& target, int team, float dmg, float speed)
            : position(pos), teamSign(team), damage(dmg), alive(true), maxDistance(200.0f), startPos(pos) {
        sf::Vector2f direction = target - pos;

        // Adjust direction for wraparound
        if (direction.x > SCREEN_WIDTH / 2) direction.x -= SCREEN_WIDTH;
        if (direction.x < -SCREEN_WIDTH / 2) direction.x += SCREEN_WIDTH;
        if (direction.y > SCREEN_HEIGHT / 2) direction.y -= SCREEN_HEIGHT;
        if (direction.y < -SCREEN_HEIGHT / 2) direction.y += SCREEN_HEIGHT;

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
        float traveledDistance = calculateWrappedDistance(startPos, position);
        if (traveledDistance > maxDistance) {
            alive = false;
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
    }

private:
    float calculateWrappedDistance(const sf::Vector2f& pos1, const sf::Vector2f& pos2) {
        sf::Vector2f delta = pos2 - pos1;

        // Adjust for wraparound
        if (delta.x > SCREEN_WIDTH / 2) delta.x -= SCREEN_WIDTH;
        if (delta.x < -SCREEN_WIDTH / 2) delta.x += SCREEN_WIDTH;
        if (delta.y > SCREEN_HEIGHT / 2) delta.y -= SCREEN_HEIGHT;
        if (delta.y < -SCREEN_HEIGHT / 2) delta.y += SCREEN_HEIGHT;

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
                if (value != 0 && debug) {
                    text.setString(std::to_string(static_cast<int>(value)));
                    text.setPosition(x * cellSize + 2, y * cellSize + 2);
                    window.draw(text);
                }
            }
        }
    }

    float getInfluenceAtPosition(const sf::Vector2f& position) const {
        int x = static_cast<int>(position.x / cellSize) % (width / cellSize);
        int y = static_cast<int>(position.y / cellSize) % (height / cellSize);

        if (x < 0) x += width / cellSize;
        if (y < 0) y += height / cellSize;

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
                int cellX = (unitX + x + mapData[0].size()) % mapData[0].size();
                int cellY = (unitY + y + mapData.size()) % mapData.size();

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
    FriendlyUnit(const sf::Vector2f& pos, bool useImg = false, const std::string& imgPath = "")
            : Unit(pos, 1, useImg, imgPath) {
        if (!useImage) {
            shape.setPointCount(3);
            shape.setPoint(0, sf::Vector2f(0, -10));
            shape.setPoint(1, sf::Vector2f(10, 10));
            shape.setPoint(2, sf::Vector2f(-10, 10));
            shape.setFillColor(sf::Color::Blue);
            shape.setPosition(position);
        }
    }

    void update(float deltaTime, const std::vector<std::shared_ptr<Unit>>& units,
                const std::vector<Obstacle>& obstacles, const InfluenceMap& influenceMap) override {
        moveTowardsTarget(deltaTime, obstacles);
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
                float dist = calculateWrappedDistance(position, unit->getPosition());
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

    float calculateWrappedDistance(const sf::Vector2f& pos1, const sf::Vector2f& pos2) {
        sf::Vector2f delta = pos2 - pos1;

        // Adjust for wraparound
        if (delta.x > SCREEN_WIDTH / 2) delta.x -= SCREEN_WIDTH;
        if (delta.x < -SCREEN_WIDTH / 2) delta.x += SCREEN_WIDTH;
        if (delta.y > SCREEN_HEIGHT / 2) delta.y -= SCREEN_HEIGHT;
        if (delta.y < -SCREEN_HEIGHT / 2) delta.y += SCREEN_HEIGHT;

        return sqrt(delta.x * delta.x + delta.y * delta.y);
    }
};

// Enemy Unit Class
class EnemyUnit : public Unit {
private:
    enum State { Idle, Attack, Retreat } state;
    std::vector<Projectile> projectiles;

public:
    EnemyUnit(const sf::Vector2f& pos, bool useImg = false, const std::string& imgPath = "")
            : Unit(pos, -1, useImg, imgPath), state(Idle) {
        speed = 80.0f;
        attackDamage = 8.0f;        // Reduced enemy attack damage
        attackCooldownTime = 2.0f;  // Increased enemy cooldown duration
        if (!useImage) {
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
                const std::vector<Obstacle>& obstacles, const InfluenceMap& influenceMap) override {
        // Use influence map to decide movement
        makeDecision(units, influenceMap);

        // FSM for enemy behavior
        switch (state) {
            case Idle:
                break;
            case Attack:
                attack(deltaTime, units);
                moveTowardsTarget(deltaTime, obstacles);
                break;
            case Retreat:
                retreat(deltaTime);
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

        // Check health to decide whether to retreat
        if (health < 30.0f) {
            state = Retreat;
            return;
        }

        // Find nearest enemy within attack range
        float minDistance = attackRange;
        std::shared_ptr<Unit> nearestEnemy = nullptr;

        for (const auto& unit : units) {
            if (unit->getTeamSign() != teamSign && unit->isAlive()) {
                float dist = calculateWrappedDistance(position, unit->getPosition());
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

        // If the influence is high (more friendly units nearby), move towards lower influence areas
        if (currentInfluence > -0.5f) {
            // Find a direction with the least positive influence
            sf::Vector2f bestDirection;
            float minInfluence = std::numeric_limits<float>::max();

            // Check eight directions (N, NE, E, SE, S, SW, W, NW)
            std::vector<sf::Vector2f> directions = {
                    {0, -1}, {1, -1}, {1, 0}, {1, 1},
                    {0, 1},  {-1, 1}, {-1, 0}, {-1, -1}
            };

            for (const auto& dir : directions) {
                sf::Vector2f checkPos = position + dir * 20.0f;
                // Wraparound logic
                if (checkPos.x < 0) checkPos.x += SCREEN_WIDTH;
                if (checkPos.x >= SCREEN_WIDTH) checkPos.x -= SCREEN_WIDTH;
                if (checkPos.y < 0) checkPos.y += SCREEN_HEIGHT;
                if (checkPos.y >= SCREEN_HEIGHT) checkPos.y -= SCREEN_HEIGHT;

                float influence = influenceMap.getInfluenceAtPosition(checkPos);
                if (influence < minInfluence) {
                    minInfluence = influence;
                    bestDirection = dir;
                }
            }

            // Set target position in the direction of least influence
            targetPosition = position + bestDirection * 50.0f;
        } else {
            // Influence is low enough, proceed to attack
            state = Attack;
            findTarget(units);
        }
    }

    void findTarget(const std::vector<std::shared_ptr<Unit>>& units) {
        // Simple AI to find the closest friendly unit
        float minDistance = std::numeric_limits<float>::max();
        std::shared_ptr<Unit> closestUnit = nullptr;

        for (const auto& unit : units) {
            if (unit->getTeamSign() != teamSign && unit->isAlive()) {
                float dist = calculateWrappedDistance(position, unit->getPosition());
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

    void retreat(float deltaTime) {
        // Move away from the target position
        sf::Vector2f direction = position - targetPosition;

        // Adjust direction for wraparound
        if (direction.x > SCREEN_WIDTH / 2) direction.x -= SCREEN_WIDTH;
        if (direction.x < -SCREEN_WIDTH / 2) direction.x += SCREEN_WIDTH;
        if (direction.y > SCREEN_HEIGHT / 2) direction.y -= SCREEN_HEIGHT;
        if (direction.y < -SCREEN_HEIGHT / 2) direction.y += SCREEN_HEIGHT;

        float distance = sqrt(direction.x * direction.x + direction.y * direction.y);
        if (distance > 1.0f) {
            direction /= distance;
            position += direction * speed * deltaTime;
        }

        // If health recovers, go back to idle
        if (health > 50.0f) {
            state = Idle;
        }
    }

    float calculateWrappedDistance(const sf::Vector2f& pos1, const sf::Vector2f& pos2) {
        sf::Vector2f delta = pos2 - pos1;

        // Adjust for wraparound
        if (delta.x > SCREEN_WIDTH / 2) delta.x -= SCREEN_WIDTH;
        if (delta.x < -SCREEN_WIDTH / 2) delta.x += SCREEN_WIDTH;
        if (delta.y > SCREEN_HEIGHT / 2) delta.y -= SCREEN_HEIGHT;
        if (delta.y < -SCREEN_HEIGHT / 2) delta.y += SCREEN_HEIGHT;

        return sqrt(delta.x * delta.x + delta.y * delta.y);
    }
};

// GUI Class for Adding Units
class GUI {
private:
    sf::Font font;
    sf::Text addFriendlyText;
    sf::Text addEnemyText;
    sf::RectangleShape friendlyButton;
    sf::RectangleShape enemyButton;

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
    }

    void draw(sf::RenderWindow& window) {
        window.draw(friendlyButton);
        window.draw(addFriendlyText);
        window.draw(enemyButton);
        window.draw(addEnemyText);
    }

    bool isFriendlyButtonPressed(const sf::Vector2f& mousePos) {
        return friendlyButton.getGlobalBounds().contains(mousePos);
    }

    bool isEnemyButtonPressed(const sf::Vector2f& mousePos) {
        return enemyButton.getGlobalBounds().contains(mousePos);
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

    // Create units
    std::vector<std::shared_ptr<Unit>> units;
    units.push_back(std::make_shared<FriendlyUnit>(sf::Vector2f(100, 100)));
    units.push_back(std::make_shared<FriendlyUnit>(sf::Vector2f(150, 150)));
    units.push_back(std::make_shared<EnemyUnit>(sf::Vector2f(700, 500)));
    units.push_back(std::make_shared<EnemyUnit>(sf::Vector2f(650, 450)));
    // Add more units as needed...

    // Create obstacles
    std::vector<Obstacle> obstacles;
    obstacles.emplace_back(sf::Vector2f(300, 200), sf::Vector2f(200, 50));
    obstacles.emplace_back(sf::Vector2f(500, 400), sf::Vector2f(50, 200));

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
                    units.push_back(std::make_shared<FriendlyUnit>(mousePos));
                } else if (gui.isEnemyButtonPressed(mousePos)) {
                    units.push_back(std::make_shared<EnemyUnit>(mousePos));
                } else if (event.mouseButton.button == sf::Mouse::Left) {
                    // Check if a unit is under the mouse
                    for (auto& unit : units) {
                        if (unit->getTeamSign() > 0 && unit->containsPoint(mousePos)) {
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
            unit->update(deltaTime, units, obstacles, influenceMap);
        }

        // Remove dead units
        units.erase(std::remove_if(units.begin(), units.end(),
                                   [](const std::shared_ptr<Unit>& unit) { return !unit->isAlive(); }),
                    units.end());

        // Rendering
        window.clear();

        // Draw influence map if debugging
        if (debug) {
            influenceMap.draw(window);
        }

        // Draw units
        for (auto& unit : units) {
            unit->draw(window);
        }

        // Draw obstacles
        for (const auto& obstacle : obstacles) {
            window.draw(obstacle.shape);
        }

        // Draw GUI
        gui.draw(window);

        window.display();
    }

    return 0;
}
