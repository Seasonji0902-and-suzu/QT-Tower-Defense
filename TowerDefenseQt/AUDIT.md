# GridGuard 验收审计报告

- 审计日期：2026-07-21
- 项目目录：C:/Users/20635/Desktop/大作业/TowerDefenseQt
- 评分文件实际位置：C:/Users/20635/Desktop/大作业/大作业.pdf
- 用户所述 requirements/大作业.pdf 在项目内不存在；本次以实际存在、共 7 页的同名 PDF 为准。
- 审计原则：只读审计，不修改源码、配置或资源。本报告是本次唯一新增的项目文件。

## 1. 审计范围与方法

本次逐页阅读了评分 PDF；逐行阅读了 12 个 C++ 头文件和源文件，以及 CMakeLists.txt、resources.qrc、styles/app.qss、两份 levels.json、README.md、ATTRIBUTION.md、doc/项目设计说明.md、启动批处理和许可文件。项目共有 666 个既有文件，其中 628 个 PNG 均完成解码检查，未发现损坏；resources.qrc 的 27 个引用全部存在。

静态审计不把 README 的自述当成功能证据。下文的“已完成”表示存在明确代码实现和构建证据，仍列出答辩前应执行的人工验证。“部分完成”表示静态结构存在，但有缺口或展示效果必须实机确认。“无法验证”表示当前审计方法无法可靠证明该运行指标。

## 2. 干净编译与启动结果

| 项目 | 结果 |
|---|---|
| Qt | 6.5.3，D:/Qt/6.5.3/mingw_64 |
| 编译器 | MinGW-w64 GCC/G++ 11.2.0，x86_64-posix-seh |
| CMake / Ninja | CMake 3.30.5，Ninja 1.12.1 |
| 构建类型 | Release |
| 纯英文外部构建目录 | C:/Users/20635/Documents/Codex/AuditBuild/GridGuardQt653 |
| 干净构建 | cmake --build ... --clean-first --parallel，10/10 步成功，退出码 0 |
| 新构建程序冒烟 | --smoke-test，Qt 运行库来自 6.5.3 Kit，退出码 0 |
| bin 离屏部署冒烟 | 清除 Qt、MinGW 环境路径后使用 qoffscreen，退出码 0 |
| bin 原生 Windows 部署冒烟 | 清除 Qt、MinGW 环境路径后使用 qwindows，退出码 0 |
| 目标格式 | PE x86-64 |

构建过程只在项目外产生文件。项目内未生成 CMakeFiles、CMakeCache、对象文件或其他中间产物。

## 3. 必选需求逐项审计

### 需求点 1：基础游戏流程与多种地形（15%）

| 子项 | 要求内容 | 状态 | 对应源文件、类和关键函数 | 静态代码证据 | 人工运行验证步骤 | 当前缺陷或潜在风险 | 对评分影响 | 修复成本 |
|---|---|---|---|---|---|---|---|---|
| 1.1 | 开始、游戏、胜利、失败界面 | 已完成 | src/mainwindow.cpp；MainWindow::createStartPage、createGamePage、createResultPage、connectController；GameController::finishGame | QStackedWidget 建立开始、帮助、选关、游戏和结果页；resultReady 根据 won 显示“胜利/失败”并切换结果页 | 从开始页进入一关；分别完成五波和放行一个敌人，确认两种结果页及按钮 | 自动 smoke 只切换三关，没有真正触发胜负；结果页布局仍需肉眼确认 | 2%；若任一结果流程不可达会直接丢分 | 低 |
| 1.2 | 二维网格不小于 5 行 × 9 列 | 已完成 | config/levels.json；LevelConfig；GameController::buildMap | 三关均为 6×10；buildMap 双循环创建 60 个格子并设置 sceneRect | 进入三关，逐关确认地图完整显示且边缘格可点击 | 视图使用 fitInView，极端 DPI 下格子尺寸和可点击精度需确认 | 2% | 低 |
| 1.3 | 敌人从入口沿路线到终点 | 已完成 | LevelConfigLoader::load；GameController::spawnEnemy、moveEnemy | JSON path 被读为 QPoint 列表；敌人从 path.first 生成，逐点移动并递增 pathIndex | 不建塔观察敌人完整行进；第二关额外观察传送前后路线 | 路线合法性主要由配置保证，加载器未严格验证所有点的连续性 | 2% | 中 |
| 1.4 | 鼠标在合法格子放塔 | 已完成 | MainWindow::eventFilter；GameController::handleSceneClick、tryBuildTower | QGraphicsView viewport 的左键事件映射到场景坐标；tryBuildTower 检查边界、地形、路径、占用和资源 | 选择六种塔，分别在合法与非法格点击，核对提示和扣费 | 双击事件与单击释放组合可能造成“放塔后立即升级”的误操作，需要实机确认 | 2% | 低 |
| 1.5 | 暂停、继续、重新开始 | 已完成 | MainWindow::createGamePage、keyPressEvent；GameController::togglePause、restartLevel | 按钮和空格连接到 togglePause；Paused 时 onTick 不调用 updateGame；restartLevel 重建当前关 | 战斗中暂停 5 秒，确认敌人、子弹、资源和用时均冻结；继续；再点击重新开始 | 暂停时仍允许建塔和升级，是否符合助教预期需口头说明 | 1% | 低 |
| 1.6 | 任一敌人到终点立即失败 | 已完成 | GameController::moveEnemy、finishGame | pathIndex 越界时立即 finishGame(false)；finishGame 有重复触发保护 | 空场开局，等待首敌到终点，确认不是扣生命而是立即失败 | HUD 固定显示“基地 1/1”，虽符合一敌即败，但可能被误解为生命系统 | 1% | 低 |
| 1.7 | 普通草地无特殊能力 | 已完成 | TerrainType::Grass；terrainFromCharacter；SpriteManager::terrain | 未识别字符和 G 映射 Grass；建造和移动逻辑对草地无额外修正 | 在草地放塔，记录基础价格并观察敌人基础速度 | 无明显代码缺口 | 1% | 低 |
| 1.8 | 黑土地建塔资源减少 30% | 已完成 | GameController::effectiveBuildCost、tryBuildTower | DarkSoil 时 qRound(baseCost × 0.70)，成功提示明确显示优惠 30% | 同一种塔先看基础价，再在黑土地建造并核对扣费；例如射手塔 90 应扣 63 | 商店按钮始终显示基础价，优惠价只在建造后提示，预览不够直观 | 1% | 低 |
| 1.9 | 石地不能建塔 | 已完成 | GameController::tryBuildTower | terrain 为 Stone 时直接返回并提示“石地不能建造防御塔” | 在多关石地尝试六种塔，确认不扣资源、不生成对象 | 石地和冰面素材色差较小，可能先造成辨识困难 | 1% | 低 |
| 1.10 | 冰面加速敌人且强化减速 | 已完成 | Enemy::movementMultiplier；GameController::moveEnemy | 冰面基础倍率 1.35；Slow 强度在冰面乘 1.5，并限制上限 0.80 | 对比同类敌人在草地、冰面的移动；让减速弹在冰面命中并观察更强减速 | tickEffects 的 onIce 参数本身未使用，真正效果位于 movementMultiplier；答辩时需讲清 | 1% | 低 |
| 1.11 | 成对传送门传送到另一格 | 已完成 | LevelConfig::portalPairs；LevelConfigLoader::load；GameController::handlePortal | 第二关配置一对 (3,4) 与 (7,4)；到达门格后重设 position 和 pathIndex，并设 0.8 秒冷却 | 第二关不建塔或只建墙，观察敌人从入口门瞬移到出口门且继续前进 | 只有第二关含传送门；配置端点未做严格越界及“必须位于路线”校验 | 1% | 中 |

