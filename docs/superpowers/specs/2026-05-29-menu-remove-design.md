# 今日菜单删除功能 - 设计文档

**日期**: 2026-05-29
**范围**: MealPage（mealpage.h + mealpage.cpp）

## 概述

在选菜阶段，用户可以将不想吃的菜从"今日菜单"列表中删除。

## UI

- `m_menuList`（QListWidget）每行从纯文本改为自定义 item widget
- 每行布局：菜品名（左）+ ✕ 删除按钮（右）
- 点击 ✕ 直接删除，无确认弹窗

## 删除逻辑

```
点击 ✕
  → 从 m_mealSelected 中移除该道菜
  → 如果 m_mealSelected 变空 → 清空 m_lockedRestaurant
  → recomputePhase() 重新计算阶段
  → refreshMenuList() 刷新列表
  → m_resultPanel 显示/隐藏按阶段决定
```

- 删除的菜不加入 `m_mealExcluded`（后续可再次抽到）
- 无确认弹窗

## 阶段回退（recomputePhase）

| 当前阶段 | 删除的菜角色 | 新阶段 |
|----------|-------------|--------|
| EXTRA_PHASE，核心角色缺失 | MEAT/VEGGIE/STAPLE | **MAIN_PHASE** |
| EXTRA_PHASE，核心仍完整 | BEVERAGE/SNACK | EXTRA_PHASE（不变） |
| MAIN_PHASE | 任意 | MAIN_PHASE（不变） |
| BREAKFAST_DRINK，主食被删 | STAPLE（饮品仍在） | **BREAKFAST_MAIN**（维持餐厅锁） |
| BREAKFAST_DRINK，全部删光 | 最后一道 | **BREAKFAST_MAIN**（解锁餐厅） |
| 任一阶段，全部删光 | 最后一道 | 初始阶段（解锁餐厅） |
| 含 FULL_MEAL 被删 | FULL_MEAL | **MAIN_PHASE** |

## 餐厅锁定

- 菜单清空 → 解锁（`m_lockedRestaurant.clear()`）
- 早餐主食被删但饮品保留 → 维持锁定
- 其他情况 → 维持现有锁定

## 新增函数

- `recomputePhase()` — 根据 `m_mealSelected` 内容重新判定阶段
- `removeDishFromMenu(int index)` — 删除入口，串联上述逻辑
- 自定义 `QWidget` 作为 list item（内联实现）

## 不变项

- `m_mealExcluded` 不受影响（删除≠换一个）
- `GachaWidget`、`Recommend`、`MainWindow` 等无需改动
- 确认开饭流程不变
