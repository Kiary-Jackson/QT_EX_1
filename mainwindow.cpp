#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QKeyEvent>
#include <QPushButton>
#include <QMessageBox>
#include <QtMath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("计算器");

    // 初始化状态
    m_firstOperand = 0;
    m_secondOperand = 0;
    m_operator = "";
    m_waitingForSecond = false;
    m_hasResult = false;
    m_error = false;

    // 数字按钮 0-9 连接到 onDigitClicked
    for (int i = 0; i <= 9; i++) {
        QPushButton *btn = findChild<QPushButton*>(QString("btn%1").arg(i));
        if (btn) connect(btn, &QPushButton::clicked, this, &MainWindow::onDigitClicked);
    }

    // 运算符按钮连接到 onOperatorClicked
    QStringList ops = {"btnAdd", "btnSub", "btnMul", "btnDiv"};
    for (const QString &name : ops) {
        QPushButton *btn = findChild<QPushButton*>(name);
        if (btn) connect(btn, &QPushButton::clicked, this, &MainWindow::onOperatorClicked);
    }

    // 其他按钮
    connect(ui->btnEqual, &QPushButton::clicked, this, &MainWindow::onEqualClicked);
    connect(ui->btnClear, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(ui->btnClearEntry, &QPushButton::clicked, this, &MainWindow::onClearEntryClicked);
    connect(ui->btnBackspace, &QPushButton::clicked, this, &MainWindow::onBackspaceClicked);
    connect(ui->btnNegate, &QPushButton::clicked, this, &MainWindow::onNegateClicked);
    connect(ui->btnDecimal, &QPushButton::clicked, this, &MainWindow::onDecimalClicked);
    connect(ui->btnPercent, &QPushButton::clicked, this, &MainWindow::onPercentClicked);

    updateDisplay();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 键盘事件处理
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();

    // 数字键 0-9
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        appendDigit(key - Qt::Key_0);
        return;
    }

    // 小键盘数字
    if (key >= Qt::Key_0 + 0x1000000 && key <= Qt::Key_9 + 0x1000000) {
        appendDigit(key - Qt::Key_0 - 0x1000000);
        return;
    }

    // 运算符
    switch (key) {
    case Qt::Key_Plus:
        setOperator("+");
        return;
    case Qt::Key_Minus:
        setOperator("-");
        return;
    case Qt::Key_Asterisk:
        setOperator("×");
        return;
    case Qt::Key_Slash:
    case Qt::Key_division:
        setOperator("÷");
        return;
    case Qt::Key_Equal:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        onEqualClicked();
        return;
    case Qt::Key_Period:
    case Qt::Key_Comma:
        onDecimalClicked();
        return;
    case Qt::Key_Backspace:
        onBackspaceClicked();
        return;
    case Qt::Key_Escape:
        onClearClicked();
        return;
    case Qt::Key_Delete:
        onClearEntryClicked();
        return;
    case Qt::Key_Percent:
        onPercentClicked();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

// 追加数字（鼠标和键盘共用）
void MainWindow::appendDigit(int digit)
{
    if (m_error)           // 错误态：先复位，再按新计算处理
        resetAll();

    QString text = ui->display->text();

    // 如果刚计算完结果，重新开始输入
    if (m_hasResult) {
        text = "0";
        m_hasResult = false;
        m_waitingForSecond = false;
        m_operator = "";
    }

    // 如果等待第二个操作数，清空显示
    if (m_waitingForSecond) {
        text = "0";
        m_waitingForSecond = false;
    }

    // 避免前导零（但允许 "0." 的情况）
    if (text == "0" && digit != 0) {
        text = QString::number(digit);
    } else if (text == "0" && digit == 0) {
        // 已经是0，不重复添加
    } else {
        text += QString::number(digit);
    }

    ui->display->setText(text);
}

// 数字按钮槽函数
void MainWindow::onDigitClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString text = btn->text();
    int digit = text.toInt();
    appendDigit(digit);
}

// 设置运算符
void MainWindow::setOperator(const QString &op)
{
    if (m_error)          // 错误态下按运算符：先清空，从 0 重新开始
        resetAll();

    double current = m_hasResult ? m_firstOperand : ui->display->text().toDouble();

    if (!m_operator.isEmpty() && m_waitingForSecond) {
        // 连续按下运算符（如 5 + ×）：只替换运算符，不触发计算
        m_operator = op;
        return;
    }

    if (!m_operator.isEmpty()) {
        // 第二操作数已输入完，先算出中间结果，实现连续运算
        m_secondOperand = current;
        if (!calculate())
            return;
    } else {
        // 第一次按运算符：记住左操作数
        m_firstOperand = current;
    }

    m_operator = op;
    m_waitingForSecond = true;
    m_hasResult = false;
}

