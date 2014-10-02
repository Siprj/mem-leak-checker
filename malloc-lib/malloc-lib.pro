TEMPLATE = lib
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    memory-hooks.cpp \
    data-chunk-storage.cpp

HEADERS += \
    config.h \
    data-chunk-storage.h \
    data-chunk-types.h \
    original-memory-fnc-pointers.h


LIBS += -fPIC -ldl

QMAKE_CXXFLAGS += -std=c++11
QMAKE_LFLAGS += -std=c++11

## Uncomment this to compile it for arm
#QMAKE_CXX = arm-linux-gnueabi-g++
#QMAKE_CC = arm-linux-gnueabi-g++
#QMAKE_LINK = arm-linux-gnueabi-g++
