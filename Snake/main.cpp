// Snake Game using SFML 3.0.1
// Include necessary SFML headers and standard C++ libraries
#include <SFML/Graphics.hpp>  // For graphics rendering (shapes, window, sprites)
#include <SFML/System.hpp>    // For system utilities (time, vectors)
#include <SFML/Window.hpp>    // For window events and input handling
#include <vector>             // For dynamic arrays to store snake segments
#include <random>             // For random number generation (food placement)
#include <deque>              // For double-ended queue (efficient snake movement)

// Game constants - defining the game parameters
const int GRID_SIZE = 20;        // Size of each grid cell in pixels
const int GRID_WIDTH = 30;       // Number of cells horizontally
const int GRID_HEIGHT = 20;      // Number of cells vertically
const float MOVE_DELAY = 0.1f;   // Delay between snake movements in seconds

// Enum to represent the direction of snake movement
enum class Direction {
    Up,    // Snake moves upward (negative Y)
    Down,  // Snake moves downward (positive Y)
    Left,  // Snake moves left (negative X)
    Right  // Snake moves right (positive X)
};

// Class representing the Snake game
class SnakeGame {
private:
    // SFML window for rendering the game
    sf::RenderWindow window;

    // Snake representation
    std::deque<sf::Vector2i> snake;  // Deque storing snake segments (head at front)
    Direction currentDirection;       // Current direction the snake is moving
    Direction nextDirection;          // Next direction (buffered input)

    // Food representation
    sf::Vector2i foodPosition;        // Position of the food on the grid

    // Random number generation
    std::random_device rd;            // Random device for seeding
    std::mt19937 gen;                 // Mersenne Twister random generator
    std::uniform_int_distribution<> distX;  // Distribution for X coordinates
    std::uniform_int_distribution<> distY;  // Distribution for Y coordinates

    // Game state variables
    bool gameOver;                    // Flag indicating if game has ended
    bool paused;                      // Flag indicating if game is paused
    int score;                        // Current score (snake length - initial length)

    // Timing variables for controlling snake movement speed
    sf::Clock moveClock;              // Clock to track time between movements

    // Visual elements
    sf::Font font;                    // Font for displaying text
    bool fontLoaded;                  // Flag to check if font was loaded successfully

    // Text objects - will be initialized after font is loaded
    std::unique_ptr<sf::Text> scoreText;      // Pointer to text object for score display
    std::unique_ptr<sf::Text> gameOverText;   // Pointer to text object for game over message
    std::unique_ptr<sf::Text> pauseText;      // Pointer to text object for pause message

public:
    // Constructor - initializes the game window and all game objects
    SnakeGame() :
        window(sf::VideoMode(sf::Vector2u(GRID_WIDTH * GRID_SIZE, GRID_HEIGHT * GRID_SIZE)),
               "Snake Game - SFML 3.0.1"),  // Create window with calculated dimensions
        gen(rd()),                           // Initialize random generator with random seed
        distX(0, GRID_WIDTH - 1),           // X distribution from 0 to grid width-1
        distY(0, GRID_HEIGHT - 1),          // Y distribution from 0 to grid height-1
        gameOver(false),                     // Game starts in playing state
        paused(false),                       // Game starts unpaused
        score(0),                            // Score starts at 0
        fontLoaded(false)                    // Font not loaded initially
    {
        // Set frame rate limit for smooth gameplay
        window.setFramerateLimit(60);

        // Initialize the snake with 3 segments in the middle of the grid
        initializeSnake();

        // Set initial direction to right
        currentDirection = Direction::Right;
        nextDirection = Direction::Right;

        // Place the first food item
        generateFood();

        // Try to load font for text display
        // You can change this path to point to any TTF font file on your system
        // Common font locations:
        // Windows: "C:/Windows/Fonts/arial.ttf"
        // Linux: "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"
        // Or put a font file in your project directory
        if (font.openFromFile("arial.ttf") ||
            font.openFromFile("C:/Windows/Fonts/arial.ttf") ||
            font.openFromFile("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf") ||
            font.openFromFile("/System/Library/Fonts/Helvetica.ttc")) {
            fontLoaded = true;
            initializeTexts();  // Initialize text objects if font loaded successfully
        }
    }

