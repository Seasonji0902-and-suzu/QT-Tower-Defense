#include "gamecontroller.h"

#include <QBrush>
#include <QCoreApplication>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QLineF>
#include <QPen>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

double distanceBetween(const QPointF &first, const QPointF &second)
{
    return QLineF(first, second).length();
}

QSize enemySpriteSize(EnemyType type)
{
    switch (type) {
    case EnemyType::Boss: return {82, 82};
    case EnemyType::Heavy: return {62, 62};
    case EnemyType::Split: return {58, 58};
    case EnemyType::Minion: return {34, 34};
    default: return {50, 50};
    }
}

double enemyHealthBarWidth(EnemyType type)
{
    return std::max(36, enemySpriteSize(type).width());
}

double pathMarkerRotation(const QPoint &direction)
{
    if (qAbs(direction.x()) >= qAbs(direction.y())) {
        return direction.x() >= 0 ? 90.0 : 270.0;
    }
    return direction.y() >= 0 ? 180.0 : 0.0;
}

} // namespace

GameController::GameController(QGraphicsScene *scene, QObject *parent)
    : QObject(parent)
    , m_scene(scene)
    , m_settings(QStringLiteral("ProgrammingPractice2026"), QStringLiteral("GridGuard"))
{
    m_timer.setTimerType(Qt::PreciseTimer);
    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, &GameController::onTick);
    m_elapsed.start();
    m_timer.start();
}

bool GameController::initialize(QString *errorMessage)
{
    m_levels = LevelConfigLoader::load(errorMessage);
    return !m_levels.isEmpty();
}

QString GameController::levelName(int index) const
{
    return index >= 0 && index < m_levels.size() ? m_levels.at(index).name : QString();
}

QString GameController::levelDescription(int index) const
{
    return index >= 0 && index < m_levels.size() ? m_levels.at(index).description : QString();
}

QString GameController::levelProgressText(int index) const
{
    if (index < 0 || index >= m_levels.size()) return {};
    const QString key = QStringLiteral("levels/%1/").arg(m_levels.at(index).id);
    const bool completed = m_settings.value(key + QStringLiteral("completed"), false).toBool();
    if (!completed) return QStringLiteral("尚未通关");
    const double bestTime = m_settings.value(key + QStringLiteral("bestTime"), 0.0).toDouble();
    const int bestScore = m_settings.value(key + QStringLiteral("bestScore"), 0).toInt();
    return QStringLiteral("已通关 · 最佳 %1 秒 · %2 分")
        .arg(bestTime, 0, 'f', 1)
        .arg(bestScore);
}

void GameController::startLevel(int index)
{
    if (index < 0 || index >= m_levels.size()) return;

    beginLevel(&m_levels[index], index, false);
}

void GameController::startTestLevel(const LevelConfig &level)
{
    m_testLevel = level;
    beginLevel(&m_testLevel, -1, true);
}

void GameController::beginLevel(const LevelConfig *level, int levelIndex, bool testLevel)
{
    if (!level) return;

    clearSceneAndObjects();
    m_currentLevelIndex = levelIndex;
    m_isTestLevel = testLevel;
    m_level = level;
    m_resources = m_level->initialResources;
    m_score = 0;
    m_nextEnemyId = 1;
    m_waveIndex = 0;
    m_spawnIndex = 0;
    m_spawnTimer = 1.8;
    m_resourceTimer = 0.0;
    m_elapsedGameSeconds = 0.0;
    m_activeSkillCooldown = 0.0;
    m_destroyedEnemies = 0;
    m_selectedCell = {-1, -1};
    m_pathCells.clear();
    for (const QPoint &cell : m_level->path) {
        m_pathCells.insert(m_level->flatIndex(cell));
    }

    buildMap();
    setState(GameState::Running);
    showMessage(QStringLiteral("关卡开始：选择右侧防御塔，再点击合法格子建造。"));
    emitHud();
    emitSelectedTowerInfo();
}

void GameController::restartLevel()
{
    if (m_isTestLevel) {
        beginLevel(&m_testLevel, -1, true);
    } else if (m_currentLevelIndex >= 0) {
        startLevel(m_currentLevelIndex);
    }
}

void GameController::stopGame()
{
    clearSceneAndObjects();
    m_level = nullptr;
    m_currentLevelIndex = -1;
    m_isTestLevel = false;
    setState(GameState::Idle);
}

void GameController::togglePause()
{
    if (m_state == GameState::Running) {
        setState(GameState::Paused);
        showMessage(QStringLiteral("游戏已暂停。"));
    } else if (m_state == GameState::Paused) {
        setState(GameState::Running);
        showMessage(QStringLiteral("游戏继续。"));
    }
}

void GameController::selectTowerType(TowerType type)
{
    m_selectedTowerType = type;
    emit selectedTowerTypeChanged(type);
    showMessage(QStringLiteral("已选择%1，基础费用 %2。").arg(towerDisplayName(type)).arg(towerBaseCost(type)));
}

