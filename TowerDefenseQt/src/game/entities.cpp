#include "entities.h"

#include "gamecontroller.h"

#include <algorithm>

Tower::Tower(TowerType type, const QPoint &cell, int hitPoints, double rangeCells,
             double attackInterval, double damage)
    : m_type(type)
    , m_cell(cell)
    , m_health(hitPoints)
    , m_maxHealth(hitPoints)
    , m_rangeCells(rangeCells)
    , m_attackInterval(attackInterval)
    , m_damage(damage)
{
}

void Tower::tick(GameController &controller, double deltaSeconds)
{
    m_cooldownRemaining -= deltaSeconds;
    if (m_cooldownRemaining <= 0.0 && performAction(controller)) {
        m_cooldownRemaining = m_attackInterval;
    }
}

void Tower::takeDamage(double amount)
{
    m_health = std::max(0.0, m_health - amount);
}

int Tower::upgradeCost() const
{
    return m_level >= 3 ? 0 : 70 * m_level;
}

bool Tower::upgrade()
{
    if (m_level >= 3) return false;
    ++m_level;
    m_maxHealth *= 1.25;
    m_health = m_maxHealth;
    m_damage *= 1.28;
    m_rangeCells += 0.15;
    m_attackInterval *= 0.90;
    return true;
}

ShooterTower::ShooterTower(const QPoint &cell)
    : Tower(TowerType::Shooter, cell, 220, 2.8, 0.75, 32.0)
{
}

QString ShooterTower::description() const
{
    return QStringLiteral("稳定发射普通子弹，造成单体伤害。");
}

bool ShooterTower::performAction(GameController &controller)
{
    Enemy *target = controller.acquireTarget(*this);
    if (!target) return false;
    controller.launchProjectile(*this, *target, AttackKind::Normal, m_damage, 0.0,
                                EffectType::None, 0.0, 0.0);
    return true;
}

SlowTower::SlowTower(const QPoint &cell)
    : Tower(TowerType::Slow, cell, 200, 2.5, 1.10, 16.0)
{
}

QString SlowTower::description() const
{
    return QStringLiteral("命中后减速 35%，持续 2.6 秒；冰面上效果增强。");
}

bool SlowTower::performAction(GameController &controller)
{
    Enemy *target = controller.acquireTarget(*this);
    if (!target) return false;
    controller.launchProjectile(*this, *target, AttackKind::Slow, m_damage, 0.0,
                                EffectType::Slow, 2.6, 0.35);
    return true;
}

SplashTower::SplashTower(const QPoint &cell)
    : Tower(TowerType::Splash, cell, 200, 2.7, 1.50, 48.0)
{
}

QString SplashTower::description() const
{
    return QStringLiteral("火箭命中后造成范围伤害，并附加灼烧。");
}

bool SplashTower::performAction(GameController &controller)
{
    Enemy *target = controller.acquireTarget(*this);
    if (!target) return false;
    controller.launchProjectile(*this, *target, AttackKind::Splash, m_damage,
                                controller.cellSize() * 0.9, EffectType::Burn, 3.2, 9.0);
    return true;
}

LaserTower::LaserTower(const QPoint &cell)
    : Tower(TowerType::Laser, cell, 190, 3.4, 1.60, 42.0)
{
}

QString LaserTower::description() const
{
    return QStringLiteral("激光穿透同一直线上的多个敌人，并短暂眩晕。");
}

bool LaserTower::performAction(GameController &controller)
{
    Enemy *target = controller.acquireTarget(*this);
    if (!target) return false;
    controller.launchProjectile(*this, *target, AttackKind::Laser, m_damage, 0.0,
                                EffectType::Stun, 0.45, 1.0);
    return true;
}

ResourceTower::ResourceTower(const QPoint &cell)
    : Tower(TowerType::Resource, cell, 160, 0.0, 5.0, 0.0)
{
    m_cooldownRemaining = 2.0;
}

QString ResourceTower::description() const
{
    return QStringLiteral("每隔一段时间生产额外资源。");
}

bool ResourceTower::performAction(GameController &controller)
{
    controller.addResources(20 + (m_level - 1) * 12, true);
    return true;
}

