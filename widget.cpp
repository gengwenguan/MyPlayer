#include "widget.h"
#include "ui_widget.h"
#include <QThread>
#include <QFile>
#include <QSettings>
#include <QTimer>
#include <QAudioFormat>
#include <QMediaDevices>  // 必须包含这个头文件

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::Widget)
    , m_client(new QTcpSocket(this))
    , m_recvBuffer(new char[1024 * 1024])
    , m_H264Decode(new C_H264Decode(this))
    , m_OpusDec(new C_OpusDec(this))
    , m_doublePcmBuffer(new char[1024 * 1024])

{
    m_ui->setupUi(this);
    this->setLayout(m_ui->verticalLayout);
    m_client->setReadBufferSize(1024 * 1024); // 设置更大的读取缓冲区
    // m_client->setProxy(QNetworkProxy::NoProxy);
    // connect(m_client, SIGNAL(readyRead()), this, SLOT(ReadDataFromServer()));
    // 定时器方式主动读取
    QTimer *readTimer = new QTimer(this);
    connect(readTimer, &QTimer::timeout, this, &Widget::ReadDataFromServer);
    readTimer->start(3);  // 10ms间隔
    // 连接成功时触发
    connect(m_client, &QTcpSocket::connected, this, [this]() {
        qDebug() << "Socket 已成功连接到服务器！" << this;
    });

    // 连接成功时触发
    connect(m_client, &QTcpSocket::disconnected, this, [this]() {
        qDebug() << "Socket 断开服务器！" << this;
        m_ui->pushButton_6->setText("实时");
        m_ui->pushButton_2->setText("回放");
        renderDefaultPic();    // 渲染默认图像
        m_waitRecvLen = 0;
    });

    // 连接出错时触发
    connect(m_client, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
        qDebug() << "连接失败：" << m_client->errorString() << "error:" << error;
        m_ui->pushButton_6->setText("实时");
        m_ui->pushButton_2->setText("回放");
        renderDefaultPic();
        m_waitRecvLen = 0;
    });

    // 设置音频格式 (根据你的PCM数据实际情况调整)
    QAudioFormat format;
    format.setSampleRate(48000);        // 采样率
    format.setChannelCount(2);         // 声道数
    format.setSampleFormat(QAudioFormat::SampleFormat::Float);

    // 检查格式支持
    m_device = QMediaDevices::defaultAudioOutput();
    QList<QAudioDevice> devs = QMediaDevices::audioOutputs();
    for(auto& dev : devs){
        qDebug() << dev.id() << " dev " << dev.description();
    }

    qDebug() << m_device.id() << " m_device " << m_device.description();
    // qDebug() << QMediaDevices::audioOutputs();
    if (!m_device.isFormatSupported(format)) {
        qWarning() << "Default format not supported, trying nearest...";
        format = m_device.preferredFormat();
        qDebug() << "Adjusted format:"
                 << "Sample rate:" << format.sampleRate()
                 << "Channels:" << format.channelCount()
                 << "Sample format:" << format.sampleFormat();
    }

    // 创建音频输出 (Qt 6 方式)
    m_audioSink = new QAudioSink(m_device, format, this);
    m_audioSink->setBufferSize(19200);  // 设置合适的缓冲区大小
    connect(m_audioSink, &QAudioSink::stateChanged, this, &Widget::handleStateChanged);

    m_ioDevice = m_audioSink->start();

    if (m_audioSink->error() != QAudio::NoError) {
        qDebug() << "AudioSink Error:" << m_audioSink->error();
    }
    qDebug() << "=== Audio System Status ===";
    qDebug() << "AudioSink State:" << m_audioSink->state();
    qDebug() << "AudioSink Error:" << m_audioSink->error();
    qDebug() << "Actual Format:" << m_audioSink->format();

    // m_ui->progressBar->setVisible(false);
    // m_ui->pushButton->setVisible(false);
    QSettings settings("MyCompany", "MyApp");
    QString lastInput = settings.value("lastInput", "").toString();
    m_ui->lineEdit->setText(lastInput);

    playback_setVisible(false); //隐藏回放相关功能按钮

    qDebug() << "play.png:" << QFile::exists(":/icon/play.png");
    QImage image(":/icon/play.png");
    m_image = image.convertToFormat(QImage::Format_RGB888);
    qDebug() << "w:" << m_image.width();
    qDebug() << "h:" << m_image.height();

    convertRGBToYUV420P(); //默认图形rgb转为YUV数据

    renderDefaultPic();    // 渲染默认图像

}

Widget::~Widget()
{
    delete m_ui;
}

int Widget::OnYuvData(AVFrame *frame)
{
    m_decVideoCount++;
    if(m_decVideoCount % m_speed == 0){  //几倍速就间隔几帧渲染一次，避免短时间渲染多帧造成渲染压力
        // qDebug() << frame->linesize[0] << " frame " << frame->linesize[1] ;
        // qDebug() << frame->width << " wh " << frame->height ;
        m_ui->openGLWidget->updateYUVFrame(frame->width, frame->height, frame->data[0], frame->data[1], frame->data[2]);
    }
    return 0;
}