void GameController::handleSceneClick(const QPointF &scenePosition)
{
    if (m_state != GameState::Running && m_state != GameState::Paused) return;
    const QPoint cell = cellFromScenePosition(scenePosition);
    if (!m_level || !m_level->contains(cell)) return;

    if (Tower *existing = towerAt(cell)) {
        m_selectedCell = cell;
        if (!m_selectionIndicator) {
            m_selectionIndicator = m_scene->addRect(0, 0, kCellSize - 6, kCellSize - 6,
                QPen(QColor(QStringLiteral("#ffe27a")), 3), Qt::NoBrush);
            m_selectionIndicator->setZValue(8.0);
        }
        m_selectionIndicator->setPos(cell.x() * kCellSize + 3, cell.y() * kCellSize + 3);
        m_selectionIndicator->show();
        showMessage(QStringLiteral("已选中%1（等级 %2）。双击格子或点击“升级”。")
                        .arg(towerDisplayName(existing->type())).arg(existing->level()));
        emitSelectedTowerInfo();
        return;
    }

    if (tryBuildTower(cell)) {
        m_selectedCell = cell;
        if (m_selectionIndicator) {
            m_selectionIndicator->setPos(cell.x() * kCellSize + 3, cell.y() * kCellSize + 3);
            m_selectionIndicator->show();
        }
        emitSelectedTowerInfo();
    }
}

void GameController::handleSceneDoubleClick(const QPointF &scenePosition)
{
    const QPoint cell = cellFromScenePosition(scenePosition);
    if (towerAt(cell)) {
        m_selectedCell = cell;
        upgradeSelectedTower();
    }
}

void GameController::upgradeSelectedTower()
{
    Tower *tower = selectedTower();
    if (!tower) {
        showMessage(QStringLiteral("请先点击一座已建造的防御塔。"));
        return;
    }
    const int cost = tower->upgradeCost();
    if (cost <= 0) {
        showMessage(QStringLiteral("该防御塔已经达到最高等级。"));
        return;
    }
    if (m_resources < cost) {
        showMessage(QStringLiteral("资源不足：升级需要 %1。").arg(cost));
        return;
    }
    m_resources -= cost;
    tower->upgrade();
    updateTowerVisual(*tower);
    showMessage(QStringLiteral("%1 已升级到等级 %2！")
                    .arg(towerDisplayName(tower->type())).arg(tower->level()));
    emitSelectedTowerInfo();
    emitHud();
}

void GameController::activateSelectedSkill()
{
    Tower *tower = selectedTower();
    if (!tower || tower->type() != TowerType::Slow) {
        showMessage(QStringLiteral("主动技能属于减速塔，请先选择一座减速塔。"));
        return;
    }
    if (m_activeSkillCooldown > 0.0) {
        showMessage(QStringLiteral("全体冻结仍在冷却：%1 秒。").arg(qCeil(m_activeSkillCooldown)));
        return;
    }
    for (const std::unique_ptr<Enemy> &enemy : m_enemies) {
        enemy->applyEffect(EffectType::Slow, 4.0, 0.60);
        enemy->applyEffect(EffectType::Stun, 0.65, 1.0);
    }
    m_activeSkillCooldown = 22.0;
    showMessage(QStringLiteral("主动技能：全体冻结！"));
    emitSelectedTowerInfo();
}

void GameController::applyCheatCode(const QString &code)
{
    const QString normalized = code.trimmed().toUpper();
    if (normalized == QStringLiteral("MONEY1000")) {
        addResources(1000, false);
        showMessage(QStringLiteral("作弊码生效：资源 +1000。"));
    } else if (normalized == QStringLiteral("CLEARWAVE")) {
        for (const std::unique_ptr<Enemy> &enemy : m_enemies) {
            enemy->applyDamage(99999.0, AttackKind::Normal);
        }
        showMessage(QStringLiteral("作弊码生效：清除当前敌人。"));
    } else {
        showMessage(QStringLiteral("未知作弊码。可用：MONEY1000、CLEARWAVE"));
    }
}

Enemy *GameController::acquireTarget(const Tower &tower)
{
    Enemy *best = nullptr;
    int bestPathIndex = -1;
    double bestDistance = std::numeric_limits<double>::max();
    const double rangePixels = tower.rangeCells() * kCellSize;
    for (const std::unique_ptr<Enemy> &enemy : m_enemies) {
        if (enemy->isDead()) continue;
        const double distance = distanceBetween(tower.scenePosition(), enemy->position());
        if (distance > rangePixels) continue;
        if (enemy->pathIndex() > bestPathIndex
            || (enemy->pathIndex() == bestPathIndex && distance < bestDistance)) {
            best = enemy.get();
            bestPathIndex = enemy->pathIndex();
            bestDistance = distance;
        }
    }
    return best;
}

