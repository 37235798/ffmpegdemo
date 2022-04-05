#define _CRT_SECURE_NO_WARNINGS
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
}
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "postproc.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib")

#define INPUT "in.flv"
#define OUTVIDEO "video.yuv"
#define OUTAUDIO "audio.pcm"
#define OUTRGB "video.rgb"

int main()
{
	int res = 0;
	int videoStream = -1;//�����Ƶ���ı��
	int audioStream = -1;//�����Ƶ���ı��
	char errBuf[BUFSIZ] = { 0 };
	FILE* fp_video = fopen(OUTVIDEO, "wb+");
	FILE* fp_audio = fopen(OUTAUDIO, "wb+");
	FILE* fp_rgb = fopen(OUTRGB, "wb+");

	//��ʼ��FFMPEG  ��������������������ñ������ͽ�����
	av_register_all();
	printf("FFmpeg's version is: %d\n", avcodec_version());

	//FFMPEG���еĲ�����Ҫͨ�����AVFormatContext������
	AVFormatContext* pFormatCtx = NULL;

	//��������Ƶ�ļ�
	//Open an input stream and read the header. The codecs are not opened.
	if ((res = avformat_open_input(&pFormatCtx, INPUT, NULL, NULL)) < 0)
	{
		av_strerror(res, errBuf, sizeof(errBuf));
		printf("%s\n", errBuf);
		return -1;
	}
	//Read packets of a media file to get stream information. This is useful for file formats with no headers such as MPEG.
	//�൱�ڶ�������� ��Ԥ������
	avformat_find_stream_info(pFormatCtx, NULL);
	av_dump_format(pFormatCtx, 0, NULL, 0); //�����Ƶ������Ϣ

	//������
	for (int i = 0; i < pFormatCtx->nb_streams; ++i)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			videoStream = i;
		else if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			audioStream = i;
	}
	if (videoStream == -1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}
	if (audioStream == -1)
	{
		printf("Didn't find a audio stream.\n");
		return -1;
	}

	///���ҽ�����    
	AVCodecContext* pVCodecCtx = pFormatCtx->streams[videoStream]->codec;
	AVCodec* pVCodec = avcodec_find_decoder(pVCodecCtx->codec_id);
	if (pVCodec == NULL)
	{
		printf("Video Codec not found.\n");
		return -1;
	}
	AVCodecContext* pACodecCtx = pFormatCtx->streams[audioStream]->codec;
	AVCodec* pACodec = avcodec_find_decoder(pACodecCtx->codec_id);
	if (pACodec == NULL)
	{
		printf("Audio Codec not found.\n");
		return -1;
	}

	///�򿪽�����
	if (avcodec_open2(pVCodecCtx, pVCodec, NULL) < 0)
	{
		printf("Could not open Video codec.\n");
		return -1;
	}
	if (avcodec_open2(pACodecCtx, pACodec, NULL) < 0)
	{
		printf("Could not open Audio codec.\n");
		return -1;
	}

	AVFrame Frame = { 0 };//����ʼ����avcodec_decode_video2�ᱨ��
	AVFrame rgbFrame;
	AVPacket packet;
	int got_picture;

	int rgbsize = avpicture_get_size(AV_PIX_FMT_RGB24, pVCodecCtx->width, pVCodecCtx->height);//����ø�ʽ�ͷֱ�����һ֡ͼ������ݴ�С
	//uint8_t* rgbBuffer = (uint8_t *)av_malloc(rgbsize * sizeof(uint8_t));//���䱣��ͼ����ڴ�
	//avpicture_fill((AVPicture *)&rgbFrame, rgbBuffer, PIX_FMT_RGB24, pVCodecCtx->width, pVCodecCtx->height);//���Լ�������ڴ�󶨵�rgbFrame��data������
	avpicture_alloc((AVPicture *)&rgbFrame, AV_PIX_FMT_RGB24, pVCodecCtx->width, pVCodecCtx->height);//ΪrgbFrame��data�����ڴ棬�����Լ�����
	SwsContext *img_convert_ctx = sws_getContext(pVCodecCtx->width, pVCodecCtx->height, AV_PIX_FMT_YUV420P, pVCodecCtx->width, pVCodecCtx->height, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, NULL, NULL, NULL);//ת��������

	while (1)
	{
		//��ȡ��Ƶ֡
		//return 0 if OK, < 0 on error or end of file
		if (av_read_frame(pFormatCtx, &packet) < 0)
		{
			break; //������Ϊ��Ƶ��ȡ����
		}
		if (packet.stream_index == videoStream)
		{
			//������Ƶ֡
			if (avcodec_decode_video2(pVCodecCtx, &Frame, &got_picture, &packet) < 0)
			{
				printf("decode Video error.\n");
				return -1;
			}
			if (got_picture)
			{
				if (Frame.format == AV_PIX_FMT_YUV420P)
				{
					//�����YUV��ʽ����Ƶ�������ݱ�����AVFrame��data[0]��data[1]��data[2]�С�
					//������Щ����ֵ�����������洢�ģ�ÿ����Ч����֮��洢��һЩ��Ч���ء�
					//������Y����Ϊ����data[0]��һ��������linesize[0] * height�����ݡ�
					//���ǳ����Ż��ȷ���Ŀ��ǣ�linesize[0]ʵ���ϲ������ڿ���width������һ���ȿ��ȴ�һЩ��ֵ��
					fwrite(Frame.data[0], Frame.linesize[0] * Frame.height, 1, fp_video);
					fwrite(Frame.data[1], Frame.linesize[1] * Frame.height / 2, 1, fp_video);
					fwrite(Frame.data[2], Frame.linesize[2] * Frame.height / 2, 1, fp_video);

					sws_scale(img_convert_ctx, (uint8_t const* const *)Frame.data, Frame.linesize, 0, pVCodecCtx->height, rgbFrame.data, rgbFrame.linesize);//ת��
					fwrite(rgbFrame.data[0], rgbsize, 1, fp_rgb);
				}
			}
		}
		else if (packet.stream_index == audioStream)
		{
			//������Ƶ֡
			if (avcodec_decode_audio4(pACodecCtx, &Frame, &got_picture, &packet) < 0)
			{
				printf("decode Audio error.\n");
				return -1;
			}
			if (got_picture)
			{
				if (Frame.format == AV_SAMPLE_FMT_S16P)//signed 16 bits, planar 16λ ƽ������
				{
					//AV_SAMPLE_FMT_S16P
					//����ÿ��data[]�������������ģ�planar����ÿ����λ��16bits
					for (int i = 0; i < Frame.linesize[0]; i += 2)
					{
						//����Ƕ�ͨ���Ļ��������c1��λ��c1��λ��c2��λ��c2��λ...
						for (int j = 0; j < Frame.channels; ++j)
							fwrite(Frame.data[j] + i, 2, 1, fp_audio);
					}
				}
				else if (Frame.format == AV_SAMPLE_FMT_FLTP)
				{
					for (int i = 0; i < Frame.linesize[0]; i += 4)
					{
						for (int j = 0; j < Frame.channels; ++j)
							fwrite(Frame.data[j] + i, 4, 1, fp_audio);
					}
				}
			}
		}
		av_free_packet(&packet);//���packet����ָ��Ļ�����
	}

	fclose(fp_video);
	fclose(fp_audio);
	fclose(fp_rgb);
	avpicture_free((AVPicture*)&rgbFrame);//�ͷ�avpicture_alloc������ڴ�
	avcodec_close(pVCodecCtx);//�رս�����
	avcodec_close(pACodecCtx);
	avformat_close_input(&pFormatCtx);//�ر�������Ƶ�ļ���avformat_free_context(pFormatCtx);�Ͳ���Ҫ��
	return 0;
}