### 需求点 2：资源与建造系统（10%）

| 子项 | 要求内容 | 状态 | 对应源文件、类和关键函数 | 静态代码证据 | 人工运行验证步骤 | 当前缺陷或潜在风险 | 对评分影响 | 修复成本 |
|---|---|---|---|---|---|---|---|---|
| 2.1 | 一种资源并在界面实时显示 | 已完成 | GameController::m_resources、emitHud；MainWindow::connectController | HUD 文本包含资源数；建造、击杀、作弊、资源塔和自然增长均会更新该值 | 观察开局资源、建造扣费、击杀奖励和作弊增加是否即时反映 | HUD 约每 0.12 秒刷新，不是每次 tick；肉眼应足够 | 2% | 低 |
| 2.2 | 资源自动增长或由特定塔产生 | 已完成 | GameController::updateGame、addResources；ResourceTower::performAction | 每秒自然增加 2；资源塔每 5 秒起产出 20，升级后产量和频率提高 | 暂停前后观察自然增长；建资源塔并等待两次产出 | 资源塔产出提示会覆盖其他战斗消息，可能影响关键反馈 | 2% | 低 |
| 2.3 | 每种塔费用不同 | 已完成 | towerBaseCost；MainWindow::makeTowerButton | 六类费用分别为 90、120、150、180、110、70，并显示在商店按钮 | 逐个选择并核对商店和实际扣费 | 费用硬编码在 enums.h，未从配置读取，但该子项不要求配置化 | 2% | 低 |
| 2.4 | 资源不足不能建并有反馈 | 已完成 | GameController::tryBuildTower | 资源小于 effectiveBuildCost 时不创建塔并显示所需与当前资源 | 花光资源后尝试最贵塔，确认对象数和资源不变 | 商店按钮不会随资源不足禁用，只在点击格子后反馈；仍满足“提示或明显反馈” | 2% | 低 |
| 2.5 | 格子占用和状态限制 | 已完成 | GameController::towerAt、tryBuildTower | 已有塔、石地、普通塔上路径、墙上门均分别拦截；重复放置不会扣费 | 同格重复建造；攻击塔放路径；墙放传送门；检查所有提示 | 墙可建在非路径格；所有靠近路线的塔都可能阻挡敌人，规则边界需解释 | 2% | 低 |

### 需求点 3：防御塔系统（20%）

