/*********************************************************************************
  *Copyright(C),Your Company
  *FileName:  tcpClient.h
  *Author:    gengwenguan
  *Date:      2024-11-16
  *Description:  h264解码类，使用ffmpeg将h264解码为YUV420数据
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

/* 锁和条件变量配合实现信号量 */
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
        /*新客户端连接事件*/
        virtual int OnYuvData(AVFrame* frame) = 0;
    };

public:
    C_H264Decode(C_Listener* pListener);
    ~C_H264Decode();

    //同步解码接口
    int Decode(char* data, int dataLen);
    //异步解码接口，使用内部的线程解码，接口调用后立即返回
    int AsyncDecode(char* data, int dataLen);

private:
    void DecThread();

private:
    C_Listener*      m_pListener;
    const AVCodec*   m_codec;
    AVCodecContext*  m_codec_context;
    AVFrame*         m_frame;
    AVPacket*        m_packet;
    C_Semaphore      m_SemEmpty;  //空槽信号量和满槽信号量
    C_Semaphore      m_SemFull;
    std::atomic<bool>m_RunFlg;
    std::thread      m_DecThread;
    std::queue<std::vector<char>> m_H264Queue;
};
