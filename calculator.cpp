#include "calculator.h"
#include "ui_calculator.h"

#include <QPushButton>
#include <QKeyEvent>
#include <cmath>

Calculator::Calculator(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Calculator)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("计算器"));

    // 按键不抢占焦点，保证键盘事件始终由主窗口接收（见 keyPressEvent）
    ui->display->setFocusPolicy(Qt::NoFocus);
    for (QPushButton *btn : findChildren<QPushButton *>())
        btn->setFocusPolicy(Qt::NoFocus);

    connectButtons();
    applyStyle();
}

Calculator::~Calculator()
{
    delete ui;
}

// ==================== 界面样式 ====================

void Calculator::applyStyle()
{
    // 仿手机计算器的深色主题：
    //  - 显示屏：黑底白字、右对齐大字号
    //  - 数字键：深灰；C/⌫ 功能键：浅灰；运算符与等号：橙色高亮
    setFixedSize(360, 640);
    setStyleSheet(QStringLiteral(R"(
        #Calculator {
            background-color: #2b2b2b;
        }
        QLineEdit#display {
            background-color: #1e1e1e;
            color: #ffffff;
            border: none;
            font-size: 34px;
            padding: 6px 10px;
        }
        QPushButton {
            border: none;
            border-radius: 10px;
            font-size: 20px;
            color: #ffffff;
            background-color: #4e4e4e;
        }
        QPushButton:hover  { background-color: #5c5c5c; }
        QPushButton:pressed { background-color: #7a7a7a; }
        QPushButton#btnClear, QPushButton#btnBackspace,
        QPushButton#btnClearEntry, QPushButton#btnPercent,
        QPushButton#btnReciprocal, QPushButton#btnSquare,
        QPushButton#btnSqrt, QPushButton#btnSign {
            background-color: #a5a5a5;
            color: #1c1c1c;
        }
        QPushButton#btnClear:hover, QPushButton#btnBackspace:hover,
        QPushButton#btnClearEntry:hover, QPushButton#btnPercent:hover,
        QPushButton#btnReciprocal:hover, QPushButton#btnSquare:hover,
        QPushButton#btnSqrt:hover, QPushButton#btnSign:hover {
            background-color: #b8b8b8;
        }
        QPushButton#btnPlus, QPushButton#btnMinus, QPushButton#btnMul,
        QPushButton#btnDiv, QPushButton#btnEquals {
            background-color: #ff9f0a;
            font-size: 24px;
        }
        QPushButton#btnPlus:hover, QPushButton#btnMinus:hover, QPushButton#btnMul:hover,
        QPushButton#btnDiv:hover, QPushButton#btnEquals:hover {
            background-color: #ffb340;
        }
    )"));
}

// ==================== 输入入口（鼠标与键盘共用） ====================

void Calculator::connectButtons()
{
    // 数字按钮 0~9：统一连接到 inputDigit，并传入对应数字
    const QList<QPushButton *> digitButtons = {
        ui->btn0, ui->btn1, ui->btn2, ui->btn3, ui->btn4,
        ui->btn5, ui->btn6, ui->btn7, ui->btn8, ui->btn9
    };
    for (int d = 0; d < digitButtons.size(); ++d) {
        connect(digitButtons[d], &QPushButton::clicked, this, [this, d] { inputDigit(d); });
    }

    connect(ui->btnPoint, &QPushButton::clicked, this, &Calculator::inputPoint);

    // 四则运算按钮：按钮文字用 × ÷ 显示，内部统一用 * / 参与运算
    connect(ui->btnPlus,  &QPushButton::clicked, this, [this] { inputOperator("+"); });
    connect(ui->btnMinus, &QPushButton::clicked, this, [this] { inputOperator("-"); });
    connect(ui->btnMul,   &QPushButton::clicked, this, [this] { inputOperator("*"); });
    connect(ui->btnDiv,   &QPushButton::clicked, this, [this] { inputOperator("/"); });

    connect(ui->btnEquals,    &QPushButton::clicked, this, &Calculator::inputEquals);
    connect(ui->btnBackspace, &QPushButton::clicked, this, &Calculator::inputBackspace);
    connect(ui->btnClear,     &QPushButton::clicked, this, &Calculator::inputClear);
    connect(ui->btnClearEntry,&QPushButton::clicked, this, &Calculator::inputClearEntry);
    connect(ui->btnPercent,   &QPushButton::clicked, this, &Calculator::inputPercent);
    connect(ui->btnReciprocal,&QPushButton::clicked, this, &Calculator::inputReciprocal);
    connect(ui->btnSquare,    &QPushButton::clicked, this, &Calculator::inputSquare);
    connect(ui->btnSqrt,      &QPushButton::clicked, this, &Calculator::inputSqrt);
    connect(ui->btnSign,      &QPushButton::clicked, this, &Calculator::inputSign);
}