| 子项 | 要求内容 | 状态 | 对应源文件、类和关键函数 | 静态代码证据 | 人工运行验证步骤 | 当前缺陷或潜在风险 | 对评分影响 | 修复成本 |
|---|---|---|---|---|---|---|---|---|
| 3.1 | 普通射手塔：周期单体子弹 | 已完成 | ShooterTower::performAction；GameController::launchProjectile、impactProjectile | 获取射程内最靠前目标，发射 Normal 子弹，命中只伤 target | 建一座射手塔，观察多个敌人时每发只减少一个敌人生命 | 子弹为追踪弹而非直线弹道，评分通常可接受 | 3% | 低 |
| 3.2 | 减速塔：限时减速 | 已完成 | SlowTower::performAction；Enemy::applyEffect、movementMultiplier | 命中造成 Slow，持续 2.6 秒、强度 0.35；到期由 tickEffects 删除 | 观察命中、减速图标、速度下降及约 2.6 秒后恢复 | 效果时长无数值倒计时，仅有图标；展示时需口头说明 | 3% | 低 |
| 3.3 | 范围塔：命中后范围伤害 | 已完成 | SplashTower::performAction；GameController::impactProjectile | 火箭命中后遍历敌人，半径 0.9 格内全部受 Splash 伤害并灼烧 | 聚集至少三敌，比较范围内外生命条 | 缺少爆炸范围圈或瞬时爆炸动画；逻辑证据充分，视觉反馈偏弱 | 3% | 中 |
| 3.4 | 穿透塔/激光塔：命中多个敌人 | 已完成 | LaserTower::performAction；GameController::applyLaserImpact | 以塔到目标的线段投影计算 22 像素带状范围，最多命中 5 个敌人 | 等敌人排成一线后观察多条生命同时下降及眩晕 | 当前只显示一枚高速激光弹，没有持续激光束，答辩时可能不易看出穿透 | 3% | 中 |
| 3.5 | 资源塔定期产资源 | 已完成 | ResourceTower::performAction、upgrade | 不直接攻击；周期 addResources；canAttack 返回 false | 单独建资源塔，计时两次产出；升级后比较产量和间隔 | performAction 依赖塔基类通用冷却，逻辑正确 | 3% | 低 |
| 3.6 | 防御墙高生命、阻挡或延缓敌人 | 已完成 | WallTower；GameController::blockingTowerFor、updateEnemies | 墙生命 900，可建路径；敌人靠近塔后停止移动并每 0.8 秒攻击 | 在路径格放墙，观察敌人停止、墙生命下降及最终消失 | blockingTowerFor 对所有塔生效，不仅墙；墙的独特性主要是可放路径和高生命 | 3% | 中 |
| 3.7 | 显示塔图像、状态或简单动画 | 已完成 | SpriteManager::tower；GameController::createTowerVisual、updateTowerVisual、emitSelectedTowerInfo | 六类 PNG、生命条、等级缩放和选中信息均有代码 | 建齐六塔，确认图标不同、生命条可见、升级后尺寸和 Lv 文本变化 | 无帧动画；题目允许“图像、状态或简单动画”三者之一，因此不构成缺失 | 1% | 低 |
| 3.8 | 攻击间隔或技能冷却 | 已完成 | Tower::tick、m_cooldownRemaining、m_attackInterval；GameController::m_activeSkillCooldown | 每塔有独立攻击/生产冷却；主动技能 22 秒冷却并在 HUD 信息中显示 | 观察六塔攻击节奏；连续点击主动技能确认被拒绝并显示剩余秒数 | 主动技能冷却是全局值，不是每座减速塔独立值；可选项展示时需说明 | 1% | 中 |

### 需求点 4：敌人与波次系统（15%）

| 子项 | 要求内容 | 状态 | 对应源文件、类和关键函数 | 静态代码证据 | 人工运行验证步骤 | 当前缺陷或潜在风险 | 对评分影响 | 修复成本 |
|---|---|---|---|---|---|---|---|---|
| 4.1 | 普通敌人 | 已完成 | NormalEnemy | 生命 110、速度 0.75、奖励 12；普通描述和独立素材 | 第一波观察普通敌人的生命和速度基准 | 没有敌人类型文字标签，只能靠外形与数值表现识别 | 2% | 低 |
| 4.2 | 快速敌人：快且低生命 | 已完成 | FastEnemy | 生命 70、速度 1.30，明确快于且脆于普通敌人 | 第二波让快速与普通并行比较 | Fast 与分裂产生的 Minion 使用相同 PNG，但尺寸不同；强制六类中 Fast 本身仍与其他五类不同 | 2% | 低 |
| 4.3 | 重甲敌人：高生命且慢 | 已完成 | HeavyEnemy | 生命 330、速度 0.43 | 第三波比较重甲与普通的生命条下降和位移 | 名称“重甲”但没有独立护甲减伤，仅题目要求的高血慢速已满足 | 2% | 低 |
| 4.4 | 抗性敌人：减免某类攻击/效果 | 已完成 | ResistantEnemy::damageMultiplier、effectMultiplier | Splash/Burn 伤害倍率 0.55，其他伤害 0.85，状态时长和强度倍率 0.45 | 用范围塔、减速塔分别攻击普通与抗性敌人并比较 | 没有数值浮字，抗性效果仅靠生命和速度差，肉眼可能不明显 | 2% | 中 |
| 4.5 | 分裂/召唤敌人 | 已完成 | SplitEnemy::onDeath；GameController::queueMinions、flushPendingSpawns | 死亡时把两个 Minion 加入待生成队列，避免遍历中修改敌人容器 | 击杀 Split，确认原位置生成两个小型敌人并继续沿当前 pathIndex 前进 | 作弊清除也会触发分裂，因此 CLEARWAVE 后仍可能出现小怪；演示时易误判 | 2% | 低 |
| 4.6 | Boss：高生命且有特殊能力 | 已完成 | BossEnemy::BossEnemy、updateSpecial | 生命 1350、护盾 320；每 6.5 秒恢复 140 护盾并召唤一个 Minion | 最后一波保留 Boss 至少 8 秒，观察护盾条恢复、召唤和消息 | Boss 特技需要等待；若火力过高会在首次触发前被击杀 | 2% | 低 |
| 4.7 | 至少 5 波且敌人种类、数量或速度变化 | 已完成 | config/levels.json；GameController::updateWave | 三关各 5 波；每波列表、数量和 spawnIntervalMs 均有变化；六个必需类型每关均出现 | 完整打一关或反复清敌，确认 1/5 至 5/5 及波间切换 | CLEARWAVE 只杀当前已生成敌人，不跳过尚未生成的本波队列，快速演示需要多次输入 | 2% | 低 |
| 4.8 | 当前波次显示在界面 | 已完成 | GameController::emitHud | HUD 显示“波次 current/total”，当前索引按 qMin 处理最终波 | 观察每波结束后 HUD 从 1/5 递增到 5/5 | 波间 2.2 秒等待时 HUD 已显示下一波，但消息写“下一波即将到来”，可接受 | 1% | 低 |

