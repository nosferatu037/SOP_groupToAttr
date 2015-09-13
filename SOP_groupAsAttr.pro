#-------------------------------------------------
#
# Project created by QtCreator 2015-09-06T22:39:32
#
#-------------------------------------------------

QT       -= core gui
TEMPLATE = lib
VERSION = $$(HOUDINI_VERSION)
CONFIG -= qt
CONFIG += plugin


TARGET = sop_groupAsAttr
INCLUDEPATH += $$(HFS)/toolkit/include
LIBS += -L$$(HFS)/dsolib \
    -lGL \
    -lGLU \
    -lX11 \
    -lXi \
    -lXmu \
    -lXext \
    -lHoudiniUI \
    -lHoudiniOPZ \
    -lHoudiniOP3 \
    -lHoudiniOP2 \
    -lHoudiniOP1 \
    -lHoudiniSIM \
    -lHoudiniGEO \
    -lHoudiniPRM \
    -lHoudiniUT
QMAKE_CXXFLAGS += -Wall \
    -W \
    -Wno-parentheses \
    -Wno-sign-compare \
    -Wno-reorder \
    -Wno-uninitialized \
    -Wunused \
    -Wno-unused-parameter \
    -Wno-deprecated \
    -DSIZEOF_VOID_P=8 \
    -D_GNU_SOURCE \
    -DLINUX \
    -m64 \
    -DAMD64 \
    -DSESI_LITTLE_ENDIAN \
    -DENABLE_THREADS \
    -DUSE_PTHREADS \
    -DENABLE_UI_THREADS \
    -DGCC3 \
    -DGCC4 \
    -DMAKING_DSO \
    -DDLLEXPORT="" \
    -shared \
#    -O0 \ #debug
    -O2 \
    -g #returns more info when crashes


DEFINES += SOP_GROUPASATTR

SOURCES += sop_groupasattr.cpp


HEADERS += sop_groupasattr.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += stevensSOP_groupAttr_code.cpp

