
#include "Delogo.h"
//#include "log/log.h"





CDelogo::CDelogo()
{
	InitializeCriticalSection(&m_csVideoASection);
}

CDelogo::~CDelogo()
{
	DeleteCriticalSection(&m_csVideoASection);
}

int CDelogo::StartDelogo(const char *pFileA, const char *pFileOut, int x, int y, int width, int height)
{
	int ret = -1;
	do
	{
		ret = OpenFileA(pFileA);
		if (ret != 0)
		{
			break;
		}

		ret = OpenOutPut(pFileOut);
		if (ret != 0)
		{
			break;
		}

		char szFilterDesc[512] = { 0 };


		_snprintf(szFilterDesc, sizeof(szFilterDesc),
			"[in0]delogo=x=%d:y=%d:w=%d:h=%d:show=0[out]", x, y, width, height);

		ret = InitFilter(szFilterDesc);
		if (ret != 0)
		{
			break;
		}

		m_iYuv420FrameSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, m_pReadCodecCtx_VideoA->width, m_pReadCodecCtx_VideoA->height, 1);
		//…Í«Î30÷°ª∫¥Ê
		m_pVideoAFifo = av_fifo_alloc(30 * m_iYuv420FrameSize);

		m_hVideoAReadThread = CreateThread(NULL, 0, VideoAReadProc, this, 0, NULL);

		m_hVideoDelogoThread = CreateThread(NULL, 0, VideoDelogoProc, this, 0, NULL);

	} while (0);

	return ret;
}

int CDelogo::WaitFinish()
{
	int ret = 0;
	do
	{
		if (NULL == m_hVideoAReadThread)
		{
			break;
		}
		WaitForSingleObject(m_hVideoAReadThread, INFINITE);

		CloseHandle(m_hVideoAReadThread);
		m_hVideoAReadThread = NULL;

		WaitForSingleObject(m_hVideoDelogoThread, INFINITE);
		CloseHandle(m_hVideoDelogoThread);
		m_hVideoDelogoThread = NULL;
	} while (0);

	return ret;
}

