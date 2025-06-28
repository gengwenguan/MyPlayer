#include "widget.h"

#include <QApplication>
// ffmpeg 是纯 C 语言的代码，在 C++ 当中不能直接进行 include
extern "C" {
#include <libavcodec/avcodec.h>
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
    qDebug() << "Runtime Qt version:" << qVersion();
    return a.exec();
}
