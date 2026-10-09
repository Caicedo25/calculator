/**
 * 计算器自动化功能测试（轻量控制台版）
 *
 * 直接构造 Calculator 主窗口：
 *  - 鼠标路径：调用 QPushButton::click() 触发与真实点击一致的信号
 *  - 键盘路径：用 QCoreApplication::sendEvent 发送 QKeyEvent
 * 覆盖：四则运算、小数输入、连续运算、除零、退格、清除与异常输入，
 * 并验证键盘输入与鼠标输入行为一致。
 */
#include <QApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QKeyEvent>
#include <QTextStream>
#include "../calculator.h"

static Calculator *calc = nullptr;
static int failures = 0;

static QPushButton *btn(const char *name) { return calc->findChild<QPushButton *>(name); }
static QLineEdit   *display()             { return calc->findChild<QLineEdit *>("display"); }

static void click(const char *name) { btn(name)->click(); }

static void pressKey(int key, const QString &text)
{
    QKeyEvent e(QEvent::KeyPress, key, Qt::NoModifier, text);
    QCoreApplication::sendEvent(calc, &e);
}

static void check(const char *name, const QString &expected)
{
    const QString actual = display()->text();
    const bool ok = (actual == expected);
    if (!ok)
        ++failures;
    QTextStream(stdout) << (ok ? "[通过] " : "[失败] ") << name
                        << "  显示=" << actual << "  期望=" << expected << "\n";
}