    // Initialize text objects after font is loaded
    void initializeTexts() {
        if (!fontLoaded) return;

        // Create and setup score text
        scoreText = std::make_unique<sf::Text>(font, "Score: 0", 20);
        scoreText->setFillColor(sf::Color::White);
        scoreText->setPosition(sf::Vector2f(10.f, 10.f));

        // Create and setup game over text
        gameOverText = std::make_unique<sf::Text>(font, "GAME OVER! Press R to restart", 30);
        gameOverText->setFillColor(sf::Color::Red);

        // Center the game over text
        sf::FloatRect textBounds = gameOverText->getLocalBounds();
        float xPos = (GRID_WIDTH * GRID_SIZE - textBounds.size.x) / 2.f;
        float yPos = (GRID_HEIGHT * GRID_SIZE - textBounds.size.y) / 2.f;
        gameOverText->setPosition(sf::Vector2f(xPos, yPos));

        // Create and setup pause text
        pauseText = std::make_unique<sf::Text>(font, "PAUSED - Press Space to continue", 25);
        pauseText->setFillColor(sf::Color::Yellow);

        // Center the pause text
        textBounds = pauseText->getLocalBounds();
        xPos = (GRID_WIDTH * GRID_SIZE - textBounds.size.x) / 2.f;
        yPos = (GRID_HEIGHT * GRID_SIZE - textBounds.size.y) / 2.f;
        pauseText->setPosition(sf::Vector2f(xPos, yPos));
    }

    // Initialize snake at the center of the grid with 3 segments
    void initializeSnake() {
        snake.clear();  // Clear any existing snake segments

        // Add 3 initial segments (snake starts horizontally)
        int startX = GRID_WIDTH / 2;   // Start at horizontal center
        int startY = GRID_HEIGHT / 2;  // Start at vertical center

        // Add segments from head to tail
        for (int i = 0; i < 3; ++i) {
            snake.push_back(sf::Vector2i(startX - i, startY));
        }
    }

    // Generate food at a random position not occupied by the snake
    void generateFood() {
        do {
            // Generate random position
            foodPosition.x = distX(gen);
            foodPosition.y = distY(gen);
        } while (isSnakePosition(foodPosition));  // Repeat if position overlaps with snake
    }

    // Check if a given position is occupied by the snake
    bool isSnakePosition(const sf::Vector2i& position) {
        // Iterate through all snake segments
        for (const auto& segment : snake) {
            if (segment == position) {
                return true;  // Position is occupied by snake
            }
        }
        return false;  // Position is free
    }

    // Main game loop - handles events, updates, and rendering
    void run() {
        // Continue running while window is open
        while (window.isOpen()) {
            handleEvents();  // Process user input
            update();        // Update game logic
            render();        // Draw everything to screen
        }
    }