### 需求点 5：攻击、子弹与效果系统（15%）

| 子项 | 要求内容 | 状态 | 对应源文件、类和关键函数 | 静态代码证据 | 人工运行验证步骤 | 当前缺陷或潜在风险 | 对评分影响 | 修复成本 |
|---|---|---|---|---|---|---|---|---|
| 5.1 | 子弹或攻击对象从塔发出并命中敌人 | 已完成 | Projectile；GameController::launchProjectile、createProjectileVisual、updateProjectiles | 每次攻击创建独立 Projectile 和 PNG；保存 origin、position、targetId，并逐 tick 追踪目标 | 建射手、减速、范围、激光塔，逐一观察对应弹体 | 弹体没有最大生存时间；但速度显著高于敌人且目标死亡时会立即过期，正常路径下不会无限存在 | 3% | 低 |
| 5.2 | 碰撞/命中判定并减少生命 | 已完成 | GameController::updateProjectiles、impactProjectile；Enemy::applyDamage | 当弹体到目标距离小于单帧步长或 6 像素时命中；伤害先扣盾再扣生命 | 观察弹体接触、血条缩短；用 Boss 验证先扣盾 | 属于“到目标点”命中判断，不是通用包围盒碰撞；仍符合“碰撞或命中判定” | 2% | 低 |
| 5.3 | 敌人靠近塔可伤塔，塔生命为 0 消失 | 已完成 | GameController::blockingTowerFor、updateEnemies、removeDeadTowers；Tower::takeDamage | 敌人距塔小于约 1.12 格时停下并周期伤害；dead tower 的视觉和 unique_ptr 被删除 | 在路径放墙并等待被打死；确认生命条下降、塔消失、敌人继续移动 | 选中的塔死亡后选择框没有立即隐藏，会残留到下一次选塔或重开，属明确小型 UX 缺陷 | 3% | 低 |
| 5.4 | 至少三种状态效果 | 已完成 | EffectType；SlowTower、SplashTower、LaserTower；Enemy::applyEffect | Slow、Burn、Stun 三种效果均有来源、参数和不同逻辑；Burn 周期伤害，Stun 令速度为 0 | 逐种触发并观察图标、位移和生命变化 | 同一敌人同时有多效果时只显示一个优先图标，无法同时展示全部状态 | 3% | 中 |
| 5.5 | 状态有持续时间、到期自动失效；多攻击对象并存正常 | 已完成 | EffectInstance::remaining；Enemy::tickEffects；GameController::m_projectiles、enemyById、erase_if | 每 tick 减 remaining 并 erase_if；弹体只保存 targetId，不保存易悬空 Enemy 指针；过期弹体及时删除 | 同时建多塔制造大量弹体；观察 Slow/Stun 到期恢复，Burn 停止扣血；连续运行 5 分钟 | 目前只有自动 smoke，没有高并发长时运行数据；逻辑安全仍需压力验证 | 4%（持续时间 2% + 多对象 2%） | 中 |

### 需求点 6：关卡、配置与进度系统（10%）

