QT       += core gui widgets

CONFIG   += c++17 console
TEMPLATE  = app
TARGET    = calculator_test

# 复用主工程的实现，直接对 Calculator 窗口做整体功能测试
SOURCES  += calculator_test.cpp ../calculator.cpp
HEADERS  += ../calculator.h
FORMS    += ../calculator.ui