void Calculator::inputDigit(int digit)
{
    // 错误状态（如除零）下按数字：先复位再重新开始输入
    if (isError)
        clearAll();

    if (waitingForOperand) {
        // 运算符/等号之后的首个数字：开始输入新的操作数
        ui->display->setText(QString::number(digit));
        waitingForOperand = false;
    } else if (ui->display->text() == "0") {
        // 前导零处理：继续输入 0 无效，非 0 数字直接替换当前的 0
        if (digit == 0)
            return;
        ui->display->setText(QString::number(digit));
    } else if (ui->display->text().length() < kMaxInputLen) {
        // 限制输入长度，防止超出 double 有效精度
        ui->display->setText(ui->display->text() + QString::number(digit));
    }
}

void Calculator::inputPoint()
{
    if (isError)
        clearAll();

    if (waitingForOperand) {
        // 运算符/等号之后先输入小数点：自动补全整数部分，即 "0."
        ui->display->setText(QStringLiteral("0."));
        waitingForOperand = false;
    } else if (!ui->display->text().contains('.')) {
        // 同一操作数内只允许出现一个小数点
        ui->display->setText(ui->display->text() + '.');
    }
    // 已含小数点则忽略本次输入，避免 "1.2.3" 这类非法数据
}

void Calculator::inputOperator(const QString &op)
{
    if (isError)
        return; // 错误状态下忽略运算符，须先按数字或 C 复位

    if (!pendingOperator.isEmpty() && !waitingForOperand) {
        // 连续运算：先算出前一步的结果，作为新的第一个操作数
        if (!calculate(pendingOperator))
            return; // 计算出错（如除零），保持错误状态
    } else if (pendingOperator.isEmpty()) {
        // 没有待计算的运算：当前显示值就是第一个操作数
        firstOperand = ui->display->text().toDouble();
    }
    // 连续按运算符时只替换 pendingOperator，相当于允许改正刚选错的运算符

    pendingOperator = op;
    waitingForOperand = true; // 下一个数字键开始输入第二个操作数
}

void Calculator::inputEquals()
{
    // "5 + ="（还没输入第二个操作数就按等号）属于异常输入，直接忽略
    if (isError || pendingOperator.isEmpty() || waitingForOperand)
        return;

    calculate(pendingOperator);
    pendingOperator.clear();
    waitingForOperand = true; // 显示结果后，等待用户开始新的输入
}

void Calculator::inputBackspace()
{
    // 仅在正在输入操作数时有效；结果显示、错误状态下按退格无效
    if (isError || waitingForOperand)
        return;

    QString text = ui->display->text();
    text.chop(1);
    ui->display->setText(text.isEmpty() ? QStringLiteral("0") : text);
}

void Calculator::inputClear()
{
    // C 键：恢复到初始状态
    clearAll();
}

void Calculator::inputClearEntry()
{
    // CE 键：只清除当前输入，保留待计算的运算
    if (isError)
        clearAll();
    else
        ui->display->setText(QStringLiteral("0"));
}

void Calculator::inputPercent()
{
    if (isError)
        return;
    double v = ui->display->text().toDouble() / 100.0;
    ui->display->setText(QString::number(v, 'g', 12));
}

void Calculator::inputReciprocal()
{
    if (isError)
        return;
    double v = ui->display->text().toDouble();
    if (v == 0.0) {
        showError(QStringLiteral("错误：除数不能为0"));
        return;
    }
    ui->display->setText(QString::number(1.0 / v, 'g', 12));
}

