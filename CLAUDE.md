# CLAUDE.md

## 项目概述

基于Qt的校园饮食决策APP——"今天吃什么"。将饮食选择游戏化为"抽卡"体验，结合模糊搜索、UCB1推荐、搭配系统、热量管理。

## 环境 & 构建

- Qt 6.10.1：`D:/Qt/6.10.1/mingw_64/`
- MinGW 13.1.0：`D:/Qt/Tools/mingw1310_64/`
- CMake 3.30：`D:/Qt/Tools/CMake_64/`

### CMake（推荐）

```bash
cd "d:/PKU/canteen2.0"
PATH="/d/Qt/Tools/CMake_64/bin:/d/Qt/Tools/mingw1310_64/bin:/d/Qt/6.10.1/mingw_64/bin:/usr/bin:/bin"

# 配置
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="D:/Qt/6.10.1/mingw_64" -DCMAKE_BUILD_TYPE=Release -B build

# 编译
cmake --build build --config Release

# 部署
cp dishes.json synonyms.json zones.json distances.db distances.json daily_records.json user.json rating_dates.json build/
cp background.png title.png bento.png ribbon.png build/
cp lion_obese.png lion_slim.png map_origin.jpg start_page_background.png build/
"D:/Qt/6.10.1/mingw_64/bin/windeployqt.exe" build/what_to_eat.exe
```

### qmake（备用）

```bash
cd "d:/PKU/canteen2.0"
PATH="/d/Qt/Tools/mingw1310_64/bin:/d/Qt/6.10.1/mingw_64/bin:/usr/bin:/bin"
"D:/Qt/6.10.1/mingw_64/bin/qmake.exe" "what_to_eat.pro" -o Makefile
"D:/Qt/Tools/mingw1310_64/bin/mingw32-make.exe" -f Makefile
# 部署同上，将 build/ 替换为 release/
```

## 技术栈

- C++17，Qt 6.10 Widgets（手写布局，无 .ui 文件）
- JSON 本地存储（QJsonDocument）
- CMake 构建（qmake 备用）

## 编码规范

- 类名 PascalCase，函数名 camelCase，成员变量 m_ 前缀
- 头文件 #ifndef / #define / #endif
- QVector/QMap/QString > STL
