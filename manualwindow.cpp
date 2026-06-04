#include "manualwindow.h"
#include "sketchyui.h"
#include "calendarwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>

static const QColor C_CREAM_M  = QColor("#FDFBF7");
static const QColor C_INK_M    = QColor("#2B2B2B");
static const QColor C_INK_L_M  = QColor("#4A4540");
static const QColor C_SHADOW_M = QColor("#3A3530");
static const QColor C_HINT_M   = QColor("#9A9590");
static const QColor C_ACCENT_M = QColor("#8C7A6A");

ManualWindow::ManualWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(600, 640);

    QVBoxLayout *mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(36, 36, 36, 28);
    mainLay->setSpacing(0);

    // 标题
    QLabel *titleLbl = new QLabel(QString::fromUtf8("用户手册"));
    QFont titleFont; titleFont.setPointSize(18);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 3.0);
    titleFont.setWeight(QFont::Bold);
    titleLbl->setFont(titleFont);
    titleLbl->setStyleSheet(QString("color: %1;").arg(C_INK_M.name()));
    mainLay->addWidget(titleLbl);

    // 副标题
    QLabel *subLbl = new QLabel(QString::fromUtf8("《寻味燕园》使用指南"));
    QFont subFont; subFont.setPointSize(11);
    subFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    subLbl->setFont(subFont);
    subLbl->setStyleSheet(QString("color: %1; padding-bottom: 8px;").arg(C_HINT_M.name()));
    mainLay->addWidget(subLbl);

    // 分隔线
    mainLay->addWidget(new ScratchyDivider(this));
    mainLay->addSpacing(8);

    // 关闭按钮
    m_closeBtn = new SketchyButton(QString::fromUtf8("×"),
        QColor("#E0D7CC"), C_SHADOW_M, this);
    m_closeBtn->setFixedSize(36, 36);
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    m_closeBtn->setStyleSheet(
        "font-size: 18px; font-weight: bold; color: #2B2B2B;"
        "font-family: 'Microsoft YaHei';");
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::close);
    m_closeBtn->move(width() - 36 - 30, 30);

    // 滚动区域
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollBar:vertical { background: transparent; width: 10px; margin: 4px 2px; }"
        "QScrollBar::handle:vertical { background: #C8BAB0; border-radius: 5px; min-height: 36px; }"
        "QScrollBar::handle:vertical:hover { background: #B0A090; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }");

    QWidget *content = new QWidget();
    content->setStyleSheet("background: transparent;");
    QVBoxLayout *contentLay = new QVBoxLayout(content);
    contentLay->setSpacing(12);
    contentLay->setContentsMargins(0, 4, 8, 8);

    // 字体定义
    QFont h2Font; h2Font.setPointSize(13);
    h2Font.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    h2Font.setWeight(QFont::Bold);

    QFont h3Font; h3Font.setPointSize(11);
    h3Font.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    h3Font.setWeight(QFont::Bold);

    QFont bodyFont; bodyFont.setPointSize(11);
    bodyFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);

    QFont tipFont; tipFont.setPointSize(10);
    tipFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    tipFont.setItalic(true);

    auto addH2 = [&](const QString &text) {
        QLabel *l = new QLabel(text);
        l->setFont(h2Font);
        l->setStyleSheet(QString("color: %1; padding-top: 12px;").arg(C_ACCENT_M.name()));
        contentLay->addWidget(l);
    };

    auto addH3 = [&](const QString &text) {
        QLabel *l = new QLabel(text);
        l->setFont(h3Font);
        l->setStyleSheet(QString("color: %1; padding-top: 8px;").arg(C_INK_L_M.name()));
        contentLay->addWidget(l);
    };

    auto addBody = [&](const QString &text) {
        QLabel *l = new QLabel(text);
        l->setWordWrap(true);
        l->setFont(bodyFont);
        l->setStyleSheet(QString("color: %1; line-height: 1.6;").arg(C_INK_L_M.name()));
        contentLay->addWidget(l);
    };

    auto addTip = [&](const QString &text) {
        QLabel *l = new QLabel(text);
        l->setWordWrap(true);
        l->setFont(tipFont);
        l->setStyleSheet(QString(
            "color: %1; background: rgba(138,122,154,0.08);"
            "border-left: 2.5px solid %2; padding: 8px 12px; margin: 4px 0;"
            "border-radius: 4px;").arg(C_HINT_M.name()).arg(C_ACCENT_M.name()));
        contentLay->addWidget(l);
    };

    // ========== 手册内容 ==========

    addH2(QString::fromUtf8("概述"));
    addBody(QString::fromUtf8(
        "\"寻味燕园\"是一款面向北大学生的校园饮食决策APP。它将\"今天吃什么\"这个每日难题游戏化，变成一次有趣的抽卡体验。\n\n"
        "打开APP，你会看到一张北大校园地图和一只可爱的攻城狮。选择你的位置，搜索你感兴趣的菜品，然后——抽卡！"
        "便当盒从天而降，亲手拉开丝带，一道菜品揭晓。系统会引导你搭配一荤一素一主食，确保营养均衡。"
        "吃完还能打分，积累成就解锁攻城狮的新皮肤，用AI分析你的饮食报告。\n\n"
        "所有数据都存储在本地，不上传任何服务器。"));

    addH2(QString::fromUtf8("第一步：进入APP，熟悉主界面"));
    addBody(QString::fromUtf8("打开程序后会看到一个欢迎页面，点击任意位置即可进入主界面。"));
    addH3(QString::fromUtf8("主界面组成"));
    addBody(QString::fromUtf8(
        "左侧侧边栏：\n"
        "  • 去吃饭 — 进入用餐页面，开始抽卡点餐\n"
        "  • 历史记录 — 查看每天的饮食记录和日历\n"
        "  • 菜品评价 — 对吃过的菜品进行评分\n"
        "  • 勋章成就 — 查看已解锁的成就和皮肤\n"
        "  • 设置 — 编辑个人资料\n"
        "  • 用户手册 — 本文档\n"
        "  • 下方显示你的头像、昵称、今日已摄入热量和建议摄入量\n\n"
        "右侧地图：一张北大校园地图，上面有一只会自动散步的攻城狮。"
        "可以使用快捷键 Ctrl++ 放大地图，Ctrl+- 缩小地图，Ctrl+0 恢复默认。"));
    addTip(QString::fromUtf8(
        "小提示：地图上的攻城狮会随着你的饮食状态改变外观——吃得健康保持苗条，吃多了会变胖哦。"));

    addH2(QString::fromUtf8("第二步：设置个人资料"));
    addBody(QString::fromUtf8("在使用之前，建议先点击侧边栏的设置，填写你的基本信息："));
    addBody(QString::fromUtf8(
        "1. 头像：点击圆形头像，从本地选择一张图片\n"
        "2. 昵称：给自己起个名字\n"
        "3. 性别、身高、体重、年龄：用于计算你的基础代谢率和每日建议摄入热量\n"
        "4. BGM音量：调节背景音乐的音量（默认播放久石让的《Summer》）\n\n"
        "填写完毕后点击保存。所有信息仅存储在本地，不会上传。"));

    addH2(QString::fromUtf8("第三步：选择位置，开始点餐"));
    addBody(QString::fromUtf8(
        "1. 在地图上点击你当前所在的位置（如教室、图书馆、宿舍区）。系统会识别你的当前位置对应的校园区域。\n"
        "2. 如果点击的位置不在可识别的区域内，会提示\"未识别区域\"。此时可以换个位置再试。\n"
        "3. 点击侧边栏的去吃饭，进入用餐页面。\n\n"
        "如果忘记选择位置，系统会弹窗提醒你先在地图上点击位置。\n\n"
        "用餐页面顶部会显示：\n"
        "  • 实时时钟：当前时间\n"
        "  • 用餐模式：系统根据当前时间自动切换——早餐（4:00-10:00）、午餐（10:00-14:00）、"
        "下午茶（14:00-17:00）、晚餐（17:00-21:00）、深夜（21:00-4:00）"));

    addH2(QString::fromUtf8("第四步：搜索与抽卡"));
    addH3(QString::fromUtf8("搜索菜品"));
    addBody(QString::fromUtf8(
        "在搜索框中输入任意关键词，点击搜索。你可以根据需要输入以下任一类型的关键词：\n"
        "  • 菜名（如\"宫保鸡丁\"）\n"
        "  • 口味标签（如\"辣\"、\"甜\"、\"清淡\"）\n"
        "  • 食堂名称或缩写（如\"家二\"会被自动识别为\"家园二层\"）\n"
        "  • 价格标签（如\"便宜\"）\n\n"
        "搜索结果会显示在下方列表中。每次搜索会生成一个标签卡片，多次搜索的标签会叠加筛选。"
        "点击标签旁的 × 可以移除该条件。"));
    addH3(QString::fromUtf8("两种抽卡方式"));
    addBody(QString::fromUtf8(
        "  • \"从搜索结果中抽卡\"：只从你筛选出来的菜品中抽，更精准\n"
        "  • \"加权抽卡（惊喜模式）\"：从当前时段所有菜品中抽，但会根据你的搜索评分和历史偏好进行加权——"
        "搜索匹配度越高、你之前评分越高的菜，被抽中的概率越大"));
    addH3(QString::fromUtf8("抽卡动画"));
    addBody(QString::fromUtf8("点击抽卡按钮后，一个便当盒会从天而降。向右拖动便当盒上的丝带——你今天的菜就揭晓了！"));

    addH2(QString::fromUtf8("第五步：搭配与确认"));
    addBody(QString::fromUtf8("系统会根据营养均衡原则，引导你搭配一餐完整的饭菜："));
    addBody(QString::fromUtf8(
        "午餐/晚餐模式：需要完成\"核心搭配\"（一荤一素一主食），完成后可选择加一份饮品或小吃。\n\n"
        "如果你抽到的是一份整餐（如面条、盖饭、披萨、套餐等），抽中即算核心搭配完成，不需要再单独搭配荤素主食。\n\n"
        "早餐模式：需要选一份主食和一份饮品。\n\n"
        "页面底部会显示你的今日菜单，每个菜品都可以点击 × 删除，重新抽取。\n\n"
        "搭配完成后，点击确认菜单，开始吃饭！，系统会弹出一张五维雷达图：\n"
        "  • 口感 — 菜品美味度的综合评分\n"
        "  • 价格 — 总价是否在合理范围内\n"
        "  • 体验 — 餐厅环境和服务的综合评分\n"
        "  • 热量 — 与你的建议摄入量对比\n"
        "  • 距离 — 你当前位置到食堂的距离\n\n"
        "满意后点击就这些！开吃！。"));

    addH2(QString::fromUtf8("第六步：饭后评分"));
    addBody(QString::fromUtf8(
        "确认开吃后，系统会自动弹出评分窗口：\n"
        "1. 列表中显示你刚才吃的所有菜品\n"
        "2. 为每道菜打1-5星\n"
        "3. 评完分后不能修改（认真对待每一餐！）\n\n"
        "你也可以随时通过侧边栏的菜品评价查看所有历史菜品的评分记录。"));

    addH2(QString::fromUtf8("第七步：查看历史与统计"));
    addBody(QString::fromUtf8(
        "点击侧边栏的历史记录：\n"
        "  • 日历视图：选择任意日期，查看那天吃了什么、花了多少钱、摄入了多少热量\n"
        "  • \"统计\"按钮：查看消费和热量变化的折线图，可以左右滑动切换时间范围\n"
        "  • \"报告\"按钮：生成AI饮食分析报告（见下文）"));

    addH2(QString::fromUtf8("第八步：AI饮食报告"));
    addBody(QString::fromUtf8(
        "在历史记录页面点击\"报告\"，可以生成AI驱动的饮食分析报告：\n"
        "1. 首次使用需要输入智谱AI的API Key（前往 https://open.bigmodel.cn 免费注册获取）\n"
        "2. 选择今日报告或一周报告\n"
        "3. 点击生成报告，AI会分析你的饮食数据，生成个性化的食光报告\n\n"
        "报告内容包含饮食风格总结、能量分析、食物多样性、消费评价和健康建议。文字采用流式输出，像在和朋友聊天。"));

    addH2(QString::fromUtf8("第九步：成就与皮肤"));
    addBody(QString::fromUtf8("点击侧边栏的勋章成就，查看你的成就收集进度。共有10项成就："));
    addBody(QString::fromUtf8(
        "  • 初来乍到 — 完成第一次饮食记录\n"
        "  • 自律干饭人 — 连续3天热量不超标\n"
        "  • 铁律干饭人 — 连续7天热量不超标\n"
        "  • 全勤吃货 — 连续7天有饮食记录\n"
        "  • 暴食魔王 — 连续3天热量超标\n"
        "  • 热量暴君 — 连续7天热量超标\n"
        "  • 味蕾收藏家 — 累计品尝50种不同菜品\n"
        "  • 百味大师 — 累计品尝100种不同菜品\n"
        "  • 深夜食堂 — 晚上21:00后完成饮食记录\n"
        "  • 豪华大餐王 — 单餐消费超过50元"));
    addBody(QString::fromUtf8(
        "每解锁一项成就，就会获得一款攻城狮皮肤。点击已解锁的成就卡片，可以给地图上的攻城狮换上对应皮肤。"
        "侧边栏的成就按钮上会有红点提示新解锁的成就。"));

    contentLay->addStretch();
    scroll->setWidget(content);
    mainLay->addWidget(scroll);
}

void ManualWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF card(12, 12, width() - 24, height() - 24);
    int seed = 61;

    QRectF shadow = card.translated(2.5, 3.5);
    QPainterPath sp = sketchyRect(shadow, seed + 100, 2.8);
    p.setBrush(C_SHADOW_M);
    p.setPen(Qt::NoPen);
    p.drawPath(sp);

    QPainterPath cp = sketchyRect(card, seed, 2.8);
    drawInkWash(&p, cp, C_CREAM_M, 18);
    drawInkBorder(&p, cp, C_INK_M, 3, 0.7);
}

void ManualWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        QWidget *child = childAt(e->pos());
        if (!child || child == this) {
            m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
            m_dragging = true;
        }
    }
    QDialog::mousePressEvent(e);
}

void ManualWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
        QPoint delta = e->globalPosition().toPoint() - frameGeometry().topLeft() - m_dragPos;
        if (delta.manhattanLength() > 4)
            move(e->globalPosition().toPoint() - m_dragPos);
    }
    QDialog::mouseMoveEvent(e);
}

void ManualWindow::mouseReleaseEvent(QMouseEvent *e)
{
    m_dragging = false;
    QDialog::mouseReleaseEvent(e);
}
