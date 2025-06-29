#include <iostream>
#include <cstdint>
#include "h264Decode.h"

C_H264Decode::C_H264Decode(C_Listener* pListener)
    :m_pListener(pListener),
    m_SemEmpty(5),
    m_SemFull(0),
    m_RunFlg(true)
{
    // 找到解码器
    m_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!m_codec) {
        std::cerr << "Codec not found" << std::endl;
        return ;
    }

    // 打开解码器
    m_codec_context = avcodec_alloc_context3(m_codec);
    if (!m_codec_context) {
        std::cerr << "Could not allocate codec context" << std::endl;
        return ;
    }

    if (avcodec_open2(m_codec_context, m_codec, NULL) < 0) {
        std::cerr << "Could not open codec" << std::endl;
        return ;
    }

    // 分配帧和包
    m_frame = av_frame_alloc();
    if (!m_frame) {
        std::cerr << "Could not allocate frame" << std::endl;
        return ;
    }
    m_packet = av_packet_alloc();
    //av_init_packet(m_packet);

    m_DecThread = std::thread([this]{ this->DecThread(); });

}

C_H264Decode::~C_H264Decode()
{
    m_RunFlg = false;
    m_SemFull.signal(); //唤醒可能睡眠的线程，保证线程能正常退出
    if(m_DecThread.joinable()){
        m_DecThread.join();
    }

    // 清理资源
    av_frame_free(&m_frame);
    av_packet_free(&m_packet);
    avcodec_free_context(&m_codec_context);
}

int C_H264Decode::Decode(char* data, int dataLen)
{
    m_packet->size = dataLen;
    m_packet->data = (uint8_t*)data;

    int ret = avcodec_send_packet(m_codec_context, m_packet);
    if (ret < 0) {
        std::cerr << "Error sending a packet for decoding ret:%d" << ret << std::endl;
        return -1;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(m_codec_context, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        else if (ret < 0) {
            std::cerr << "Error during decoding" << std::endl;
            break;
        }

        //std::cout << "w=" << m_frame->width << "  h=" << m_frame->height 
        //    << "linesize[0] =" << m_frame->linesize[0] 
        //    << "linesize[1] =" << m_frame->linesize[1]
        //    << "linesize[2] =" << m_frame->linesize[2]
        //    << std::endl;
        m_pListener->OnYuvData(m_frame);

    }
	return 0;
}

int C_H264Decode::AsyncDecode(char *data, int dataLen)
{
    m_SemEmpty.wait();

    std::vector<char> tmpData(data, data + dataLen);

    m_H264Queue.push(std::move(tmpData));

    m_SemFull.signal();
    return 0;
}

void C_H264Decode::DecThread()
{
    while(m_RunFlg){
        m_SemFull.wait();

        if(m_H264Queue.size() == 0)
            continue;

        std::vector<char> tmpData = std::move(m_H264Queue.front());
        m_H264Queue.pop();
        //调用同步解码接口进行解码
        Decode(tmpData.data(),tmpData.size());

        m_SemEmpty.signal();
    }
}

