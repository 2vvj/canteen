# 团队共享接口文档 —— "今天吃什么" Qt 大作业

## 一、文件归属（谁写哪些文件）

| 成员 | 文件 | 说明 |
| ------ | ------ | ------ |
| **成员C** | `dishdata.h/cpp`、`dishes.json` | 数据结构 + JSON读写 + 菜品数据 |
| **成员C** | `cardpool.h/cpp` | 卡池抽卡 |
| **成员C** | `fuzzysearch.h/cpp` | 模糊搜索 |
| **成员C** | `recommend.h/cpp` | 推荐算法 |
| **队友A** | `mainwindow.h/cpp` | 主窗口UI |
| **队友B** | `pathfinder.h/cpp` | A*寻路 |
| **队友B** | `healthtracker.h/cpp` | 热量追踪 |
| **队友B** | `radarwidget.h/cpp` | 雷达图 |
| 共享 | `what_to_eat.pro` | 项目文件，谁加新文件谁更新 |
| 共享 | `main.cpp` | 入口，已写好 |

---

## 二、共享数据结构（dishdata.h）

队友不需要理解实现，只需要知道有哪些字段可以用。

```cpp
// ===== 卡池等级 =====
enum PoolTier {
    TIER_BUDGET = 0,   // 吃土: 价格 <= 15
    TIER_MODERATE = 1, // 小资: 15 < 价格 <= 30
    TIER_SPLURGE = 2   // 放纵: 价格 > 30
};

// ===== 菜品（一道菜的所有信息）=====
struct Dish {
    QString name;           // 菜名，如 "牛肉拉面"
    QString restaurant;     // 餐厅名，如 "一食堂二楼"
    double price;           // 价格（元）
    double calories;        // 热量（kcal）
    int prepTime;           // 出餐时间（分钟）
    QStringList tags;       // 标签，如 {"面食", "肉类", "汤类"}
    QPointF location;       // 餐厅坐标 (x, y)
    double baseWeight;      // 基础推荐权重 0~10
    int drawCount;          // 已被抽次数（算法用）
    double totalRating;     // 累计评分（算法用）
    double tastyScore;      // 美味度 0~10  ← 雷达图用
    double healthScore;     // 健康度 0~10  ← 雷达图用

    PoolTier tier() const;  // 根据价格自动判断卡池等级
};

// ===== 用户数据 =====
struct UserProfile {
    QString name;
    double dailyCalorieLimit;       // 每日热量上限，默认 2500
    double todayCalories;           // 今日已摄入
    QMap<QString, int> chooseCount; // 菜名 -> 被选次数
    QMap<QString, double> ratings;  // 菜名 -> 评分(0~10)
    QStringList recentChoices;      // 最近10次选择
};

// ===== 数据读写工具（全是静态函数，直接调用）=====
namespace DishData {
    QVector<Dish> loadDishes("dishes.json");          // 加载菜品
    bool saveDishes(文件名, 菜品列表);                  // 保存菜品
    UserProfile loadUserProfile(文件名);               // 加载用户
    bool saveUserProfile(文件名, 用户数据);             // 保存用户
    QVector<UserProfile> loadAllUsers(文件名);         // 加载所有用户
    QVector<Dish> filterByTier(菜品列表, 卡池等级);     // 按价格筛选
}
```

---

## 三、你提供的 API（队友A在主窗口里调用这些）

### CardPool —— 卡池抽卡

```cpp
// 创建（需要菜品列表和用户数据）
CardPool pool(所有菜品, 用户数据);

// 设置"祈愿"关键词（加权抽取）
pool.setWishKeyword("火锅");    // 匹配的菜权重 x5

// 从指定卡池抽一张卡
Dish result = pool.draw(TIER_BUDGET);    // 吃土卡池
Dish result = pool.draw(TIER_MODERATE);  // 小资卡池
Dish result = pool.draw(TIER_SPLURGE);   // 放纵卡池
// 如果卡池为空，返回的 Dish 的 name 是空字符串 ""

// 用户确认选择后调用（触发"反重复衰减"，同一道菜越选概率越低）
pool.confirmChoice("牛肉拉面");

// 获取某个卡池的所有菜品（用于UI展示）
QVector<Dish> list = pool.dishesInPool(TIER_BUDGET);

// 设置热量惩罚系数（由队友B的 HealthTracker 调用）
pool.setCaloriePenaltyFactor(0.5);  // 0~1，越小惩罚越重
```

### FuzzySearch —— 模糊搜索