void GameController::launchProjectile(const Tower &tower, Enemy &target, AttackKind kind,
                                      double damage, double splashRadiusPixels, EffectType effect,
                                      double effectDuration, double effectMagnitude)
{
    double speed = 420.0;
    if (kind == AttackKind::Splash) speed = 285.0;
    if (kind == AttackKind::Laser) speed = 850.0;
    auto projectile = std::make_unique<Projectile>(kind, target.id(), tower.scenePosition(), speed,
                                                   damage, splashRadiusPixels, effect,
                                                   effectDuration, effectMagnitude);
    createProjectileVisual(*projectile);
    m_projectiles.push_back(std::move(projectile));
}

void GameController::addResources(int amount, bool announce)
{
    m_resources += amount;
    if (announce) showMessage(QStringLiteral("资源塔产出 +%1。").arg(amount));
    emitHud();
}

void GameController::queueMinions(const QPointF &position, int pathIndex, int count)
{
    for (int i = 0; i < count; ++i) {
        m_pendingSpawns.append({EnemyType::Minion,
                                position + QPointF((i - count / 2.0) * 8.0, i * 3.0),
                                pathIndex});
    }
}

void GameController::showMessage(const QString &message)
{
    emit messageChanged(message);
}

void GameController::onTick()
{
    const double deltaSeconds = std::clamp(m_elapsed.restart() / 1000.0, 0.001, 0.050);
    ++m_frameCounter;
    m_fpsTimer += deltaSeconds;
    if (m_fpsTimer >= 1.0) {
        m_fps = qRound(m_frameCounter / m_fpsTimer);
        m_frameCounter = 0;
        m_fpsTimer = 0.0;
    }

    if (m_state == GameState::Running) updateGame(deltaSeconds);

    m_hudTimer += deltaSeconds;
    if (m_hudTimer >= 0.12) {
        emitHud();
        m_hudTimer = 0.0;
    }
}

void GameController::clearSceneAndObjects()
{
    m_towers.clear();
    m_enemies.clear();
    m_projectiles.clear();
    m_pendingSpawns.clear();
    m_selectionIndicator = nullptr;
    if (m_scene) m_scene->clear();
}

void GameController::buildMap()
{
    if (!m_scene || !m_level) return;
    m_scene->setSceneRect(0, 0, m_level->columns * kCellSize, m_level->rows * kCellSize);
    for (int row = 0; row < m_level->rows; ++row) {
        for (int column = 0; column < m_level->columns; ++column) {
            const QPoint cell(column, row);
            const TerrainType terrain = m_level->terrainAt(cell);
            QPixmap pixmap;
            if (isPathCell(cell) && terrain != TerrainType::Ice && terrain != TerrainType::Portal) {
                pixmap = m_sprites.path({kCellSize, kCellSize});
            } else {
                pixmap = m_sprites.terrain(terrain, {kCellSize, kCellSize});
            }
            QGraphicsPixmapItem *item = m_scene->addPixmap(pixmap);
            item->setPos(column * kCellSize, row * kCellSize);
            item->setZValue(0.0);
            item->setToolTip(QStringLiteral("(%1, %2)").arg(column).arg(row));

            if (isPathCell(cell)) {
                QGraphicsPixmapItem *marker =
                    m_scene->addPixmap(m_sprites.pathMarker({kCellSize, kCellSize}));
                marker->setPos(column * kCellSize, row * kCellSize);
                marker->setZValue(0.8);
                marker->setOpacity(terrain == TerrainType::Portal ? 0.28 : 0.48);
                marker->setTransformOriginPoint(kCellSize / 2.0, kCellSize / 2.0);

                const int pathIndex = pathIndexOfCell(cell);
                QPoint direction(0, -1);
                if (pathIndex >= 0 && pathIndex + 1 < m_level->path.size()) {
                    direction = m_level->path.at(pathIndex + 1) - cell;
                } else if (pathIndex > 0) {
                    direction = cell - m_level->path.at(pathIndex - 1);
                }
                marker->setRotation(pathMarkerRotation(direction));
            }
        }
    }
}

void GameController::updateGame(double deltaSeconds)
{
    m_elapsedGameSeconds += deltaSeconds;
    m_activeSkillCooldown = std::max(0.0, m_activeSkillCooldown - deltaSeconds);
    m_resourceTimer += deltaSeconds;
    while (m_resourceTimer >= 1.0) {
        m_resources += 2;
        m_resourceTimer -= 1.0;
    }

    updateWave(deltaSeconds);
    updateTowers(deltaSeconds);
    updateEnemies(deltaSeconds);
    processDeadEnemies();
    flushPendingSpawns();
    updateProjectiles(deltaSeconds);
    processDeadEnemies();
    flushPendingSpawns();
    removeDeadTowers();
}

