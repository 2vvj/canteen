#include <QApplication>
#include "RadarChartWidget.h" // 确保文件名与你保存的头文件一致

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 1. 创建雷达图实例
    RadarChartWidget w;
    w.resize(500, 500); // 设置一个合适的初始大小
    w.setWindowTitle("干饭抽卡模拟器 - 雷达图预览");

    // 2. 模拟导入具体数据（即你说的“写死”数据来试运行）
    // 维度顺序：美味, 价格, 距离, 健康, 体验
    RadarData mockData(95.0f, 80.0f, 70.0f, 40.0f, 90.0f);

    // 3. 调用接口传入数据和名称
    w.setData(mockData, "【金色传说】二食堂特供黄焖鸡");

    // 4. 设置颜色（比如模拟金色品质）
    w.setMainColor(QColor(255, 215, 0));

    w.show();
    return a.exec();
}