bool ResourceTower::upgrade()
{
    if (!Tower::upgrade()) return false;
    m_attackInterval *= 0.86;
    return true;
}

WallTower::WallTower(const QPoint &cell)
    : Tower(TowerType::Wall, cell, 900, 0.0, 3600.0, 0.0)
{
}

QString WallTower::description() const
{
    return QStringLiteral("可建在路径上，阻挡敌人并承受攻击。");
}

bool WallTower::performAction(GameController &)
{
    return false;
}

bool WallTower::upgrade()
{
    if (m_level >= 3) return false;
    ++m_level;
    m_maxHealth *= 1.55;
    m_health = m_maxHealth;
    return true;
}

std::unique_ptr<Tower> makeTower(TowerType type, const QPoint &cell)
{
    switch (type) {
    case TowerType::Shooter: return std::make_unique<ShooterTower>(cell);
    case TowerType::Slow: return std::make_unique<SlowTower>(cell);
    case TowerType::Splash: return std::make_unique<SplashTower>(cell);
    case TowerType::Laser: return std::make_unique<LaserTower>(cell);
    case TowerType::Resource: return std::make_unique<ResourceTower>(cell);
    case TowerType::Wall: return std::make_unique<WallTower>(cell);
    }
    return {};
}

Enemy::Enemy(int id, EnemyType type, double hitPoints, double speedCellsPerSecond, int reward)
    : m_id(id)
    , m_type(type)
    , m_health(hitPoints)
    , m_maxHealth(hitPoints)
    , m_speedCellsPerSecond(speedCellsPerSecond)
    , m_reward(reward)
{
}

double Enemy::damageMultiplier(AttackKind) const
{
    return 1.0;
}

double Enemy::effectMultiplier(EffectType) const
{
    return 1.0;
}

void Enemy::updateSpecial(GameController &, double)
{
}

void Enemy::onDeath(GameController &)
{
}

void Enemy::applyDamage(double amount, AttackKind kind)
{
    double remainingDamage = amount * damageMultiplier(kind);
    if (m_shield > 0.0) {
        const double absorbed = std::min(m_shield, remainingDamage);
        m_shield -= absorbed;
        remainingDamage -= absorbed;
    }
    m_health = std::max(0.0, m_health - remainingDamage);
}

void Enemy::applyEffect(EffectType type, double duration, double magnitude)
{
    if (type == EffectType::None) return;
    const double resistance = effectMultiplier(type);
    duration *= resistance;
    magnitude *= resistance;

    for (EffectInstance &effect : m_effects) {
        if (effect.type == type) {
            effect.remaining = std::max(effect.remaining, duration);
            effect.magnitude = std::max(effect.magnitude, magnitude);
            return;
        }
    }
    m_effects.push_back({type, duration, magnitude, 0.5});
}

void Enemy::tickEffects(double deltaSeconds, bool onIce)
{
    for (EffectInstance &effect : m_effects) {
        effect.remaining -= deltaSeconds;
        if (effect.type == EffectType::Burn) {
            effect.tickRemaining -= deltaSeconds;
            while (effect.tickRemaining <= 0.0) {
                applyDamage(effect.magnitude * 0.5, AttackKind::Burn);
                effect.tickRemaining += 0.5;
            }
        }
    }
    std::erase_if(m_effects, [](const EffectInstance &effect) {
        return effect.remaining <= 0.0;
    });
    m_portalCooldown = std::max(0.0, m_portalCooldown - deltaSeconds);
}

double Enemy::movementMultiplier(bool onIce) const
{
    double multiplier = onIce ? 1.35 : 1.0;
    for (const EffectInstance &effect : m_effects) {
        if (effect.type == EffectType::Slow) {
            const double strength = onIce ? std::min(0.80, effect.magnitude * 1.5) : effect.magnitude;
            multiplier *= std::max(0.15, 1.0 - strength);
        }
        if (effect.type == EffectType::Stun) multiplier = 0.0;
    }
    return multiplier;
}

bool Enemy::stunned() const
{
    return std::any_of(m_effects.cbegin(), m_effects.cend(), [](const EffectInstance &effect) {
        return effect.type == EffectType::Stun;
    });
}