void GameController::updateWave(double deltaSeconds)
{
    if (!m_level || m_waveIndex >= m_level->waves.size()) return;
    const WaveConfig &wave = m_level->waves.at(m_waveIndex);

    if (m_spawnIndex < wave.enemies.size()) {
        m_spawnTimer -= deltaSeconds;
        if (m_spawnTimer <= 0.0) {
            spawnEnemy(wave.enemies.at(m_spawnIndex));
            ++m_spawnIndex;
            m_spawnTimer = wave.spawnIntervalMs / 1000.0;
        }
        return;
    }

    if (m_enemies.empty() && m_pendingSpawns.isEmpty()) {
        ++m_waveIndex;
        m_spawnIndex = 0;
        m_spawnTimer = 2.2;
        if (m_waveIndex >= m_level->waves.size()) {
            finishGame(true);
        } else {
            showMessage(QStringLiteral("第 %1 波已完成，下一波即将到来。")
                            .arg(m_waveIndex));
        }
    }
}

void GameController::updateTowers(double deltaSeconds)
{
    for (const std::unique_ptr<Tower> &tower : m_towers) {
        if (!tower->isDead()) tower->tick(*this, deltaSeconds);
        updateTowerVisual(*tower);
    }
}

void GameController::updateEnemies(double deltaSeconds)
{
    for (const std::unique_ptr<Enemy> &enemyPtr : m_enemies) {
        Enemy &enemy = *enemyPtr;
        if (enemy.isDead()) continue;

        const QPoint cell = cellFromScenePosition(enemy.position());
        const bool onIce = m_level && m_level->terrainAt(cell) == TerrainType::Ice;
        enemy.tickEffects(deltaSeconds, onIce);
        if (enemy.isDead()) continue;
        enemy.updateSpecial(*this, deltaSeconds);

        if (Tower *blocker = blockingTowerFor(enemy)) {
            enemy.setAttackCooldown(enemy.attackCooldown() - deltaSeconds);
            if (enemy.attackCooldown() <= 0.0) {
                const double damage = enemy.type() == EnemyType::Boss ? 90.0 : 42.0;
                blocker->takeDamage(damage);
                enemy.setAttackCooldown(0.80);
                showMessage(QStringLiteral("敌人正在攻击%1！").arg(towerDisplayName(blocker->type())));
            }
        } else if (!enemy.stunned()) {
            moveEnemy(enemy, deltaSeconds);
        }
        updateEnemyVisual(enemy);
    }
}

void GameController::updateProjectiles(double deltaSeconds)
{
    for (const std::unique_ptr<Projectile> &projectilePtr : m_projectiles) {
        Projectile &projectile = *projectilePtr;
        Enemy *target = enemyById(projectile.targetId);
        if (!target || target->isDead()) {
            projectile.expired = true;
            continue;
        }

        const QLineF line(projectile.position, target->position());
        const double distance = line.length();
        const double step = projectile.speedPixelsPerSecond * deltaSeconds;
        if (distance <= step || distance < 6.0) {
            projectile.position = target->position();
            impactProjectile(projectile, *target);
            projectile.expired = true;
        } else {
            const QPointF direction = (target->position() - projectile.position) / distance;
            projectile.position += direction * step;
        }

        if (projectile.visual) {
            projectile.visual->setPos(projectile.position);
            projectile.visual->setRotation(-line.angle() + 90.0);
        }
    }

    std::erase_if(m_projectiles, [this](const std::unique_ptr<Projectile> &projectile) {
        if (!projectile->expired) return false;
        if (projectile->visual) {
            m_scene->removeItem(projectile->visual);
            delete projectile->visual;
        }
        return true;
    });
}

void GameController::processDeadEnemies()
{
    std::erase_if(m_enemies, [this](const std::unique_ptr<Enemy> &enemy) {
        if (!enemy->isDead()) return false;
        enemy->onDeath(*this);
        m_resources += enemy->reward();
        m_score += enemy->reward() * 10;
        ++m_destroyedEnemies;
        removeVisual(enemy->visual());
        return true;
    });
}

void GameController::removeDeadTowers()
{
    std::erase_if(m_towers, [this](const std::unique_ptr<Tower> &tower) {
        if (!tower->isDead()) return false;
        if (tower->cell() == m_selectedCell) m_selectedCell = {-1, -1};
        removeVisual(tower->visual());
        showMessage(QStringLiteral("一座%1被摧毁了！").arg(towerDisplayName(tower->type())));
        return true;
    });
    emitSelectedTowerInfo();
}

void GameController::flushPendingSpawns()
{
    const QVector<PendingSpawn> pending = std::move(m_pendingSpawns);
    m_pendingSpawns.clear();
    for (const PendingSpawn &spawn : pending) {
        spawnEnemyAt(spawn.type, spawn.position, spawn.pathIndex);
    }
}

Enemy *GameController::spawnEnemy(EnemyType type)
{
    if (!m_level || m_level->path.isEmpty()) return nullptr;
    return spawnEnemyAt(type, cellCenter(m_level->path.first()), 1);
}

