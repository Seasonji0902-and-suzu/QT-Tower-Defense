#include "mainwindow.h"

#include "game/gamecontroller.h"

#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_scene(new QGraphicsScene(this))
    , m_controller(new GameController(m_scene, this))
{
    setWindowTitle(QStringLiteral("网格守卫 · Grid Guard"));
    setMinimumSize(1120, 700);
    resize(1180, 760);

    QString errorMessage;
    if (!m_controller->initialize(&errorMessage)) {
        QMessageBox::critical(this, QStringLiteral("配置错误"), errorMessage);
    }

    m_pages = new QStackedWidget(this);
    m_startPage = createStartPage();
    m_helpPage = createHelpPage();
    m_levelPage = createLevelPage();
    m_gamePage = createGamePage();
    m_resultPage = createResultPage();
    m_pages->addWidget(m_startPage);
    m_pages->addWidget(m_helpPage);
    m_pages->addWidget(m_levelPage);
    m_pages->addWidget(m_gamePage);
    m_pages->addWidget(m_resultPage);
    setCentralWidget(m_pages);

    connectController();
    refreshLevelButtons();
    updateTowerButtonSelection(TowerType::Shooter);
}

void MainWindow::startSmokeScenario()
{
    openLevel(0);
    m_controller->applyCheatCode(QStringLiteral("MONEY1000"));
    m_controller->selectTowerType(TowerType::Shooter);
    m_controller->handleSceneClick(QPointF(32.0, 32.0));
    m_controller->selectTowerType(TowerType::Resource);
    m_controller->handleSceneClick(QPointF(96.0, 32.0));
    m_controller->selectTowerType(TowerType::Wall);
    m_controller->handleSceneClick(QPointF(96.0, 160.0));
    QTimer::singleShot(700, this, [this] { openLevel(1); });
    QTimer::singleShot(1400, this, [this] { openLevel(2); });
}

QWidget *MainWindow::createStartPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(110, 70, 110, 70);
    layout->setSpacing(18);

    layout->addStretch();
    QLabel *title = new QLabel(QStringLiteral("网格守卫"), page);
    title->setObjectName(QStringLiteral("titleLabel"));
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel *subtitle = new QLabel(
        QStringLiteral("2D 网格塔防 · Qt 6 / C++ 面向对象课程项目"), page);
    subtitle->setObjectName(QStringLiteral("subtitleLabel"));
    subtitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitle);

    QHBoxLayout *preview = new QHBoxLayout;
    preview->setSpacing(34);
    const QStringList icons = {
        QStringLiteral(":/assets/towers/shooter.png"),
        QStringLiteral(":/assets/towers/splash.png"),
        QStringLiteral(":/assets/enemies/boss.png")
    };
    for (const QString &path : icons) {
        QLabel *image = new QLabel(page);
        image->setPixmap(QPixmap(path).scaled(82, 82, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        image->setAlignment(Qt::AlignCenter);
        preview->addWidget(image);
    }
    layout->addLayout(preview);

    QPushButton *start = makeMenuButton(QStringLiteral("开始游戏"), page);
    QPushButton *help = makeMenuButton(QStringLiteral("玩法与评分点"), page);
    QPushButton *quit = makeMenuButton(QStringLiteral("退出"), page);
    quit->setObjectName(QStringLiteral("dangerButton"));
    start->setMaximumWidth(360);
    help->setMaximumWidth(360);
    quit->setMaximumWidth(360);
    layout->addWidget(start, 0, Qt::AlignHCenter);
    layout->addWidget(help, 0, Qt::AlignHCenter);
    layout->addWidget(quit, 0, Qt::AlignHCenter);
    layout->addStretch();

    connect(start, &QPushButton::clicked, this, &MainWindow::showLevelPage);
    connect(help, &QPushButton::clicked, this, [this] { m_pages->setCurrentWidget(m_helpPage); });
    connect(quit, &QPushButton::clicked, qApp, &QApplication::quit);
    return page;
}

QWidget *MainWindow::createHelpPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(80, 50, 80, 50);
    layout->setSpacing(14);

    QLabel *title = new QLabel(QStringLiteral("玩法与功能说明"), page);
    title->setObjectName(QStringLiteral("sectionTitle"));
    layout->addWidget(title);

    QLabel *text = new QLabel(page);
    text->setWordWrap(true);
    text->setTextFormat(Qt::RichText);
    text->setText(QStringLiteral(
        "<h3>操作</h3>"
        "<p>在右侧选择防御塔，点击合法格子建造。点击已建塔可查看状态，双击或点击按钮升级。"
        "空格键暂停/继续；减速塔拥有“全体冻结”主动技能。</p>"
        "<h3>地形</h3>"
        "<p>草地无特殊效果；黑土地建造费用降低 30%；石地禁止建造；冰面让敌人加速但强化减速；"
        "传送门会把敌人送往另一格。</p>"
        "<h3>塔与敌人</h3>"
        "<p>六类塔：射手、减速、范围、激光、资源和防御墙。六类敌人：普通、快速、重甲、抗性、"
        "分裂和 Boss。游戏包含减速、灼烧、眩晕三种限时效果。</p>"
        "<h3>调试作弊码</h3>"
        "<p>MONEY1000 增加资源；CLEARWAVE 清除当前敌人。输入框位于游戏右侧。</p>"));
    layout->addWidget(text, 1);

    QPushButton *back = makeMenuButton(QStringLiteral("返回"), page);
    back->setMaximumWidth(240);
    layout->addWidget(back, 0, Qt::AlignLeft);
    connect(back, &QPushButton::clicked, this, [this] { m_pages->setCurrentWidget(m_startPage); });
    return page;
}

