QT -= gui

CONFIG += console
CONFIG -= app_bundle

# QMAKE_CXXFLAGS +=
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.c

HEADERS +=

LIBS += -lusb-1.0 -lftdi

DISTFILES += \
    LICENSE \
    README.md
