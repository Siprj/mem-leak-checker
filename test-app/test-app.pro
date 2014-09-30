TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

LIBS += -ldl

QMAKE_CXXFLAGS += -std=c++11 -pthread
QMAKE_LFLAGS += -std=c++11 -pthread
