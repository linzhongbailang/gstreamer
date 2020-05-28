#ifndef _JPEG_THUMB_H_
#define _JPEG_THUMB_H_
#include "DVR_SDK_DEF.h"

class CJpegThumb
{
public:
	CJpegThumb(void);
	~CJpegThumb();

	int  MakeThumb(const char* filename, unsigned char *pPreviewBuf, int nSize);

private:
	unsigned char* ReadJpeg(const char* path, int& width, int& height);
	unsigned char* do_Stretch_Linear(int w_Dest, int h_Dest, int bit_depth, unsigned char *src, int w_Src, int h_Src);
};
#endif//_JPEG_THUMB_H_