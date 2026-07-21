#include "mainwindow.h"

#include "game/gamecontroller.h"

#include <QApplication>
#include <QFileDialog>
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
    m_editorLevel = LevelConfigLoader::createEditorTemplate();

    m_pages = new QStackedWidget(this);
    m_startPage = createStartPage();
    m_helpPage = createHelpPage();
    m_levelPage = createLevelPage();
    m_editorPage = createEditorPage();
    m_gamePage = createGamePage();
    m_resultPage = createResultPage();
    m_pages->addWidget(m_startPage);
    m_pages->addWidget(m_helpPage);
    m_pages->addWidget(m_levelPage);
    m_pages->addWidget(m_editorPage);
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
    QPushButton *editor = makeMenuButton(QStringLiteral("简单关卡编辑器"), page);
    QPushButton *help = makeMenuButton(QStringLiteral("玩法与评分点"), page);
    QPushButton *quit = makeMenuButton(QStringLiteral("退出"), page);
    quit->setObjectName(QStringLiteral("dangerButton"));
    start->setMaximumWidth(360);
    editor->setMaximumWidth(360);
    help->setMaximumWidth(360);
    quit->setMaximumWidth(360);
    layout->addWidget(start, 0, Qt::AlignHCenter);
    layout->addWidget(editor, 0, Qt::AlignHCenter);
    layout->addWidget(help, 0, Qt::AlignHCenter);
    layout->addWidget(quit, 0, Qt::AlignHCenter);
    layout->addStretch();

    connect(start, &QPushButton::clicked, this, &MainWindow::showLevelPage);
    connect(editor, &QPushButton::clicked, this, &MainWindow::showEditorPage);
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
    QPushButton *editor = makeMenuButton(QStringLiteral("打开简单关卡编辑器"), page);
    editor->setMaximumWidth(300);
    layout->addWidget(editor, 0, Qt::AlignLeft);
    layout->addStretch();

    QPushButton *back = makeMenuButton(QStringLiteral("返回主菜单"), page);
    back->setMaximumWidth(240);
    layout->addWidget(back, 0, Qt::AlignLeft);
    connect(back, &QPushButton::clicked, this, [this] { m_pages->setCurrentWidget(m_startPage); });
    connect(editor, &QPushButton::clicked, this, &MainWindow::showEditorPage);
    return page;
}

