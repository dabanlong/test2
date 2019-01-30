TEMPLATE = app

DEFINES += \

QT += core gui widgets concurrent opengl

CONFIG += debug c++14

HEADERS += test9.h \

SOURCES += main.cpp \
           test9.cpp \

QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage

LIBS += -lgcov
