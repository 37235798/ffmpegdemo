#pragma once

#include <Windows.h>
#include <string>

#ifdef	__cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavdevice/avdevice.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avutil.h"
#include "libavutil/fifo.h"
#include "libavutil/frame.h"
#include "libavutil/imgutils.h"

#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"


#ifdef __cplusplus
};
#endif

class CDelogo
{
public:
	CDelogo();
	~CDelogo();
public:
	int StartDelogo(const char *pFileA, const char *pFileOut, int x, int y, int width, int height);
	int WaitFinish();
private:
	int OpenFileA(const char *pFileA);
	int OpenOutPut(const char *pFileOut);
	int InitFilter(const char* filter_desc);
private:
	static DWORD WINAPI VideoAReadProc(LPVOID lpParam);
	void VideoARead();


	static DWORD WINAPI VideoDelogoProc(LPVOID lpParam);
	void VideoDelogo();
private:
	AVFormatContext *m_pFormatCtx_FileA = NULL;

	AVCodecContext *m_pReadCodecCtx_VideoA = NULL;
	AVCodec *m_pReadCodec_VideoA = NULL;


	AVCodecContext	*m_pCodecEncodeCtx_Video = NULL;
	AVFormatContext *m_pFormatCtx_Out = NULL;

	AVFifoBuffer *m_pVideoAFifo = NULL;


	int m_iVideoWidth = 1920;
	int m_iVideoHeight = 1080;
	int m_iYuv420FrameSize = 0;
private:
	AVFilterGraph* m_pFilterGraph = NULL;
	AVFilterContext* m_pFilterCtxSrcVideoA = NULL;
	AVFilterContext* m_pFilterCtxSink = NULL;
private:
	CRITICAL_SECTION m_csVideoASection;
	HANDLE m_hVideoAReadThread = NULL;
	HANDLE m_hVideoDelogoThread = NULL;
};






