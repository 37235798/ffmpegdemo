#pragma once
#include "iostream"

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
};

class Muxer
{
public:
	Muxer();
	~Muxer();
	// ����ļ� ����<0ֵ�쳣
	// ��ʼ��
	int Init(const char *url);
	// ��Դ�ͷ�
	void DeInit();
	// ������
	int AddStream(AVCodecContext *codec_ctx);

	// д��
	int SendHeader();
	int SendPacket(AVPacket *packet);
	int SendTrailer();

	int Open(); // avio open

	int GetAudioStreamIndex();
	int GetVideoStreamIndex();
private:
	AVFormatContext *fmt_ctx_ = NULL;
	std::string url_ = "";

	// ������������
	AVCodecContext *aud_codec_ctx_ = NULL;
	AVStream *aud_stream_ = NULL;
	AVCodecContext *vid_codec_ctx_ = NULL;
	AVStream *vid_stream_ = NULL;

	int audio_index_ = -1;
	int video_index_ = -1;
};