Enemy *GameController::spawnEnemyAt(EnemyType type, const QPointF &position, int pathIndex)
{
    auto enemy = makeEnemy(type, m_nextEnemyId++);
    if (!enemy) return nullptr;
    enemy->setPosition(position);
    const int maximumPathIndex = m_level ? static_cast<int>(m_level->path.size()) : 1;
    enemy->setPathIndex(std::clamp(pathIndex, 1, maximumPathIndex));
    createEnemyVisual(*enemy);
    Enemy *raw = enemy.get();
    m_enemies.push_back(std::move(enemy));
    return raw;
}

void GameController::createEnemyVisual(Enemy &enemy)
{
    const QSize size = enemySpriteSize(enemy.type());
    QPixmap pixmap = m_sprites.enemy(enemy.type(), size);
    enemy.visual().sprite = m_scene->addPixmap(pixmap);
    enemy.visual().sprite->setOffset(-pixmap.width() / 2.0, -pixmap.height() / 2.0);
    enemy.visual().sprite->setPos(enemy.position());
    enemy.visual().sprite->setZValue(4.0);

    const double width = enemyHealthBarWidth(enemy.type());
    enemy.visual().healthBackground = m_scene->addRect(0, 0, width, 5,
        Qt::NoPen, QBrush(QColor(QStringLiteral("#371e24"))));
    enemy.visual().healthFill = m_scene->addRect(0, 0, width, 5,
        Qt::NoPen, QBrush(QColor(QStringLiteral("#63d471"))));
    enemy.visual().shieldFill = m_scene->addRect(0, 0, width, 3,
        Qt::NoPen, QBrush(QColor(QStringLiteral("#71c7ff"))));
    enemy.visual().healthBackground->setZValue(6.0);
    enemy.visual().healthFill->setZValue(6.1);
    enemy.visual().shieldFill->setZValue(6.2);
    updateEnemyVisual(enemy);
}

void GameController::createTowerVisual(Tower &tower)
{
    QPixmap pixmap = m_sprites.tower(tower.type(), {48, 48});
    tower.visual().sprite = m_scene->addPixmap(pixmap);
    tower.visual().sprite->setOffset(-pixmap.width() / 2.0, -pixmap.height() / 2.0);
    tower.visual().sprite->setPos(tower.scenePosition());
    tower.visual().sprite->setZValue(2.0);
    tower.visual().healthBackground = m_scene->addRect(0, 0, 42, 4,
        Qt::NoPen, QBrush(QColor(QStringLiteral("#371e24"))));
    tower.visual().healthFill = m_scene->addRect(0, 0, 42, 4,
        Qt::NoPen, QBrush(QColor(QStringLiteral("#72d888"))));
    tower.visual().healthBackground->setZValue(5.0);
    tower.visual().healthFill->setZValue(5.1);
    updateTowerVisual(tower);
}

void GameController::createProjectileVisual(Projectile &projectile)
{
    const QSize size = projectile.kind == AttackKind::Splash ? QSize(22, 22) : QSize(14, 18);
    QPixmap pixmap = m_sprites.projectile(projectile.kind, size);
    projectile.visual = m_scene->addPixmap(pixmap);
    projectile.visual->setOffset(-pixmap.width() / 2.0, -pixmap.height() / 2.0);
    projectile.visual->setPos(projectile.position);
    projectile.visual->setZValue(7.0);
}

