#pragma once

#include "enums.h"

#include <QHash>
#include <QPixmap>
#include <QSize>

class SpriteManager {
public:
    QPixmap terrain(TerrainType type, const QSize &size);
    QPixmap path(const QSize &size);
    QPixmap pathMarker(const QSize &size);
    QPixmap tower(TowerType type, const QSize &size);
    QPixmap enemy(EnemyType type, const QSize &size);
    QPixmap projectile(AttackKind type, const QSize &size);
    QPixmap effect(EffectType type, const QSize &size);

private:
    QPixmap load(const QString &resourcePath, const QSize &size);
    QHash<QString, QPixmap> m_cache;
};