// 每个用例从全新窗口开始
static void newCalc()
{
    delete calc;
    calc = new Calculator();
    calc->show();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // ---- 基础四则运算（鼠标） ----
    newCalc();
    click("btn5"); click("btnPlus"); click("btn3"); click("btnEquals");
    check("加法 5+3", "8");

    newCalc();
    click("btn9"); click("btnMinus"); click("btn4"); click("btnEquals");
    check("减法 9-4", "5");

    newCalc();
    click("btn6"); click("btnMul"); click("btn7"); click("btnEquals");
    check("乘法 6×7", "42");

    newCalc();
    click("btn8"); click("btnDiv"); click("btn2"); click("btnEquals");
    check("除法 8÷2", "4");

    // ---- 小数输入 ----
    newCalc();
    click("btn1"); click("btnPoint"); click("btn5"); click("btnPlus");
    click("btn2"); click("btnPoint"); click("btn2"); click("btn5"); click("btnEquals");
    check("小数加法 1.5+2.25", "3.75");

    newCalc();
    click("btn1"); click("btnPoint"); click("btn2"); click("btnPoint"); click("btn3");
    check("重复小数点被忽略 1.2.3", "1.23");

    newCalc();
    click("btn0"); click("btn0"); click("btn0"); click("btn5");
    check("前导零 0005", "5");

    newCalc();
    click("btnPoint"); click("btn5");
    check("先输入小数点补全为 0.5", "0.5");

    // ---- 浮点尾差 ----
    newCalc();
    click("btn0"); click("btnPoint"); click("btn1"); click("btnPlus");
    click("btn0"); click("btnPoint"); click("btn2"); click("btnEquals");
    check("0.1+0.2 显示 0.3", "0.3");

    // ---- 连续运算与结果后继续 ----
    newCalc();
    click("btn5"); click("btnPlus"); click("btn3"); click("btnMul"); click("btn2"); click("btnEquals");
    check("连续运算 5+3×2", "16");

    newCalc();
    click("btn2"); click("btn0"); click("btnMinus"); click("btn8"); click("btnEquals");
    click("btnDiv"); click("btn3"); click("btnEquals");
    check("结果后继续运算 20-8=12, ÷3", "4");

    newCalc();
    click("btn2"); click("btnPlus"); click("btn3"); click("btnEquals");
    click("btn7"); click("btn0");
    check("计算完成后输入数字开始新操作数", "70");

    // ---- 连续运算符：后者替换前者 ----
    newCalc();
    click("btn5"); click("btnPlus"); click("btnMul"); click("btn3"); click("btnEquals");
    check("连续运算符 5+×3", "15");

    // ---- 等号异常输入 ----
    newCalc();
    click("btn5"); click("btnPlus"); click("btnEquals");
    check("5+＝ 缺少第二操作数被忽略", "5");

    // ---- 除零 ----
    newCalc();
    click("btn9"); click("btnDiv"); click("btn0"); click("btnEquals");
    check("除零 9÷0", "错误：除数不能为0");

    newCalc();
    click("btn9"); click("btnDiv"); click("btn0"); click("btnEquals");
    click("btnPlus"); click("btnEquals");
    check("错误状态下运算符被忽略", "错误：除数不能为0");

    newCalc();
    click("btn9"); click("btnDiv"); click("btn0"); click("btnEquals");
    click("btn4"); click("btnDiv"); click("btn2"); click("btnEquals");
    check("错误后按数字复位重算", "2");

    newCalc();
    click("btn5"); click("btnDiv"); click("btn0"); click("btnPoint"); click("btnEquals");
    check("0. 也视为除数 0", "错误：除数不能为0");

    // ---- 退格 ----
    newCalc();
    click("btn1"); click("btn2"); click("btn3"); click("btnBackspace");
    check("退格 123→12", "12");

    newCalc();
    click("btn1"); click("btnBackspace"); click("btnBackspace"); click("btn7");
    check("退到空显示 0 后可继续输入", "7");

    newCalc();
    click("btn2"); click("btnPlus"); click("btn3"); click("btnEquals"); click("btnBackspace");
    check("结果状态退格无效", "5");

    // ---- 清除 ----
    newCalc();
    click("btn9"); click("btn9"); click("btnClear");
    check("清除归零", "0");
    click("btn3"); click("btnPlus"); click("btn3"); click("btnEquals");
    check("清除后重新计算 3+3", "6");

    // ---- 键盘输入，与鼠标行为一致 ----
    newCalc();
    pressKey(Qt::Key_1, "1"); pressKey(Qt::Key_2, "2"); pressKey(Qt::Key_Period, ".");
    pressKey(Qt::Key_5, "5"); pressKey(Qt::Key_Asterisk, "*"); pressKey(Qt::Key_2, "2");
    pressKey(Qt::Key_Enter, "\r");
    check("键盘 12.5*2 回车", "25");

    newCalc();
    click("btn6"); click("btnDiv");                 // 前半段用鼠标
    pressKey(Qt::Key_3, "3"); pressKey(Qt::Key_Equal, "="); // 后半段用键盘
    check("鼠标键盘混合输入 6/3", "2");

    newCalc();
    pressKey(Qt::Key_8, "8"); pressKey(Qt::Key_Escape, QString(QChar(27)));
    check("键盘 Esc 清除", "0");

    newCalc();
    click("btn1"); click("btn2"); click("btn3");
    pressKey(Qt::Key_Backspace, "\b");
    check("键盘退格 123→12", "12");
    pressKey(Qt::Key_Delete, QString(QChar(127)));
    check("键盘 Delete 清除", "0");

    // ---- CE ----
    newCalc();
    click("btn5"); click("btnPlus"); click("btn3"); click("btnClearEntry");
    click("btn2"); click("btnEquals");
    check("CE 清除当前输入 5+3→CE→2", "7");

    // ---- 百分号 % ----
    newCalc();
    click("btn5"); click("btn0"); click("btnPercent");
    check("50% = 0.5", "0.5");

    // ---- 倒数 1/x ----
    newCalc();
    click("btn4"); click("btnReciprocal");
    check("1/4 = 0.25", "0.25");

    newCalc();
    click("btn0"); click("btnReciprocal");
    check("1/0 错误", "错误：除数不能为0");

    // ---- 平方 x² ----
    newCalc();
    click("btn5"); click("btnSquare");
    check("5² = 25", "25");

    // ---- 平方根 ²√x ----
    newCalc();
    click("btn1"); click("btn6"); click("btnSqrt");
    check("√16 = 4", "4");

    newCalc();
    click("btn4"); click("btnSign"); click("btnSqrt");
    check("√(-4) 错误", "错误：负数不能开平方");

    // ---- 正负号 ± ----
    newCalc();
    click("btn5"); click("btnSign");
    check("±5 = -5", "-5");
    click("btnSign");
    check("--5 = 5", "5");
    click("btnSign"); click("btnPlus"); click("btn3"); click("btnEquals");
    check("-5+3 = -2", "-2");

    delete calc;

    QTextStream(stdout) << (failures == 0 ? "\n全部测试通过\n" : QString("\n失败 %1 项\n").arg(failures));
    return failures == 0 ? 0 : 1;
}