```cpp
// 搜索（所有参数一次性传入）
QVector<Dish> results = FuzzySearch::search(
    "面食",         // 用户输入的关键词
    所有菜品列表,    // m_allDishes
    QPointF(50, 0)  // 用户当前坐标（队友B的寻路模块提供）
);

// 支持的搜索方式：
// "面食" / "米饭" → 按标签匹配
// "麦丹劳" → 自动纠错匹配"麦当劳"
// "辣的" → 筛选辛辣标签
// "出餐快" → 优先短出餐时间
// "宿舍附近" → 按距离排序（调用 PathFinder 算距离）
// "健康" / "低卡" → 筛选健康标签

// 编辑距离函数（队友也可以直接用）
int dist = FuzzySearch::levenshteinDistance("麦丹劳", "麦当劳"); // → 1
```

### Recommend —— 推荐排序

```cpp
// 两种模式
enum Mode { UCB1_MODE, COLLABORATIVE_MODE };

// 对一组菜品排序，返回排序后的列表
QVector<Dish> ranked = Recommend::rank(
    候选菜品列表,    // 待排序的菜品
    用户数据,        // 当前用户
    UCB1_MODE,      // 或 COLLABORATIVE_MODE
    所有用户列表     // 协同过滤用（可选参数，UCB1模式可以不传）
);

// UCB1 模式：平衡"推荐你爱吃的"和"推荐新菜尝尝"
// 协同过滤：找口味相似的用户，推荐他们爱吃但你没吃过的
```

---

## 四、队友B 需要提供的接口

队友B 开发寻路、健康、雷达图模块。以下是建议的函数签名。

### PathFinder（寻路）

```cpp
class PathFinder {
public:
    // 计算两点之间的逻辑距离（米）
    static double calculateDistance(QPointF from, QPointF to);
    
    // 按距离排序餐厅列表
    static QVector<Dish> sortByDistance(QVector<Dish> dishes, QPointF from);
};
```

### HealthTracker（热量追踪）

```cpp
class HealthTracker {
public:
    double todayCalories() const;          // 今日已摄入
    double calorieLimit() const;           // 每日上限
    double caloriePenalty(double calories); // 热量越界的权重惩罚系数(0~1)
    // 惩罚系数用法：热量超过90%上限时，对高热量菜返回<1的值
    // 这个值传给 CardPool::setCaloriePenaltyFactor()
};
```

### RadarWidget（雷达图）

```cpp
class RadarWidget : public QWidget {
public:
    // 设置要展示的菜品，读取 tastyScore, healthScore, price, distance 四个维度
    void setDish(const Dish &dish);
};
```

---

## 五、编码约定

- 文件名用英文小写，单词间不加分隔符（例：`dishdata.h`）
- 类名用大驼峰：`CardPool`、`FuzzySearch`
- 函数名用驼峰（首字母小写）：`calculateDistance()`
- 成员变量用 `m_` 前缀：`m_allDishes`、`m_user`
- 头文件防护：

  ```cpp
  #ifndef FILENAME_H
  #define FILENAME_H
  // ... 内容 ...
  #endif
  ```

- 用 Qt 类型优先于 STL：`QString` > `std::string`，`QVector` > `std::vector`
- `#include` 自己的头文件用双引号，Qt 头文件用尖括号

---

## 六、项目文件 (.pro) 更新规则

谁新增了 `.h` / `.cpp` 文件，谁就在 `what_to_eat.pro` 的对应段落加一行。

当前内容：

```qmake
SOURCES  += main.cpp \
            mainwindow.cpp \
            dishdata.cpp \
            cardpool.cpp \
            fuzzysearch.cpp \
            recommend.cpp

HEADERS  += mainwindow.h \
            dishdata.h \
            cardpool.h \
            fuzzysearch.h \
            recommend.h
```

队友A 加上 `mainwindow.cpp`（已存在），队友B 加上 `pathfinder.cpp`、`healthtracker.cpp`、`radarwidget.cpp` 等。

---

## 七、构建命令

```bash
# 设置环境（每次新开终端都要执行）
export PATH="D:/Qt/Tools/mingw1310_64/bin:D:/Qt/6.5.3/mingw_64/bin:$PATH"

# 生成 Makefile
qmake "what_to_eat.pro" -o Makefile

# 编译
mingw32-make -f Makefile

# 首次编译后部署 DLL（只需做一次）
windeployqt release/what_to_eat.exe
```

> Qt 安装在 `D:/Qt/6.5.3/mingw_64/`，编译器在 `D:/Qt/Tools/mingw1310_64/`
