#include <iostream>
#include <fstream>
#include <string>
using namespace std;

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")

int main(int argc, char* argv[])
{
	string inputFileName = "output.h264";
	ifstream inputFileStream(inputFileName, ios::binary);

	//初始化解码器
	AVCodec* decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
	AVCodecContext* context = avcodec_alloc_context3(decoder);
	avcodec_open2(context, NULL, NULL);
	AVCodecParserContext* parserContext = av_parser_init(AV_CODEC_ID_H264);

	//申请packet空间
	AVPacket* packet = av_packet_alloc();
	unsigned char inputBuffer[4096] = { 0 };

	//申请frame空间
	AVFrame* frame = av_frame_alloc();

	while (!inputFileStream.eof())
	{
		//读取H264文件
		inputFileStream.read((char*)inputBuffer, sizeof(inputBuffer));
		if (inputFileStream.gcount() <= 0) break;

		int dataSize = inputFileStream.gcount();
		unsigned char* data = inputBuffer;

		while (dataSize > 0)
		{
			//解析文件输入数据，将读取到的数据按帧拆分
			int result = av_parser_parse2(
				parserContext,
				context,
				&packet->data,
				&packet->size,
				inputBuffer, inputFileStream.gcount(),
				AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0
			);
			//移动至下一帧
			data += result;
			dataSize -= result;

			if (packet->size)
			{
				//将packet发送至解码线程
				result = avcodec_send_packet(context, packet);
				if (result < 0) break;
				while (result >= 0)
				{
					//获取解码后的AV FRAME
					result = avcodec_receive_frame(context, frame);
					cout << frame->pkt_size << endl;
				}
			}
		}
	}

	av_parser_close(parserContext);
	avcodec_free_context(&context);
	av_frame_free(&frame);
	av_packet_free(&packet);

}