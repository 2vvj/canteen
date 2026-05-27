# CLAUDE.md

## 项目概述
基于Qt的校园饮食决策APP——"今天吃什么"。将饮食选择游戏化为"抽卡"体验，结合模糊搜索、UCB1推荐、搭配系统、热量管理。

## 环境 & 构建

- Qt 6.5.3：`D:/Qt/6.5.3/mingw_64/`
- MinGW 13.1.0：`D:/Qt/Tools/mingw1310_64/`

```bash
cd "c:/Users/cfk20/Desktop/程设大作业"
PATH="/d/Qt/Tools/mingw1310_64/bin:/d/Qt/6.5.3/mingw_64/bin:/usr/bin:/bin"

# 编译
"D:/Qt/6.5.3/mingw_64/bin/qmake.exe" "what_to_eat.pro" -o Makefile
"D:/Qt/Tools/mingw1310_64/bin/mingw32-make.exe" -f Makefile

# 部署
cp dishes.json synonyms.json zones.json distances.db distances.json release/
cp background.png title.png bento.png ribbon.png release/
cp lion_obese.png lion_slim.png map_origin.jpg start_page_background.png release/
"D:/Qt/6.5.3/mingw_64/bin/windeployqt.exe" release/what_to_eat.exe
```

## 技术栈
- C++17，Qt 6.5 Widgets（手写布局，无 .ui 文件）
- JSON 本地存储（QJsonDocument）
- qmake 构建

## 编码规范
- 类名 PascalCase，函数名 camelCase，成员变量 m_ 前缀
- 头文件 #ifndef / #define / #endif
- QVector/QMap/QString > STL
