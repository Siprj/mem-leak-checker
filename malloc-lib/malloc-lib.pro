TEMPLATE = lib
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    my-malloc.cpp

LIBS += -fPIC -ldl

OTHER_FILES += \
    README.md \
    my-malloc.graphml

QMAKE_CXXFLAGS += -std=c++11

HEADERS += \
    config.h
