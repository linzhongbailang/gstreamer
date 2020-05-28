#ifndef _OSD_STRING_H_
#define _OSD_STRING_H_
#include "DVR_RECORDER_DEF.h"
#include "OsdDef.h"
#include "OsdFont.h"
#include "OsdDc.h"
#include "OsdDef.h"
#include "BMP.h"

#define THUMB_BMP
#define BMP_WIGTH  44
#define BMP_HEIGHT 30
class COsdString
{
public:
    COsdString(void);
    ~COsdString(void);

public:
	int SetString(OSD_SRC_PARAM* srcparam, BMPIMAGE* bmpimg);
	
private:
	DVR_U8* GetRasterBuf(const char* str, uint& width, uint& height);
private:
    VD_SIZE 		m_size;
};
#endif