QWidget *MainWindow::createLevelPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(70, 45, 70, 45);
    layout->setSpacing(16);

    QLabel *title = new QLabel(QStringLiteral("选择关卡"), page);
    title->setObjectName(QStringLiteral("sectionTitle"));
    layout->addWidget(title);

    for (int index = 0; index < m_controller->levelCount(); ++index) {
        QPushButton *button = new QPushButton(page);
        button->setMinimumHeight(105);
        button->setIcon(QIcon(QStringLiteral(":/assets/tiles/portal.png")));
        button->setIconSize({64, 64});
        button->setStyleSheet(QStringLiteral("text-align: left; padding: 14px 22px;"));
        layout->addWidget(button);
        m_levelButtons.append(button);
        connect(button, &QPushButton::clicked, this, [this, index] { openLevel(index); });
    }
    layout->addStretch();

    QPushButton *back = makeMenuButton(QStringLiteral("返回主菜单"), page);
    back->setMaximumWidth(240);
    layout->addWidget(back, 0, Qt::AlignLeft);
    connect(back, &QPushButton::clicked, this, [this] { m_pages->setCurrentWidget(m_startPage); });
    return page;
}

QWidget *MainWindow::createGamePage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *root = new QVBoxLayout(page);
    root->setContentsMargins(18, 16, 18, 16);
    root->setSpacing(10);

    m_hudLabel = new QLabel(page);
    m_hudLabel->setObjectName(QStringLiteral("hudLabel"));
    m_hudLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_hudLabel);

    QHBoxLayout *body = new QHBoxLayout;
    body->setSpacing(14);
    m_view = new QGraphicsView(m_scene, page);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    m_view->viewport()->installEventFilter(this);
    body->addWidget(m_view, 1);

    QFrame *panel = new QFrame(page);
    panel->setObjectName(QStringLiteral("panel"));
    panel->setFixedWidth(330);
    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(14, 14, 14, 14);
    panelLayout->setSpacing(8);

    QLabel *shopTitle = new QLabel(QStringLiteral("防御塔商店"), panel);
    shopTitle->setObjectName(QStringLiteral("sectionTitle"));
    panelLayout->addWidget(shopTitle);

    QGridLayout *shop = new QGridLayout;
    const QVector<TowerType> types = {
        TowerType::Shooter, TowerType::Slow, TowerType::Splash,
        TowerType::Laser, TowerType::Resource, TowerType::Wall
    };
    for (int index = 0; index < types.size(); ++index) {
        QPushButton *button = makeTowerButton(types.at(index));
        shop->addWidget(button, index / 2, index % 2);
    }
    panelLayout->addLayout(shop);

    m_towerInfoLabel = new QLabel(QStringLiteral("点击已建造的防御塔查看状态。"), panel);
    m_towerInfoLabel->setWordWrap(true);
    m_towerInfoLabel->setMinimumHeight(118);
    m_towerInfoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    panelLayout->addWidget(m_towerInfoLabel);

    QHBoxLayout *towerActions = new QHBoxLayout;
    QPushButton *upgrade = new QPushButton(QStringLiteral("升级"), panel);
    QPushButton *skill = new QPushButton(QStringLiteral("主动技能"), panel);
    towerActions->addWidget(upgrade);
    towerActions->addWidget(skill);
    panelLayout->addLayout(towerActions);

    m_cheatInput = new QLineEdit(panel);
    m_cheatInput->setPlaceholderText(QStringLiteral("作弊码（回车执行）"));
    panelLayout->addWidget(m_cheatInput);

    QHBoxLayout *controls = new QHBoxLayout;
    m_pauseButton = new QPushButton(QStringLiteral("暂停"), panel);
    QPushButton *restart = new QPushButton(QStringLiteral("重新开始"), panel);
    controls->addWidget(m_pauseButton);
    controls->addWidget(restart);
    panelLayout->addLayout(controls);

    QPushButton *menu = new QPushButton(QStringLiteral("返回选关"), panel);
    menu->setObjectName(QStringLiteral("dangerButton"));
    panelLayout->addWidget(menu);
    panelLayout->addStretch();
    body->addWidget(panel);
    root->addLayout(body, 1);

    m_messageLabel = new QLabel(QStringLiteral("准备就绪。"), page);
    m_messageLabel->setObjectName(QStringLiteral("messageLabel"));
    m_messageLabel->setAlignment(Qt::AlignCenter);
    root->addWidget(m_messageLabel);

    connect(upgrade, &QPushButton::clicked, m_controller, &GameController::upgradeSelectedTower);
    connect(skill, &QPushButton::clicked, m_controller, &GameController::activateSelectedSkill);
    connect(m_pauseButton, &QPushButton::clicked, m_controller, &GameController::togglePause);
    connect(restart, &QPushButton::clicked, m_controller, &GameController::restartLevel);
    connect(menu, &QPushButton::clicked, this, &MainWindow::showLevelPage);
    connect(m_cheatInput, &QLineEdit::returnPressed, this, [this] {
        m_controller->applyCheatCode(m_cheatInput->text());
        m_cheatInput->clear();
    });
    return page;
}

