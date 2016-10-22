TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    thread_pool_simple.c

HEADERS += \
    threadpool.h

LIBS += -pthread
