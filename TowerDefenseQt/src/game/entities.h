#pragma once

#include "enums.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QPoint>
#include <QPointF>
#include <QString>
#include <QVector>

#include <memory>
#include <vector>

class GameController;

struct SpriteVisual {
    QGraphicsPixmapItem *sprite = nullptr;
    QGraphicsRectItem *healthBackground = nullptr;
    QGraphicsRectItem *healthFill = nullptr;
    QGraphicsRectItem *shieldFill = nullptr;
    QGraphicsPixmapItem *effectIcon = nullptr;
    QGraphicsPixmapItem *slowAura = nullptr;
};

struct EffectInstance {
    EffectType type = EffectType::None;
    double remaining = 0.0;
    double magnitude = 0.0;
    double tickRemaining = 0.5;
};

class Tower {
public:
    Tower(TowerType type, const QPoint &cell, int hitPoints, double rangeCells,
          double attackInterval, double damage);
    virtual ~Tower() = default;

    virtual QString description() const = 0;
    virtual bool performAction(GameController &controller) = 0;
    virtual bool canAttack() const { return true; }

    void tick(GameController &controller, double deltaSeconds);
    void takeDamage(double amount);

    TowerType type() const { return m_type; }
    const QPoint &cell() const { return m_cell; }
    const QPointF &scenePosition() const { return m_scenePosition; }
    void setScenePosition(const QPointF &position) { m_scenePosition = position; }
    double rangeCells() const { return m_rangeCells; }
    double damage() const { return m_damage; }
    double health() const { return m_health; }
    double maxHealth() const { return m_maxHealth; }
    bool isDead() const { return m_health <= 0.0; }
    int level() const { return m_level; }
    double cooldownRemaining() const { return m_cooldownRemaining; }
    double attackInterval() const { return m_attackInterval; }
    SpriteVisual &visual() { return m_visual; }
    const SpriteVisual &visual() const { return m_visual; }

    virtual int upgradeCost() const;
    virtual bool upgrade();

protected:
    TowerType m_type;
    QPoint m_cell;
    QPointF m_scenePosition;
    double m_health;
    double m_maxHealth;
    double m_rangeCells;
    double m_attackInterval;
    double m_cooldownRemaining = 0.25;
    double m_damage;
    int m_level = 1;
    SpriteVisual m_visual;
};

class ShooterTower final : public Tower {
public:
    explicit ShooterTower(const QPoint &cell);
    QString description() const override;
    bool performAction(GameController &controller) override;
};

class SlowTower final : public Tower {
public:
    explicit SlowTower(const QPoint &cell);
    QString description() const override;
    bool performAction(GameController &controller) override;
};

class SplashTower final : public Tower {
public:
    explicit SplashTower(const QPoint &cell);
    QString description() const override;
    bool performAction(GameController &controller) override;
};

class LaserTower final : public Tower {
public:
    explicit LaserTower(const QPoint &cell);
    QString description() const override;
    bool performAction(GameController &controller) override;
};

class ResourceTower final : public Tower {
public:
    explicit ResourceTower(const QPoint &cell);
    QString description() const override;
    bool performAction(GameController &controller) override;
    bool canAttack() const override { return false; }
    bool upgrade() override;
};

class WallTower final : public Tower {
public:
    explicit WallTower(const QPoint &cell);
    QString description() const override;
    bool performAction(GameController &controller) override;
    bool canAttack() const override { return false; }
    bool upgrade() override;
};

std::unique_ptr<Tower> makeTower(TowerType type, const QPoint &cell);

class Enemy {
public:
    Enemy(int id, EnemyType type, double hitPoints, double speedCellsPerSecond, int reward);
    virtual ~Enemy() = default;

    virtual QString description() const = 0;
    virtual double damageMultiplier(AttackKind kind) const;
    virtual double effectMultiplier(EffectType type) const;
    virtual void updateSpecial(GameController &controller, double deltaSeconds);
    virtual void onDeath(GameController &controller);

    void applyDamage(double amount, AttackKind kind);
    void applyEffect(EffectType type, double duration, double magnitude);
    void tickEffects(double deltaSeconds, bool onIce);
    double movementMultiplier(bool onIce) const;
    bool stunned() const;

    int id() const { return m_id; }
    EnemyType type() const { return m_type; }
    double health() const { return m_health; }
    double maxHealth() const { return m_maxHealth; }
    double shield() const { return m_shield; }
    double maxShield() const { return m_maxShield; }
    void addShield(double amount);
    bool isDead() const { return m_health <= 0.0; }
    double speedCellsPerSecond() const { return m_speedCellsPerSecond; }
    int reward() const { return m_reward; }
    QPointF position() const { return m_position; }
    void setPosition(const QPointF &position) { m_position = position; }
    int pathIndex() const { return m_pathIndex; }
    void setPathIndex(int index) { m_pathIndex = index; }
    double attackCooldown() const { return m_attackCooldown; }
    void setAttackCooldown(double value) { m_attackCooldown = value; }
    double portalCooldown() const { return m_portalCooldown; }
    void setPortalCooldown(double value) { m_portalCooldown = value; }
    const std::vector<EffectInstance> &effects() const { return m_effects; }
    SpriteVisual &visual() { return m_visual; }
    const SpriteVisual &visual() const { return m_visual; }

protected:
    int m_id;
    EnemyType m_type;
    double m_health;
    double m_maxHealth;
    double m_shield = 0.0;
    double m_maxShield = 0.0;
    double m_speedCellsPerSecond;
    int m_reward;
    QPointF m_position;
    int m_pathIndex = 1;
    double m_attackCooldown = 0.0;
    double m_portalCooldown = 0.0;
    double m_specialTimer = 0.0;
    std::vector<EffectInstance> m_effects;
    SpriteVisual m_visual;
};

class NormalEnemy final : public Enemy {
public:
    NormalEnemy(int id, EnemyType type = EnemyType::Normal);
    QString description() const override;
};

class FastEnemy final : public Enemy {
public:
    explicit FastEnemy(int id);
    QString description() const override;
};

class HeavyEnemy final : public Enemy {
public:
    explicit HeavyEnemy(int id);
    QString description() const override;
};

class ResistantEnemy final : public Enemy {
public:
    explicit ResistantEnemy(int id);
    QString description() const override;
    double damageMultiplier(AttackKind kind) const override;
    double effectMultiplier(EffectType type) const override;
};

class SplitEnemy final : public Enemy {
public:
    explicit SplitEnemy(int id);
    QString description() const override;
    void onDeath(GameController &controller) override;
};

class BossEnemy final : public Enemy {
public:
    explicit BossEnemy(int id);
    QString description() const override;
    void updateSpecial(GameController &controller, double deltaSeconds) override;
};

std::unique_ptr<Enemy> makeEnemy(EnemyType type, int id);

class Projectile {
public:
    Projectile(AttackKind kind, int targetId, const QPointF &position, double speedPixelsPerSecond,
               double damage, double splashRadiusPixels, EffectType effect,
               double effectDuration, double effectMagnitude);

    AttackKind kind;
    int targetId;
    QPointF origin;
    QPointF position;
    double speedPixelsPerSecond;
    double damage;
    double splashRadiusPixels;
    EffectType effect;
    double effectDuration;
    double effectMagnitude;
    bool expired = false;
    QGraphicsPixmapItem *visual = nullptr;
};