QWidget *MainWindow::createResultPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(150, 80, 150, 80);
    layout->setSpacing(18);
    layout->addStretch();

    m_resultTitle = new QLabel(page);
    m_resultTitle->setObjectName(QStringLiteral("titleLabel"));
    m_resultTitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_resultTitle);

    m_resultSummary = new QLabel(page);
    m_resultSummary->setObjectName(QStringLiteral("hudLabel"));
    m_resultSummary->setAlignment(Qt::AlignCenter);
    m_resultSummary->setMinimumHeight(170);
    layout->addWidget(m_resultSummary);

    m_nextLevelButton = makeMenuButton(QStringLiteral("进入下一关"), page);
    QPushButton *retry = makeMenuButton(QStringLiteral("重新挑战"), page);
    QPushButton *levels = makeMenuButton(QStringLiteral("返回选关"), page);
    m_nextLevelButton->setMaximumWidth(320);
    retry->setMaximumWidth(320);
    levels->setMaximumWidth(320);
    layout->addWidget(m_nextLevelButton, 0, Qt::AlignHCenter);
    layout->addWidget(retry, 0, Qt::AlignHCenter);
    layout->addWidget(levels, 0, Qt::AlignHCenter);
    layout->addStretch();

    connect(m_nextLevelButton, &QPushButton::clicked, this, [this] {
        openLevel(m_controller->currentLevelIndex() + 1);
    });
    connect(retry, &QPushButton::clicked, this, [this] {
        openLevel(m_controller->currentLevelIndex());
    });
    connect(levels, &QPushButton::clicked, this, &MainWindow::showLevelPage);
    return page;
}