| 子项 | 要求内容 | 状态 | 对应源文件、类和关键函数 | 静态代码证据 | 人工运行验证步骤 | 当前缺陷或潜在风险 | 对评分影响 | 修复成本 |
|---|---|---|---|---|---|---|---|---|
| 6.1 | 至少 3 关且关卡有区别 | 已完成 | config/levels.json；MainWindow::createLevelPage | 三关均为 6×10，但路线长度、初始资源、地形、传送门、敌人数和间隔不同 | 逐关进入，截图地图、初始资源和波次差异 | 三关均使用同一套塔属性，差异主要来自地图和波次，已满足“至少一类区别” | 3% | 低 |
| 6.2 | 至少一类数据从配置文件读取 | 已完成 | LevelConfigLoader::load；CMakeLists.txt post-build；resources.qrc | 外部 config/levels.json 读取地图、地形、路线、门、初始资源、波次和间隔；构建后复制到 exe/config | 在项目副本中更改第一关初始资源后运行，确认 HUD 变化；再恢复副本 | 外部文件存在但 JSON 语法错误时不会回退到 qrc 内置副本；只在“打不开”时回退。字段语义校验也较弱 | 2% | 中 |
| 6.3 | 通关后进入下一关或返回选关 | 已完成 | MainWindow::createResultPage、connectController | 胜利时显示“进入下一关”；最后一关隐藏该按钮；结果页始终提供返回选关和重新挑战 | 胜利后分别测试下一关、重试、返回选关；最后一关确认下一关按钮隐藏 | 需要人工真实胜利才能覆盖，现有 smoke 未触发 | 2% | 低 |
| 6.4 | 记录每关通关结果 | 已完成 | GameController::saveResult、levelProgressText；QSettings | 保存 lastWon、lastTime、lastScore；胜利另存 completed、bestTime、bestScore，并在选关页显示 | 通关后退出程序再启动，确认“已通关”和最佳数据仍存在；失败后确认不会清除 completed | QSettings::sync 结果未检查；极端受限用户配置环境下可能静默保存失败 | 2% | 中 |
| 6.5 | 作弊码立即实现某功能 | 已完成 | GameController::applyCheatCode；MainWindow 的 QLineEdit | MONEY1000 增加 1000 资源；CLEARWAVE 对当前已生成敌人造成致死伤害；未知码有提示 | 测试两个有效码、大小写和空格、一个无效码 | CLEARWAVE 名称容易被理解为“整波跳过”，实际不处理尚未生成的敌人 | 1% | 低 |

### 需求点 7：用户体验与程序性能（15%）

| 子项 | 要求内容 | 状态 | 对应源文件、类和关键函数 | 静态代码证据 | 人工运行验证步骤 | 当前缺陷或潜在风险 | 对评分影响 | 修复成本 |
|---|---|---|---|---|---|---|---|---|
| 7.1 | 界面友好，资源、生命、波次、关卡清晰 | 部分完成 | MainWindow::createGamePage；GameController::emitHud；styles/app.qss | HUD 同时显示资源、基地、波次、关卡、用时、FPS；右侧商店和控制区分区明确 | 在 100%、125%、150% DPI 下检查 1120×700 最小窗口，无裁切、重叠或过小字体 | “基地 1/1”始终固定；商店不预览黑土地折扣；未做多 DPI 实测 | 3% 仍依赖人工视觉验收 | 中 |
| 7.2 | 塔、敌人、子弹、状态效果视觉清晰 | 部分完成 | resources.qrc；SpriteManager；create/update Visual 系列函数 | 25 个运行时 PNG 均存在且可解码；塔、六类敌人、四弹体、三效果均有映射和层级 | 建齐六塔并观察六敌、四弹体、三状态；检查暗色背景上的可见性 | 石地和冰面色差很小；Fast 与 Minion 同图仅尺寸不同；激光穿透和范围爆炸视觉不强；多状态只显一个图标 | 3% 有展示扣分风险 | 中 |
| 7.3 | 鼠标、放置、选塔、暂停自然且不易误操作 | 部分完成 | MainWindow::eventFilter、makeTowerButton；GameController::handleSceneClick、togglePause | 选中按钮有动态属性样式；非法放置均有文字反馈；暂停按钮和空格均可用 | 快速单击、双击、窗口缩放、暂停时建塔、资源不足等组合测试 | 双击升级与单击建造事件顺序可能导致意外升级；选中塔死亡后选择框残留 | 2% | 低 |
| 7.4 | 胜负、受伤、塔攻击等关键事件反馈明显 | 部分完成 | resultReady；健康条/护盾条；弹体；showMessage | 胜负独立页面；攻击有弹体；受伤通过血条；塔被攻击/摧毁、Boss 技能、波次完成均有消息 | 依次观察胜利、失败、敌人受伤、塔攻击、塔被毁、Boss 护盾和波次切换 | 消息共用单行，频繁事件可覆盖；无受击闪烁、伤害数字、爆炸动画或音效 | 2% | 中 |
| 7.5 | 普通场景稳定 30 FPS 以上 | 无法验证 | GameController::GameController、onTick、emitHud；SpriteManager::load | 单一 PreciseTimer 16 ms；delta 上限 50 ms；Pixmap 有尺寸缓存；HUD 显示 tick 频率 | 正常打一关和高敌量场景各运行 3 分钟，记录 HUD 最低 FPS；同时拖动窗口、连续建塔 | HUD 的 FPS 是计时器 tick 频率，不是严格渲染帧率；2.3 秒 smoke 不足以证明稳定 30 FPS | 3% 全部仍有运行验收风险 | 中 |
| 7.6 | 不频繁崩溃，无明显泄漏或无效对象堆积 | 部分完成 | unique_ptr 容器；clearSceneAndObjects、processDeadEnemies、removeDeadTowers、updateProjectiles、removeVisual | Tower、Enemy、Projectile 由 unique_ptr 管理；死亡/过期用 erase_if；QGraphicsItem 删除路径明确；只有一个持续 QTimer | 连续运行 15 分钟，反复重开和切三关；观察任务管理器内存；用调试器覆盖胜负和大量分裂/Boss 召唤 | 尚无 Sanitizer、Dr.Memory 或长时数据；SpriteManager 缓存不清空但键空间有限；主计时器在菜单/结果页仍持续 60Hz 唤醒 | 2% 部分依赖动态证据 | 中 |

