// Game Rắn Săn Mồi sử dụng SFML 3.0.1
// Tính năng: Viền wrap-around (xuyên tường) và lưu điểm cao
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <random>
#include <deque>
#include <fstream>
#include <string>

// ============= CÁC HẰNG SỐ GAME =============
const int GRID_SIZE = 20;        // Kích thước mỗi ô trong lưới (pixels)
const int GRID_WIDTH = 30;       // Số ô theo chiều ngang
const int GRID_HEIGHT = 20;      // Số ô theo chiều dọc
const float MOVE_DELAY = 0.1f;   // Thời gian giữa các lần di chuyển (giây)

// ============= ENUM HƯỚNG DI CHUYỂN =============
enum class Direction {
    Up,    // Đi lên
    Down,  // Đi xuống
    Left,  // Đi trái
    Right  // Đi phải
};

// ============= LỚP GAME RẮN SĂN MỒI =============
class SnakeGame {
private:
    // --- Cửa sổ game ---
    sf::RenderWindow window;

    // --- Dữ liệu con rắn ---
    std::deque<sf::Vector2i> snake;  // Danh sách các đoạn thân rắn (đầu ở đầu deque)
    Direction currentDirection;       // Hướng hiện tại của rắn
    Direction nextDirection;          // Hướng tiếp theo (lưu input của người chơi)

    // --- Dữ liệu thức ăn ---
    sf::Vector2i foodPosition;        // Vị trí của thức ăn trên lưới

    // --- Bộ tạo số ngẫu nhiên ---
    std::random_device rd;            // Thiết bị sinh số ngẫu nhiên
    std::mt19937 gen;                 // Bộ sinh số Mersenne Twister
    std::uniform_int_distribution<> distX;  // Phân phối cho tọa độ X
    std::uniform_int_distribution<> distY;  // Phân phối cho tọa độ Y

    // --- Trạng thái game ---
    bool gameOver;                    // Cờ đánh dấu game kết thúc
    bool paused;                      // Cờ đánh dấu game tạm dừng
    int score;                        // Điểm hiện tại
    int highScore;                    // Điểm cao nhất
    std::string highScoreFile;        // Tên file lưu điểm cao

    // --- Đồng hồ thời gian ---
    sf::Clock moveClock;              // Đồng hồ để kiểm soát tốc độ di chuyển

    // --- Font và text hiển thị ---
    sf::Font font;                    // Font chữ
    bool fontLoaded;                  // Cờ đánh dấu font đã load thành công
    std::unique_ptr<sf::Text> scoreText;      // Text hiển thị điểm số
    std::unique_ptr<sf::Text> highScoreText;  // Text hiển thị điểm cao
    std::unique_ptr<sf::Text> gameOverText;   // Text hiển thị game over
    std::unique_ptr<sf::Text> pauseText;      // Text hiển thị tạm dừng

public:
    // ============= CONSTRUCTOR - KHỞI TẠO GAME =============
    SnakeGame() :
        window(sf::VideoMode(sf::Vector2u(GRID_WIDTH * GRID_SIZE, GRID_HEIGHT * GRID_SIZE)),
               "Snake Game - Wrap-around & High Score"),
        gen(rd()),                           // Khởi tạo bộ sinh số ngẫu nhiên
        distX(0, GRID_WIDTH - 1),           // Phân phối X từ 0 đến chiều rộng - 1
        distY(0, GRID_HEIGHT - 1),          // Phân phối Y từ 0 đến chiều cao - 1
        gameOver(false),                     // Game bắt đầu chưa kết thúc
        paused(false),                       // Game bắt đầu không tạm dừng
        score(0),                            // Điểm bắt đầu = 0
        highScore(0),                        // Điểm cao mặc định = 0
        highScoreFile("highscore.txt"),      // File lưu điểm cao
        fontLoaded(false)                    // Font chưa load
    {
        window.setFramerateLimit(60);  // Giới hạn 60 FPS

        // Khởi tạo con rắn ban đầu
        initializeSnake();

        // Đặt hướng ban đầu sang phải
        currentDirection = Direction::Right;
        nextDirection = Direction::Right;

        // Tạo thức ăn đầu tiên
        generateFood();

        // Đọc điểm cao từ file
        loadHighScore();

        // Thử load font từ nhiều vị trí khác nhau
        if (font.openFromFile("arial.ttf") ||
            font.openFromFile("C:/Windows/Fonts/arial.ttf") ||
            font.openFromFile("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf") ||
            font.openFromFile("/System/Library/Fonts/Helvetica.ttc")) {
            fontLoaded = true;
            initializeTexts();  // Khởi tạo các text hiển thị
        }
    }

