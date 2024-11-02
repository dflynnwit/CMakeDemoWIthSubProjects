// Updated Bytecode Interpreter example to maintain shapes across frames, ensuring they are persistently drawn on the SFML window.

#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>


enum class OpCode {
    DRAW_RECTANGLE,
    DRAW_CIRCLE,
    SET_COLOR,
    END
};

struct Instruction {
    OpCode opCode;
    std::vector<float> operands;
};

class BytecodeInterpreter {
public:
    BytecodeInterpreter(const std::vector<Instruction>& program)
            : program(program), currentIndex(0), currentColor(sf::Color::White) {}

    void run() {
        // Execute all instructions, storing the shapes
        while (currentIndex < program.size()) {
            executeInstruction();
        }
    }

    void render(sf::RenderWindow& window) {
        // Render all stored shapes each frame
        for (const auto& shape : shapes) {
            window.draw(*shape);
        }
    }

private:
    void executeInstruction() {
        const auto& instruction = program[currentIndex];
        switch (instruction.opCode) {
            case OpCode::DRAW_RECTANGLE: {
                auto rectangle = std::make_unique<sf::RectangleShape>();
                rectangle->setSize(sf::Vector2f(instruction.operands[0], instruction.operands[1]));
                rectangle->setPosition(instruction.operands[2], instruction.operands[3]);
                rectangle->setFillColor(currentColor);
                shapes.push_back(std::move(rectangle));
                break;
            }
            case OpCode::DRAW_CIRCLE: {
                auto circle = std::make_unique<sf::CircleShape>();
                circle->setRadius(instruction.operands[0]);
                circle->setPosition(instruction.operands[1], instruction.operands[2]);
                circle->setFillColor(currentColor);
                shapes.push_back(std::move(circle));
                break;
            }
            case OpCode::SET_COLOR: {
                currentColor = sf::Color(static_cast<sf::Uint8>(instruction.operands[0]),
                                         static_cast<sf::Uint8>(instruction.operands[1]),
                                         static_cast<sf::Uint8>(instruction.operands[2]));
                break;
            }
            case OpCode::END:
                currentIndex = program.size(); // End execution
                return;
            default:
                std::cerr << "Unknown OpCode encountered!" << std::endl;
                return;
        }
        currentIndex++;
    }

    std::vector<Instruction> program;
    size_t currentIndex;
    sf::Color currentColor;
    std::vector<std::unique_ptr<sf::Drawable>> shapes;
};

std::vector<Instruction> loadProgramFromFile(const std::string& filename) {
    std::vector<Instruction> program;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open bytecode file: " << filename << std::endl;
        return program;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string opCodeStr;
        iss >> opCodeStr;

        OpCode opCode;
        if (opCodeStr == "DRAW_RECTANGLE") {
            opCode = OpCode::DRAW_RECTANGLE;
        } else if (opCodeStr == "DRAW_CIRCLE") {
            opCode = OpCode::DRAW_CIRCLE;
        } else if (opCodeStr == "SET_COLOR") {
            opCode = OpCode::SET_COLOR;
        } else if (opCodeStr == "END") {
            opCode = OpCode::END;
        } else {
            std::cerr << "Unknown OpCode in file: " << opCodeStr << std::endl;
            continue;
        }

        std::vector<float> operands;
        float operand;
        while (iss >> operand) {
            operands.push_back(operand);
        }

        program.push_back({opCode, operands});
    }

    return program;
}

int main() {
    // Create an SFML window
    sf::RenderWindow window(sf::VideoMode(800, 600), "Bytecode Interpreter Example");

    // Load the bytecode program from an external file
    std::vector<Instruction> program = loadProgramFromFile("bytecode.txt");

    BytecodeInterpreter interpreter(program);
    interpreter.run(); // Execute all instructions once to store shapes

    // Main loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        interpreter.render(window); // Render all stored shapes each frame
        window.display();
    }

    return 0;
}