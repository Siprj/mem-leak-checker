TEMPLATE = lib
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    memory-hooks.cpp

LIBS += -fPIC -ldl

QMAKE_CXXFLAGS += -std=c++11
QMAKE_LFLAGS += -std=c++11

HEADERS += \
    config.h