QWidget *MainWindow::createEditorPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *root = new QVBoxLayout(page);
    root->setContentsMargins(28, 22, 28, 22);
    root->setSpacing(10);

    QLabel *title = new QLabel(QStringLiteral("简单关卡编辑器 · 6×10"), page);
    title->setObjectName(QStringLiteral("sectionTitle"));
    root->addWidget(title);

    QLabel *guide = new QLabel(
        QStringLiteral("坐标格式为（列, 行）。选择工具后点击格子；道路按敌人行进顺序添加，第一格是入口、最后一格是出口。"),
        page);
    guide->setWordWrap(true);
    root->addWidget(guide);

    QHBoxLayout *body = new QHBoxLayout;
    body->setSpacing(18);
    QGridLayout *grid = new QGridLayout;
    grid->setSpacing(3);
    for (int row = 0; row < 6; ++row) {
        for (int column = 0; column < 10; ++column) {
            QPushButton *cell = new QPushButton(page);
            cell->setFixedSize(68, 68);
            cell->setIconSize({42, 42});
            cell->setToolTip(QStringLiteral("坐标（%1, %2）").arg(column).arg(row));
            grid->addWidget(cell, row, column);
            m_editorCells.append(cell);
            connect(cell, &QPushButton::clicked, this, [this, row, column] {
                handleEditorCell(row, column);
            });
        }
    }
    body->addLayout(grid, 1);

    QFrame *panel = new QFrame(page);
    panel->setObjectName(QStringLiteral("panel"));
    panel->setFixedWidth(330);
    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(14, 14, 14, 14);
    panelLayout->setSpacing(7);
    QLabel *toolTitle = new QLabel(QStringLiteral("格子工具"), panel);
    toolTitle->setObjectName(QStringLiteral("sectionTitle"));
    panelLayout->addWidget(toolTitle);

    const QVector<QString> toolNames = {
        QStringLiteral("草地"), QStringLiteral("黑土地"), QStringLiteral("石地"),
        QStringLiteral("冰面"), QStringLiteral("道路（按顺序）"), QStringLiteral("传送门（依次两格）")
    };
    const QVector<QString> toolIcons = {
        QStringLiteral(":/assets/tiles/grass.png"), QStringLiteral(":/assets/tiles/dark_soil.png"),
        QStringLiteral(":/assets/tiles/stone.png"), QStringLiteral(":/assets/tiles/ice.png"),
        QStringLiteral(":/assets/tiles/path.png"), QStringLiteral(":/assets/tiles/portal.png")
    };
    QGridLayout *tools = new QGridLayout;
    for (int index = 0; index < toolNames.size(); ++index) {
        QPushButton *button = new QPushButton(toolNames.at(index), panel);
        button->setCheckable(true);
        button->setIcon(QIcon(toolIcons.at(index)));
        button->setIconSize({32, 32});
        button->setMinimumHeight(48);
        tools->addWidget(button, index / 2, index % 2);
        m_editorToolButtons.append(button);
        connect(button, &QPushButton::clicked, this, [this, index] {
            selectEditorTool(static_cast<EditorTool>(index));
        });
    }
    panelLayout->addLayout(tools);

    QHBoxLayout *editActions = new QHBoxLayout;
    QPushButton *clearPath = new QPushButton(QStringLiteral("清空道路"), panel);
    QPushButton *reset = new QPushButton(QStringLiteral("全部重置"), panel);
    editActions->addWidget(clearPath);
    editActions->addWidget(reset);
    panelLayout->addLayout(editActions);

    QLabel *hint = new QLabel(
        QStringLiteral("传送跳跃：把一对传送门都加入道路，并让它们在道路顺序中前后相接；这一步允许坐标不相邻。"),
        panel);
    hint->setWordWrap(true);
    panelLayout->addWidget(hint);

    QPushButton *save = new QPushButton(QStringLiteral("保存 JSON"), panel);
    QPushButton *load = new QPushButton(QStringLiteral("加载 JSON"), panel);
    QPushButton *test = makeMenuButton(QStringLiteral("测试游玩"), panel);
    panelLayout->addWidget(save);
    panelLayout->addWidget(load);
    panelLayout->addWidget(test);
    panelLayout->addStretch();

    QPushButton *back = new QPushButton(QStringLiteral("返回主菜单"), panel);
    back->setObjectName(QStringLiteral("dangerButton"));
    panelLayout->addWidget(back);
    body->addWidget(panel);
    root->addLayout(body, 1);

    m_editorStatusLabel = new QLabel(page);
    m_editorStatusLabel->setObjectName(QStringLiteral("messageLabel"));
    m_editorStatusLabel->setAlignment(Qt::AlignCenter);
    m_editorStatusLabel->setWordWrap(true);
    root->addWidget(m_editorStatusLabel);

    connect(clearPath, &QPushButton::clicked, this, &MainWindow::clearEditorPath);
    connect(reset, &QPushButton::clicked, this, &MainWindow::resetEditorLevel);
    connect(save, &QPushButton::clicked, this, &MainWindow::saveEditorLevel);
    connect(load, &QPushButton::clicked, this, &MainWindow::loadEditorLevel);
    connect(test, &QPushButton::clicked, this, &MainWindow::startEditorTest);
    connect(back, &QPushButton::clicked, this, [this] {
        m_controller->stopGame();
        m_pages->setCurrentWidget(m_startPage);
    });

    selectEditorTool(EditorTool::Grass);
    refreshEditorGrid();
    setEditorStatus(QStringLiteral("请选择工具并编辑地图。保存和测试前会自动检查路径与传送门。"));
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
    connect(menu, &QPushButton::clicked, this, [this] {
        if (m_controller->isTestLevel()) showEditorPage();
        else showLevelPage();
    });
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
        if (m_controller->isTestLevel()) startEditorTest();
        else openLevel(m_controller->currentLevelIndex());
    });
    connect(levels, &QPushButton::clicked, this, [this] {
        if (m_controller->isTestLevel()) showEditorPage();
        else showLevelPage();
    });
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
        m_nextLevelButton->setVisible(!m_controller->isTestLevel()
                                      && won && next < m_controller->levelCount());
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

