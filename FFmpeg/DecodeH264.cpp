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

	//��ʼ��������
	AVCodec* decoder = avcodec_find_decoder(AV_CODEC_ID_H264);
	AVCodecContext* context = avcodec_alloc_context3(decoder);
	avcodec_open2(context, NULL, NULL);
	AVCodecParserContext* parserContext = av_parser_init(AV_CODEC_ID_H264);

	//����packet�ռ�
	AVPacket* packet = av_packet_alloc();
	unsigned char inputBuffer[4096] = { 0 };

	//����frame�ռ�
	AVFrame* frame = av_frame_alloc();

	while (!inputFileStream.eof())
	{
		//��ȡH264�ļ�
		inputFileStream.read((char*)inputBuffer, sizeof(inputBuffer));
		if (inputFileStream.gcount() <= 0) break;

		int dataSize = inputFileStream.gcount();
		unsigned char* data = inputBuffer;

		while (dataSize > 0)
		{
			//�����ļ��������ݣ�����ȡ�������ݰ�֡���
			int result = av_parser_parse2(
				parserContext,
				context,
				&packet->data,
				&packet->size,
				inputBuffer, inputFileStream.gcount(),
				AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0
			);
			//�ƶ�����һ֡
			data += result;
			dataSize -= result;

			if (packet->size)
			{
				//��packet�����������߳�
				result = avcodec_send_packet(context, packet);
				if (result < 0) break;
				while (result >= 0)
				{
					//��ȡ������AV FRAME
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