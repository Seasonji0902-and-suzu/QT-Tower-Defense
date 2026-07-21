#include "levelconfig.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

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

QChar terrainToCharacter(TerrainType type)
{
    switch (type) {
    case TerrainType::DarkSoil: return QChar('D');
    case TerrainType::Stone: return QChar('S');
    case TerrainType::Ice: return QChar('I');
    case TerrainType::Portal: return QChar('P');
    case TerrainType::Grass: return QChar('G');
    }
    return QChar('G');
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

QString enemyToString(EnemyType type)
{
    switch (type) {
    case EnemyType::Fast: return QStringLiteral("fast");
    case EnemyType::Heavy: return QStringLiteral("heavy");
    case EnemyType::Resistant: return QStringLiteral("resistant");
    case EnemyType::Split: return QStringLiteral("split");
    case EnemyType::Boss: return QStringLiteral("boss");
    case EnemyType::Minion: return QStringLiteral("minion");
    case EnemyType::Normal: return QStringLiteral("normal");
    }
    return QStringLiteral("normal");
}

QPoint pointFromJson(const QJsonValue &value)
{
    const QJsonArray point = value.toArray();
    return point.size() >= 2 ? QPoint(point.at(0).toInt(), point.at(1).toInt()) : QPoint();
}

QJsonArray pointToJson(const QPoint &point)
{
    return QJsonArray{point.x(), point.y()};
}

QVector<LevelConfig> parseLevels(const QByteArray &data, QString *errorMessage)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
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

        for (const QJsonValue &pointValue : object.value(QStringLiteral("path")).toArray()) {
            level.path.append(pointFromJson(pointValue));
        }

        for (const QJsonValue &pairValue : object.value(QStringLiteral("portals")).toArray()) {
            const QJsonArray pair = pairValue.toArray();
            if (pair.size() != 2) continue;
            const QPoint first = pointFromJson(pair.at(0));
            const QPoint second = pointFromJson(pair.at(1));
            if (!level.contains(first) || !level.contains(second)) continue;
            level.portalPairs.insert(level.flatIndex(first), level.flatIndex(second));
            level.portalPairs.insert(level.flatIndex(second), level.flatIndex(first));
        }

        for (const QJsonValue &waveValue : object.value(QStringLiteral("waves")).toArray()) {
            const QJsonObject waveObject = waveValue.toObject();
            WaveConfig wave;
            wave.spawnIntervalMs = waveObject.value(QStringLiteral("spawnIntervalMs")).toInt(900);
            for (const QJsonValue &enemyValue : waveObject.value(QStringLiteral("enemies")).toArray()) {
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
        *errorMessage = QStringLiteral("配置中没有有效关卡（至少 5×9、路径非空且包含 5 个波次）。");
    }
    return result;
}

QJsonObject levelToJson(const LevelConfig &level)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), level.id);
    object.insert(QStringLiteral("name"), level.name);
    object.insert(QStringLiteral("description"), level.description);
    object.insert(QStringLiteral("rows"), level.rows);
    object.insert(QStringLiteral("columns"), level.columns);
    object.insert(QStringLiteral("initialResources"), level.initialResources);

    QJsonArray terrainRows;
    for (int row = 0; row < level.rows; ++row) {
        QString rowText;
        rowText.reserve(level.columns);
        for (int column = 0; column < level.columns; ++column) {
            rowText.append(terrainToCharacter(level.terrainAt(QPoint(column, row))));
        }
        terrainRows.append(rowText);
    }
    object.insert(QStringLiteral("terrain"), terrainRows);

    QJsonArray path;
    for (const QPoint &point : level.path) path.append(pointToJson(point));
    object.insert(QStringLiteral("path"), path);

    QJsonArray portals;
    for (auto iterator = level.portalPairs.cbegin(); iterator != level.portalPairs.cend(); ++iterator) {
        if (iterator.key() >= iterator.value()) continue;
        const QPoint first(iterator.key() % level.columns, iterator.key() / level.columns);
        const QPoint second(iterator.value() % level.columns, iterator.value() / level.columns);
        portals.append(QJsonArray{pointToJson(first), pointToJson(second)});
    }
    object.insert(QStringLiteral("portals"), portals);

    QJsonArray waves;
    for (const WaveConfig &wave : level.waves) {
        QJsonArray enemies;
        for (EnemyType type : wave.enemies) enemies.append(enemyToString(type));
        QJsonObject waveObject;
        waveObject.insert(QStringLiteral("spawnIntervalMs"), wave.spawnIntervalMs);
        waveObject.insert(QStringLiteral("enemies"), enemies);
        waves.append(waveObject);
    }
    object.insert(QStringLiteral("waves"), waves);
    return object;
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
    return parseLevels(file.readAll(), errorMessage);
}

