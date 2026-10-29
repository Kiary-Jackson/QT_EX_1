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
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;  // 键盘事件

private slots:
    void onDigitClicked();    // 数字按钮（0-9）
    void onOperatorClicked(); // 运算符按钮（+ - × ÷）
    void onEqualClicked();    // 等号
    void onClearClicked();    // 清除 C
    void onClearEntryClicked(); // 清除当前输入 CE
    void onBackspaceClicked();  // 退格
    void onNegateClicked();   // 正负号 ±
    void onDecimalClicked();  // 小数点
    void onPercentClicked();  // 百分号 %

private:
    void calculate();         // 执行运算
    void appendDigit(int digit); // 追加数字
    void setOperator(const QString &op); // 设置运算符
    void updateDisplay();     // 更新显示

    Ui::MainWindow *ui;

    // 计算器状态
    double m_firstOperand;    // 第一个操作数
    double m_secondOperand;   // 第二个操作数
    QString m_operator;       // 当前运算符
    bool m_waitingForSecond;  // 是否等待第二个操作数
    bool m_hasResult;         // 是否刚计算完结果
};

#endif // MAINWINDOW_H
