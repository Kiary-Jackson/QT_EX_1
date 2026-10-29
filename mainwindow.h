#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent *event) override;  // 键盘事件（只做按键→动作映射）

private slots:
    void onDigitClicked();        // 数字按钮（0-9）
    void onOperatorClicked();     // 运算符按钮（+ - × ÷）
    void onEqualClicked();        // 等号
    void onClearClicked();        // 清除 C
    void onClearEntryClicked();   // 清除当前输入 CE
    void onBackspaceClicked();    // 退格
    void onNegateClicked();       // 正负号 ±
    void onDecimalClicked();      // 小数点
    void onPercentClicked();      // 百分号 %

private:
    // —— 输入与运算 ——
    void appendDigit(int digit);                     // 追加数字
    void setOperator(const QString &op);             // 设置/替换运算符
    bool calculate();                                // 执行运算，false 表示失败（除零/溢出）
    void beginNewOperand();                          // 开始输入"新操作数"时复位输入缓冲区
    void resetAll();                                 // C / 错误恢复：复位全部状态
    void showError(const QString &msg);              // 进入错误态

    // —— 显示 ——
    QString displayText() const;
    void setDisplayText(const QString &text);
    void updateExpression();                         // 刷新上方历史表达式
    static QString formatNumber(double value);       // 数值 → 显示字符串
    void setErrorStyle(bool on);                     // 错误态：文字变红
    void applyStyle();                               // 应用样式表（QSS）

    Ui::MainWindow *ui;

    // ===== 计算器状态 =====
    double  m_firstOperand    = 0;      // 第一个操作数（同时充当累加器）
    double  m_secondOperand   = 0;      // 第二个操作数（只在计算前一刻保存）
    QString m_operator;                 // 挂起的运算符，空串表示"无挂起运算"
    QString m_expression;               // 历史表达式，如 "12.5 + "
    bool    m_waitingForSecond = false; // 刚按下运算符，下一个数字应覆盖显示
    bool    m_hasResult        = false; // 刚算完结果，下一个数字应开启新计算
    bool    m_error            = false; // 处于错误态（如除零）
};

#endif // MAINWINDOW_H
