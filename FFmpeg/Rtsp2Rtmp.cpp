extern "C"
{
#include "libavformat\avformat.h"
#include "libavutil\time.h"
}

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

int Error(int res)
{
	char buf[1024] = { 0 };
	av_strerror(res, buf, sizeof(buf));
	printf("error : %s.\n", buf);
	return res;
}

int main(int argc, char* argv[])
{
	const char* inUrl = "rtsp://184.72.239.149/vod/mp4://BigBuckBunny_175k.mov";//�����Ǳ����ļ�
	const char* outUrl = "rtmp://123.207.71.137/live/test";

	//��ʼ�����з�װ��
	av_register_all();

	//��ʼ�������
	avformat_network_init();

	int res = 0;
	//���ļ������װ�ļ�ͷ
	//�����װ������
	AVFormatContext* ictx = NULL;
	//����rtspЭ����ʱ���ֵ
	AVDictionary *opts = NULL;
	av_dict_set(&opts, "max_delay", "500", 0);
	if ((res = avformat_open_input(&ictx, inUrl, NULL, &opts)) != 0)
		return Error(res);

	//��ȡ����Ƶ����Ϣ
	if ((res = avformat_find_stream_info(ictx, NULL)) < 0)
		return Error(res);
	av_dump_format(ictx, 0, inUrl, 0);

	//�������������
	AVFormatContext* octx = NULL;
	if ((res = avformat_alloc_output_context2(&octx, NULL, "flv", outUrl) < 0))
		return Error(res);

	//���������
	//���������AVStream
	for (int i = 0; i < ictx->nb_streams; ++i)
	{
		//���������
		AVStream* out = avformat_new_stream(octx, ictx->streams[i]->codec->codec);
		if (out == NULL)
		{
			printf("new stream error.\n");
			return -1;
		}
		//����������Ϣ
		if ((res = avcodec_copy_context(out->codec, ictx->streams[i]->codec)) != 0)
			return Error(res);
		//out->codec->codec_tag = 0;//��ǲ���Ҫ���±����
	}
	av_dump_format(octx, 0, outUrl, 1);

	//rtmp����
	//��io
	//@param s Used to return the pointer to the created AVIOContext.In case of failure the pointed to value is set to NULL.
	res = avio_open(&octx->pb, outUrl, AVIO_FLAG_WRITE);
	if (octx->pb == NULL)
		return Error(res);

	//д��ͷ��Ϣ
	//avformat_write_header���ܻ�ı�����timebase
	if ((res = avformat_write_header(octx, NULL)) < 0)
		return Error(res);

	long long  begintime = av_gettime();
	long long  realdts = 0;
	long long  caldts = 0;
	AVPacket pkt;
	while (true)
	{
		if ((res = av_read_frame(ictx, &pkt)) != 0)
			break;
		if (pkt.size <= 0)//��ȡrtspʱpkt.size���ܻ����0
			continue;
		//ת��pts��dts��duration
		pkt.pts = pkt.pts * av_q2d(ictx->streams[pkt.stream_index]->time_base) / av_q2d(octx->streams[pkt.stream_index]->time_base);
		pkt.dts = pkt.dts * av_q2d(ictx->streams[pkt.stream_index]->time_base) / av_q2d(octx->streams[pkt.stream_index]->time_base);
		pkt.duration = pkt.duration * av_q2d(ictx->streams[pkt.stream_index]->time_base) / av_q2d(octx->streams[pkt.stream_index]->time_base);
		pkt.pos = -1;//byte position in stream, -1 if unknown

		//�ļ�����������ʱ
		//av_usleep(30 * 1000);
		/*realdts = av_gettime() - begintime;
		caldts = 1000 * 1000 * pkt.pts * av_q2d(octx->streams[pkt.stream_index]->time_base);
		if (caldts > realdts)
			av_usleep(caldts - realdts);*/

		if ((res = av_interleaved_write_frame(octx, &pkt)) < 0)//����,����֮��pkt��pts��dts��Ȼ���������ˣ�����ǰ�漸֡����Ϊdtsû������������-22����
			Error(res);

		av_free_packet(&pkt);//����pkt�ڲ�������ڴ�
	}
	av_write_trailer(octx);//д�ļ�β

	system("pause");
	return 0;
}