int Widget::OnOutputPcm(char *data, int dataLen)
{
    // 计算单声道样本数（每个样本4字节 = 32位浮点数）
    int sampleCount = dataLen / 4;

    float *src = reinterpret_cast<float*>(data);           // 源单声道数据
    float *dst = reinterpret_cast<float*>(m_doublePcmBuffer.get()); // 目标双声道数据

    // 转换为双声道：每个单声道样本复制到左右声道
    for (int i = 0; i < sampleCount; ++i) {
        float sample = src[i];
        dst[i*2] = sample;     // 左声道
        dst[i*2+1] = sample;   // 右声道
    }

    // 将双声道数据写入设备
    m_ioDevice->write(m_doublePcmBuffer.get(), dataLen*2);

    // qDebug() << " Sink->bytesFree: " << m_audioSink->bytesFree();
    // qDebug() << " Sink->state: " << m_audioSink->state();

    return 0;
}

void Widget::on_pushButton_6_clicked()
{
    if(m_ui->pushButton_6->text().compare("实时") == 0){
        qDebug() << m_ui->lineEdit->text() ;
        m_client->connectToHost(m_ui->lineEdit->text(), 56050);
        if(m_client->isOpen() && m_client->isValid()){
            m_ui->pushButton_6->setText("关闭");
            m_ui->pushButton_2->setVisible(false);
        }

        // qDebug() <<  m_client->isValid() << m_client->isOpen();
    }else{
        m_client->close();
        m_ui->pushButton_6->setText("实时");
        m_ui->pushButton_2->setVisible(true);
        renderDefaultPic();    // 渲染默认图像
    }

    QSettings settings("MyCompany", "MyApp");
    settings.setValue("lastInput", m_ui->lineEdit->text());
}


void Widget::ReadDataFromServer()
{
    if(!m_client->isOpen() || !m_client->isValid())
        return;
    //qDebug() << "ReadDataFromServer";
    if(m_waitRecvLen == 0){
        if(m_client->bytesAvailable() < 4)
            return;
        // 读取前4字节作为数据长度
        QByteArray lengthArray = m_client->read(4);
        if (lengthArray.size() != 4) {
            qDebug() << "Failed to read length bytes";
            m_client->close();
            m_ui->pushButton_6->setText("实时");
            renderDefaultPic();    // 渲染默认图像
            m_waitRecvLen = 0;
            return;
        }
        // 手动解析大端序 4 字节
        m_waitRecvLen =
            static_cast<quint32>(static_cast<quint8>(lengthArray[0]) << 24) |
            static_cast<quint32>(static_cast<quint8>(lengthArray[1]) << 16) |
            static_cast<quint32>(static_cast<quint8>(lengthArray[2]) << 8)  |
            static_cast<quint32>(static_cast<quint8>(lengthArray[3]));
        return;
    }

    if(m_client->bytesAvailable() < m_waitRecvLen){
        // qDebug() << "Avail:" << m_client->bytesAvailable() << "m_waitRecvLen:" << m_waitRecvLen;
        return;  //待读取长度不足时先返回，等待下次读取
    }

    qint64 ret = m_client->read(m_recvBuffer.get(),m_waitRecvLen);
    if (ret != m_waitRecvLen) {
        qDebug() << "Failed to read complete data ret：" << ret << "a:" << m_client->bytesAvailable();
        m_client->close();
        m_ui->pushButton_6->setText("实时");
        renderDefaultPic();    // 渲染默认图像
        m_waitRecvLen = 0;
        return;
    }

    //第一个字节数据为音视频标记数据和进度信息（回放的时候有，在视频包中），之后才是音视频实际负载数据
    bool bvideo = m_recvBuffer[0] & 0x80;
    if(!bvideo){
        m_OpusDec->DecodeOpus(m_recvBuffer.get()+1, m_waitRecvLen-1);
        m_waitRecvLen = 0;
        return;
    }

    int  progress = m_recvBuffer[0] & 0x7f;
    if(m_ui->pushButton_2->text().compare("关闭") == 0 &&
        !m_sliderPressed &&
        m_ui->horizontalSlider->value() != progress)
    {  //正在回放且没有手动拖动进度条且与当前进度不同时更新进度
        if(m_countChgProgress == 0){
            m_ui->horizontalSlider->blockSignals(true); //取消信号触发
            m_ui->horizontalSlider->setSliderPosition(progress);
            m_ui->horizontalSlider->blockSignals(false);
        }else{
            //每次减一，到0时再调整，避免刚手动拖动后又收到服务器发来之前的进度，进度条反复跳动
            m_countChgProgress--;
        }
    }

    m_H264Decode->AsyncDecode(m_recvBuffer.get()+1, m_waitRecvLen-1);
    // qDebug() << "Received data length:" << m_waitRecvLen ;
    m_waitRecvLen = 0;
    return;
}