QPushButton *MainWindow::makeMenuButton(const QString &text, QWidget *parent)
{
    QPushButton *button = new QPushButton(text, parent);
    button->setMinimumHeight(46);
    return button;
}

QPushButton *MainWindow::makeTowerButton(TowerType type)
{
    QPushButton *button = new QPushButton(
        QStringLiteral("%1\n%2").arg(towerDisplayName(type)).arg(towerBaseCost(type)), m_gamePage);
    QString iconPath;
    switch (type) {
    case TowerType::Shooter: iconPath = QStringLiteral(":/assets/towers/shooter.png"); break;
    case TowerType::Slow: iconPath = QStringLiteral(":/assets/towers/slow.png"); break;
    case TowerType::Splash: iconPath = QStringLiteral(":/assets/towers/splash.png"); break;
    case TowerType::Laser: iconPath = QStringLiteral(":/assets/towers/laser.png"); break;
    case TowerType::Resource: iconPath = QStringLiteral(":/assets/towers/resource.png"); break;
    case TowerType::Wall: iconPath = QStringLiteral(":/assets/towers/wall.png"); break;
    }
    button->setIcon(QIcon(iconPath));
    button->setIconSize({42, 42});
    button->setMinimumHeight(68);
    button->setProperty("towerSelected", false);
    m_towerButtons.emplace(type, button);
    connect(button, &QPushButton::clicked, this, [this, type] {
        m_controller->selectTowerType(type);
    });
    return button;
}

void MainWindow::connectController()
{
    m_controller->hudChanged = [this](const QString &text) { m_hudLabel->setText(text); };
    m_controller->messageChanged = [this](const QString &text) { m_messageLabel->setText(text); };
    m_controller->towerInfoChanged = [this](const QString &text) { m_towerInfoLabel->setText(text); };
    m_controller->selectedTowerTypeChanged = [this](TowerType type) {
        updateTowerButtonSelection(type);
    };
    m_controller->stateChanged = [this](GameState state) {
        if (m_pauseButton) {
            m_pauseButton->setText(state == GameState::Paused ? QStringLiteral("继续")
                                                              : QStringLiteral("暂停"));
        }
    };
    m_controller->resultReady = [this](bool won, const QString &summary) {
        m_resultTitle->setText(won ? QStringLiteral("胜利") : QStringLiteral("失败"));
        m_resultSummary->setText(summary);
        const int next = m_controller->currentLevelIndex() + 1;
        m_nextLevelButton->setVisible(won && next < m_controller->levelCount());
        m_pages->setCurrentWidget(m_resultPage);
    };
}

void MainWindow::refreshLevelButtons()
{
    for (int index = 0; index < m_levelButtons.size(); ++index) {
        m_levelButtons[index]->setText(QStringLiteral("%1\n%2\n%3")
            .arg(m_controller->levelName(index))
            .arg(m_controller->levelDescription(index))
            .arg(m_controller->levelProgressText(index)));
    }
}

void MainWindow::openLevel(int index)
{
    if (index < 0 || index >= m_controller->levelCount()) return;
    m_controller->startLevel(index);
    m_pages->setCurrentWidget(m_gamePage);
    QTimer::singleShot(0, this, [this] {
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    });
}

void MainWindow::showLevelPage()
{
    m_controller->stopGame();
    refreshLevelButtons();
    m_pages->setCurrentWidget(m_levelPage);
}

void MainWindow::updateTowerButtonSelection(TowerType type)
{
    for (auto &[buttonType, button] : m_towerButtons) {
        button->setProperty("towerSelected", buttonType == type);
        button->style()->unpolish(button);
        button->style()->polish(button);
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (m_view && watched == m_view->viewport()) {
        if (event->type() == QEvent::MouseButtonRelease) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                m_controller->handleSceneClick(m_view->mapToScene(mouseEvent->position().toPoint()));
                return true;
            }
        }
        if (event->type() == QEvent::MouseButtonDblClick) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                m_controller->handleSceneDoubleClick(m_view->mapToScene(mouseEvent->position().toPoint()));
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (m_pages->currentWidget() == m_gamePage && event->key() == Qt::Key_Space) {
        m_controller->togglePause();
        return;
    }
    QMainWindow::keyPressEvent(event);
}
