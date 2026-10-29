#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QKeyEvent>
#include <QPushButton>
#include <QStyle>
#include <QtMath>

namespace {
// 显示区最多容纳的字符数，防止连续按键把数字无限追加（也避免显示溢出）
constexpr int kMaxInputLength = 16;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("计算器"));
    applyStyle();

    // 显示区字体在代码里设置，便于按文本长度动态缩放字号
    QFont displayFont;
    displayFont.setFamilies({QStringLiteral("Segoe UI"), QStringLiteral("Microsoft YaHei UI")});
    displayFont.setPixelSize(40);
    ui->display->setFont(displayFont);

    // 10 个数字按钮用循环统一连接，避免 10 段重复代码
    for (int i = 0; i <= 9; ++i) {
        if (auto *btn = findChild<QPushButton *>(QStringLiteral("btn%1").arg(i)))
            connect(btn, &QPushButton::clicked, this, &MainWindow::onDigitClicked);
    }

    // 4 个运算符按钮同样用循环连接
    const QStringList opButtons{QStringLiteral("btnAdd"), QStringLiteral("btnSub"),
                                QStringLiteral("btnMul"), QStringLiteral("btnDiv")};
    for (const QString &name : opButtons) {
        if (auto *btn = findChild<QPushButton *>(name))
            connect(btn, &QPushButton::clicked, this, &MainWindow::onOperatorClicked);
    }

    // 功能按钮
    connect(ui->btnEqual,      &QPushButton::clicked, this, &MainWindow::onEqualClicked);
    connect(ui->btnClear,      &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(ui->btnClearEntry, &QPushButton::clicked, this, &MainWindow::onClearEntryClicked);
    connect(ui->btnBackspace,  &QPushButton::clicked, this, &MainWindow::onBackspaceClicked);
    connect(ui->btnNegate,     &QPushButton::clicked, this, &MainWindow::onNegateClicked);
    connect(ui->btnDecimal,    &QPushButton::clicked, this, &MainWindow::onDecimalClicked);
    connect(ui->btnPercent,    &QPushButton::clicked, this, &MainWindow::onPercentClicked);

    resetAll();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ============================ 键盘事件 ============================
// 键盘只负责"按键 → 语义动作"的映射，具体处理全部交给与鼠标共用的槽函数，
// 因此鼠标输入与键盘输入的行为天然一致，不会出现两套逻辑。
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    const int key = event->key();

    // 主键盘数字 0-9
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        appendDigit(key - Qt::Key_0);
        return;
    }
    // 小键盘数字：Qt 用小键盘标志位区分，偏移量为 0x1000000
    if (key >= Qt::Key_0 + 0x1000000 && key <= Qt::Key_9 + 0x1000000) {
        appendDigit(key - Qt::Key_0 - 0x1000000);
        return;
    }

    switch (key) {
    case Qt::Key_Plus:      setOperator(QStringLiteral("+")); return;
    case Qt::Key_Minus:     setOperator(QStringLiteral("-")); return;
    case Qt::Key_Asterisk:  setOperator(QStringLiteral("×")); return;
    case Qt::Key_Slash:
    case Qt::Key_division:  setOperator(QStringLiteral("÷")); return;

    case Qt::Key_Equal:
    case Qt::Key_Return:
    case Qt::Key_Enter:     onEqualClicked(); return;

    case Qt::Key_Period:
    case Qt::Key_Comma:     onDecimalClicked(); return;

    case Qt::Key_Backspace: onBackspaceClicked(); return;
    case Qt::Key_Escape:    onClearClicked(); return;        // Esc = C
    case Qt::Key_Delete:    onClearEntryClicked(); return;   // Del = CE
    case Qt::Key_Percent:   onPercentClicked(); return;
    default: break;
    }

    QMainWindow::keyPressEvent(event);   // 未识别的键交回父类，保证不影响其它快捷键
}

// ============================ 输入处理 ============================

// 每当要开始输入"一个新的操作数"时调用：把输入缓冲区复位为 "0"。
// 这是避免"新操作数继承上一个操作数的数字/小数点"的关键。
void MainWindow::beginNewOperand()
{
    if (m_error || m_hasResult) {
        // 错误恢复 或 计算结果后继续输入 → 相当于开始一轮全新的计算
        resetAll();
    } else if (m_waitingForSecond) {
        // 刚按下运算符 → 只清空当前输入，保留第一操作数与运算符
        m_waitingForSecond = false;
        setDisplayText(QStringLiteral("0"));
    }
}

