#include <iostream>
extern "C" {
#include <libavformat/avformat.h>
}
#pragma comment(lib, "avformat.lib")
int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cout << "Usage: [programe] [intput_video_path/url]" << std::endl;
		return -1;
	}
	char *inputUrl = argv[1];
	AVFormatContext *fmtCtx = NULL;
	int ret;
	ret = avformat_network_init();
	if (ret < 0) {
		std::cout << "avformat_network_init failed " << std::endl;
		return -1;
	}
	fmtCtx = avformat_alloc_context();
	if (NULL == fmtCtx) {
		std::cout << "avformat_alloc_context failed " << std::endl;
		return -2;
	}
	ret = avformat_open_input(&fmtCtx, inputUrl, NULL, NULL);
	if (ret < 0) {
		std::cout << "avformat_open_input failed " << std::endl;
		return -3;
	}

	ret = avformat_find_stream_info(fmtCtx, NULL);
	if (ret < 0) {
		std::cout << "avformat_find_stream_info failed " << std::endl;
		return -4;
	}

	int videoIndex = -1;

	int nbStreams = fmtCtx->nb_streams;
	for (int i = 0; i < nbStreams; i++) {
		// fmtCtx->streams[i]->codec 新版ffmpeg已弃用
		if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoIndex = i;
		}
	}
	std::cout << "----------输入流信息---------------" << std::endl;
	av_dump_format(fmtCtx, 0, inputUrl, NULL);
	long duration = fmtCtx->duration;  //微秒数
	std::cout << "duration: " << duration << std::endl;
	std::cout << "视频时常为: " << duration / 1000.0 / 1000.0 << "s" << std::endl;
	system("pause");
	return 0;
}