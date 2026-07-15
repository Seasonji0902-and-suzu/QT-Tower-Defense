#include "levelconfig.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

TerrainType terrainFromCharacter(QChar value)
{
    switch (value.toLatin1()) {
    case 'D': return TerrainType::DarkSoil;
    case 'S': return TerrainType::Stone;
    case 'I': return TerrainType::Ice;
    case 'P': return TerrainType::Portal;
    default: return TerrainType::Grass;
    }
}

EnemyType enemyFromString(const QString &value)
{
    if (value == QStringLiteral("fast")) return EnemyType::Fast;
    if (value == QStringLiteral("heavy")) return EnemyType::Heavy;
    if (value == QStringLiteral("resistant")) return EnemyType::Resistant;
    if (value == QStringLiteral("split")) return EnemyType::Split;
    if (value == QStringLiteral("boss")) return EnemyType::Boss;
    if (value == QStringLiteral("minion")) return EnemyType::Minion;
    return EnemyType::Normal;
}

QPoint pointFromJson(const QJsonValue &value)
{
    const QJsonArray point = value.toArray();
    return point.size() >= 2 ? QPoint(point.at(0).toInt(), point.at(1).toInt()) : QPoint();
}

} // namespace

QVector<LevelConfig> LevelConfigLoader::load(QString *errorMessage)
{
    const QString externalPath = QCoreApplication::applicationDirPath()
        + QStringLiteral("/config/levels.json");

    QFile file(externalPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        file.setFileName(QStringLiteral(":/config/levels.json"));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("无法读取关卡配置文件：%1").arg(file.errorString());
            }
            return {};
        }
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("关卡 JSON 格式错误：%1").arg(parseError.errorString());
        }
        return {};
    }

    QVector<LevelConfig> result;
    const QJsonArray levels = document.object().value(QStringLiteral("levels")).toArray();
    for (const QJsonValue &levelValue : levels) {
        const QJsonObject object = levelValue.toObject();
        LevelConfig level;
        level.id = object.value(QStringLiteral("id")).toString();
        level.name = object.value(QStringLiteral("name")).toString();
        level.description = object.value(QStringLiteral("description")).toString();
        level.rows = object.value(QStringLiteral("rows")).toInt();
        level.columns = object.value(QStringLiteral("columns")).toInt();
        level.initialResources = object.value(QStringLiteral("initialResources")).toInt(300);

        const QJsonArray terrainRows = object.value(QStringLiteral("terrain")).toArray();
        for (int row = 0; row < level.rows; ++row) {
            const QString rowText = row < terrainRows.size() ? terrainRows.at(row).toString() : QString();
            for (int column = 0; column < level.columns; ++column) {
                const QChar terrainCharacter = column < rowText.size() ? rowText.at(column) : QChar('G');
                level.terrain.append(terrainFromCharacter(terrainCharacter));
            }
        }

        const QJsonArray path = object.value(QStringLiteral("path")).toArray();
        for (const QJsonValue &pointValue : path) {
            level.path.append(pointFromJson(pointValue));
        }

        const QJsonArray portals = object.value(QStringLiteral("portals")).toArray();
        for (const QJsonValue &pairValue : portals) {
            const QJsonArray pair = pairValue.toArray();
            if (pair.size() != 2) continue;
            const QPoint first = pointFromJson(pair.at(0));
            const QPoint second = pointFromJson(pair.at(1));
            level.portalPairs.insert(level.flatIndex(first), level.flatIndex(second));
            level.portalPairs.insert(level.flatIndex(second), level.flatIndex(first));
        }

        const QJsonArray waves = object.value(QStringLiteral("waves")).toArray();
        for (const QJsonValue &waveValue : waves) {
            const QJsonObject waveObject = waveValue.toObject();
            WaveConfig wave;
            wave.spawnIntervalMs = waveObject.value(QStringLiteral("spawnIntervalMs")).toInt(900);
            const QJsonArray enemies = waveObject.value(QStringLiteral("enemies")).toArray();
            for (const QJsonValue &enemyValue : enemies) {
                wave.enemies.append(enemyFromString(enemyValue.toString()));
            }
            level.waves.append(wave);
        }

        if (level.rows >= 5 && level.columns >= 9 && !level.path.isEmpty()
            && level.waves.size() >= 5) {
            result.append(level);
        }
    }

    if (result.isEmpty() && errorMessage) {
        *errorMessage = QStringLiteral("配置中没有有效关卡（至少 5×9、5 个波次）。");
    }
    return result;
}