void GameController::updateEnemyVisual(Enemy &enemy)
{
    SpriteVisual &visual = enemy.visual();
    if (!visual.sprite) return;
    visual.sprite->setPos(enemy.position());
    const QSize spriteSize = enemySpriteSize(enemy.type());
    const double width = enemyHealthBarWidth(enemy.type());
    const double yOffset = spriteSize.height() / 2.0 + 9.0;
    const double healthRatio = enemy.maxHealth() > 0.0 ? enemy.health() / enemy.maxHealth() : 0.0;
    visual.healthBackground->setPos(enemy.position().x() - width / 2.0,
                                    enemy.position().y() - yOffset);
    visual.healthFill->setPos(enemy.position().x() - width / 2.0,
                              enemy.position().y() - yOffset);
    visual.healthFill->setRect(0, 0, width * std::clamp(healthRatio, 0.0, 1.0), 5);
    const double shieldRatio = enemy.maxShield() > 0.0 ? enemy.shield() / enemy.maxShield() : 0.0;
    visual.shieldFill->setPos(enemy.position().x() - width / 2.0,
                              enemy.position().y() - yOffset - 4.0);
    visual.shieldFill->setRect(0, 0, width * std::clamp(shieldRatio, 0.0, 1.0), 3);
    visual.shieldFill->setVisible(shieldRatio > 0.001);

    bool hasSlow = false;
    bool hasBurn = false;
    bool hasStun = false;
    for (const EffectInstance &effect : enemy.effects()) {
        hasSlow = hasSlow || effect.type == EffectType::Slow;
        hasBurn = hasBurn || effect.type == EffectType::Burn;
        hasStun = hasStun || effect.type == EffectType::Stun;
    }

    if (hasSlow) {
        const int auraSize = spriteSize.width() + 24;
        const QPixmap aura = m_sprites.effect(EffectType::Slow, {auraSize, auraSize});
        if (!visual.slowAura) {
            visual.slowAura = m_scene->addPixmap(aura);
            visual.slowAura->setZValue(3.5);
            visual.slowAura->setOpacity(0.62);
        } else {
            visual.slowAura->setPixmap(aura);
        }
        visual.slowAura->setOffset(-aura.width() / 2.0, -aura.height() / 2.0);
        visual.slowAura->setPos(enemy.position());
        visual.slowAura->show();
    } else if (visual.slowAura) {
        visual.slowAura->hide();
    }

    const EffectType visibleEffect = hasStun ? EffectType::Stun
        : hasBurn ? EffectType::Burn
        : hasSlow ? EffectType::Slow
                  : EffectType::None;
    if (visibleEffect == EffectType::None) {
        if (visual.effectIcon) visual.effectIcon->hide();
    } else {
        const QSize iconSize = visibleEffect == EffectType::Slow ? QSize(30, 30)
                                                                 : QSize(22, 22);
        const QPixmap icon = m_sprites.effect(visibleEffect, iconSize);
        if (!visual.effectIcon) {
            visual.effectIcon = m_scene->addPixmap(icon);
            visual.effectIcon->setZValue(6.5);
        } else {
            visual.effectIcon->setPixmap(icon);
        }
        visual.effectIcon->setOffset(-icon.width() / 2.0, -icon.height() / 2.0);
        visual.effectIcon->setPos(enemy.position().x() + width / 2.0 + 2.0,
                                  enemy.position().y() - yOffset + 2.0);
        visual.effectIcon->show();
    }
}

void GameController::updateTowerVisual(Tower &tower)
{
    SpriteVisual &visual = tower.visual();
    if (!visual.sprite) return;
    const double ratio = tower.maxHealth() > 0.0 ? tower.health() / tower.maxHealth() : 0.0;
    visual.healthBackground->setPos(tower.scenePosition().x() - 21.0,
                                    tower.scenePosition().y() - 29.0);
    visual.healthFill->setPos(tower.scenePosition().x() - 21.0,
                              tower.scenePosition().y() - 29.0);
    visual.healthFill->setRect(0, 0, 42.0 * std::clamp(ratio, 0.0, 1.0), 4);
    visual.sprite->setScale(1.0 + (tower.level() - 1) * 0.08);
}

void GameController::removeVisual(SpriteVisual &visual)
{
    auto remove = [this](QGraphicsItem *item) {
        if (!item) return;
        m_scene->removeItem(item);
        delete item;
    };
    remove(visual.sprite);
    remove(visual.healthBackground);
    remove(visual.healthFill);
    remove(visual.shieldFill);
    remove(visual.effectIcon);
    remove(visual.slowAura);
    visual = {};
}

QPoint GameController::cellFromScenePosition(const QPointF &scenePosition) const
{
    return {qFloor(scenePosition.x() / kCellSize), qFloor(scenePosition.y() / kCellSize)};
}

QPointF GameController::cellCenter(const QPoint &cell) const
{
    return {cell.x() * kCellSize + kCellSize / 2.0,
            cell.y() * kCellSize + kCellSize / 2.0};
}

bool GameController::isPathCell(const QPoint &cell) const
{
    return m_level && m_pathCells.contains(m_level->flatIndex(cell));
}

int GameController::pathIndexOfCell(const QPoint &cell) const
{
    if (!m_level) return -1;
    for (int index = 0; index < m_level->path.size(); ++index) {
        if (m_level->path.at(index) == cell) return index;
    }
    return -1;
}

Tower *GameController::towerAt(const QPoint &cell) const
{
    for (const std::unique_ptr<Tower> &tower : m_towers) {
        if (!tower->isDead() && tower->cell() == cell) return tower.get();
    }
    return nullptr;
}

Tower *GameController::selectedTower() const
{
    return towerAt(m_selectedCell);
}

Tower *GameController::blockingTowerFor(const Enemy &enemy) const
{
    Tower *nearest = nullptr;
    double nearestDistance = kCellSize * 1.12;
    for (const std::unique_ptr<Tower> &tower : m_towers) {
        if (tower->isDead()) continue;
        const double distance = distanceBetween(enemy.position(), tower->scenePosition());
        if (distance < nearestDistance) {
            nearest = tower.get();
            nearestDistance = distance;
        }
    }
    return nearest;
}

Enemy *GameController::enemyById(int id) const
{
    for (const std::unique_ptr<Enemy> &enemy : m_enemies) {
        if (enemy->id() == id) return enemy.get();
    }
    return nullptr;
}

