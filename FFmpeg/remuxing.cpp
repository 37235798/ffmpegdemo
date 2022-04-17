#include <iostream>
#include <thread>
using namespace std;
extern "C" {
#include <libavformat/avformat.h>
}
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")

int main(int argc, char* argv[])
{
	const char* inputFileUrl = "test2.mp4";

	//���װ
	AVFormatContext* inputContext = nullptr;

	//������������ȡͷ����Ϣ
	avformat_open_input(&inputContext, inputFileUrl, NULL, NULL);

	//��ȡ��ý����Ϣ
	avformat_find_stream_info(inputContext, NULL);

	//��ӡ��Ƶ��װ��Ϣ
	av_dump_format(inputContext, 0, inputFileUrl, 0);

	//������Ƶ������Ƶ��
	AVStream* audioInputStream = nullptr;
	AVStream* videoInputStream = nullptr;

	for (int i = 0; i < inputContext->nb_streams; i++)
	{
		if (inputContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			audioInputStream = inputContext->streams[i];
		else if (inputContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			videoInputStream = inputContext->streams[i];
		else continue;
	}

	//�ط�װ
	const char* outputFileUrl = "test_output.mp4";
	AVFormatContext* outputContext = nullptr;

	//��������ļ���׺�Ʋ��װ��ʽ
	avformat_alloc_output_context2(&outputContext, NULL, NULL, outputFileUrl);


	//avformat_new_stream ����Ϊ���������һ����Ƶͨ������Ƶͨ������Ҫ���������ĺͱ�������ΪNULL
	AVStream* videoOutputStream = avformat_new_stream(outputContext, NULL);
	AVStream* audioOutputStream = avformat_new_stream(outputContext, NULL);


	//avio_open ����д�������Ƶ�ļ�����Ҫ���������ĵ�IOָ�롢���·���ʹ�ģʽ
    // AVIO_FLAG_READ  1   ֻ��
    //AVIO_FLAG_WRITE 2    ֻд
    //AVIO_FLAG_READ_WRITE (AVIO_FLAG_READ|AVIO_FLAG_WRITE)   �ɶ���д

	avio_open(&outputContext->pb, outputFileUrl, AVIO_FLAG_WRITE);
	//��������Ƶ���ķ�װ���ø����������Ƶ��

	//���ڽ����������������ø��������������Ҫ���������������ָ���������������ָ��
	videoOutputStream->time_base = videoInputStream->time_base;
	avcodec_parameters_copy(videoOutputStream->codecpar, videoInputStream->codecpar);

	//���ڽ����������������ø��������������Ҫ���������������ָ���������������ָ��
	audioOutputStream->time_base = audioInputStream->time_base;
	avcodec_parameters_copy(audioOutputStream->codecpar, audioInputStream->codecpar);


	//д���ļ�ͷ��Ϣ
	avformat_write_header(outputContext, NULL);
	av_dump_format(outputContext, 0, outputFileUrl, 1);

	//��ȡ������
	AVPacket packet;
	for (;;)
	{
		//���ڽ��������е�����Ƶ��Ϣ��֡��װΪpacket���󣬹���һ�������贫�������������ĺ�����װ�����ݵ�packet����
		int result = av_read_frame(inputContext, &packet);
		if (result != 0) break;

		if (packet.stream_index == videoOutputStream->index)
		{
			cout << "��Ƶ:";
		}
		else if (packet.stream_index == audioOutputStream->index) {
			cout << "��Ƶ:";
		}
		cout << packet.pts << " : " << packet.dts << " :" << packet.size << endl;

		//���ڽ���ȡ��packetд������������贫������������ĺ�packet������Ҫע����Ǵ˺������Զ��ͷ�packet�������õ���Դ������Ҫ�����ֹ��ͷ�
		av_interleaved_write_frame(outputContext, &packet);
	}

	av_write_trailer(outputContext);
	avformat_close_input(&inputContext);
	avio_closep(&outputContext->pb);
	avformat_free_context(outputContext);
	outputContext = nullptr;
	return 0;
}