    // ============= ĐỌC ĐIỂM CAO TỪ FILE =============
    void loadHighScore() {
        std::ifstream file(highScoreFile);  // Mở file để đọc

        if (file.is_open()) {
            // Nếu file tồn tại và mở được
            file >> highScore;  // Đọc điểm cao
            file.close();       // Đóng file
        } else {
            // Nếu file không tồn tại, khởi tạo điểm cao = 0
            highScore = 0;
        }
    }

    // ============= LƯU ĐIỂM CAO VÀO FILE =============
    void saveHighScore() {
        std::ofstream file(highScoreFile);  // Mở file để ghi (tạo mới nếu chưa có)

        if (file.is_open()) {
            file << highScore;  // Ghi điểm cao vào file
            file.close();       // Đóng file
        }
    }

    // ============= KHỞI TẠO CÁC TEXT HIỂN THỊ =============
    void initializeTexts() {
        if (!fontLoaded) return;  // Không làm gì nếu font chưa load

        // --- Text điểm số hiện tại ---
        scoreText = std::make_unique<sf::Text>(font, "Score: 0", 20);
        scoreText->setFillColor(sf::Color::White);  // Màu trắng
        scoreText->setPosition(sf::Vector2f(10.f, 10.f));  // Góc trên trái

        // --- Text điểm cao nhất ---
        highScoreText = std::make_unique<sf::Text>(font, "High Score: " + std::to_string(highScore), 20);
        highScoreText->setFillColor(sf::Color::Yellow);  // Màu vàng
        highScoreText->setPosition(sf::Vector2f(10.f, 35.f));  // Dưới text điểm số

        // --- Text game over ---
        gameOverText = std::make_unique<sf::Text>(font, "GAME OVER! Press R to restart", 30);
        gameOverText->setFillColor(sf::Color::Red);  // Màu đỏ

        // Căn giữa text game over
        sf::FloatRect textBounds = gameOverText->getLocalBounds();
        float xPos = (GRID_WIDTH * GRID_SIZE - textBounds.size.x) / 2.f;
        float yPos = (GRID_HEIGHT * GRID_SIZE - textBounds.size.y) / 2.f;
        gameOverText->setPosition(sf::Vector2f(xPos, yPos));

        // --- Text tạm dừng ---
        pauseText = std::make_unique<sf::Text>(font, "PAUSED - Press Space to continue", 25);
        pauseText->setFillColor(sf::Color::Yellow);  // Màu vàng

        // Căn giữa text pause
        textBounds = pauseText->getLocalBounds();
        xPos = (GRID_WIDTH * GRID_SIZE - textBounds.size.x) / 2.f;
        yPos = (GRID_HEIGHT * GRID_SIZE - textBounds.size.y) / 2.f;
        pauseText->setPosition(sf::Vector2f(xPos, yPos));
    }

    // ============= KHỞI TẠO CON RẮN =============
    void initializeSnake() {
        snake.clear();  // Xóa hết các đoạn thân cũ

        // Vị trí bắt đầu ở giữa màn hình
        int startX = GRID_WIDTH / 2;
        int startY = GRID_HEIGHT / 2;

        // Tạo 3 đoạn thân ban đầu (nằm ngang)
        for (int i = 0; i < 3; ++i) {
            snake.push_back(sf::Vector2i(startX - i, startY));
        }
    }

    // ============= TẠO THỨC ĂN NGẪU NHIÊN =============
    void generateFood() {
        do {
            // Tạo vị trí ngẫu nhiên
            foodPosition.x = distX(gen);
            foodPosition.y = distY(gen);
        } while (isSnakePosition(foodPosition));  // Lặp lại nếu trùng với thân rắn
    }