void Widget::handleStateChanged(QAudio::State state)
{
    qDebug() << "Audio state changed to:";
    switch (state) {
    case QAudio::ActiveState:
        qDebug() << "Active - audio is running";
        break;
    case QAudio::SuspendedState:
        qDebug() << "Suspended - audio is suspended";
        break;
    case QAudio::StoppedState:
        qDebug() << "Stopped - audio is stopped";
        // Check error
        if(m_audioSink->error() != QAudio::NoError) {
            qDebug() << "Audio error:" << m_audioSink->error();
        }
        break;
    case QAudio::IdleState:
        qDebug() << "Idle - no data available";
        break;
    default:
        qDebug() << "Unknown state";
    }
}

void Widget::playback_setVisible(bool visible)
{

    m_ui->horizontalSlider->setVisible(visible);
    m_ui->pushButton->setVisible(visible);
    m_ui->pushButton_3->setVisible(visible);
    m_ui->pushButton_4->setVisible(visible);
    m_ui->pushButton_5->setVisible(visible);
    m_ui->pushButton_7->setVisible(visible);
}

//向服务器发送的控制消息：0~100进度条拖动、101快退、102快进、104上一个文件、105下一个文件、107加速、108停止加速
void Widget::send_ctrl_message(char msg)
{
    m_client->write(&msg, 1);
}


void Widget::on_pushButton_2_clicked()
{
    if(m_ui->pushButton_2->text().compare("回放") == 0){
        m_client->connectToHost(m_ui->lineEdit->text(), 56060);
        if(m_client->isOpen() && m_client->isValid()){
            m_ui->pushButton_2->setText("关闭");
            m_ui->pushButton_6->setVisible(false);
            playback_setVisible(true);
        }
    }else{
        m_client->close();
        m_ui->pushButton_2->setText("回放");
        m_ui->pushButton_6->setVisible(true);
        playback_setVisible(false);
        renderDefaultPic();    // 渲染默认图像
    }

    QSettings settings("MyCompany", "MyApp");
    settings.setValue("lastInput", m_ui->lineEdit->text());
}



void Widget::on_horizontalSlider_sliderPressed()
{
    m_sliderPressed = true;
}


void Widget::on_horizontalSlider_sliderReleased()
{
    m_sliderPressed = false;
}


void Widget::on_horizontalSlider_valueChanged(int value)
{
    qDebug() << "valueChanged:" << value;
    send_ctrl_message(value);
    m_countChgProgress = 20; //服务器发来20次调整进度条消息之后再调整
}

void Widget::on_pushButton_clicked()
{
    send_ctrl_message(PrevFile);
}


void Widget::on_pushButton_3_clicked()
{
    send_ctrl_message(NextFile);
}


void Widget::on_pushButton_5_clicked()
{
    send_ctrl_message(FastRewind);
}


void Widget::on_pushButton_4_clicked()
{
    send_ctrl_message(FastForward);
}


void Widget::on_pushButton_7_clicked()
{
    if(m_ui->pushButton_7->text().compare("x3") == 0){
        send_ctrl_message(SpeedUp);
        m_ui->pushButton_7->setText("x1");
        m_speed = 3;
    }else{
        send_ctrl_message(StopSpeedUp);
        m_ui->pushButton_7->setText("x3");
        m_speed = 1;
    }
}

bool Widget::convertRGBToYUV420P()
{
    int width = m_image.width();
    int height = m_image.height();
    m_yData.resize(width * height);
    m_uData.resize((width / 2) * (height / 2));
    m_vData.resize((width / 2) * (height / 2));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            QRgb rgb = m_image.pixel(x, y);
            int r = qRed(rgb);
            int g = qGreen(rgb);
            int b = qBlue(rgb);

            // RGB转YUV公式（BT.601标准）
            int Y = (0.299 * r + 0.587 * g + 0.114 * b);
            int U = (-0.14713 * r - 0.28886 * g + 0.436 * b + 128);
            int V = (0.615 * r - 0.51499 * g - 0.10001 * b + 128);

            m_yData[y * width + x] = qBound(0, Y, 255);

            if (x % 2 == 0 && y % 2 == 0) {
                int uvIdx = (y / 2) * (width / 2) + (x / 2);
                m_uData[uvIdx] = qBound(0, U, 255);
                m_vData[uvIdx] = qBound(0, V, 255);
            }
        }
    }
    return true;
}

void Widget::renderDefaultPic(){
    if(m_image.width() > 0 && m_yData.size() > 0){
        m_ui->openGLWidget->updateYUVFrame(m_image.width(), m_image.height(), m_yData.data(), m_uData.data(), m_vData.data());
    }
}
