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

	//解封装
	AVFormatContext* inputContext = nullptr;

	//打开输入流并读取头部信息
	avformat_open_input(&inputContext, inputFileUrl, NULL, NULL);

	//获取流媒体信息
	avformat_find_stream_info(inputContext, NULL);

	//打印视频封装信息
	av_dump_format(inputContext, 0, inputFileUrl, 0);

	//分离音频流和视频流
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

	//重封装
	const char* outputFileUrl = "test_output.mp4";
	AVFormatContext* outputContext = nullptr;

	//根据输出文件后缀推测封装格式
	avformat_alloc_output_context2(&outputContext, NULL, NULL, outputFileUrl);


	//avformat_new_stream 用于为输出流创建一个视频通道和音频通道，需要传入上下文和编码器可为NULL
	AVStream* videoOutputStream = avformat_new_stream(outputContext, NULL);
	AVStream* audioOutputStream = avformat_new_stream(outputContext, NULL);


	//avio_open 用于写入输出视频文件，需要传入上下文的IO指针、输出路径和打开模式
    // AVIO_FLAG_READ  1   只读
    //AVIO_FLAG_WRITE 2    只写
    //AVIO_FLAG_READ_WRITE (AVIO_FLAG_READ|AVIO_FLAG_WRITE)   可读可写

	avio_open(&outputContext->pb, outputFileUrl, AVIO_FLAG_WRITE);
	//将输入视频流的封装配置复制至输出视频流

	//用于将输入流上下文配置复制至输出流，需要传入输出流编码器指针和输入流编码器指针
	videoOutputStream->time_base = videoInputStream->time_base;
	avcodec_parameters_copy(videoOutputStream->codecpar, videoInputStream->codecpar);

	//用于将输入流上下文配置复制至输出流，需要传入输出流编码器指针和输入流编码器指针
	audioOutputStream->time_base = audioInputStream->time_base;
	avcodec_parameters_copy(audioOutputStream->codecpar, audioInputStream->codecpar);


	//写入文件头信息
	avformat_write_header(outputContext, NULL);
	av_dump_format(outputContext, 0, outputFileUrl, 1);

	//读取输入流
	AVPacket packet;
	for (;;)
	{
		//用于将输入流中的音视频信息按帧封装为packet对象，供进一步处理。需传入输入流上下文和用于装载数据的packet对象
		int result = av_read_frame(inputContext, &packet);
		if (result != 0) break;

		if (packet.stream_index == videoOutputStream->index)
		{
			cout << "视频:";
		}
		else if (packet.stream_index == audioOutputStream->index) {
			cout << "音频:";
		}
		cout << packet.pts << " : " << packet.dts << " :" << packet.size << endl;

		//用于将读取的packet写入至输出流，需传入输出流上下文和packet对象。需要注意的是此函数会自动释放packet对象引用的资源，不需要我们手工释放
		av_interleaved_write_frame(outputContext, &packet);
	}

	av_write_trailer(outputContext);
	avformat_close_input(&inputContext);
	avio_closep(&outputContext->pb);
	avformat_free_context(outputContext);
	outputContext = nullptr;
	return 0;
}