    // ============= KIỂM TRA VỊ TRÍ CÓ PHẢI LÀ THÂN RẮN =============
    bool isSnakePosition(const sf::Vector2i& position) {
        // Duyệt qua tất cả các đoạn thân rắn
        for (const auto& segment : snake) {
            if (segment == position) {
                return true;  // Vị trí bị chiếm bởi rắn
            }
        }
        return false;  // Vị trí trống
    }

    // ============= XỬ LÝ WRAP-AROUND (XUYÊN TƯỜNG) =============
    sf::Vector2i wrapPosition(sf::Vector2i position) {
        // Nếu ra ngoài biên trái -> xuất hiện bên phải
        if (position.x < 0) position.x = GRID_WIDTH - 1;

        // Nếu ra ngoài biên phải -> xuất hiện bên trái
        if (position.x >= GRID_WIDTH) position.x = 0;

        // Nếu ra ngoài biên trên -> xuất hiện dưới
        if (position.y < 0) position.y = GRID_HEIGHT - 1;

        // Nếu ra ngoài biên dưới -> xuất hiện trên
        if (position.y >= GRID_HEIGHT) position.y = 0;

        return position;
    }

    // ============= VÒNG LẶP CHÍNH CỦA GAME =============
    void run() {
        while (window.isOpen()) {
            handleEvents();  // Xử lý input từ người chơi
            update();        // Cập nhật logic game
            render();        // Vẽ lên màn hình
        }
    }

