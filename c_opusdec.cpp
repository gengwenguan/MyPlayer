#include "c_opusdec.h"
#include "QDebug.h"

C_OpusDec::C_OpusDec(C_Listener* pListener)
    :m_pListener(pListener)
{
    // 找到Opus解码器
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
    if (!codec) {
       qDebug() << "Could not find AV_CODEC_ID_OPUS decoder";
        return;
    }

    // 分配解码上下文
    m_codec_ctx = avcodec_alloc_context3(codec);
    if(!m_codec_ctx){
         qDebug() << "Could not alloc m_codec_ctx";
        return;
    }

    // 设置解码参数
    m_codec_ctx->sample_rate = 48000;
    m_codec_ctx->ch_layout = AV_CHANNEL_LAYOUT_MONO;
    // m_codec_ctx->channel_layout = AV_CH_LAYOUT_MONO;
    // m_codec_ctx->channels = 1;
    m_codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;

    // 打开解码器
    if (avcodec_open2(m_codec_ctx, codec, nullptr) < 0) {
         qDebug() << "Could not open decoder";
        return;
    }

    //ffmpeg里面opus解码器解出的格式默认为AV_SAMPLE_FMT_FLTP格式

}

C_OpusDec::~C_OpusDec()
{
    avcodec_close(m_codec_ctx);
    avcodec_free_context(&m_codec_ctx);
}

int C_OpusDec::DecodeOpus(char* data, int dataLen)
{
    // 分配AVPacket
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        qDebug() << "Could not alloc packet";
        return -1;
    }

    // 设置packet数据
    pkt->data = (uint8_t *)data;
    pkt->size = dataLen;

    // 发送数据到解码器
    int ret = avcodec_send_packet(m_codec_ctx, pkt);
    if (ret < 0) {
        qDebug() << "Error sending packet to decoder";
        av_packet_free(&pkt);
        return ret;
    }

    // 分配AVFrame用于接收解码后的数据
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        qDebug() << "Could not alloc frame";
        av_packet_free(&pkt);
        return -1;
    }

    // 循环接收解码后的数据
    while (ret >= 0) {
        ret = avcodec_receive_frame(m_codec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
           qDebug() << "Error during decoding";
            break;
        }

        // printf("Sample format: %s\n", av_get_sample_fmt_name((enum AVSampleFormat)frame->format));
        // printf("nb_samples%d", frame->nb_samples);

        // 回调PCM数据
        if (m_pListener) {

            m_pListener->OnOutputPcm((char *)frame->data[0], frame->linesize[0]);
        }
    }

    // 释放资源
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}