QVector<LevelConfig> LevelConfigLoader::loadFromFile(const QString &filePath,
                                                     QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法打开文件：%1").arg(file.errorString());
        }
        return {};
    }
    return parseLevels(file.readAll(), errorMessage);
}

bool LevelConfigLoader::saveToFile(const QString &filePath, const LevelConfig &level,
                                   QString *errorMessage)
{
    QString validationError;
    if (!validateEditorLevel(level, &validationError)) {
        if (errorMessage) *errorMessage = validationError;
        return false;
    }

    QJsonObject root;
    root.insert(QStringLiteral("levels"), QJsonArray{levelToJson(level)});
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法创建文件：%1").arg(file.errorString());
        }
        return false;
    }
    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("写入文件失败：%1").arg(file.errorString());
        }
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("保存文件失败：%1").arg(file.errorString());
        }
        return false;
    }
    return true;
}

bool LevelConfigLoader::validateEditorLevel(const LevelConfig &level, QString *errorMessage)
{
    auto fail = [errorMessage](const QString &message) {
        if (errorMessage) *errorMessage = message;
        return false;
    };

    if (level.rows != 6 || level.columns != 10) {
        return fail(QStringLiteral("编辑器关卡必须使用 6×10 网格。"));
    }
    if (level.terrain.size() != level.rows * level.columns) {
        return fail(QStringLiteral("地形数据数量不等于 60 格。"));
    }
    if (level.path.isEmpty()) {
        return fail(QStringLiteral("路径不能为空：请按敌人行进顺序点击道路格。"));
    }
    for (int index = 0; index < level.path.size(); ++index) {
        if (!level.contains(level.path.at(index))) {
            return fail(QStringLiteral("路径第 %1 格坐标越界。").arg(index + 1));
        }
        if (index == 0) continue;
        const QPoint previous = level.path.at(index - 1);
        const QPoint current = level.path.at(index);
        const int distance = qAbs(previous.x() - current.x()) + qAbs(previous.y() - current.y());
        const bool portalJump = level.portalPairs.value(level.flatIndex(previous), -1)
            == level.flatIndex(current);
        if (distance != 1 && !portalJump) {
            return fail(QStringLiteral("路径第 %1 格与第 %2 格不相邻；只有一对传送门之间允许跳跃。")
                            .arg(index).arg(index + 1));
        }
    }

    QVector<int> portalCells;
    for (int index = 0; index < level.terrain.size(); ++index) {
        if (level.terrain.at(index) == TerrainType::Portal) portalCells.append(index);
    }
    if (!portalCells.isEmpty() && portalCells.size() != 2) {
        return fail(QStringLiteral("传送门可以不设置；如果设置，则必须正好设置一对。"));
    }
    if (portalCells.size() == 2) {
        const int first = portalCells.at(0);
        const int second = portalCells.at(1);
        if (level.portalPairs.value(first, -1) != second
            || level.portalPairs.value(second, -1) != first) {
            return fail(QStringLiteral("传送门配对数据无效，请重新依次点击两格传送门。"));
        }
    }
    if (level.waves.size() < 5) {
        return fail(QStringLiteral("测试关卡必须包含至少 5 个波次。"));
    }
    return true;
}

LevelConfig LevelConfigLoader::createEditorTemplate()
{
    LevelConfig level;
    level.id = QStringLiteral("editor_test");
    level.name = QStringLiteral("编辑器测试关卡");
    level.description = QStringLiteral("由简单关卡编辑器创建，不影响正式三关。界面坐标为（列, 行）。");
    level.rows = 6;
    level.columns = 10;
    level.initialResources = 500;
    level.terrain.fill(TerrainType::Grass, level.rows * level.columns);

    level.waves = {
        {{EnemyType::Normal, EnemyType::Normal, EnemyType::Normal}, 1050},
        {{EnemyType::Fast, EnemyType::Normal, EnemyType::Fast, EnemyType::Normal}, 850},
        {{EnemyType::Heavy, EnemyType::Normal, EnemyType::Heavy}, 1100},
        {{EnemyType::Resistant, EnemyType::Split, EnemyType::Fast, EnemyType::Resistant}, 900},
        {{EnemyType::Heavy, EnemyType::Split, EnemyType::Resistant, EnemyType::Boss}, 950}
    };
    return level;
}