int GameController::effectiveBuildCost(TowerType type, const QPoint &cell) const
{
    int cost = towerBaseCost(type);
    if (m_level && m_level->terrainAt(cell) == TerrainType::DarkSoil) {
        cost = qRound(cost * 0.70);
    }
    return cost;
}

bool GameController::tryBuildTower(const QPoint &cell)
{
    if (!m_level || !m_level->contains(cell)) return false;
    const TerrainType terrain = m_level->terrainAt(cell);
    const bool onPath = isPathCell(cell);

    if (towerAt(cell)) {
        showMessage(QStringLiteral("这个格子已经有防御塔。"));
        return false;
    }
    if (terrain == TerrainType::Stone) {
        showMessage(QStringLiteral("石地不能建造防御塔。"));
        return false;
    }
    if (m_selectedTowerType != TowerType::Wall && onPath) {
        showMessage(QStringLiteral("攻击塔不能建在敌人路径上。"));
        return false;
    }
    if (m_selectedTowerType == TowerType::Wall
        && terrain == TerrainType::Portal) {
        showMessage(QStringLiteral("传送门上不能放置防御墙。"));
        return false;
    }

    const int cost = effectiveBuildCost(m_selectedTowerType, cell);
    if (m_resources < cost) {
        showMessage(QStringLiteral("资源不足：需要 %1，当前只有 %2。")
                        .arg(cost).arg(m_resources));
        return false;
    }

    std::unique_ptr<Tower> tower = makeTower(m_selectedTowerType, cell);
    if (!tower) return false;
    tower->setScenePosition(cellCenter(cell));
    createTowerVisual(*tower);
    m_resources -= cost;
    m_towers.push_back(std::move(tower));
    showMessage(terrain == TerrainType::DarkSoil
                    ? QStringLiteral("黑土地优惠 30%，建造花费 %1。").arg(cost)
                    : QStringLiteral("成功建造%1，花费 %2。")
                          .arg(towerDisplayName(m_selectedTowerType)).arg(cost));
    emitHud();
    return true;
}

void GameController::moveEnemy(Enemy &enemy, double deltaSeconds)
{
    if (!m_level || enemy.pathIndex() >= m_level->path.size()) {
        finishGame(false);
        return;
    }

    const QPoint currentCell = cellFromScenePosition(enemy.position());
    const bool onIce = m_level->terrainAt(currentCell) == TerrainType::Ice;
    double remainingMove = enemy.speedCellsPerSecond() * kCellSize
        * enemy.movementMultiplier(onIce) * deltaSeconds;

    while (remainingMove > 0.0 && m_state == GameState::Running) {
        if (enemy.pathIndex() >= m_level->path.size()) {
            finishGame(false);
            return;
        }

        const QPoint targetCell = m_level->path.at(enemy.pathIndex());
        const QPointF target = cellCenter(targetCell);
        const QLineF line(enemy.position(), target);
        const double distance = line.length();
        if (distance <= remainingMove || distance < 0.01) {
            enemy.setPosition(target);
            remainingMove -= distance;
            enemy.setPathIndex(enemy.pathIndex() + 1);
            handlePortal(enemy, targetCell);
        } else {
            const QPointF direction = (target - enemy.position()) / distance;
            enemy.setPosition(enemy.position() + direction * remainingMove);
            if (enemy.visual().sprite) enemy.visual().sprite->setRotation(-line.angle() + 90.0);
            remainingMove = 0.0;
        }
    }
}

void GameController::handlePortal(Enemy &enemy, const QPoint &cell)
{
    if (!m_level || enemy.portalCooldown() > 0.0
        || m_level->terrainAt(cell) != TerrainType::Portal) return;

    const int destinationFlat = m_level->portalPairs.value(m_level->flatIndex(cell), -1);
    if (destinationFlat < 0) return;
    const QPoint destination(destinationFlat % m_level->columns,
                             destinationFlat / m_level->columns);
    const int destinationPathIndex = pathIndexOfCell(destination);
    if (destinationPathIndex < 0) return;

    enemy.setPosition(cellCenter(destination));
    enemy.setPathIndex(destinationPathIndex + 1);
    enemy.setPortalCooldown(0.8);
    showMessage(QStringLiteral("敌人通过传送门改变了位置。"));
}

void GameController::impactProjectile(Projectile &projectile, Enemy &target)
{
    if (projectile.kind == AttackKind::Splash) {
        for (const std::unique_ptr<Enemy> &enemy : m_enemies) {
            if (enemy->isDead()) continue;
            if (distanceBetween(enemy->position(), target.position()) <= projectile.splashRadiusPixels) {
                enemy->applyDamage(projectile.damage, AttackKind::Splash);
                enemy->applyEffect(projectile.effect, projectile.effectDuration,
                                   projectile.effectMagnitude);
            }
        }
        return;
    }
    if (projectile.kind == AttackKind::Laser) {
        applyLaserImpact(projectile, target);
        return;
    }

    target.applyDamage(projectile.damage, projectile.kind);
    target.applyEffect(projectile.effect, projectile.effectDuration, projectile.effectMagnitude);
}