    // Handle all window events and user input
    void handleEvents() {
        // Process all pending events
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            // Check if window close was requested
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // Handle keyboard input
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                // Get the pressed key
                sf::Keyboard::Key key = keyPressed->code;

                // Handle game over state
                if (gameOver) {
                    if (key == sf::Keyboard::Key::R) {
                        resetGame();  // Restart the game
                    }
                    continue;  // Skip other input processing
                }

                // Handle pause toggle
                if (key == sf::Keyboard::Key::Space) {
                    paused = !paused;
                    continue;
                }

                // Handle direction changes (only when not paused)
                if (!paused) {
                    // Update next direction based on key press
                    // Prevent snake from moving back into itself
                    if (key == sf::Keyboard::Key::Up && currentDirection != Direction::Down) {
                        nextDirection = Direction::Up;
                    }
                    else if (key == sf::Keyboard::Key::Down && currentDirection != Direction::Up) {
                        nextDirection = Direction::Down;
                    }
                    else if (key == sf::Keyboard::Key::Left && currentDirection != Direction::Right) {
                        nextDirection = Direction::Left;
                    }
                    else if (key == sf::Keyboard::Key::Right && currentDirection != Direction::Left) {
                        nextDirection = Direction::Right;
                    }
                }
            }
        }
    }

    // Update game logic (snake movement, collision detection, etc.)
    void update() {
        // Don't update if game is over or paused
        if (gameOver || paused) {
            return;
        }

        // Check if enough time has passed for the snake to move
        if (moveClock.getElapsedTime().asSeconds() >= MOVE_DELAY) {
            // Reset the movement clock
            moveClock.restart();

            // Update current direction to the buffered next direction
            currentDirection = nextDirection;

            // Calculate new head position based on current direction
            sf::Vector2i newHead = snake.front();  // Get current head position

            switch (currentDirection) {
                case Direction::Up:
                    newHead.y--;  // Move up (decrease Y)
                    break;
                case Direction::Down:
                    newHead.y++;  // Move down (increase Y)
                    break;
                case Direction::Left:
                    newHead.x--;  // Move left (decrease X)
                    break;
                case Direction::Right:
                    newHead.x++;  // Move right (increase X)
                    break;
            }

            // Check for wall collisions (hitting the boundaries)
            if (newHead.x < 0 || newHead.x >= GRID_WIDTH ||
                newHead.y < 0 || newHead.y >= GRID_HEIGHT) {
                gameOver = true;
                return;
            }

            // Check for self-collision (snake hitting its own body)
            for (size_t i = 0; i < snake.size(); ++i) {
                if (snake[i] == newHead) {
                    gameOver = true;
                    return;
                }
            }

            // Add new head to the front of the snake
            snake.push_front(newHead);

            // Check if snake ate the food
            if (newHead == foodPosition) {
                // Increase score
                score++;

                // Generate new food position
                generateFood();

                // Update score display if font is loaded
                if (fontLoaded && scoreText) {
                    scoreText->setString("Score: " + std::to_string(score));
                }

                // Don't remove tail (snake grows)
            } else {
                // Remove tail segment (snake moves without growing)
                snake.pop_back();
            }
        }
    }

    // Render all game objects to the window
    void render() {
        // Clear the window with a dark background color
        window.clear(sf::Color(30, 30, 30));

        // Draw the game grid (optional - for visual clarity)
        drawGrid();

        // Draw the snake
        for (size_t i = 0; i < snake.size(); ++i) {
            // Create a rectangle for each segment
            // Size is GRID_SIZE - 2 to leave a small gap between segments
            sf::RectangleShape segment(sf::Vector2f(GRID_SIZE - 2.f, GRID_SIZE - 2.f));

            // Head is brighter green, body segments are darker
            if (i == 0) {
                segment.setFillColor(sf::Color(0, 255, 0));  // Bright green for head
            } else {
                segment.setFillColor(sf::Color(0, 200, 0));  // Darker green for body
            }

            // Position the segment (with 1 pixel border for grid effect)
            float xPos = static_cast<float>(snake[i].x * GRID_SIZE + 1);
            float yPos = static_cast<float>(snake[i].y * GRID_SIZE + 1);
            segment.setPosition(sf::Vector2f(xPos, yPos));
            window.draw(segment);
        }

        // Draw the food
        sf::RectangleShape food(sf::Vector2f(GRID_SIZE - 2.f, GRID_SIZE - 2.f));
        food.setFillColor(sf::Color::Red);  // Red color for food
        float foodX = static_cast<float>(foodPosition.x * GRID_SIZE + 1);
        float foodY = static_cast<float>(foodPosition.y * GRID_SIZE + 1);
        food.setPosition(sf::Vector2f(foodX, foodY));
        window.draw(food);

        // Draw text elements if font is loaded
        if (fontLoaded) {
            // Draw score text
            if (scoreText) {
                window.draw(*scoreText);
            }

            // Draw game over text if game has ended
            if (gameOver && gameOverText) {
                window.draw(*gameOverText);
            }

            // Draw pause text if game is paused
            if (paused && !gameOver && pauseText) {
                window.draw(*pauseText);
            }
        }

        // Display everything on the screen
        window.display();
    }

    // Draw grid lines for visual clarity (optional)
    void drawGrid() {
        // Set grid line color (very dark gray)
        sf::Color gridColor(50, 50, 50);

        // Draw vertical lines
        for (int x = 0; x <= GRID_WIDTH; ++x) {
            sf::RectangleShape line(sf::Vector2f(1.f, GRID_HEIGHT * GRID_SIZE));
            line.setPosition(sf::Vector2f(x * GRID_SIZE, 0.f));
            line.setFillColor(gridColor);
            window.draw(line);
        }

        // Draw horizontal lines
        for (int y = 0; y <= GRID_HEIGHT; ++y) {
            sf::RectangleShape line(sf::Vector2f(GRID_WIDTH * GRID_SIZE, 1.f));
            line.setPosition(sf::Vector2f(0.f, y * GRID_SIZE));
            line.setFillColor(gridColor);
            window.draw(line);
        }
    }

    // Reset the game to initial state
    void resetGame() {
        // Reset game state flags
        gameOver = false;
        paused = false;
        score = 0;

        // Reset snake
        initializeSnake();
        currentDirection = Direction::Right;
        nextDirection = Direction::Right;

        // Generate new food
        generateFood();

        // Reset movement clock
        moveClock.restart();

        // Update score display if font is loaded
        if (fontLoaded && scoreText) {
            scoreText->setString("Score: 0");
        }
    }
};

// Main function - entry point of the program
int main() {
    // Create an instance of the Snake game
    SnakeGame game;

    // Run the game loop
    game.run();

    // Return 0 to indicate successful program execution
    return 0;
}