int CDelogo::OpenFileA(const char *pFileA)
{
	int ret = -1;

	do
	{
		if ((ret = avformat_open_input(&m_pFormatCtx_FileA, pFileA, 0, 0)) < 0) {
			printf("Could not open input file.");
			break;
		}
		if ((ret = avformat_find_stream_info(m_pFormatCtx_FileA, 0)) < 0) {
			printf("Failed to retrieve input stream information");
			break;
		}

		if (m_pFormatCtx_FileA->streams[0]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
		{
			break;
		}

		m_pReadCodec_VideoA = (AVCodec *)avcodec_find_decoder(m_pFormatCtx_FileA->streams[0]->codecpar->codec_id);

		m_pReadCodecCtx_VideoA = avcodec_alloc_context3(m_pReadCodec_VideoA);

		if (m_pReadCodecCtx_VideoA == NULL)
		{
			break;
		}
		avcodec_parameters_to_context(m_pReadCodecCtx_VideoA, m_pFormatCtx_FileA->streams[0]->codecpar);

		m_iVideoWidth = m_pReadCodecCtx_VideoA->width;
		m_iVideoHeight = m_pReadCodecCtx_VideoA->height;

		m_pReadCodecCtx_VideoA->framerate = m_pFormatCtx_FileA->streams[0]->r_frame_rate;

		if (avcodec_open2(m_pReadCodecCtx_VideoA, m_pReadCodec_VideoA, NULL) < 0)
		{
			break;
		}

		ret = 0;
	} while (0);


	return ret;
}


int CDelogo::OpenOutPut(const char *pFileOut)
{
	int iRet = -1;

	AVStream *pAudioStream = NULL;
	AVStream *pVideoStream = NULL;

	do
	{
		avformat_alloc_output_context2(&m_pFormatCtx_Out, NULL, NULL, pFileOut);

		{
			AVCodec* pCodecEncode_Video = (AVCodec *)avcodec_find_encoder(m_pFormatCtx_Out->oformat->video_codec);

			m_pCodecEncodeCtx_Video = avcodec_alloc_context3(pCodecEncode_Video);
			if (!m_pCodecEncodeCtx_Video)
			{
				break;
			}

			pVideoStream = avformat_new_stream(m_pFormatCtx_Out, pCodecEncode_Video);
			if (!pVideoStream)
			{
				break;
			}

			int frameRate = 10;
			m_pCodecEncodeCtx_Video->flags |= AV_CODEC_FLAG_QSCALE;
			m_pCodecEncodeCtx_Video->bit_rate = 4000000;
			m_pCodecEncodeCtx_Video->rc_min_rate = 4000000;
			m_pCodecEncodeCtx_Video->rc_max_rate = 4000000;
			m_pCodecEncodeCtx_Video->bit_rate_tolerance = 4000000;
			m_pCodecEncodeCtx_Video->time_base.den = frameRate;
			m_pCodecEncodeCtx_Video->time_base.num = 1;

			m_pCodecEncodeCtx_Video->width = m_iVideoWidth;
			m_pCodecEncodeCtx_Video->height = m_iVideoHeight;
			//pH264Encoder->pCodecCtx->frame_number = 1;
			m_pCodecEncodeCtx_Video->gop_size = 12;
			m_pCodecEncodeCtx_Video->max_b_frames = 0;
			m_pCodecEncodeCtx_Video->thread_count = 4;
			m_pCodecEncodeCtx_Video->pix_fmt = AV_PIX_FMT_YUV420P;
			m_pCodecEncodeCtx_Video->codec_id = AV_CODEC_ID_H264;
			m_pCodecEncodeCtx_Video->codec_type = AVMEDIA_TYPE_VIDEO;

			av_opt_set(m_pCodecEncodeCtx_Video->priv_data, "b-pyramid", "none", 0);
			av_opt_set(m_pCodecEncodeCtx_Video->priv_data, "preset", "superfast", 0);
			av_opt_set(m_pCodecEncodeCtx_Video->priv_data, "tune", "zerolatency", 0);

			if (m_pFormatCtx_Out->oformat->flags & AVFMT_GLOBALHEADER)
				m_pCodecEncodeCtx_Video->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			if (avcodec_open2(m_pCodecEncodeCtx_Video, pCodecEncode_Video, 0) < 0)
			{
				//±‡¬Î∆˜¥Úø™ ß∞‹£¨ÕÀ≥ˆ≥Ã–Ú
				break;
			}
		}

		if (!(m_pFormatCtx_Out->oformat->flags & AVFMT_NOFILE))
		{
			if (avio_open(&m_pFormatCtx_Out->pb, pFileOut, AVIO_FLAG_WRITE) < 0)
			{
				break;
			}
		}

		avcodec_parameters_from_context(pVideoStream->codecpar, m_pCodecEncodeCtx_Video);

		if (avformat_write_header(m_pFormatCtx_Out, NULL) < 0)
		{
			break;
		}

		iRet = 0;
	} while (0);


	if (iRet != 0)
	{
		if (m_pCodecEncodeCtx_Video != NULL)
		{
			avcodec_free_context(&m_pCodecEncodeCtx_Video);
			m_pCodecEncodeCtx_Video = NULL;
		}

		if (m_pFormatCtx_Out != NULL)
		{
			avformat_free_context(m_pFormatCtx_Out);
			m_pFormatCtx_Out = NULL;
		}
	}

	return iRet;
}


DWORD WINAPI CDelogo::VideoAReadProc(LPVOID lpParam)
{
	CDelogo *pVideoMerge = (CDelogo *)lpParam;
	if (pVideoMerge != NULL)
	{
		pVideoMerge->VideoARead();
	}
	return 0;
}

void CDelogo::VideoARead()
{
	AVFrame *pFrame;
	pFrame = av_frame_alloc();

	int y_size = m_pReadCodecCtx_VideoA->width * m_pReadCodecCtx_VideoA->height;

	char *pY = new char[y_size];
	char *pU = new char[y_size / 4];
	char *pV = new char[y_size / 4];

	AVPacket packet = { 0 };
	int ret = 0;
	while (1)
	{
		av_packet_unref(&packet);

		ret = av_read_frame(m_pFormatCtx_FileA, &packet);
		if (ret == AVERROR(EAGAIN))
		{
			continue;
		}
		else if (ret == AVERROR_EOF)
		{
			break;
		}
		else if (ret < 0)
		{
			break;
		}

		ret = avcodec_send_packet(m_pReadCodecCtx_VideoA, &packet);

		if (ret >= 0)
		{
			ret = avcodec_receive_frame(m_pReadCodecCtx_VideoA, pFrame);
			if (ret == AVERROR(EAGAIN))
			{
				continue;
			}
			else if (ret == AVERROR_EOF)
			{
				break;
			}
			else if (ret < 0) {
				break;
			}

			while (1)
			{
				if (av_fifo_space(m_pVideoAFifo) >= m_iYuv420FrameSize)
				{
					///Y
					int contY = 0;
					for (int i = 0; i < pFrame->height; i++)
					{
						memcpy(pY + contY, pFrame->data[0] + i * pFrame->linesize[0], pFrame->width);
						contY += pFrame->width;
					}


					///U
					int contU = 0;
					for (int i = 0; i < pFrame->height / 2; i++)
					{
						memcpy(pU + contU, pFrame->data[1] + i * pFrame->linesize[1], pFrame->width / 2);
						contU += pFrame->width / 2;
					}


					///V
					int contV = 0;
					for (int i = 0; i < pFrame->height / 2; i++)
					{
						memcpy(pV + contV, pFrame->data[2] + i * pFrame->linesize[2], pFrame->width / 2);
						contV += pFrame->width / 2;
					}


					EnterCriticalSection(&m_csVideoASection);
					av_fifo_generic_write(m_pVideoAFifo, pY, y_size, NULL);
					av_fifo_generic_write(m_pVideoAFifo, pU, y_size / 4, NULL);
					av_fifo_generic_write(m_pVideoAFifo, pV, y_size / 4, NULL);
					LeaveCriticalSection(&m_csVideoASection);

					break;
				}
				else
				{
					Sleep(100);
				}
			}

		}


		if (ret == AVERROR(EAGAIN))
		{
			continue;
		}
	}

	av_frame_free(&pFrame);
	delete[] pY;
	delete[] pU;
	delete[] pV;
}

DWORD WINAPI CDelogo::VideoDelogoProc(LPVOID lpParam)
{
	CDelogo *pVideoMerge = (CDelogo *)lpParam;
	if (pVideoMerge != NULL)
	{
		pVideoMerge->VideoDelogo();
	}
	return 0;
}


void CDelogo::VideoDelogo()
{
	int ret = 0;


	DWORD dwBeginTime = ::GetTickCount();


	AVFrame *pFrameVideoA = av_frame_alloc();
	uint8_t *videoA_buffer_yuv420 = (uint8_t *)av_malloc(m_iYuv420FrameSize);
	av_image_fill_arrays(pFrameVideoA->data, pFrameVideoA->linesize, videoA_buffer_yuv420, AV_PIX_FMT_YUV420P, m_pReadCodecCtx_VideoA->width, m_pReadCodecCtx_VideoA->height, 1);

	pFrameVideoA->width = m_iVideoWidth;
	pFrameVideoA->height = m_iVideoHeight;
	pFrameVideoA->format = AV_PIX_FMT_YUV420P;


	AVFrame* pFrame_out = av_frame_alloc();
	uint8_t *out_buffer_yuv420 = (uint8_t *)av_malloc(m_iYuv420FrameSize);
	av_image_fill_arrays(pFrame_out->data, pFrame_out->linesize, out_buffer_yuv420, AV_PIX_FMT_YUV420P, m_iVideoWidth, m_iVideoHeight, 1);

	AVPacket packet = { 0 };
	int iPicCount = 0;

	while (1)
	{
		if (NULL == m_pVideoAFifo)
		{
			break;
		}

		int iVideoASize = av_fifo_size(m_pVideoAFifo);

		if (iVideoASize >= m_iYuv420FrameSize)
		{
			EnterCriticalSection(&m_csVideoASection);
			av_fifo_generic_read(m_pVideoAFifo, videoA_buffer_yuv420, m_iYuv420FrameSize, NULL);
			LeaveCriticalSection(&m_csVideoASection);


			pFrameVideoA->pkt_dts = pFrameVideoA->pts = av_rescale_q_rnd(iPicCount, m_pCodecEncodeCtx_Video->time_base, m_pFormatCtx_Out->streams[0]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pFrameVideoA->pkt_duration = 0;
			pFrameVideoA->pkt_pos = -1;


			ret = av_buffersrc_add_frame(m_pFilterCtxSrcVideoA, pFrameVideoA);
			if (ret < 0)
			{
				break;
			}

			ret = av_buffersink_get_frame(m_pFilterCtxSink, pFrame_out);
			if (ret < 0)
			{
				//printf("Mixer: failed to call av_buffersink_get_frame_flags\n");
				break;
			}

			pFrame_out->pkt_dts = pFrame_out->pts = av_rescale_q_rnd(iPicCount, m_pCodecEncodeCtx_Video->time_base, m_pFormatCtx_Out->streams[0]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pFrame_out->pkt_duration = 0;
			pFrame_out->pkt_pos = -1;

			pFrame_out->width = m_iVideoWidth;
			pFrame_out->height = m_iVideoHeight;
			pFrame_out->format = AV_PIX_FMT_YUV420P;

			ret = avcodec_send_frame(m_pCodecEncodeCtx_Video, pFrame_out);

			ret = avcodec_receive_packet(m_pCodecEncodeCtx_Video, &packet);

			av_write_frame(m_pFormatCtx_Out, &packet);

			iPicCount++;
		}
		else
		{
			if (m_hVideoAReadThread == NULL)
			{
				break;
			}
			Sleep(1);
		}
	}

	av_write_trailer(m_pFormatCtx_Out);
	avio_close(m_pFormatCtx_Out->pb);

	av_frame_free(&pFrameVideoA);
}


int CDelogo::InitFilter(const char* filter_desc)
{
	int ret = 0;

	char args_videoA[512];
	const char* pad_name_videoA = "in0";

	AVFilter* filter_src_videoA = (AVFilter *)avfilter_get_by_name("buffer");
	AVFilter* filter_sink = (AVFilter *)avfilter_get_by_name("buffersink");

	AVFilterInOut* filter_output_videoA = avfilter_inout_alloc();
	AVFilterInOut* filter_input = avfilter_inout_alloc();
	m_pFilterGraph = avfilter_graph_alloc();

	AVRational timeBase;
	timeBase.num = 1;
	timeBase.den = 10;


	AVRational timeAspect;
	timeAspect.num = 0;
	timeAspect.den = 1;


	_snprintf(args_videoA, sizeof(args_videoA),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		m_iVideoWidth, m_iVideoHeight, AV_PIX_FMT_YUV420P,
		timeBase.num, timeBase.den,
		timeAspect.num,
		timeAspect.den);

	AVFilterInOut* filter_outputs[1];
	do
	{
		ret = avfilter_graph_create_filter(&m_pFilterCtxSrcVideoA, filter_src_videoA, pad_name_videoA, args_videoA, NULL, m_pFilterGraph);
		if (ret < 0)
		{
			break;
		}


		ret = avfilter_graph_create_filter(&m_pFilterCtxSink, filter_sink, "out", NULL, NULL, m_pFilterGraph);
		if (ret < 0)
		{
			break;
		}

		ret = av_opt_set_bin(m_pFilterCtxSink, "pix_fmts", (uint8_t*)&m_pCodecEncodeCtx_Video->pix_fmt, sizeof(m_pCodecEncodeCtx_Video->pix_fmt), AV_OPT_SEARCH_CHILDREN);



		filter_output_videoA->name = av_strdup(pad_name_videoA);
		filter_output_videoA->filter_ctx = m_pFilterCtxSrcVideoA;
		filter_output_videoA->pad_idx = 0;
		filter_output_videoA->next = NULL;


		filter_input->name = av_strdup("out");
		filter_input->filter_ctx = m_pFilterCtxSink;
		filter_input->pad_idx = 0;
		filter_input->next = NULL;

		//filter_outputs[0] = filter_output_videoPad;
		filter_outputs[0] = filter_output_videoA;


		ret = avfilter_graph_parse_ptr(m_pFilterGraph, filter_desc, &filter_input, filter_outputs, NULL);
		if (ret < 0)
		{
			break;
		}


		ret = avfilter_graph_config(m_pFilterGraph, NULL);
		if (ret < 0)
		{
			break;
		}

		ret = 0;

	} while (0);


	avfilter_inout_free(&filter_input);
	av_free(filter_src_videoA);
	avfilter_inout_free(filter_outputs);

	char* temp = avfilter_graph_dump(m_pFilterGraph, NULL);

	return ret;
}