void Enemy::addShield(double amount)
{
    m_maxShield = std::max(m_maxShield, amount);
    m_shield = std::min(m_maxShield, m_shield + amount);
}

NormalEnemy::NormalEnemy(int id, EnemyType type)
    : Enemy(id, type, type == EnemyType::Minion ? 45.0 : 110.0,
            type == EnemyType::Minion ? 1.05 : 0.75,
            type == EnemyType::Minion ? 4 : 12)
{
}

QString NormalEnemy::description() const
{
    return m_type == EnemyType::Minion ? QStringLiteral("小型敌人") : QStringLiteral("普通敌人");
}

FastEnemy::FastEnemy(int id)
    : Enemy(id, EnemyType::Fast, 70.0, 1.30, 14)
{
}

QString FastEnemy::description() const
{
    return QStringLiteral("高速低生命敌人");
}

HeavyEnemy::HeavyEnemy(int id)
    : Enemy(id, EnemyType::Heavy, 330.0, 0.43, 28)
{
}

QString HeavyEnemy::description() const
{
    return QStringLiteral("高生命重甲敌人");
}

ResistantEnemy::ResistantEnemy(int id)
    : Enemy(id, EnemyType::Resistant, 190.0, 0.66, 25)
{
}

QString ResistantEnemy::description() const
{
    return QStringLiteral("抵抗范围攻击和状态效果");
}

double ResistantEnemy::damageMultiplier(AttackKind kind) const
{
    return (kind == AttackKind::Splash || kind == AttackKind::Burn) ? 0.55 : 0.85;
}

double ResistantEnemy::effectMultiplier(EffectType) const
{
    return 0.45;
}

SplitEnemy::SplitEnemy(int id)
    : Enemy(id, EnemyType::Split, 165.0, 0.64, 22)
{
}

QString SplitEnemy::description() const
{
    return QStringLiteral("死亡后分裂成两个小敌人");
}

void SplitEnemy::onDeath(GameController &controller)
{
    controller.queueMinions(position(), pathIndex(), 2);
}

BossEnemy::BossEnemy(int id)
    : Enemy(id, EnemyType::Boss, 1350.0, 0.33, 250)
{
    m_specialTimer = 5.0;
    m_maxShield = 320.0;
    m_shield = 320.0;
}

QString BossEnemy::description() const
{
    return QStringLiteral("周期恢复护盾并召唤小敌人");
}

void BossEnemy::updateSpecial(GameController &controller, double deltaSeconds)
{
    m_specialTimer -= deltaSeconds;
    if (m_specialTimer <= 0.0) {
        addShield(140.0);
        controller.queueMinions(position(), pathIndex(), 1);
        controller.showMessage(QStringLiteral("Boss 恢复护盾并召唤了增援！"));
        m_specialTimer = 6.5;
    }
}

std::unique_ptr<Enemy> makeEnemy(EnemyType type, int id)
{
    switch (type) {
    case EnemyType::Normal: return std::make_unique<NormalEnemy>(id);
    case EnemyType::Fast: return std::make_unique<FastEnemy>(id);
    case EnemyType::Heavy: return std::make_unique<HeavyEnemy>(id);
    case EnemyType::Resistant: return std::make_unique<ResistantEnemy>(id);
    case EnemyType::Split: return std::make_unique<SplitEnemy>(id);
    case EnemyType::Boss: return std::make_unique<BossEnemy>(id);
    case EnemyType::Minion: return std::make_unique<NormalEnemy>(id, EnemyType::Minion);
    }
    return {};
}

Projectile::Projectile(AttackKind kind, int targetId, const QPointF &position,
                       double speedPixelsPerSecond, double damage, double splashRadiusPixels,
                       EffectType effect, double effectDuration, double effectMagnitude)
    : kind(kind)
    , targetId(targetId)
    , origin(position)
    , position(position)
    , speedPixelsPerSecond(speedPixelsPerSecond)
    , damage(damage)
    , splashRadiusPixels(splashRadiusPixels)
    , effect(effect)
    , effectDuration(effectDuration)
    , effectMagnitude(effectMagnitude)
{
}
