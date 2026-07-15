#pragma once

#include "game/enums.h"

#include <QMainWindow>

#include <map>

class GameController;
class QFrame;
class QGraphicsScene;
class QGraphicsView;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);
    void startSmokeScenario();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QWidget *createStartPage();
    QWidget *createHelpPage();
    QWidget *createLevelPage();
    QWidget *createGamePage();
    QWidget *createResultPage();
    QPushButton *makeMenuButton(const QString &text, QWidget *parent = nullptr);
    QPushButton *makeTowerButton(TowerType type);
    void connectController();
    void refreshLevelButtons();
    void openLevel(int index);
    void showLevelPage();
    void updateTowerButtonSelection(TowerType type);

    QStackedWidget *m_pages = nullptr;
    QWidget *m_startPage = nullptr;
    QWidget *m_helpPage = nullptr;
    QWidget *m_levelPage = nullptr;
    QWidget *m_gamePage = nullptr;
    QWidget *m_resultPage = nullptr;

    QGraphicsScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;
    GameController *m_controller = nullptr;

    QLabel *m_hudLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
    QLabel *m_towerInfoLabel = nullptr;
    QLabel *m_resultTitle = nullptr;
    QLabel *m_resultSummary = nullptr;
    QLineEdit *m_cheatInput = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_nextLevelButton = nullptr;
    QVector<QPushButton *> m_levelButtons;
    std::map<TowerType, QPushButton *> m_towerButtons;
};
