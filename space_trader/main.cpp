#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cmath>
#include <random>

// Constants
const float PI = 3.14159f;
const float MAX_THRUST = 100.0f;
const float FRICTION = 0.98f;

// Observer Pattern
class Observer {
public:
    virtual void onNotify(const std::string& message) = 0;
};

class NotificationCenter {
    std::vector<Observer*> observers;
public:
    void addObserver(Observer* observer) {
        observers.push_back(observer);
    }

    void notify(const std::string& message) {
        for (auto observer : observers) {
            observer->onNotify(message);
        }
    }
};

// Cargo and Trade System
struct CargoItem {
    std::string name;
    int quantity;
};

// Ship Component System
struct Component {
    std::string type;
    sf::Vector2f position;
    float rotation;
    sf::Vector2f size;
};

class Ship : public Observer {
public:
    sf::Vector2f position;
    float rotation;
    float velocityX = 0.0f, velocityY = 0.0f;
    float thrust = 0.0f;
    bool hasTarget = false;
    sf::Vector2f targetPosition;
    std::vector<CargoItem> cargo;
    NotificationCenter* notificationCenter;
    std::vector<Component> components;

    Ship(float x, float y, NotificationCenter* nc) : position(x, y), rotation(0), notificationCenter(nc) {
        // Add some cargo to start
        cargo.push_back({"Fuel", 100});
        cargo.push_back({"Food", 50});
        cargo.push_back({"Metal", 30});
        notificationCenter->addObserver(this);

        // Add ship components (e.g., hull and thrusters)
        components.push_back({"HULL", {0, 0}, 0, {40, 20}});
        components.push_back({"THRUSTER", {-20, -10}, 180, {10, 5}});
        components.push_back({"THRUSTER", {20, -10}, 180, {10, 5}});
    }

    void addCargo(const std::string& name, int quantity) {
        for (auto& item : cargo) {
            if (item.name == name) {
                item.quantity += quantity;
                return;
            }
        }
        cargo.push_back({name, quantity});
    }

    void setTarget(float x, float y) {
        targetPosition = sf::Vector2f(x, y);
        hasTarget = true;
    }

    void addComponent(const std::string& type, sf::Vector2f position, float rotation, sf::Vector2f size) {
        components.push_back({type, position, rotation, size});
    }

    void removeComponent(const std::string& type) {
        components.erase(std::remove_if(components.begin(), components.end(), [&](const Component& comp) {
            return comp.type == type;
        }), components.end());
    }

    void onNotify(const std::string& message) override {
        std::cout << "Ship received message: " << message << std::endl;
    }

    void update() {
        // Update ship physics and handle movement towards target
        if (hasTarget) {
            float angleToTarget = atan2(targetPosition.y - position.y, targetPosition.x - position.x) * 180 / PI;
            float angleDifference = angleToTarget - rotation;

            if (angleDifference > 180) angleDifference -= 360;
            if (angleDifference < -180) angleDifference += 360;

            rotation += angleDifference * 0.1f; // Smooth rotation towards target

            thrust = MAX_THRUST * 0.1f;
            velocityX += cos(rotation * PI / 180) * thrust;
            velocityY += sin(rotation * PI / 180) * thrust;

            if (std::hypot(targetPosition.x - position.x, targetPosition.y - position.y) < 10) {
                hasTarget = false;
                thrust = 0;
                velocityX = 0;
                velocityY = 0;
            }
        }

        // Apply velocity with inertia and friction
        position.x += velocityX;
        position.y += velocityY;
        velocityX *= FRICTION;
        velocityY *= FRICTION;
    }

    void draw(sf::RenderWindow& window) {
        for (const auto& component : components) {
            sf::RectangleShape shape(component.size);
            shape.setOrigin(component.size.x / 2, component.size.y / 2);
            shape.setPosition(position + component.position);
            shape.setRotation(rotation + component.rotation);

            if (component.type == "HULL")
                shape.setFillColor(sf::Color::Green);
            else if (component.type == "THRUSTER")
                shape.setFillColor(sf::Color::Red);

            window.draw(shape);
        }
    }

    // Method to handle player controls
    void handleInput() {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
            thrust = MAX_THRUST * 0.1f;
            velocityX += cos(rotation * PI / 180) * thrust;
            velocityY += sin(rotation * PI / 180) * thrust;
            std::cout << "W is pressed" << std::endl;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            rotation -= 2.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            rotation += 2.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            thrust = -MAX_THRUST * 0.05f;
            velocityX += cos(rotation * PI / 180) * thrust;
            velocityY += sin(rotation * PI / 180) * thrust;
        }
    }
};

// AI Ship for procedural generation
class AIShip : public Ship {
public:
    AIShip(float x, float y, NotificationCenter* nc) : Ship(x, y, nc) {
        // Add some procedural components
        std::default_random_engine generator;
        std::uniform_real_distribution<float> distX(-50.0, 50.0);
        std::uniform_real_distribution<float> distY(-50.0, 50.0);
        std::uniform_int_distribution<int> componentType(0, 2);

        for (int i = 0; i < 3; ++i) {
            std::string type = componentType(generator) == 0 ? "HULL" : "THRUSTER";
            addComponent(type, {distX(generator), distY(generator)}, 0, {20, 10});
        }
    }

    void updateAI() {
        // Basic AI movement logic (randomly wander for now)
        if (!hasTarget) {
            setTarget(position.x + (std::rand() % 100 - 50), position.y + (std::rand() % 100 - 50));
        }
        update();
    }
};

// UI for displaying cargo and trade menu
class TradeMenu : public Observer {
    Ship& ship;
    sf::Font font;
    sf::Text text;
public:
    TradeMenu(Ship& ship) : ship(ship) {
        if (!font.loadFromFile("arial.ttf")) { // Ensure you have this font file
            std::cerr << "Failed to load font!" << std::endl;
        }
        text.setFont(font);
        text.setCharacterSize(14);
        text.setFillColor(sf::Color::White);
    }

    void draw(sf::RenderWindow& window) {
        std::string cargoInfo = "Cargo:\n";
        for (const auto& item : ship.cargo) {
            cargoInfo += item.name + ": " + std::to_string(item.quantity) + "\n";
        }
        text.setString(cargoInfo);
        text.setPosition(10, 10);
        window.draw(text);
    }

    void onNotify(const std::string& message) override {
        std::cout << "Trade Menu received message: " << message << std::endl;
    }
};

// Main function
int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Ship Trade & Logistics Demo");
    NotificationCenter notificationCenter;

    // Ship and Trade Menu setup
    Ship playerShip(400, 300, &notificationCenter);
    TradeMenu tradeMenu(playerShip);
    AIShip aiShip(200, 150, &notificationCenter);

    notificationCenter.addObserver(&tradeMenu);

    // Simulated trade message
    notificationCenter.notify("New trade offer: Buy 10 units of Fuel for 50 credits!");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            // Set target on mouse click for player ship
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                float mouseX = event.mouseButton.x;
                float mouseY = event.mouseButton.y;
                playerShip.setTarget(mouseX, mouseY);
            }
        }

        playerShip.handleInput();
        playerShip.update();
        aiShip.updateAI();

        window.clear();
        playerShip.draw(window);
        aiShip.draw(window);
        tradeMenu.draw(window);
        window.display();
    }

    return 0;
}
