#pragma once

#include <QString>

enum class TerrainType {
    Grass,
    DarkSoil,
    Stone,
    Ice,
    Portal
};

enum class TowerType {
    Shooter,
    Slow,
    Splash,
    Laser,
    Resource,
    Wall
};

enum class EnemyType {
    Normal,
    Fast,
    Heavy,
    Resistant,
    Split,
    Boss,
    Minion
};

enum class AttackKind {
    Normal,
    Slow,
    Splash,
    Laser,
    Burn
};

enum class EffectType {
    None,
    Slow,
    Burn,
    Stun
};

enum class GameState {
    Idle,
    Running,
    Paused,
    Won,
    Lost
};

inline QString towerDisplayName(TowerType type)
{
    switch (type) {
    case TowerType::Shooter: return QStringLiteral("射手塔");
    case TowerType::Slow: return QStringLiteral("减速塔");
    case TowerType::Splash: return QStringLiteral("范围塔");
    case TowerType::Laser: return QStringLiteral("激光塔");
    case TowerType::Resource: return QStringLiteral("资源塔");
    case TowerType::Wall: return QStringLiteral("防御墙");
    }
    return {};
}

inline int towerBaseCost(TowerType type)
{
    switch (type) {
    case TowerType::Shooter: return 90;
    case TowerType::Slow: return 120;
    case TowerType::Splash: return 150;
    case TowerType::Laser: return 180;
    case TowerType::Resource: return 110;
    case TowerType::Wall: return 70;
    }
    return 0;
}

