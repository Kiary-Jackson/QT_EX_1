#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QKeyEvent>
#include <QPushButton>
#include <QMessageBox>

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
    QString text = ui->display->text();
    double current = text.toDouble();

    // 如果已有运算符且正在等待第二个操作数，先计算中间结果（连续运算）
    if (!m_operator.isEmpty() && m_waitingForSecond) {
        m_firstOperand = current;
        calculate();
        m_operator = op;
    } else if (!m_operator.isEmpty()) {
        // 第一次设置运算符
        m_firstOperand = current;
        m_operator = op;
    } else {
        m_firstOperand = current;
        m_operator = op;
    }

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

// 执行运算
void MainWindow::calculate()
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
            ui->display->setText("错误：除数不能为0");
            m_operator = "";
            m_hasResult = true;
            return;
        }
        result = m_firstOperand / m_secondOperand;
    }

    // 格式化结果，去掉多余的零
    QString resultStr;
    if (result == static_cast<int>(result)) {
        resultStr = QString::number(static_cast<int>(result));
    } else {
        resultStr = QString::number(result, 'g', 10);
    }

    ui->display->setText(resultStr);
    m_firstOperand = result;
    m_operator = "";
    m_hasResult = true;
}

// 等号
void MainWindow::onEqualClicked()
{
    if (m_operator.isEmpty()) return;

    QString text = ui->display->text();
    m_secondOperand = text.toDouble();
    calculate();
    m_waitingForSecond = false;
}

// 清除 C（全部清除）
void MainWindow::onClearClicked()
{
    ui->display->setText("0");
    m_firstOperand = 0;
    m_secondOperand = 0;
    m_operator = "";
    m_waitingForSecond = false;
    m_hasResult = false;
}

// 清除当前输入 CE
void MainWindow::onClearEntryClicked()
{
    ui->display->setText("0");
}

// 退格
void MainWindow::onBackspaceClicked()
{
    QString text = ui->display->text();
    if (text == "错误：除数不能为0") {
        onClearClicked();
        return;
    }
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
    QString text = ui->display->text();
    double value = text.toDouble();
    value /= 100.0;

    QString resultStr;
    if (value == static_cast<int>(value)) {
        resultStr = QString::number(static_cast<int>(value));
    } else {
        resultStr = QString::number(value, 'g', 10);
    }
    ui->display->setText(resultStr);
}

// 更新显示
void MainWindow::updateDisplay()
{
    ui->display->setText("0");
}