void Calculator::inputSquare()
{
    if (isError)
        return;
    double v = ui->display->text().toDouble();
    ui->display->setText(QString::number(v * v, 'g', 12));
}

void Calculator::inputSqrt()
{
    if (isError)
        return;
    double v = ui->display->text().toDouble();
    if (v < 0.0) {
        showError(QStringLiteral("错误：负数不能开平方"));
        return;
    }
    ui->display->setText(QString::number(std::sqrt(v), 'g', 12));
}

void Calculator::inputSign()
{
    if (isError)
        return;
    QString text = ui->display->text();
    if (text == QStringLiteral("0"))
        return;
    if (text.startsWith('-'))
        text.remove(0, 1);
    else
        text.prepend('-');
    ui->display->setText(text);
}

// ==================== 内部处理 ====================

bool Calculator::calculate(const QString &op)
{
    // 第二个操作数取自当前显示内容（"0." 之类文本 toDouble 后为 0）
    const double secondOperand = ui->display->text().toDouble();
    double result = 0.0;

    if (op == "+") {
        result = firstOperand + secondOperand;
    } else if (op == "-") {
        result = firstOperand - secondOperand;
    } else if (op == "*") {
        result = firstOperand * secondOperand;
    } else if (op == "/") {
        if (secondOperand == 0.0) {
            showError(QStringLiteral("错误：除数不能为0"));
            return false;
        }
        result = firstOperand / secondOperand;
    }

    firstOperand = result; // 供连续运算使用
    // 以 'g' 格式保留 12 位有效数字，消除 0.1+0.2=0.30000000000000004 这类浮点尾差
    ui->display->setText(QString::number(result, 'g', 12));
    return true;
}

void Calculator::showError(const QString &msg)
{
    ui->display->setText(msg);
    isError = true;
    pendingOperator.clear(); // 放弃未完成的运算，等待用户复位
}

void Calculator::clearAll()
{
    // 复位所有状态：显示清零、放弃未完成的运算、退出错误状态
    ui->display->setText(QStringLiteral("0"));
    firstOperand = 0.0;
    pendingOperator.clear();
    waitingForOperand = true;
    isError = false;
}

void Calculator::keyPressEvent(QKeyEvent *event)
{
    // 键盘输入与鼠标按钮共用同一套 inputXxx 处理逻辑，保证两者行为完全一致。
    // 按钮与显示框均不抢占焦点（NoFocus），键盘事件始终由主窗口接收。
    const QString text = event->text();

    // 数字 0~9（主键盘与小键盘均可）
    if (text.size() == 1 && text.at(0).isDigit()) {
        inputDigit(text.at(0).digitValue());
        return;
    }
    // 小数点（小键盘的 . 同样产生 "."）
    if (text == QStringLiteral(".")) {
        inputPoint();
        return;
    }
    // 四则运算符
    if (text == "+" || text == "-" || text == "*" || text == "/") {
        inputOperator(text);
        return;
    }

    // 功能键
    switch (event->key()) {
    case Qt::Key_Enter:      // 小键盘回车
    case Qt::Key_Return:     // 主键盘回车
    case Qt::Key_Equal:      // =
        inputEquals();
        break;
    case Qt::Key_Backspace:
        inputBackspace();
        break;
    case Qt::Key_Escape:     // Esc
    case Qt::Key_Delete:
    case Qt::Key_C:          // 字母 c / C
        inputClear();
        break;
    case Qt::Key_Percent:    // %
        inputPercent();
        break;
    case Qt::Key_S:          // s -> sqrt
        inputSqrt();
        break;
    case Qt::Key_R:          // r -> reciprocal
        inputReciprocal();
        break;
    case Qt::Key_Q:          // q -> square
        inputSquare();
        break;
    case Qt::Key_N:          // n -> sign
        inputSign();
        break;
    case Qt::Key_E:          // e -> clear entry
        inputClearEntry();
        break;
    default:
        QMainWindow::keyPressEvent(event); // 其余按键交给基类
    }
}
