# 网格守卫（Grid Guard）

使用 C++20 与 Qt 6 Widgets 开发的 2D 网格塔防课程项目。

## 最快体验

- 双击项目根目录的 `运行游戏.bat`；或
- 直接双击 `bin/GridGuard.exe`。

`bin` 已包含 Qt 与 MinGW 运行库，不要求另一台 Windows 10/11 电脑安装 Qt。

## Qt Creator 打开方法

1. 启动 Qt Creator。
2. 点击“打开项目（Open Project）”。
3. 选择本目录的 `CMakeLists.txt`。
4. Kit 选择 **Desktop Qt 6.5.3 MinGW 64-bit**。
5. 因 Qt 6.5 MinGW 对中文构建路径支持不好，把 Build directory 改为纯英文路径，
   例如 `C:\Users\20635\Documents\Codex\QtBuild\GridGuard`。
6. 点击“配置项目（Configure Project）”。
7. 点击左下角绿色三角形，或按 `Ctrl+R` 编译运行。

也可以双击 `用QtCreator打开.bat`，它会直接用本机 Qt Creator 打开 `CMakeLists.txt`。

## 主要操作

- 点击商店中的防御塔，再点击地图合法格子建造。
- 防御墙可以放在路径上；其他塔不能放在路径上。
- 点击已建塔查看属性；双击或点击“升级”进行升级。
- 选择减速塔后可释放“全体冻结”主动技能。
- 空格暂停/继续。
- 作弊码：`MONEY1000`、`CLEARWAVE`。

## 素材与绘制方式

塔、敌人、地形、子弹和状态效果全部由 PNG 素材驱动，通过
`QGraphicsPixmapItem` 显示。项目没有使用 `paintEvent` 或 `QPainter`
手绘游戏对象。素材映射集中在 `src/game/spritemanager.cpp`，以后替换同名 PNG 即可换皮。

生命条与选中框使用 `QGraphicsRectItem` 作为界面状态提示，不属于角色或地图美术素材。

## 配置

三关地图和波次位于 `config/levels.json`。构建后 CMake 会把该文件复制到程序旁边的
`config` 文件夹；程序优先读取外部文件，读取失败时回退到 Qt 资源中的副本。

## 已知环境事项

Qt 6.5.3 MinGW 的工具链不能可靠地在中文目录中生成中间文件。本工程源码可以保留在
`桌面/大作业`，但 Qt Creator 的 Build directory 必须使用纯英文路径。项目已关闭
AUTOMOC，进一步避免中文源码路径造成自动生成文件错误。