## 4. 面向对象与代码结构

- Tower 是抽象基类，performAction 和 description 为纯虚函数，六个派生类通过 makeTower 工厂和 unique_ptr 多态调用。
- Enemy 是抽象基类，damageMultiplier、effectMultiplier、updateSpecial 和 onDeath 为虚函数；抗性、分裂和 Boss 通过覆写实现差异。
- LevelConfig、WaveConfig 封装配置数据；SpriteManager 封装资源映射和缓存；GameController 集中游戏状态机、战斗、波次和对象生命周期。
- Projectile 目前是数据类而非继承体系，但不同 AttackKind 由 impactProjectile 多态式分派；评分原文不强制子弹继承。
- 代码没有 QPainter 或 paintEvent；地图、塔、敌人、弹体和效果使用 QGraphicsPixmapItem，生命条和选择框使用 QGraphicsRectItem。

## 5. 指定专项检查

### 5.1 开始、游玩、胜利、失败

结论：代码链完整。MainWindow 页面创建与 GameController::finishGame 之间有直接回调；胜利由所有波次结束触发，失败由任一敌人 pathIndex 越界触发。自动 smoke 没有覆盖胜负，必须按第 8 节人工测试。

### 5.2 暂停、继续、重新开始

结论：已实现。Paused 时只停止 updateGame，主计时器仍运行以保持 UI；重新开始清空对象、重建地图并复位资源、波次、用时、技能冷却和分数。风险是暂停仍允许建塔/升级，需要在答辩中解释这是设计选择。

### 5.3 五种地形和传送门

结论：逻辑齐全。三关地形字符统计如下：

- meadow_gate：D 6、G 46、I 1、S 7。
- portal_pass：D 6、G 45、I 3、P 2、S 4；含一对门。
- frozen_core：D 7、G 45、I 2、S 6。

所有地图尺寸、路线点边界和普通相邻跳转均通过静态数据检查；第二关唯一非相邻跳转正好对应传送门对。

### 5.4 六类防御塔和六类敌人

结论：六塔和六个必需敌人均有独立枚举、构造类、工厂分支、数值和素材映射。Minion 是第七个内部敌人类型，不替代六个必需类型。Fast 与 Minion 共用图片但显示尺寸不同，不影响必需六类互相区分。

### 5.5 五个波次、三个关卡

结论：三关各 5 波。每关的必需敌人集合均为 boss、fast、heavy、normal、resistant、split；每波敌人数分别为：

- 第一关：6、6、5、6、6。
- 第二关：6、6、5、6、6。
- 第三关：7、6、6、7、7。

### 5.6 子弹、碰撞、塔被攻击和死亡

结论：均有实现。弹体按 targetId 追踪，避免持有 Enemy 裸指针；目标死亡时弹体过期；命中后区分单体、范围和激光穿透。敌人靠近塔停止并周期攻击；塔生命归零后视觉和对象被移除。已发现的小缺陷是选中塔死亡后选择框未同步隐藏。

### 5.7 至少三种有持续时间的状态

结论：Slow、Burn、Stun 三种均有 remaining；applyEffect 合并同类效果并延长到较大剩余时间，tickEffects 到期 erase。Burn 每 0.5 秒结算；Slow 改速度；Stun 令移动倍率为 0。多状态同时存在时只显示一个图标。

### 5.8 配置读取与异常回退

结论：部分完成。

- 正常路径：优先读取 applicationDirPath/config/levels.json。
- 文件打不开：回退到 qrc 的 :/config/levels.json。
- 外部文件存在但 JSON 语法错误：直接返回空关卡，不再尝试 qrc，程序弹“配置错误”但仍显示没有关卡按钮的页面。
- 语义校验只检查 rows≥5、columns≥9、path 非空、waves≥5；未严格校验路径连续、端点边界、传送门合法、每波非空和字段类型。
- 未知敌人字符串静默变成 Normal；未知地形字符静默变成 Grass，可能掩盖配置拼写错误。

### 5.9 通关记录与作弊码

结论：已实现。QSettings 保存最后结果、用时、分数、通关标记、最佳用时和最高分；选关按钮显示进度。MONEY1000 和 CLEARWAVE 均有代码和 UI 输入。需注意 CLEARWAVE 不清理尚未生成的敌人。

### 5.10 对象释放、悬空指针、重复计时器和泄漏风险

结论：静态设计较安全，但仍需动态验证。

- 所有 Tower、Enemy、Projectile 使用 unique_ptr。
- 弹体只保存目标 ID，每帧重新查找，不保存 Enemy 指针。
- m_level 指向初始化后不再扩容的 m_levels 元素，当前生命周期安全。
- SpriteVisual 的裸指针由 QGraphicsScene 创建；单对象死亡走 removeItem + delete，整关清理走 scene.clear。
- clearSceneAndObjects 先销毁逻辑对象再 scene.clear；逻辑析构不访问视觉指针，因此未见明显双删。
- 游戏只有一个持续 QTimer，16 ms；smoke 场景和 fitInView 使用有限 singleShot，不是重复游戏计时器。
- 效果、弹体、死敌、死塔和待生成队列都有清理路径。
- SpriteManager 缓存没有淘汰，但当前请求尺寸组合有限，属于有界常驻缓存。
- 主计时器在 Idle、Paused、Won、Lost 时仍持续触发，属于低级 CPU 唤醒风险，不是内存泄漏。
- 选中塔死亡后 selectionIndicator 残留是视觉状态错误，不是悬空指针。

