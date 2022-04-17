#include <iostream>
#include "Delogo.h"
#include <vector>

#ifdef	__cplusplus
extern "C"
{
#endif

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "postproc.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib")


#ifdef __cplusplus
};
#endif



std::string Unicode_to_Utf8(const std::string & str)
{
	int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
	wchar_t * pwBuf = new wchar_t[nwLen + 1];//一定要加1，不然会出现尾巴 
	ZeroMemory(pwBuf, nwLen * 2 + 2);
	::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);
	int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
	char * pBuf = new char[nLen + 1];
	ZeroMemory(pBuf, nLen + 1);
	::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);
	std::string retStr(pBuf);
	delete[]pwBuf;
	delete[]pBuf;
	pwBuf = NULL;
	pBuf = NULL;
	return retStr;
}


int main()
{
	CDelogo cCDelogo;
	const char *pFileA = "E:\\learn\\ffmpeg\\FfmpegFilterTest\\x64\\Release\\in-computer_drawmovie.mp4";

	const char *pFileOut = "E:\\learn\\ffmpeg\\FfmpegFilterTest\\x64\\Release\\in-computer_drawmovie_delogo.mp4";

	int x = 100;
	int y = 300;
	int width = 480;
	int height = 320;
	cCDelogo.StartDelogo(pFileA, pFileOut, x, y, width, height);
	cCDelogo.WaitFinish();
	return 0;
}



