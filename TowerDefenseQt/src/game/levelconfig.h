#pragma once

#include "enums.h"

#include <QHash>
#include <QPoint>
#include <QString>
#include <QVector>

struct WaveConfig {
    QVector<EnemyType> enemies;
    int spawnIntervalMs = 900;
};

struct LevelConfig {
    QString id;
    QString name;
    QString description;
    int rows = 0;
    int columns = 0;
    int initialResources = 300;
    QVector<TerrainType> terrain;
    QVector<QPoint> path;
    QHash<int, int> portalPairs;
    QVector<WaveConfig> waves;

    int flatIndex(const QPoint &cell) const { return cell.y() * columns + cell.x(); }
    bool contains(const QPoint &cell) const
    {
        return cell.x() >= 0 && cell.x() < columns && cell.y() >= 0 && cell.y() < rows;
    }
    TerrainType terrainAt(const QPoint &cell) const
    {
        return contains(cell) ? terrain.value(flatIndex(cell), TerrainType::Grass) : TerrainType::Stone;
    }
};

class LevelConfigLoader {
public:
    static QVector<LevelConfig> load(QString *errorMessage = nullptr);
    static QVector<LevelConfig> loadFromFile(const QString &filePath,
                                             QString *errorMessage = nullptr);
    static bool saveToFile(const QString &filePath, const LevelConfig &level,
                           QString *errorMessage = nullptr);
    static bool validateEditorLevel(const LevelConfig &level,
                                    QString *errorMessage = nullptr);
    static LevelConfig createEditorTemplate();
};
