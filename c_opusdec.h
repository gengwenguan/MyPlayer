/*********************************************************************************
  *Copyright(C),Your Company
  *FileName:  opusDec.h
  *Author:    gengwenguan
  *Date:      2024-12-16
  *Description:  Opus解码为PCM音频，单通道、16位采样深度，48k采样率
**********************************************************************************/
#pragma once

extern "C" {
// #include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

class C_OpusDec
{
public:
    class C_Listener{
    public:
        virtual int OnOutputPcm(char* data, int dataLen) = 0;
    };
public:
    C_OpusDec(C_Listener* pListener);
    ~C_OpusDec();

    // 解码Opus数据
    int DecodeOpus(char* data, int dataLen);

private:
    C_Listener* m_pListener;
    AVCodecContext *m_codec_ctx; //opus解码器上下文
};