    // ============= XỬ LÝ SỰ KIỆN VÀ INPUT =============
    void handleEvents() {
        // Xử lý tất cả các sự kiện đang chờ
        while (const std::optional<sf::Event> event = window.pollEvent()) {

            // Sự kiện đóng cửa sổ
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // Sự kiện nhấn phím
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                sf::Keyboard::Key key = keyPressed->code;

                // Nếu game over, chỉ xử lý phím R (restart)
                if (gameOver) {
                    if (key == sf::Keyboard::Key::R) {
                        resetGame();
                    }
                    continue;
                }

                // Phím Space để tạm dừng/tiếp tục
                if (key == sf::Keyboard::Key::Space) {
                    paused = !paused;  // Đảo ngược trạng thái pause
                    continue;
                }

                // Xử lý phím điều khiển (chỉ khi không pause)
                if (!paused) {
                    // Mũi tên lên - không cho đi ngược lại khi đang đi xuống
                    if (key == sf::Keyboard::Key::Up && currentDirection != Direction::Down) {
                        nextDirection = Direction::Up;
                    }
                    // Mũi tên xuống - không cho đi ngược lại khi đang đi lên
                    else if (key == sf::Keyboard::Key::Down && currentDirection != Direction::Up) {
                        nextDirection = Direction::Down;
                    }
                    // Mũi tên trái - không cho đi ngược lại khi đang đi phải
                    else if (key == sf::Keyboard::Key::Left && currentDirection != Direction::Right) {
                        nextDirection = Direction::Left;
                    }
                    // Mũi tên phải - không cho đi ngược lại khi đang đi trái
                    else if (key == sf::Keyboard::Key::Right && currentDirection != Direction::Left) {
                        nextDirection = Direction::Right;
                    }
                }
            }
        }
    }

    // ============= CẬP NHẬT LOGIC GAME =============
    void update() {
        // Không cập nhật nếu game over hoặc pause
        if (gameOver || paused) {
            return;
        }

        // Kiểm tra đã đủ thời gian để di chuyển chưa
        if (moveClock.getElapsedTime().asSeconds() >= MOVE_DELAY) {
            moveClock.restart();  // Reset đồng hồ

            // Cập nhật hướng hiện tại
            currentDirection = nextDirection;

            // Tính vị trí đầu mới dựa vào hướng di chuyển
            sf::Vector2i newHead = snake.front();

            switch (currentDirection) {
                case Direction::Up:
                    newHead.y--;  // Đi lên (giảm Y)
                    break;
                case Direction::Down:
                    newHead.y++;  // Đi xuống (tăng Y)
                    break;
                case Direction::Left:
                    newHead.x--;  // Đi trái (giảm X)
                    break;
                case Direction::Right:
                    newHead.x++;  // Đi phải (tăng X)
                    break;
            }

            // Áp dụng wrap-around (xuyên tường)
            newHead = wrapPosition(newHead);

            // Kiểm tra va chạm với chính thân mình
            for (size_t i = 0; i < snake.size(); ++i) {
                if (snake[i] == newHead) {
                    gameOver = true;

                    // Cập nhật và lưu điểm cao nếu phá kỷ lục
                    if (score > highScore) {
                        highScore = score;
                        saveHighScore();  // Lưu vào file

                        if (fontLoaded && highScoreText) {
                            highScoreText->setString("High Score: " + std::to_string(highScore));
                        }
                    }
                    return;
                }
            }

            // Thêm đầu mới vào đầu deque
            snake.push_front(newHead);

            // Kiểm tra ăn được thức ăn không
            if (newHead == foodPosition) {
                score++;  // Tăng điểm
                generateFood();  // Tạo thức ăn mới

                // Cập nhật text điểm số
                if (fontLoaded && scoreText) {
                    scoreText->setString("Score: " + std::to_string(score));
                }

                // Cập nhật điểm cao nếu vượt qua
                if (score > highScore) {
                    highScore = score;
                    saveHighScore();  // Lưu ngay vào file

                    if (fontLoaded && highScoreText) {
                        highScoreText->setString("High Score: " + std::to_string(highScore));
                    }
                }

                // Không xóa đuôi -> rắn dài ra
            } else {
                // Xóa đuôi -> rắn di chuyển bình thường
                snake.pop_back();
            }
        }
    }

    // ============= VẼ TẤT CẢ LÊN MÀN HÌNH =============
    void render() {
        // Xóa màn hình với màu xám đậm
        window.clear(sf::Color(30, 30, 30));

        // Vẽ con rắn
        for (size_t i = 0; i < snake.size(); ++i) {
            sf::RectangleShape segment(sf::Vector2f(GRID_SIZE, GRID_SIZE));

            if (i == 0) {
                // Đầu rắn màu xanh lá sáng
                segment.setFillColor(sf::Color(0, 255, 0));
            } else {
                // Thân rắn màu xanh lá đậm hơn
                segment.setFillColor(sf::Color(0, 200, 0));
            }

            // Đặt vị trí của đoạn thân
            float xPos = static_cast<float>(snake[i].x * GRID_SIZE);
            float yPos = static_cast<float>(snake[i].y * GRID_SIZE);
            segment.setPosition(sf::Vector2f(xPos, yPos));
            window.draw(segment);
        }

        // Vẽ thức ăn
        sf::RectangleShape food(sf::Vector2f(GRID_SIZE, GRID_SIZE));
        food.setFillColor(sf::Color::Red);  // Màu đỏ
        float foodX = static_cast<float>(foodPosition.x * GRID_SIZE);
        float foodY = static_cast<float>(foodPosition.y * GRID_SIZE);
        food.setPosition(sf::Vector2f(foodX, foodY));
        window.draw(food);

        // Vẽ các text nếu font đã load
        if (fontLoaded) {
            // Vẽ điểm số
            if (scoreText) {
                window.draw(*scoreText);
            }

            // Vẽ điểm cao
            if (highScoreText) {
                window.draw(*highScoreText);
            }

            // Vẽ text game over nếu game kết thúc
            if (gameOver && gameOverText) {
                window.draw(*gameOverText);
            }

            // Vẽ text pause nếu đang tạm dừng
            if (paused && !gameOver && pauseText) {
                window.draw(*pauseText);
            }
        }

        // Hiển thị tất cả lên màn hình
        window.display();
    }

    // ============= RESET GAME VỀ TRẠNG THÁI BAN ĐẦU =============
    void resetGame() {
        gameOver = false;
        paused = false;
        score = 0;

        // Khởi tạo lại rắn
        initializeSnake();
        currentDirection = Direction::Right;
        nextDirection = Direction::Right;

        // Tạo thức ăn mới
        generateFood();

        // Reset đồng hồ di chuyển
        moveClock.restart();

        // Cập nhật text điểm số
        if (fontLoaded && scoreText) {
            scoreText->setString("Score: 0");
        }
    }
};

// ============= HÀM MAIN - ĐIỂM BẮT ĐẦU CHƯƠNG TRÌNH =============
int main() {
    // Tạo đối tượng game
    SnakeGame game;

    // Chạy game loop
    game.run();

    // Kết thúc chương trình
    return 0;
}
