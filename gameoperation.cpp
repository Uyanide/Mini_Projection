#include "gameoperation.h"

const GameOperation::Point GameOperation::START_A = std::make_pair(203, 437);
const GameOperation::Point GameOperation::START_B = std::make_pair(187, 961);

const std::array<GameOperation::Point, 10> GameOperation::BUTTON_POS = {
    std::make_pair(248, 1725),
    std::make_pair(128, 1914),
    std::make_pair(340, 1914),
    std::make_pair(555, 1914),
    std::make_pair(762, 1914),
    std::make_pair(980, 1914),
    std::make_pair(248, 2095),
    std::make_pair(458, 2095),
    std::make_pair(670, 2095),
    std::make_pair(883, 2095)};

const GameOperation::Point GameOperation::CLEAR_POS = std::make_pair(874, 1730);

const GameOperation::Point GameOperation::SIGN_POS = std::make_pair(156, 470);

const GameOperation::Point GameOperation::START_POS = std::make_pair(312, 2090);

const GameOperation::Point GameOperation::CLOSE_POS = std::make_pair(985, 592);

static inline quint8 convertGray(QRgb color) {
    return qRed(color) * 0.299 +
           qGreen(color) * 0.587 +
           qBlue(color) * 0.114;
}

GameOperation::GameOperation(QObject *parent) : QThread(parent) {}

GameOperation::~GameOperation() {}

void GameOperation::run() {
    while (true) {
        emit requestImage();
        msleep(1000);
    }
}

void GameOperation::handleImage(QImage image) {
    switch (state) {
        case INIT: {
            state = GameState::WAITING;
            handleImage(image);
            return;
        }
        case WAITING: {
            if (isPlayPage(image)) {
                state = GameState::PROCESSING;
                handleImage(image);
                return;
            } else {
                emit appendLog("Waiting for play page...");
                return;
            }
        }
        case PROCESSING: {
            if (!isPlayPage(image)) {
                state = GameState::WAITING;
                handleImage(image);
                return;
            }
            int res = analyseGrids(image);
            emit appendLog("Analyse result: " + QString::number(res));
            operation(res);
            rep++;
            emit appendLog(QString::number(MAX_REPEAT - rep) + QString(" attempts left"));
            if (rep >= MAX_REPEAT) {
                if (res != last_result) {
                    if (rety < MAX_RETRY) {
                        emit appendLog("Results mismatch, retrying...", LogType::WARNING);
                        rep = MAX_REPEAT - 1;
                        rety++;
                    } else {
                        resetValues();
                        emit appendLog("Results mismatch, aborting...", LogType::ERROR);
                        state = GameState::FAIL;
                        handleImage(image);
                        return;
                    }
                } else {
                    curr++;
                    resetValues();
                    appendLog("Completed " + QString::number(curr) + " times", LogType::GOOD);
                    state = GameState::SUCCESS;
                }
            }
            last_result = res;
            break;
        }
        case SUCCESS: {
            if (!isPlayPage(image)) {
                state = GameState::WAITING;
                handleImage(image);
                return;
            }
            appendLog("Waiting for next round...");
            break;
        }
        case FAIL: {
            emit failed();
            break;
        }
    }
}

void GameOperation::resetValues() {
    rep = 0;
    rety = 0;
    last_result = -1;
}

bool GameOperation::isPlayPage(QImage image) {
    return convertGray(image.pixel(SIGN_POS.first, SIGN_POS.second)) > SIGN_THRESHOLD;
}

int GameOperation::analyseGrids(QImage image) {
    int count = 0;
    for (int x = 0; x < 5; x++) {
        for (int y = 0; y < 5; y++) {
            int ax = START_A.first + x * GRID_WIDTH;
            int ay = START_A.second + y * GRID_WIDTH;
            int bx = START_B.first + x * GRID_WIDTH;
            int by = START_B.second + y * GRID_WIDTH;
            if (convertGray(image.pixel(ax, ay)) < GRID_THRESHOLD || convertGray(image.pixel(bx, by)) < GRID_THRESHOLD) {
                count++;
            }
        }
    }
    return count;
}

void GameOperation::operation(int result) {
    QString numStr = QString::number(result);
    emit tap(CLEAR_POS.first, CLEAR_POS.second);
    for (QChar c : numStr) {
        emit tap(BUTTON_POS[c.digitValue()].first, BUTTON_POS[c.digitValue()].second);
    }
}