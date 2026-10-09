#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Calculator; }
QT_END_NAMESPACE

/**
 * @brief 带键盘事件的计算器主窗口
 *
 * 状态管理：
 *  - firstOperand      第一个操作数（已输入运算符后锁定）
 *  - pendingOperator   待执行的运算符，空串表示当前没有待计算的操作
 *  - waitingForOperand 为 true 时，下一个数字键将开始输入新的操作数
 *  - isError           错误状态（如除数为零），此时忽略运算符/等号，按数字或清除恢复
 */
class Calculator : public QMainWindow
{
    Q_OBJECT

public:
    explicit Calculator(QWidget *parent = nullptr);
    ~Calculator();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    // —— 输入入口：鼠标按键与键盘事件最终都调用下面这些函数，复用同一套逻辑 ——
    void inputDigit(int digit);          // 数字 0~9
    void inputPoint();                   // 小数点
    void inputOperator(const QString &op); // 四则运算符 + - * /
    void inputEquals();                  // 等号 =
    void inputBackspace();               // 退格
    void inputClear();                   // 清除 C
    void inputClearEntry();              // 清除当前输入 CE
    void inputPercent();                 // 百分号 %
    void inputReciprocal();              // 倒数 1/x
    void inputSquare();                  // 平方 x²
    void inputSqrt();                    // 平方根 ²√x
    void inputSign();                    // 正负号 ±

private:
    bool calculate(const QString &op);   // 执行二元运算；除数为零时置错误状态并返回 false
    void showError(const QString &msg);  // 显示错误信息并进入错误状态
    void clearAll();                     // 恢复初始状态
    void connectButtons();               // 建立按钮信号与输入入口的连接
    void applyStyle();                   // 深色主题样式表

    Ui::Calculator *ui;
    double  firstOperand      = 0.0;
    QString pendingOperator;
    bool    waitingForOperand = true;
    bool    isError           = false;

    static const int kMaxInputLen = 15;  // 限制输入长度，防止超出 double 精度
};

#endif // CALCULATOR_H