void GameController::applyLaserImpact(const Projectile &projectile, Enemy &target)
{
    const QPointF start = projectile.origin;
    const QPointF end = target.position();
    const QPointF segment = end - start;
    const double lengthSquared = QPointF::dotProduct(segment, segment);
    if (lengthSquared <= 0.01) return;

    int hitCount = 0;
    for (const std::unique_ptr<Enemy> &enemy : m_enemies) {
        if (enemy->isDead()) continue;
        const QPointF relative = enemy->position() - start;
        const double projection = QPointF::dotProduct(relative, segment) / lengthSquared;
        if (projection < -0.05 || projection > 1.35) continue;
        const QPointF closest = start + segment * projection;
        if (distanceBetween(closest, enemy->position()) <= 22.0) {
            enemy->applyDamage(projectile.damage, AttackKind::Laser);
            enemy->applyEffect(projectile.effect, projectile.effectDuration,
                               projectile.effectMagnitude);
            if (++hitCount >= 5) break;
        }
    }
}

void GameController::setState(GameState state)
{
    if (m_state == state) return;
    m_state = state;
    emit stateChanged(state);
}

void GameController::finishGame(bool won)
{
    if (m_state == GameState::Won || m_state == GameState::Lost) return;
    if (won) m_score += m_resources * 2 + qMax(0, 1200 - qRound(m_elapsedGameSeconds) * 4);
    setState(won ? GameState::Won : GameState::Lost);
    if (!m_isTestLevel) saveResult(won);
    const QString summary = QStringLiteral("关卡：%1\n用时：%2 秒\n消灭敌人：%3\n最终资源：%4\n得分：%5")
        .arg(m_level ? m_level->name : QString())
        .arg(m_elapsedGameSeconds, 0, 'f', 1)
        .arg(m_destroyedEnemies)
        .arg(m_resources)
        .arg(m_score);
    emit resultReady(won, summary);
}

void GameController::saveResult(bool won)
{
    if (!m_level) return;
    const QString key = QStringLiteral("levels/%1/").arg(m_level->id);
    m_settings.setValue(key + QStringLiteral("lastWon"), won);
    m_settings.setValue(key + QStringLiteral("lastTime"), m_elapsedGameSeconds);
    m_settings.setValue(key + QStringLiteral("lastScore"), m_score);
    if (won) {
        m_settings.setValue(key + QStringLiteral("completed"), true);
        const double previousTime = m_settings.value(key + QStringLiteral("bestTime"), 0.0).toDouble();
        if (previousTime <= 0.0 || m_elapsedGameSeconds < previousTime) {
            m_settings.setValue(key + QStringLiteral("bestTime"), m_elapsedGameSeconds);
        }
        const int previousScore = m_settings.value(key + QStringLiteral("bestScore"), 0).toInt();
        if (m_score > previousScore) {
            m_settings.setValue(key + QStringLiteral("bestScore"), m_score);
        }
    }
    m_settings.sync();
}

void GameController::emitHud()
{
    if (!m_level) {
        emit hudChanged(QString());
        return;
    }
    const int displayedWave = qMin(m_waveIndex + 1, static_cast<int>(m_level->waves.size()));
    const QString pauseText = m_state == GameState::Paused ? QStringLiteral(" · 已暂停") : QString();
    emit hudChanged(QStringLiteral("资源 %1   基地 1/1   波次 %2/%3   关卡 %4   用时 %5s   FPS %6%7")
                        .arg(m_resources)
                        .arg(displayedWave)
                        .arg(m_level->waves.size())
                        .arg(m_currentLevelIndex + 1)
                        .arg(m_elapsedGameSeconds, 0, 'f', 1)
                        .arg(m_fps)
                        .arg(pauseText));
}

void GameController::emitSelectedTowerInfo()
{
    Tower *tower = selectedTower();
    if (!tower) {
        emit towerInfoChanged(QStringLiteral("点击已建造的防御塔查看状态。"));
        return;
    }
    const QString cooldown = m_activeSkillCooldown <= 0.0
        ? QStringLiteral("就绪") : QStringLiteral("%1 秒").arg(qCeil(m_activeSkillCooldown));
    emit towerInfoChanged(QStringLiteral("%1  Lv.%2\n生命 %3/%4\n%5\n升级费用：%6\n减速塔技能冷却：%7")
                              .arg(towerDisplayName(tower->type()))
                              .arg(tower->level())
                              .arg(qRound(tower->health()))
                              .arg(qRound(tower->maxHealth()))
                              .arg(tower->description())
                              .arg(tower->upgradeCost() > 0 ? QString::number(tower->upgradeCost())
                                                           : QStringLiteral("已满级"))
                              .arg(cooldown));
}
