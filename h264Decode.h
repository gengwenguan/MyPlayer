/*********************************************************************************
  *Copyright(C),Your Company
  *FileName:  tcpClient.h
  *Author:    gengwenguan
  *Date:      2024-11-16
  *Description:  h264�����࣬ʹ��ffmpeg��h264����ΪYUV420����
**********************************************************************************/
#pragma once
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <queue>
#include <vector>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

/* ���������������ʵ���ź��� */
class C_Semaphore
{
public:
    C_Semaphore(int value = 0) : count{ value } { }

    void wait() {
        std::unique_lock <std::mutex > lock{ mutex };
        condition.wait(lock, [this](){ return count > 0; });
        --count;
    }
    void  signal() {
        {
            std::lock_guard <std::mutex > lock{ mutex };
            ++count;
        }
        condition.notify_one();  // notify one !
    }
    int GetCount() { return count; }

private:
    int count;
    std::mutex mutex;
    std::condition_variable condition;
};

class C_H264Decode
{
public:
    class C_Listener
    {
    public:
        virtual ~C_Listener() = default;
        /*�¿ͻ��������¼�*/
        virtual int OnYuvData(AVFrame* frame) = 0;
    };

public:
    C_H264Decode(C_Listener* pListener);
    ~C_H264Decode();

    //ͬ������ӿ�
    int Decode(char* data, int dataLen);
    //�첽����ӿڣ�ʹ���ڲ����߳̽��룬�ӿڵ��ú���������
    int AsyncDecode(char* data, int dataLen);

private:
    void DecThread();

private:
    C_Listener*      m_pListener;
    const AVCodec*   m_codec;
    AVCodecContext*  m_codec_context;
    AVFrame*         m_frame;
    AVPacket*        m_packet;
    C_Semaphore      m_SemEmpty;  //�ղ��ź����������ź���
    C_Semaphore      m_SemFull;
    std::atomic<bool>m_RunFlg;
    std::thread      m_DecThread;
    std::queue<std::vector<char>> m_H264Queue;
};
