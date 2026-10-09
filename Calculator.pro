
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG   += c++17

TEMPLATE  = app
TARGET    = Calculator

SOURCES  += main.cpp calculator.cpp
HEADERS  += calculator.h
FORMS    += calculator.ui
