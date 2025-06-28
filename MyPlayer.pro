QT       += core gui openglwidgets network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    c_opusdec.cpp \
    h264Decode.cpp \
    main.cpp \
    widget.cpp \
    yuv420pwidget.cpp

HEADERS += \
    c_opusdec.h \
    h264Decode.h \
    widget.h \
    yuv420pwidget.h

FORMS += \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32: {
    FFMPEG_HOME=C:\GreenProgram\ffmpeg
    #设置 ffmpeg 的头文件
    INCLUDEPATH += $$FFMPEG_HOME/include

    #设置导入库的目录一边程序可以找到导入库
    # -L ：指定导入库的目录
    # -l ：指定要导入的 库名称
    LIBS +=  -L$$FFMPEG_HOME/lib \
             -lavcodec \
             -lavdevice \
             -lavfilter \
            -lavformat \
            -lavutil \
            -lpostproc \
            -lswresample \
            -lswscale
}

RESOURCES += \
    res.qrc
