TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

LIBS += -ldl

QMAKE_CXXFLAGS = -std=c++11 -pthread -Wl,--no-as-needed -mcpu=arm926ej-s -mfloat-abi=soft -funwind-tables -rdynamic
QMAKE_LFLAGS = -std=c++11 -pthread -Wl,--no-as-needed -mcpu=arm926ej-s -mfloat-abi=soft -funwind-tables -rdynamic

## Uncomment this to compile it for arm
QMAKE_CXX = arm-linux-gnueabi-g++
QMAKE_CC = arm-linux-gnueabi-g++
QMAKE_LINK = arm-linux-gnueabi-g++