void MainWindow::showEditorPage()
{
    m_controller->stopGame();
    refreshEditorGrid();
    m_pages->setCurrentWidget(m_editorPage);
}

void MainWindow::selectEditorTool(EditorTool tool)
{
    m_editorTool = tool;
    const int selectedIndex = static_cast<int>(tool);
    for (int index = 0; index < m_editorToolButtons.size(); ++index) {
        QPushButton *button = m_editorToolButtons.at(index);
        button->setChecked(index == selectedIndex);
        button->setStyleSheet(index == selectedIndex
            ? QStringLiteral("QPushButton { border: 3px solid #f3c969; font-weight: bold; }")
            : QString());
    }
}

void MainWindow::handleEditorCell(int row, int column)
{
    const QPoint cell(column, row);
    if (!m_editorLevel.contains(cell)) return;

    if (m_editorTool == EditorTool::Path) {
        const int existingIndex = m_editorLevel.path.indexOf(cell);
        if (existingIndex >= 0 && existingIndex == m_editorLevel.path.size() - 1) {
            m_editorLevel.path.removeLast();
            setEditorStatus(QStringLiteral("已撤销最后一个道路格，当前道路共 %1 格。")
                                .arg(m_editorLevel.path.size()));
        } else if (existingIndex >= 0) {
            setEditorStatus(QStringLiteral("坐标（%1, %2）已在道路中；只能点击道路末格进行撤销。")
                                .arg(column).arg(row), true);
        } else {
            m_editorLevel.path.append(cell);
            setEditorStatus(QStringLiteral("已添加道路第 %1 格：坐标（%2, %3）。")
                                .arg(m_editorLevel.path.size()).arg(column).arg(row));
        }
        refreshEditorGrid();
        return;
    }

    if (m_editorTool == EditorTool::Portal) {
        if (m_editorPortalFirst.x() >= 0 && m_editorPortalSecond.x() >= 0) {
            clearEditorPortals();
        }
        if (m_editorPortalFirst.x() < 0) {
            m_editorPortalFirst = cell;
            m_editorLevel.terrain[m_editorLevel.flatIndex(cell)] = TerrainType::Portal;
            setEditorStatus(QStringLiteral("已设置传送门 A（%1, %2），请再点击另一格设置传送门 B。")
                                .arg(column).arg(row));
        } else if (cell == m_editorPortalFirst) {
            setEditorStatus(QStringLiteral("传送门 B 不能与 A 位于同一格。"), true);
        } else {
            m_editorPortalSecond = cell;
            m_editorLevel.terrain[m_editorLevel.flatIndex(cell)] = TerrainType::Portal;
            const int first = m_editorLevel.flatIndex(m_editorPortalFirst);
            const int second = m_editorLevel.flatIndex(m_editorPortalSecond);
            m_editorLevel.portalPairs.clear();
            m_editorLevel.portalPairs.insert(first, second);
            m_editorLevel.portalPairs.insert(second, first);
            setEditorStatus(QStringLiteral("传送门已配对：A（%1, %2）↔ B（%3, %4）。")
                                .arg(m_editorPortalFirst.x()).arg(m_editorPortalFirst.y())
                                .arg(column).arg(row));
        }
        refreshEditorGrid();
        return;
    }

    if (m_editorLevel.terrainAt(cell) == TerrainType::Portal) clearEditorPortals();
    TerrainType terrain = TerrainType::Grass;
    switch (m_editorTool) {
    case EditorTool::DarkSoil: terrain = TerrainType::DarkSoil; break;
    case EditorTool::Stone: terrain = TerrainType::Stone; break;
    case EditorTool::Ice: terrain = TerrainType::Ice; break;
    case EditorTool::Grass:
    case EditorTool::Path:
    case EditorTool::Portal: terrain = TerrainType::Grass; break;
    }
    m_editorLevel.terrain[m_editorLevel.flatIndex(cell)] = terrain;
    setEditorStatus(QStringLiteral("已更新坐标（%1, %2）的地形。道路顺序不受影响。")
                        .arg(column).arg(row));
    refreshEditorGrid();
}