void MainWindow::appendDigit(int digit)
{
    if (m_hasResult || m_waitingForSecond || m_error)
        beginNewOperand();

    QString text = displayText();
    if (text.length() >= kMaxInputLength)   // 长度保护，避免无限追加
        return;

    if (text == QLatin1String("0"))          // 抑制前导零：0 被新数字替换
        text = QString::number(digit);
    else if (text == QLatin1String("-0"))    // ± 之后输入数字：-0 → -5
        text = QLatin1String("-") + QString::number(digit);
    else
        text += QString::number(digit);

    setDisplayText(text);
}

void MainWindow::onDigitClicked()
{
    if (auto *btn = qobject_cast<QPushButton *>(sender()))
        appendDigit(btn->text().toInt());
}

// 小数点去重：显示区文本就是"当前正在输入的操作数"，因此只要文本里已经有
// 小数点就不再追加；退格把小数点删掉之后，又可以重新输入。
void MainWindow::onDecimalClicked()
{
    if (m_hasResult || m_waitingForSecond || m_error)
        beginNewOperand();

    QString text = displayText();
    if (text.length() >= kMaxInputLength)
        return;

    if (!text.contains(QLatin1Char('.'))) {
        text += QLatin1Char('.');   // 核心：同一操作数只允许一个小数点
        setDisplayText(text);
    }
}

// ============================ 运算符与运算 ============================

// 运算符状态机的核心：根据"是否已有挂起运算 / 第二操作数是否已输入"
// 决定本次按键是记录第一操作数、替换运算符，还是先算中间结果（连续运算）。
void MainWindow::setOperator(const QString &op)
{
    if (m_error)             // 错误态下按运算符：先清空，再以 0 作为第一操作数
        resetAll();

    // 刚算完结果时直接用内部 double 结果继续，避免回读被格式化过的显示字符串而丢精度
    double current = m_hasResult ? m_firstOperand : displayText().toDouble();

    if (!m_operator.isEmpty()) {
        if (m_waitingForSecond) {
            m_operator = op;                 // 连按运算符（如 5 + ×）：只替换运算符
            updateExpression();
            return;
        }
        m_secondOperand = current;           // 第二操作数已输入完 → 先算中间结果
        if (!calculate())                    // 除零等异常，calculate() 内部已进入错误态
            return;
    } else {
        m_firstOperand = current;            // 第一次按运算符：记住左操作数
    }

    m_operator          = op;
    m_waitingForSecond  = true;              // 下一次数字将覆盖显示
    m_hasResult         = false;
    m_expression        = formatNumber(m_firstOperand) + QLatin1Char(' ') + op + QLatin1Char(' ');
    updateExpression();
}

void MainWindow::onOperatorClicked()
{
    if (auto *btn = qobject_cast<QPushButton *>(sender()))
        setOperator(btn->text());
}

void MainWindow::onEqualClicked()
{
    if (m_error) {          // 错误态下按 = ：清空重来
        resetAll();
        return;
    }
    if (m_operator.isEmpty())   // 没有挂起运算，"=" 无意义，直接忽略
        return;

    if (m_waitingForSecond)     // "12 + =" 这种缺少第二操作数的情况：用第一操作数补齐
        m_secondOperand = m_firstOperand;
    else
        m_secondOperand = displayText().toDouble();

    m_expression = formatNumber(m_firstOperand) + QLatin1Char(' ') + m_operator
                 + QLatin1Char(' ') + formatNumber(m_secondOperand) + QStringLiteral(" =");

    if (calculate())            // calculate() 内部会写回结果并清空运算符
        m_waitingForSecond = false;

    updateExpression();
}

