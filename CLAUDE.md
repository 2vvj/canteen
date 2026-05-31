# CLAUDE.md

## 项目概述

基于Qt的校园饮食决策APP——"今天吃什么"。将饮食选择游戏化为"抽卡"体验，结合模糊搜索、UCB1推荐、搭配系统、热量管理。

## 环境

- Qt 6.5.3：`D:/Qt/6.5.3/mingw_64/`
- MinGW 13.1.0：`D:/Qt/Tools/mingw1310_64/`
- Qt Creator：`D:/Qt/Tools/QtCreator/bin/qtcreator.exe`

注意：系统自带 MinGW 8.1.0（`/d/Mingw/mingw64/`）与 Qt 6.5.3 不兼容，**必须使用 Qt 自带的 MinGW 13.1.0**。

## 构建 & 部署

### 命令行（qmake）

```bash
cd "c:/Users/cfk20/canteen"
PATH="/d/Qt/Tools/mingw1310_64/bin:/d/Qt/6.5.3/mingw_64/bin:/usr/bin:/bin"

# 编译（每次 git pull 之后执行）
"D:/Qt/6.5.3/mingw_64/bin/qmake.exe" "what_to_eat.pro" -o Makefile
mingw32-make -f Makefile

# 部署（复制资源 + Qt DLL）
cp dishes.json synonyms.json zones.json distances.json background.png \
   title.png bento.png ribbon.png lion_obese.png lion_slim.png \
   map_origin.jpg start_page_background.png release/
"D:/Qt/6.5.3/mingw_64/bin/windeployqt.exe" release/what_to_eat.exe
```

注意：`.pro` 文件无改动时只需要执行 `mingw32-make`，新增/删除 `.cpp`/`.h` 后才需要重新 `qmake`。

### Qt Creator（UI 界面）

1. 打开 Qt Creator：双击 `D:/Qt/Tools/QtCreator/bin/qtcreator.exe`
2. 打开项目：File → Open File or Project → 选择 `what_to_eat.pro`
3. 配置 Kit：选择 **Desktop Qt 6.5.3 MinGW 64-bit**
4. 左下角切换到 **Release** 模式
5. 编译：点击左下角锤子图标（或 Ctrl+B）
6. 编译产物在 `release/` 目录下
7. 部署（拷贝资源 + Qt DLL）仍需命令行执行上面第 3 步

## 运行时数据文件

程序从 exe 所在目录读写以下文件：

| 文件 | 说明 |
|------|------|
| `dishes.json` | 菜品数据库 |
| `synonyms.json` | 标签同义词 |
| `zones.json` | 校园区域 |
| `distances.json` | 区域距离 |
| `user.json` | 用户设置 & 评分 |
| `daily_records.json` | 每日饮食记录（运行时生成） |
| `rating_dates.json` | 评分日期（运行时生成） |
| `eating_times.json` | 食用时间含时分（运行时生成） |

## 技术栈

- C++17，Qt 6.5 Widgets（手写布局，无 .ui 文件）
- JSON 本地存储（QJsonDocument）
- qmake 构建

## 编码规范

- 类名 PascalCase，函数名 camelCase，成员变量 m_ 前缀
- 头文件 #ifndef / #define / #endif
- QVector/QMap/QString > STL