// 运算符按钮槽函数
void MainWindow::onOperatorClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString op = btn->text();
    setOperator(op);
}

// 执行运算。返回 false 表示运算失败（已进入错误态）
bool MainWindow::calculate()
{
    double result = 0;

    if (m_operator == "+") {
        result = m_firstOperand + m_secondOperand;
    } else if (m_operator == "-") {
        result = m_firstOperand - m_secondOperand;
    } else if (m_operator == "×") {
        result = m_firstOperand * m_secondOperand;
    } else if (m_operator == "÷") {
        if (m_secondOperand == 0) {
            showError("除数不能为 0");
            return false;
        }
        result = m_firstOperand / m_secondOperand;
    } else {
        return false;
    }

    // 格式化结果，去掉多余的零
    ui->display->setText(formatNumber(result));
    m_firstOperand = result;
    m_operator = "";
    m_hasResult = true;
    m_error = false;
    return true;
}

// 进入错误态：清空运算符与操作数，让下一次输入从零开始
void MainWindow::showError(const QString &msg)
{
    ui->display->setText(msg);
    m_error = true;
    m_firstOperand = 0;
    m_secondOperand = 0;
    m_operator = "";
    m_waitingForSecond = false;
    m_hasResult = true;   // 下一次按数字/小数点会自动重新开始
}

// 复位全部状态
void MainWindow::resetAll()
{
    ui->display->setText("0");
    m_firstOperand = 0;
    m_secondOperand = 0;
    m_operator = "";
    m_waitingForSecond = false;
    m_hasResult = false;
    m_error = false;
}

// 等号
void MainWindow::onEqualClicked()
{
    if (m_error) {         // 错误态下按 =：清空重来
        resetAll();
        return;
    }
    if (m_operator.isEmpty()) return;

    QString text = ui->display->text();
    m_secondOperand = text.toDouble();
    calculate();
    m_waitingForSecond = false;
}

// 清除 C（全部清除）
void MainWindow::onClearClicked()
{
    resetAll();
}

// 清除当前输入 CE
void MainWindow::onClearEntryClicked()
{
    if (m_error) {          // 错误态下 CE 等同重新开始
        resetAll();
        return;
    }
    ui->display->setText("0");
    m_hasResult = false;
}

// 退格
void MainWindow::onBackspaceClicked()
{
    if (m_error) {          // 错误提示不允许逐字退格
        resetAll();
        return;
    }
    QString text = ui->display->text();
    if (text.length() > 1) {
        text.chop(1);
        if (text == "-") text = "0";
    } else {
        text = "0";
    }
    ui->display->setText(text);
}

// 正负号
void MainWindow::onNegateClicked()
{
    if (m_error) {
        resetAll();
        return;
    }
    QString text = ui->display->text();
    if (text == "0") return;
    if (text.startsWith("-")) {
        text = text.mid(1);
    } else {
        text = "-" + text;
    }
    ui->display->setText(text);
}

// 小数点
void MainWindow::onDecimalClicked()
{
    if (m_error)           // 错误态：不允许把小数点拼到错误提示后面
        resetAll();

    QString text = ui->display->text();

    // 如果刚计算完结果，重新开始
    if (m_hasResult) {
        text = "0";
        m_hasResult = false;
        m_waitingForSecond = false;
        m_operator = "";
    }

    // 如果等待第二个操作数，清空显示
    if (m_waitingForSecond) {
        text = "0";
        m_waitingForSecond = false;
    }

    // 避免重复小数点
    if (!text.contains('.')) {
        text += ".";
    }

    ui->display->setText(text);
}

// 百分号
void MainWindow::onPercentClicked()
{
    if (m_error) {
        resetAll();
        return;
    }
    const double value = ui->display->text().toDouble() / 100.0;
    ui->display->setText(formatNumber(value));
    m_hasResult = false;      // 百分号之后的值仍属于当前操作数
}

// 数值 → 显示字符串：整数去掉多余的 ".0"，非整数保留 12 位有效数字
QString MainWindow::formatNumber(double value)
{
    if (!qIsFinite(value))
        return QStringLiteral("错误");

    // 用 qRound64 判断整数，避免 static_cast<int> 在超过 2^31 时溢出
    if (qAbs(value) < 1e15 && value == static_cast<double>(qRound64(value)))
        return QString::number(qRound64(value));

    return QString::number(value, 'g', 12);
}

// 更新显示
void MainWindow::updateDisplay()
{
    ui->display->setText("0");
}
