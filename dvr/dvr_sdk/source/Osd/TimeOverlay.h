#ifndef _OSD_TIME_OVERLAY_H_
#define _OSD_TIME_OVERLAY_H_
#include "DVR_RECORDER_DEF.h"
#include <gst/canbuf/canbuf.h>
#include "OsdDef.h"

class CTimeOverlay
{
public:
    CTimeOverlay(void);
    virtual ~CTimeOverlay(void);

public:
	virtual int IaccValueFormat(int idx, OSD_SRC_PARAM* srcparam, Ofilm_Can_Data_T *pCanData, int startpos, bool format);
	virtual int TimeOverlayFormat(int idx, OSD_SRC_PARAM* srcparam, Ofilm_Can_Data_T *pCanData, int& timeend, bool format);
	virtual int SetImage(DVR_U8 *ysrc, int srcw, int h, int& off, int size=OSD_TIME_OFFSET_NUM, int type=OSD_TITLE_TIME){return -1;};
	virtual int SetImage( DVR_U8* destdata, DVR_U8 *ysrc, int srcw, int h, int& off, int size=OSD_TIME_OFFSET_NUM, int type=OSD_TITLE_TIME);
    virtual int SetOSDParm(CAPTURE_TITLE_PARAM* param);
    virtual int GetOSDParm(CAPTURE_TITLE_PARAM* param);
	virtual DVR_U8* GetOSDBuf(void){return     m_timebuf;};
	virtual void SetOSDBuf(void* buffer, int size){  m_timebuf = (DVR_U8*)buffer;m_buffsize=  size;};

protected:
    int Init(void);
	DVR_U8* GetWData(int type);
	DVR_U8* GetFontData(int type);

protected:
	CAPTURE_TITLE_PARAM m_param;
    char 			m_string[256];
    DVR_U8* 		m_timebuf;
    int 		    m_buffsize;
};

class CTimeOverlayEx : public CTimeOverlay
{
public:
    CTimeOverlayEx(void);
    virtual ~CTimeOverlayEx(void);

public:
	virtual int TimeOverlayFormat(int idx, OSD_SRC_PARAM* srcparam, Ofilm_Can_Data_T *pCanData, int& timeend, bool format);
	virtual int SetImage(DVR_U8 *ysrc, int srcw, int h, int& off, int size=OSD_TIME_OFFSET_NUM, int type=OSD_TITLE_TIME);
};

#endif