// 执行一次运算。返回 false 表示本次运算失败（已进入错误态）。
bool MainWindow::calculate()
{
    double result = 0.0;

    // 注意：运算符中含有非 ASCII 字符（× ÷），必须用 QStringLiteral，
    // 若写成 QLatin1String("÷") 会按 Latin-1 解释 UTF-8 字节而永远比较失败。
    if (m_operator == QStringLiteral("+")) {
        result = m_firstOperand + m_secondOperand;
    } else if (m_operator == QStringLiteral("-")) {
        result = m_firstOperand - m_secondOperand;
    } else if (m_operator == QStringLiteral("×")) {
        result = m_firstOperand * m_secondOperand;
    } else if (m_operator == QStringLiteral("÷")) {
        if (qFuzzyIsNull(m_secondOperand) || m_secondOperand == 0.0) {
            showError(QStringLiteral("除数不能为 0"));
            return false;
        }
        result = m_firstOperand / m_secondOperand;
    } else {
        return false;
    }

    if (!qIsFinite(result)) {
        showError(QStringLiteral("结果超出范围"));
        return false;
    }

    setDisplayText(formatNumber(result));

    m_firstOperand     = result;   // 结果同时成为下一轮计算的第一操作数
    m_secondOperand    = 0.0;
    m_operator.clear();            // 运算已消费掉运算符
    m_hasResult        = true;     // 下一次数字输入应开启新计算
    m_waitingForSecond = false;
    return true;
}

// ============================ 清除 / 退格 ============================

// C：清除全部状态，回到初始界面
void MainWindow::onClearClicked()
{
    resetAll();
}

// CE：只清除当前输入，保留挂起的运算符和第一操作数
void MainWindow::onClearEntryClicked()
{
    if (m_error) {
        resetAll();
        return;
    }
    setDisplayText(QStringLiteral("0"));
    m_hasResult = false;
    if (!m_operator.isEmpty())
        m_waitingForSecond = true;   // 清完之后重新输入第二个操作数
}

void MainWindow::onBackspaceClicked()
{
    if (m_error) {                   // 错误提示不允许逐字退格
        resetAll();
        return;
    }
    if (m_hasResult || m_waitingForSecond)
        return;                      // 结果显示中 / 等待第二操作数时退格无意义

    QString text = displayText();
    text.chop(1);
    if (text.isEmpty() || text == QLatin1String("-"))
        text = QStringLiteral("0");  // 删空或只剩负号时回到 0
    setDisplayText(text);
}

void MainWindow::onNegateClicked()
{
    if (m_error) {
        resetAll();
        return;
    }

    QString text = displayText();
    if (text == QLatin1String("0"))
        return;

    if (text.startsWith(QLatin1Char('-')))
        text.remove(0, 1);
    else
        text.prepend(QLatin1Char('-'));

    setDisplayText(text);
    if (m_hasResult)                 // 对结果取负：同步更新内部值，后续运算才不会用回旧结果
        m_firstOperand = text.toDouble();
}

void MainWindow::onPercentClicked()
{
    if (m_error) {
        resetAll();
        return;
    }

    const double value = displayText().toDouble() / 100.0;
    setDisplayText(formatNumber(value));
    m_hasResult = false;             // 百分号之后的值仍属于当前操作数，可继续输入/运算
    if (!m_operator.isEmpty())
        m_waitingForSecond = false;
}

// ============================ 状态与显示 ============================

void MainWindow::resetAll()
{
    m_firstOperand     = 0.0;
    m_secondOperand    = 0.0;
    m_operator.clear();
    m_expression.clear();
    m_waitingForSecond = false;
    m_hasResult        = false;
    m_error            = false;
    setErrorStyle(false);
    setDisplayText(QStringLiteral("0"));
    updateExpression();
}

void MainWindow::showError(const QString &msg)
{
    setDisplayText(msg);
    setErrorStyle(true);
    m_error            = true;
    m_firstOperand     = 0.0;
    m_secondOperand    = 0.0;
    m_operator.clear();
    m_expression.clear();
    m_waitingForSecond = false;
    m_hasResult        = true;   // 下一次数字/小数点输入自动重新开始
    updateExpression();
}

QString MainWindow::displayText() const
{
    return ui->display->text();
}

void MainWindow::setDisplayText(const QString &text)
{
    ui->display->setText(text);

    // 按文本长度自动缩放字号：结果很长或错误提示时缩小，避免文字超出显示区
    const int len = text.length();
    int pixelSize = 40;
    if (len > 12)      pixelSize = 20;
    else if (len > 9)  pixelSize = 26;
    else if (len > 6)  pixelSize = 32;

    if (ui->display->font().pixelSize() != pixelSize) {
        QFont f = ui->display->font();
        f.setPixelSize(pixelSize);
        ui->display->setFont(f);
    }
}