### 5.11 资源相对路径

结论：游戏运行时资源满足相对/内嵌访问要求。

- 所有 PNG、QSS 和内置 JSON 使用 Qt 资源路径 :/...。
- 可编辑 JSON 使用 applicationDirPath/config/levels.json，相对于可执行文件。
- CMake 只用 CMAKE_CURRENT_SOURCE_DIR 和 TARGET_FILE_DIR 复制配置。
- 源码中未发现用户盘符绝对运行路径。
- 用QtCreator打开.bat 硬编码 D:/Qt/Tools/QtCreator/bin/qtcreator.exe；它是本机开发辅助脚本而非游戏资源，但换电脑会失效。
- README 中的 C:/Users/... 只是构建路径示例，不参与运行。

### 5.12 bin 在未安装 Qt 的 Windows 10/11 上运行

结论：本机模拟通过，异机仍属于部分验证。

- bin 含 GridGuard.exe、Qt6Core/Gui/Widgets 6.5.3、三个 MinGW 运行库、qwindows、qoffscreen 和 Windows Vista style 插件。
- 可执行文件和依赖均为 x86-64；bin/config/levels.json 与源码配置 SHA-256 一致。
- 清除 PATH 中 Qt/MinGW 后，离屏插件和原生 qwindows 插件两次 smoke 均退出码 0，说明没有依赖本机 D:/Qt 环境。
- 当前主机报告 Windows 10.0.26200 x64；尚未在真正全新 Windows 10 64 位和 Windows 11 机器上双击测试，不能把本机模拟等同于完整异机验收。
- qwindows 的直接依赖均为项目内 Qt/MinGW DLL 或 Windows 系统 DLL；未发现额外第三方 DLL 缺口。

### 5.13 报告、视频、自评表和最终提交目录

结论：提交材料明显未完成，是当前最高风险。

1. 报告：doc 只有 项目设计说明.md。它已包含模块关系、运行流程、完成功能和素材来源，但没有运行截图；AI 声明的“本人独立贡献”仍是提示占位文字，必须由本人如实完成。建议最终导出为便于助教打开的 PDF，但原要求未强制格式。
2. 视频：项目没有 presentation 目录。上级目录的 大作业讲解.mp4 时长 607.36 秒，即 10:07.36，且从上下文看是课程讲解参考，不是本项目展示；即使当作提交视频，也超过 PDF 规定的 6 分钟。
3. 自评表：现有 大作业自评表_2025100000_明鑫.xlsx 位于项目上级，不在 doc。表内 55 个功能项中仅前 24 项填 1、后 31 项填 0，与当前代码明显不一致；文件名中的学号/姓名也像模板占位。valid.py 只检查文件名正则和 B 列是否为 0/1，不能证明身份和真实性。PDF 明确规定缺少或不真实自评表时按实际卷面分 ×0.6。
4. 最终目录：当前有 src、doc、bin，但没有 presentation；也没有以“学号_姓名”命名的最终压缩包。
5. src 自包含性：当前 CMakeLists.txt、resources.qrc、assets、config 和 styles 位于项目根而不在 src 文件夹内。若严格按 PDF 将压缩包内的 src 当作“完整工程文件”，单独的 src 目录无法直接 CMake 编译。这是提交组织风险，不是源码功能缺失。
6. requirements：项目内没有 requirements 目录；评分 PDF 本身并未要求随作业提交，因此这只影响本次审计路径，不是评分材料缺失。
7. bin：11 个文件，已通过本机受限 PATH 的 qwindows 启动测试，当前状态相对完整。

## 6. 可选需求快照

| 可选项 | 状态 | 证据与风险 |
|---|---|---|
| 关卡编辑器（5%） | 未完成 | 没有编辑器 UI、保存地图或加载用户编辑地图的实现 |
| 塔消耗资源升级、属性提升、等级可区分（2%） | 已完成 | GameController::upgradeSelectedTower；Tower::upgrade；等级 1-3，属性提升且精灵缩放 |
| 至少三类塔有不同升级效果（2%） | 已完成 | 普通攻击塔使用通用伤害/射程/间隔路线；ResourceTower 额外提升产量和生产频率；WallTower 采用高生命专属路线 |
| 至少一种主动技能并有冷却（1%） | 已完成 | 减速塔“全体冻结”，对全体施加 Slow 和 Stun，22 秒冷却 |

功能总分封顶 100%。可选项的静态证据可用于弥补必选项的展示或运行扣分，但不能替代自评表、报告和视频要求。

## 7. A：按“得分收益 / 修复成本”排序的任务列表

以下仅是审计建议，本次没有执行任何修复。

