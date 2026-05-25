#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>
#include "MainWindow.h"

// 检查列是否存在的辅助函数
static bool columnExists(QSqlQuery &query, const QString &table, const QString &col) {
    query.exec(QString("PRAGMA table_info(%1)").arg(table));
    while (query.next()) {
        if (query.value(1).toString() == col)
            return true;
    }
    return false;
}

bool initDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    QString dbPath = QCoreApplication::applicationDirPath() + "/food_simulator.db";
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "数据库打开失败:" << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS Restaurants ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "name TEXT NOT NULL UNIQUE, "
               "dist_dorm INTEGER, "
               "dist_library INTEGER, "
               "dist_building INTEGER)");

    query.exec("CREATE TABLE IF NOT EXISTS Dishes ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "restaurant_id INTEGER, "
               "name TEXT NOT NULL, "
               "taste REAL DEFAULT 50, "
               "price REAL DEFAULT 0, "
               "health REAL DEFAULT 50, "
               "experience REAL DEFAULT 50, "
               "base_weight REAL DEFAULT 1.0, "
               "current_weight REAL DEFAULT 1.0, "
               "FOREIGN KEY(restaurant_id) REFERENCES Restaurants(id))");

    // 新增字段迁移（兼容已有数据库）
    if (!columnExists(query, "Dishes", "calories"))
        query.exec("ALTER TABLE Dishes ADD COLUMN calories REAL DEFAULT 0");
    if (!columnExists(query, "Dishes", "role"))
        query.exec("ALTER TABLE Dishes ADD COLUMN role TEXT DEFAULT 'FULL_MEAL'");
    if (!columnExists(query, "Dishes", "morning"))
        query.exec("ALTER TABLE Dishes ADD COLUMN morning INTEGER DEFAULT 0");
    if (!columnExists(query, "Dishes", "noon"))
        query.exec("ALTER TABLE Dishes ADD COLUMN noon INTEGER DEFAULT 1");
    if (!columnExists(query, "Dishes", "evening"))
        query.exec("ALTER TABLE Dishes ADD COLUMN evening INTEGER DEFAULT 1");
    if (!columnExists(query, "Dishes", "tags"))
        query.exec("ALTER TABLE Dishes ADD COLUMN tags TEXT DEFAULT ''");

    query.exec("CREATE TABLE IF NOT EXISTS History ("
               "dish_id INTEGER PRIMARY KEY UNIQUE, "
               "rating INTEGER, "
               "timestamp TEXT, "
               "FOREIGN KEY(dish_id) REFERENCES Dishes(id))");

    query.exec("CREATE TABLE IF NOT EXISTS DailyRecords ("
               "date TEXT PRIMARY KEY, "
               "dishes TEXT DEFAULT '', "
               "calories REAL DEFAULT 0, "
               "expenses REAL DEFAULT 0.0)");

    // 如果菜品表为空，插入伪数据用于开发测试
    QSqlQuery countQuery;
    countQuery.exec("SELECT COUNT(*) FROM Dishes");
    countQuery.next();
    if (countQuery.value(0).toInt() == 0) {
        // 插入 3 家餐厅
        query.exec("INSERT INTO Restaurants (name, dist_dorm, dist_library, dist_building) VALUES "
                   "('一食堂二楼', 200, 350, 150), "
                   "('农园食堂', 500, 120, 400), "
                   "('校外美食街', 900, 850, 700)");

        // 插入菜品 (restaurant_id 对应上面的 INSERT 顺序: 1=一食堂二楼, 2=农园食堂, 3=校外美食街)
        query.exec("INSERT INTO Dishes (restaurant_id, name, taste, price, health, experience, "
                   "calories, role, morning, noon, evening, tags, base_weight, current_weight) VALUES "
                   "(1, '牛肉拉面',        85, 12, 70, 80, 650, 'FULL_MEAL', 1,1,0, '面食,拉面,清真', 1.0, 1.0), "
                   "(1, '麻辣香锅',        90, 28, 50, 75, 850, 'MEAT',      0,1,1, '辣,川菜,下饭', 1.0, 1.0), "
                   "(1, '水饺(猪肉白菜)',  75, 15, 65, 60, 550, 'FULL_MEAL', 0,1,1, '饺子,面食', 1.0, 1.0), "
                   "(2, '两荤一素套餐',    70, 14, 80, 55, 700, 'FULL_MEAL', 0,1,1, '套餐,实惠', 1.0, 1.0), "
                   "(2, '铁板炒饭',        80, 16, 45, 70, 600, 'STAPLE',    0,1,1, '炒饭,铁板', 1.0, 1.0), "
                   "(3, '烤肉拌饭',        95, 32, 40, 85, 780, 'FULL_MEAL', 0,1,1, '烤肉,拌饭,韩式', 1.0, 1.0), "
                   "(3, '麦当劳套餐',      88, 35, 30, 90, 900, 'FULL_MEAL', 0,0,1, '快餐,汉堡', 1.0, 1.0)");

        // 插入几条历史评分记录，方便测试历史窗口
        query.exec("INSERT INTO History (dish_id, rating, timestamp) VALUES "
                   "(1, 4, '2026-05-19 12:30'), "
                   "(2, 5, '2026-05-18 18:15'), "
                   "(4, 3, '2026-05-18 12:00'), "
                   "(7, 2, '2026-05-17 20:30')");

        qDebug() << "伪数据已插入！";
    }

    // 如果 DailyRecords 表为空，插入测试数据
    QSqlQuery drCount;
    drCount.exec("SELECT COUNT(*) FROM DailyRecords");
    drCount.next();
    if (drCount.value(0).toInt() == 0) {
        query.exec("INSERT INTO DailyRecords (date, dishes, calories, expenses) VALUES "
                   "('2026-05-15', '牛肉拉面', 650, 12.0), "
                   "('2026-05-16', '麻辣香锅、米饭', 850, 28.0), "
                   "('2026-05-17', '水饺(猪肉白菜)', 550, 15.0), "
                   "('2026-05-18', '两荤一素套餐', 700, 14.0), "
                   "('2026-05-19', '铁板炒饭', 600, 16.0), "
                   "('2026-05-20', '烤肉拌饭、饮料', 780, 35.0), "
                   "('2026-05-21', '麦当劳套餐', 900, 38.0)");
        qDebug() << "日历测试数据已插入！";
    }

    qDebug() << "数据库连接成功，路径:" << dbPath;
    return true;
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    if (!initDatabase()) {
        return -1;
    }

    MainWindow w;
    w.show();
    return a.exec();
}
