#ifndef GAMEOPERATION_H
#define GAMEOPERATION_H

#include <QImage>
#include <QThread>
#include <array>
#include <utility>

class GameOperation : public QThread {
    Q_OBJECT

   public:
    enum GameState {
        INIT,
        WAITING,
        PROCESSING,
        SUCCESS,
        FAIL
    };

    enum LogType {
        INFO = 0,
        WARNING,
        ERROR,
        GOOD
    };

    using Point = std::pair<int, int>;

    GameOperation(QObject* parent = nullptr);
    ~GameOperation();

    void run() override;

   private:
    inline bool isPlayPage(QImage image);
    int analyseGrids(QImage image);
    void operation(int result);
    void resetValues();

   signals:
    void requestImage();
    void appendLog(QString log, LogType type = LogType::INFO);
    void failed();
    void tap(int x, int y);

   public slots:
    void handleImage(QImage image);

   private:
    static constexpr int MAX_REPEAT = 3;
    static constexpr int MAX_RETRY = 3;

    GameState state = GameState::INIT;

    int rep = 0;
    int rety = 0;
    int curr = 0;
    int last_result = 0;

    static constexpr uint8_t GRID_THRESHOLD = 125;
    static const Point START_A;
    static const Point START_B;
    static constexpr uint16_t GRID_WIDTH = 336 / 4;

    static const std::array<Point, 10> BUTTON_POS;

    static const Point CLEAR_POS;

    static constexpr uint8_t SIGN_THRESHOLD = 200;
    static const Point SIGN_POS;

    static const Point START_POS;
    static const Point CLOSE_POS;
};

#endif  // GAMEOPERATION_H