1. 正确填写自评表、替换真实学号姓名，并放入最终 doc。收益极高：避免整卷 ×0.6；成本低。
2. 完成 AI 工具使用声明中的本人独立贡献，确保本人能解释关键代码。收益极高：避免学术诚信风险；成本低。
3. 整理最终“学号_姓名”压缩包，使其明确包含可独立编译的 src、doc、自评表、bin、presentation。收益极高；成本中。
4. 录制不超过 6 分钟的本项目展示视频并放入 presentation。收益高，直接关系展示评分和提交完整性；成本中。
5. 用另一台未安装 Qt 的 Windows 10/11 电脑双击 bin/GridGuard.exe，并记录截图。收益高；成本低。
6. 在项目副本上补测“外部 JSON 语法损坏”并修正为也能回退内置配置，同时增强字段语义校验。收益中高，可避免答辩开局无关卡；成本中。
7. 完成第 8 节的 10 分钟人工冒烟并留存截图，尤其覆盖真实胜负、传送门、塔死亡、三状态到期和存档。收益中高；成本低。
8. 做 15 分钟压力运行和内存观察，确认普通及高敌量场景 HUD 不低于 30 FPS。收益中，覆盖需求点 7 的 5 分；成本中。
9. 强化石地/冰面、激光穿透、范围爆炸和多状态视觉区分，并在选中塔死亡时隐藏选择框。收益中，偏展示分；成本中。
10. 处理开发辅助脚本的 D:/Qt 绝对路径，或在提交说明中明确由 Qt Creator 打开 CMakeLists。收益低；成本低。

## 8. B：答辩时最可能翻车的前 5 个问题

1. “请现场解释继承和多态在哪里。”如果不能讲清 Tower::performAction、Enemy 的四个虚函数、工厂和 unique_ptr，容易被认为只会运行不会解释。
2. “为什么修改了 levels.json 后程序没有关卡？”外部 JSON 只要能打开但语法错误，就不会回退内置配置。
3. “请快速展示胜利和五波。”CLEARWAVE 只杀已生成敌人，分裂敌人还会生成小怪，不能一次跳过整波；演示节奏容易失控或超过视频时长。
4. “防御墙与普通塔的阻挡区别是什么？”当前所有靠近路径的塔都可让敌人停下并被攻击；墙的主要差异是可放路径和高生命，需要准确解释。
5. “提交包为什么没有 presentation，src 为什么不能单独编译，自评表为什么还是模板数据？”这三项是当前最直观、最可能直接造成提交或倍率扣分的问题。

## 9. C：推荐的 10 分钟人工冒烟测试流程

请在项目副本或最终 bin 上执行；涉及配置破坏的步骤只对测试副本操作。

| 时间 | 操作 | 通过标准 |
|---|---|---|
| 0:00-0:40 | 启动程序，查看开始页、帮助页、选关页 | 页面无裁切；帮助内容可读；三关和历史进度显示 |
| 0:40-1:40 | 进入第一关，输入 MONEY1000；在草地、黑土地、石地、路径和同一格尝试建塔 | 黑土地正确打七折；石地、非法路径和重复格拒绝且不扣资源 |
| 1:40-2:40 | 建齐射手、减速、范围、激光、资源、墙 | 六图标可区分；费用不同；资源塔产出；墙可放路径 |
| 2:40-4:10 | 让敌人进入火力区，观察普通、快、重甲、抗性、分裂和 Boss | 单体、范围、穿透；Slow/Burn/Stun 出现并到期；Split 生成两小怪；Boss 恢复盾并召唤 |
| 4:10-4:50 | 让墙被攻击直至死亡 | 墙生命下降、归零消失、敌人继续；记录选择框是否残留 |
| 4:50-5:30 | 战斗中暂停 5 秒，再继续；随后重新开始 | 暂停时移动、资源、用时均冻结；继续无跳帧；重开全部复位 |
| 5:30-6:30 | 进入第二关，观察一名敌人通过传送门 | 敌人从 (3,4) 跳到 (7,4) 并继续；没有来回传送或越界 |
| 6:30-7:30 | 使用 CLEARWAVE 多次完成五波，触发胜利；点击下一关和返回选关 | 结果页正确；下一关可进入；进度页显示已通关、最佳用时和分数 |
| 7:30-8:20 | 重开第二关不建塔，等待首敌到终点 | 任一敌人到终点立即失败；失败页的重试和选关可用 |
| 8:20-9:00 | 退出并重新启动 | QSettings 通关记录仍在；bin 从自身 config 读取，不依赖 Qt 安装目录 |
| 9:00-10:00 | 高敌量战斗并观察 HUD FPS、任务管理器内存；在测试副本移走外部 config 后启动 | 普通场景 FPS 不低于 30；内存不持续无界增长；缺少外部 config 时内置 qrc 回退可进入三关 |

额外单独测试：在测试副本中把外部 levels.json 改成非法 JSON 后启动。当前预期会弹配置错误且无关卡，这用于确认第 5.8 节记录的真实缺陷，不应在答辩现场临时尝试。

## 10. 审计结论

从代码证据看，需求点 1-6 的全部评分子项都有明确实现；需求点 7 的界面、视觉、交互、反馈、FPS 和长期稳定性仍需要人工运行证据，其中稳定 30 FPS 当前无法由静态审计证明。项目的技术主体已经达到可验收状态，但最终提交材料目前不合格：缺 presentation、视频不符合时长/用途、自评表位置和内容不真实、AI 独立贡献未填写、src 目录不自包含。

本报告到此停止；未根据审计结果修改任何实现。