void MainWindow::refreshEditorGrid()
{
    for (int row = 0; row < m_editorLevel.rows; ++row) {
        for (int column = 0; column < m_editorLevel.columns; ++column) {
            const QPoint cell(column, row);
            QPushButton *button = m_editorCells.value(m_editorLevel.flatIndex(cell));
            if (!button) continue;
            const TerrainType terrain = m_editorLevel.terrainAt(cell);
            const int pathIndex = m_editorLevel.path.indexOf(cell);

            QString iconPath;
            QString terrainName;
            switch (terrain) {
            case TerrainType::Grass:
                iconPath = QStringLiteral(":/assets/tiles/grass.png");
                terrainName = QStringLiteral("草地");
                break;
            case TerrainType::DarkSoil:
                iconPath = QStringLiteral(":/assets/tiles/dark_soil.png");
                terrainName = QStringLiteral("黑土地");
                break;
            case TerrainType::Stone:
                iconPath = QStringLiteral(":/assets/tiles/stone.png");
                terrainName = QStringLiteral("石地");
                break;
            case TerrainType::Ice:
                iconPath = QStringLiteral(":/assets/tiles/ice.png");
                terrainName = QStringLiteral("冰面");
                break;
            case TerrainType::Portal:
                iconPath = QStringLiteral(":/assets/tiles/portal.png");
                terrainName = QStringLiteral("传送门");
                break;
            }
            if (pathIndex >= 0 && terrain != TerrainType::Portal) {
                iconPath = QStringLiteral(":/assets/tiles/path.png");
            }
            button->setIcon(QIcon(iconPath));

            QString text;
            if (pathIndex >= 0) {
                if (m_editorLevel.path.size() == 1) text = QStringLiteral("入口/出口");
                else if (pathIndex == 0) text = QStringLiteral("入口 1");
                else if (pathIndex == m_editorLevel.path.size() - 1) {
                    text = QStringLiteral("出口 %1").arg(pathIndex + 1);
                } else {
                    text = QString::number(pathIndex + 1);
                }
            } else if (cell == m_editorPortalFirst) {
                text = QStringLiteral("A");
            } else if (cell == m_editorPortalSecond) {
                text = QStringLiteral("B");
            }
            button->setText(text);
            button->setToolTip(QStringLiteral("坐标（%1, %2） · %3%4")
                                   .arg(column).arg(row).arg(terrainName)
                                   .arg(pathIndex >= 0
                                       ? QStringLiteral(" · 道路第 %1 格").arg(pathIndex + 1)
                                       : QString()));
            const QString border = pathIndex >= 0
                ? QStringLiteral("3px solid #f3c969")
                : QStringLiteral("1px solid #637083");
            button->setStyleSheet(QStringLiteral(
                "QPushButton { background-color: #273444; border: %1; color: white; "
                "font-size: 11px; font-weight: bold; padding: 2px; }").arg(border));
        }
    }
}

void MainWindow::clearEditorPath()
{
    m_editorLevel.path.clear();
    refreshEditorGrid();
    setEditorStatus(QStringLiteral("道路已清空，请从入口开始重新按顺序点击。"));
}

void MainWindow::resetEditorLevel()
{
    m_editorLevel = LevelConfigLoader::createEditorTemplate();
    m_editorPortalFirst = {-1, -1};
    m_editorPortalSecond = {-1, -1};
    selectEditorTool(EditorTool::Grass);
    refreshEditorGrid();
    setEditorStatus(QStringLiteral("编辑器已重置为 6×10 全草地空白地图。"));
}

