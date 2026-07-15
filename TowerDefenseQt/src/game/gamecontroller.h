#pragma once

#include "entities.h"
#include "levelconfig.h"
#include "spritemanager.h"

#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QObject>
#include <QPoint>
#include <QSet>
#include <QSettings>
#include <QTimer>

#include <memory>
#include <functional>
#include <vector>

class GameController final : public QObject {
public:
    explicit GameController(QGraphicsScene *scene, QObject *parent = nullptr);

    bool initialize(QString *errorMessage = nullptr);
    int levelCount() const { return m_levels.size(); }
    QString levelName(int index) const;
    QString levelDescription(int index) const;
    QString levelProgressText(int index) const;

    void startLevel(int index);
    void restartLevel();
    void stopGame();
    void togglePause();
    void selectTowerType(TowerType type);
    void handleSceneClick(const QPointF &scenePosition);
    void handleSceneDoubleClick(const QPointF &scenePosition);
    void upgradeSelectedTower();
    void activateSelectedSkill();
    void applyCheatCode(const QString &code);

    int currentLevelIndex() const { return m_currentLevelIndex; }
    int cellSize() const { return kCellSize; }
    GameState state() const { return m_state; }
    TowerType selectedTowerType() const { return m_selectedTowerType; }

    Enemy *acquireTarget(const Tower &tower);
    void launchProjectile(const Tower &tower, Enemy &target, AttackKind kind, double damage,
                          double splashRadiusPixels, EffectType effect,
                          double effectDuration, double effectMagnitude);
    void addResources(int amount, bool announce = false);
    void queueMinions(const QPointF &position, int pathIndex, int count);
    void showMessage(const QString &message);

    std::function<void(const QString &)> hudChanged = [](const QString &) {};
    std::function<void(const QString &)> messageChanged = [](const QString &) {};
    std::function<void(GameState)> stateChanged = [](GameState) {};
    std::function<void(bool, const QString &)> resultReady = [](bool, const QString &) {};
    std::function<void(const QString &)> towerInfoChanged = [](const QString &) {};
    std::function<void(TowerType)> selectedTowerTypeChanged = [](TowerType) {};

private:
    void onTick();
    struct PendingSpawn {
        EnemyType type = EnemyType::Minion;
        QPointF position;
        int pathIndex = 1;
    };

    static constexpr int kCellSize = 64;

    void clearSceneAndObjects();
    void buildMap();
    void updateGame(double deltaSeconds);
    void updateWave(double deltaSeconds);
    void updateTowers(double deltaSeconds);
    void updateEnemies(double deltaSeconds);
    void updateProjectiles(double deltaSeconds);
    void processDeadEnemies();
    void removeDeadTowers();
    void flushPendingSpawns();

    Enemy *spawnEnemy(EnemyType type);
    Enemy *spawnEnemyAt(EnemyType type, const QPointF &position, int pathIndex);
    void createEnemyVisual(Enemy &enemy);
    void createTowerVisual(Tower &tower);
    void createProjectileVisual(Projectile &projectile);
    void updateEnemyVisual(Enemy &enemy);
    void updateTowerVisual(Tower &tower);
    void removeVisual(SpriteVisual &visual);

    QPoint cellFromScenePosition(const QPointF &scenePosition) const;
    QPointF cellCenter(const QPoint &cell) const;
    bool isPathCell(const QPoint &cell) const;
    int pathIndexOfCell(const QPoint &cell) const;
    Tower *towerAt(const QPoint &cell) const;
    Tower *selectedTower() const;
    Tower *blockingTowerFor(const Enemy &enemy) const;
    Enemy *enemyById(int id) const;
    int effectiveBuildCost(TowerType type, const QPoint &cell) const;
    bool tryBuildTower(const QPoint &cell);

    void moveEnemy(Enemy &enemy, double deltaSeconds);
    void handlePortal(Enemy &enemy, const QPoint &cell);
    void impactProjectile(Projectile &projectile, Enemy &target);
    void applyLaserImpact(const Projectile &projectile, Enemy &target);
    void setState(GameState state);
    void finishGame(bool won);
    void saveResult(bool won);
    void emitHud();
    void emitSelectedTowerInfo();

    QGraphicsScene *m_scene = nullptr;
    SpriteManager m_sprites;
    QVector<LevelConfig> m_levels;
    const LevelConfig *m_level = nullptr;
    int m_currentLevelIndex = -1;
    QSet<int> m_pathCells;

    std::vector<std::unique_ptr<Tower>> m_towers;
    std::vector<std::unique_ptr<Enemy>> m_enemies;
    std::vector<std::unique_ptr<Projectile>> m_projectiles;
    QVector<PendingSpawn> m_pendingSpawns;

    QTimer m_timer;
    QElapsedTimer m_elapsed;
    GameState m_state = GameState::Idle;
    TowerType m_selectedTowerType = TowerType::Shooter;
    QPoint m_selectedCell{-1, -1};
    QGraphicsRectItem *m_selectionIndicator = nullptr;

    int m_resources = 0;
    int m_score = 0;
    int m_nextEnemyId = 1;
    int m_waveIndex = 0;
    int m_spawnIndex = 0;
    double m_spawnTimer = 1.5;
    double m_resourceTimer = 0.0;
    double m_elapsedGameSeconds = 0.0;
    double m_activeSkillCooldown = 0.0;
    double m_hudTimer = 0.0;
    double m_fpsTimer = 0.0;
    int m_frameCounter = 0;
    int m_fps = 60;
    int m_destroyedEnemies = 0;

    QSettings m_settings;
};
