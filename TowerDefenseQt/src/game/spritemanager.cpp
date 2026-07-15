#include "spritemanager.h"

QPixmap SpriteManager::load(const QString &resourcePath, const QSize &size)
{
    const QString key = resourcePath + QStringLiteral("@%1x%2").arg(size.width()).arg(size.height());
    const auto existing = m_cache.constFind(key);
    if (existing != m_cache.cend()) return existing.value();

    QPixmap source(resourcePath);
    const QPixmap scaled = source.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_cache.insert(key, scaled);
    return scaled;
}

QPixmap SpriteManager::terrain(TerrainType type, const QSize &size)
{
    switch (type) {
    case TerrainType::DarkSoil: return load(QStringLiteral(":/assets/tiles/dark_soil.png"), size);
    case TerrainType::Stone: return load(QStringLiteral(":/assets/tiles/stone.png"), size);
    case TerrainType::Ice: return load(QStringLiteral(":/assets/tiles/ice.png"), size);
    case TerrainType::Portal: return load(QStringLiteral(":/assets/tiles/portal.png"), size);
    case TerrainType::Grass: return load(QStringLiteral(":/assets/tiles/grass.png"), size);
    }
    return {};
}

QPixmap SpriteManager::path(const QSize &size)
{
    return load(QStringLiteral(":/assets/tiles/path.png"), size);
}

QPixmap SpriteManager::tower(TowerType type, const QSize &size)
{
    switch (type) {
    case TowerType::Shooter: return load(QStringLiteral(":/assets/towers/shooter.png"), size);
    case TowerType::Slow: return load(QStringLiteral(":/assets/towers/slow.png"), size);
    case TowerType::Splash: return load(QStringLiteral(":/assets/towers/splash.png"), size);
    case TowerType::Laser: return load(QStringLiteral(":/assets/towers/laser.png"), size);
    case TowerType::Resource: return load(QStringLiteral(":/assets/towers/resource.png"), size);
    case TowerType::Wall: return load(QStringLiteral(":/assets/towers/wall.png"), size);
    }
    return {};
}

QPixmap SpriteManager::enemy(EnemyType type, const QSize &size)
{
    switch (type) {
    case EnemyType::Normal: return load(QStringLiteral(":/assets/enemies/normal.png"), size);
    case EnemyType::Fast: return load(QStringLiteral(":/assets/enemies/fast.png"), size);
    case EnemyType::Heavy: return load(QStringLiteral(":/assets/enemies/heavy.png"), size);
    case EnemyType::Resistant: return load(QStringLiteral(":/assets/enemies/resistant.png"), size);
    case EnemyType::Split: return load(QStringLiteral(":/assets/enemies/split.png"), size);
    case EnemyType::Boss: return load(QStringLiteral(":/assets/enemies/boss.png"), size);
    case EnemyType::Minion: return load(QStringLiteral(":/assets/enemies/minion.png"), size);
    }
    return {};
}

QPixmap SpriteManager::projectile(AttackKind type, const QSize &size)
{
    switch (type) {
    case AttackKind::Slow: return load(QStringLiteral(":/assets/projectiles/slow.png"), size);
    case AttackKind::Splash: return load(QStringLiteral(":/assets/projectiles/rocket.png"), size);
    case AttackKind::Laser: return load(QStringLiteral(":/assets/projectiles/laser.png"), size);
    default: return load(QStringLiteral(":/assets/projectiles/bullet.png"), size);
    }
}

QPixmap SpriteManager::effect(EffectType type, const QSize &size)
{
    switch (type) {
    case EffectType::Burn: return load(QStringLiteral(":/assets/effects/burn.png"), size);
    case EffectType::Stun: return load(QStringLiteral(":/assets/effects/stun.png"), size);
    case EffectType::Slow: return load(QStringLiteral(":/assets/projectiles/slow.png"), size);
    case EffectType::None: break;
    }
    return {};
}