void MainWindow::clearEditorPortals()
{
    for (int index = 0; index < m_editorLevel.terrain.size(); ++index) {
        if (m_editorLevel.terrain.at(index) == TerrainType::Portal) {
            m_editorLevel.terrain[index] = TerrainType::Grass;
        }
    }
    m_editorLevel.portalPairs.clear();
    m_editorPortalFirst = {-1, -1};
    m_editorPortalSecond = {-1, -1};
}

void MainWindow::syncEditorPortalPoints()
{
    m_editorPortalFirst = {-1, -1};
    m_editorPortalSecond = {-1, -1};
    for (auto iterator = m_editorLevel.portalPairs.cbegin();
         iterator != m_editorLevel.portalPairs.cend(); ++iterator) {
        if (iterator.key() >= iterator.value()) continue;
        m_editorPortalFirst = QPoint(iterator.key() % m_editorLevel.columns,
                                     iterator.key() / m_editorLevel.columns);
        m_editorPortalSecond = QPoint(iterator.value() % m_editorLevel.columns,
                                      iterator.value() / m_editorLevel.columns);
        break;
    }
}

void MainWindow::saveEditorLevel()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("保存编辑器关卡"), QStringLiteral("editor_level.json"),
        QStringLiteral("JSON 文件 (*.json)"));
    if (filePath.isEmpty()) return;
    if (!filePath.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
        filePath += QStringLiteral(".json");
    }

    QString errorMessage;
    if (!LevelConfigLoader::saveToFile(filePath, m_editorLevel, &errorMessage)) {
        setEditorStatus(QStringLiteral("保存失败：%1").arg(errorMessage), true);
        QMessageBox::critical(this, QStringLiteral("保存失败"), errorMessage);
        return;
    }
    setEditorStatus(QStringLiteral("保存成功：%1").arg(filePath));
}

void MainWindow::loadEditorLevel()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("加载编辑器关卡"), QString(), QStringLiteral("JSON 文件 (*.json)"));
    if (filePath.isEmpty()) return;

    QString errorMessage;
    const QVector<LevelConfig> levels = LevelConfigLoader::loadFromFile(filePath, &errorMessage);
    if (levels.isEmpty()) {
        setEditorStatus(QStringLiteral("加载失败：%1").arg(errorMessage), true);
        QMessageBox::critical(this, QStringLiteral("加载失败"), errorMessage);
        return;
    }
    if (!LevelConfigLoader::validateEditorLevel(levels.first(), &errorMessage)) {
        setEditorStatus(QStringLiteral("加载失败：%1").arg(errorMessage), true);
        QMessageBox::critical(this, QStringLiteral("加载失败"), errorMessage);
        return;
    }

    m_editorLevel = levels.first();
    syncEditorPortalPoints();
    refreshEditorGrid();
    setEditorStatus(QStringLiteral("已加载：%1").arg(filePath));
}

void MainWindow::startEditorTest()
{
    QString errorMessage;
    if (!LevelConfigLoader::validateEditorLevel(m_editorLevel, &errorMessage)) {
        setEditorStatus(QStringLiteral("无法测试：%1").arg(errorMessage), true);
        QMessageBox::critical(this, QStringLiteral("地图检查失败"), errorMessage);
        return;
    }

    m_controller->startTestLevel(m_editorLevel);
    m_pages->setCurrentWidget(m_gamePage);
    QTimer::singleShot(0, this, [this] {
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    });
}

void MainWindow::setEditorStatus(const QString &message, bool error)
{
    if (!m_editorStatusLabel) return;
    m_editorStatusLabel->setText(message);
    m_editorStatusLabel->setStyleSheet(error
        ? QStringLiteral("color: #ff8b8b; font-weight: bold;")
        : QStringLiteral("color: #dce9f7;"));
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