// 错误态切换：通过动态属性让 QSS 把显示文字改成红色
void MainWindow::setErrorStyle(bool on)
{
    if (ui->display->property("error").toBool() == on)
        return;
    ui->display->setProperty("error", on);
    ui->display->style()->unpolish(ui->display);
    ui->display->style()->polish(ui->display);
}

void MainWindow::updateExpression()
{
    ui->lblExpression->setText(m_expression);
}

// 数值 → 显示字符串：整数去掉多余的 ".0"，非整数限制有效位数
QString MainWindow::formatNumber(double value)
{
    if (!qIsFinite(value))
        return QStringLiteral("错误");

    // 绝对值在 qint64 安全范围内且为整数时，按整数输出，避免出现 1e+15 之类的写法
    if (qAbs(value) < 1e15 && value == static_cast<double>(qRound64(value)))
        return QString::number(qRound64(value));

    return QString::number(value, 'g', 12);
}

// ============================ 样式表 ============================
// 用 QSS 统一控制配色、圆角、字号与悬停/按下状态：
//   数字键 = 白底；功能键 = 浅灰底；运算符 = 淡橙底橙字；等号 = 实心橙
void MainWindow::applyStyle()
{
    setStyleSheet(QStringLiteral(R"(
        QWidget#centralWidget {
            background-color: #f4f4f7;
        }

        QLabel#lblExpression {
            color: #9b9ba2;
            font-size: 14px;
            font-family: "Segoe UI", "Microsoft YaHei UI";
        }

        /* 字号由代码按文本长度动态调整，这里只控制配色与边距 */
        QLineEdit#display {
            background: transparent;
            border: none;
            color: #17171a;
            padding: 0px 6px 10px 6px;
        }
        /* 错误态：文字变红，提示更醒目 */
        QLineEdit#display[error="true"] {
            color: #d93a3a;
        }

        QPushButton {
            background-color: #ffffff;
            border: 1px solid #e6e6eb;
            border-radius: 14px;
            color: #17171a;
            font-size: 21px;
            font-family: "Segoe UI", "Microsoft YaHei UI";
        }
        QPushButton:hover  { background-color: #f1f1f6; }
        QPushButton:pressed { background-color: #e2e2ea; }

        QPushButton#btnClear, QPushButton#btnClearEntry, QPushButton#btnBackspace,
        QPushButton#btnPercent, QPushButton#btnNegate {
            background-color: #ececf2;
            border: 1px solid #e1e1e9;
            color: #4a4a52;
            font-size: 17px;
        }
        QPushButton#btnClear:hover, QPushButton#btnClearEntry:hover, QPushButton#btnBackspace:hover,
        QPushButton#btnPercent:hover, QPushButton#btnNegate:hover {
            background-color: #e2e2ea;
        }
        QPushButton#btnClear:pressed, QPushButton#btnClearEntry:pressed, QPushButton#btnBackspace:pressed,
        QPushButton#btnPercent:pressed, QPushButton#btnNegate:pressed {
            background-color: #d6d6e0;
        }

        QPushButton#btnAdd, QPushButton#btnSub, QPushButton#btnMul, QPushButton#btnDiv {
            background-color: #fff3e8;
            border: 1px solid #ffd8b6;
            color: #e87511;
            font-size: 23px;
            font-weight: bold;
        }
        QPushButton#btnAdd:hover, QPushButton#btnSub:hover,
        QPushButton#btnMul:hover, QPushButton#btnDiv:hover {
            background-color: #ffe8d2;
        }
        QPushButton#btnAdd:pressed, QPushButton#btnSub:pressed,
        QPushButton#btnMul:pressed, QPushButton#btnDiv:pressed {
            background-color: #ffdcc0;
        }

        QPushButton#btnEqual {
            background-color: #ff8c1a;
            border: none;
            border-radius: 14px;
            color: #ffffff;
            font-size: 24px;
            font-weight: bold;
        }
        QPushButton#btnEqual:hover   { background-color: #ff9d3d; }
        QPushButton#btnEqual:pressed { background-color: #ef7f0d; }
    )"));
}
