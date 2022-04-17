#include <vector>
extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}
class AudioEncoder
{
public:
	AudioEncoder();
	~AudioEncoder();
	int InitAAC(int channels, int sample_rate, int bit_rate);
	//    int InitMP3(/*int channels, int sample_rate, int bit_rate*/);
	void DeInit();  // �ͷ���Դ
	AVPacket *Encode(AVFrame *frame, int stream_index, int64_t pts, int64_t time_base);
	int Encode(AVFrame *farme, int stream_index, int64_t pts, int64_t time_base,
		std::vector<AVPacket *> &packets);
	int GetFrameSize(); // ��ȡһ֡���� ÿ��ͨ����Ҫ���ٸ�������
	int GetSampleFormat();  // ��������Ҫ�Ĳ�����ʽ
	AVCodecContext *GetCodecContext();
	int GetChannels();
	int GetSampleRate();
private:
	int channels_ = 2;
	int sample_rate_ = 44100;
	int bit_rate_ = 128 * 1024;
	int64_t pts_ = 0;
	AVCodecContext * codec_ctx_ = NULL;
};


