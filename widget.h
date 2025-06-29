#ifndef WIDGET_H
#define WIDGET_H
#include <memory>
#include <QWidget>
#include <QTcpSocket>
//#include <AudioIODevice.h>
#include <QAudioSink>
#include "h264Decode.h"
#include "c_opusdec.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
             , public C_H264Decode::C_Listener
             , public C_OpusDec::C_Listener
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    int OnYuvData(AVFrame* frame) override;

    int OnOutputPcm(char* data, int dataLen) override;

private slots:
    void on_pushButton_6_clicked();
    void ReadDataFromServer();

    void handleStateChanged(QAudio::State state);

    void on_pushButton_2_clicked();

    void on_horizontalSlider_sliderPressed();

    void on_horizontalSlider_sliderReleased();

    void on_horizontalSlider_valueChanged(int value);


    void on_pushButton_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_7_clicked();

private:
    void playback_setVisible(bool visible); //隐藏回放相关功能按钮


    bool convertRGBToYUV420P();

    void renderDefaultPic();//渲染默认图形

    // 控制消息枚举（普通C枚举）
    enum ControlMsg {
        FastRewind = 101,   // 快退
        FastForward = 102,  // 快进
        PrevFile = 104,     // 上一个文件
        NextFile = 105,     // 下一个文件
        SpeedUp = 107,      // 加速
        StopSpeedUp = 108   // 停止加速
    };
    //向服务器发送的控制消息：0~100进度条拖动、101快退、102快进、104上一个文件、105下一个文件、107加速、108停止加速
    void send_ctrl_message(char msg);      //向服务器发送控制消息

private:
    Ui::Widget *m_ui;
    QTcpSocket* m_client;
    qint64 m_waitRecvLen = 0;
    std::unique_ptr<char[]> m_recvBuffer; //1M的接收缓冲区
    std::unique_ptr<C_H264Decode> m_H264Decode;
    std::unique_ptr<C_OpusDec>    m_OpusDec;
    QAudioDevice m_device;
    // AudioIODevice *m_audioDevice;
    QAudioSink *m_audioSink;  // Qt 6 使用 QAudioSink 替代 QAudioOutput
    QIODevice  *m_ioDevice;
    std::unique_ptr<char[]> m_doublePcmBuffer; //双声道pcm缓冲区

    bool m_sliderPressed = false;         //进度条是否按下
    int  m_countChgProgress = 0;
    unsigned int  m_speed = 1;            //播放的倍速
    unsigned int  m_decVideoCount = 0;    //解码的视频帧数

    QImage m_image;  //默认底图
    QVector<uint8_t> m_yData;
    QVector<uint8_t> m_uData;
    QVector<uint8_t> m_vData;
};
#endif // WIDGET_H
