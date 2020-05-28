#ifndef _OSD_TITILE_H_
#define _OSD_TITILE_H_
#include <string.h>
#include <stdint.h>
#include <vector>
#include <gst/canbuf/canbuf.h>
#include "DVR_RECORDER_DEF.h"
#include "RecorderAsyncOp.h"
#include "OsdDef.h"


using std::vector;
class CTimeOverlay;
class CTimeOverlayEx;
class COSDTitle
{
public:
    COSDTitle(void);
    virtual ~COSDTitle(void);
public:
    virtual int SetOSDParm(CAPTURE_TITLE_PARAM* param);
    virtual int GetOSDParm(CAPTURE_TITLE_PARAM* param);
	virtual int GetOSDParam(uint idx, CAPTURE_TITLE_PARAM *param);
	virtual int SetPicOverlayParam(OSD_POINT* pPoint);
	virtual int SetOverlay(Ofilm_Can_Data_T *pCanData, OSD_SRC_PARAM* srcparam);
	virtual int SetOverlay(Ofilm_Can_Data_T *pCanData, OSD_SRC_PARAM* srcparam, void* buffer, int size){return -1;};
	static int GetTitleWidth(int type, ushort* witdh, ushort* height);
protected:
	virtual int  GetImageYPos(int idx, int srcw, int srch, int h, int x);
	virtual int  GetImageUVPos(int idx, int srcw, int srch, int h);
	virtual int  ConvertGearType(int pos);
	virtual int  ConvertAccType(int pos);
	virtual int	 ConvertDasType(int type);
	virtual bool CheckVehicleData(Ofilm_Can_Data_T *pCanData, int type, bool force = false);
	virtual int  SpeedOverlay(DVR_U8* pImage, DVR_U16 vehicleSpeed, int x, int y, int pos);
	virtual int  SetDasOverlay(Ofilm_Can_Data_T *pCanData, DVR_U8* pImage, int srcwidth, int x, int y);
	virtual int  PicOverlayFormat(int idx, OSD_SRC_PARAM* srcparam, Ofilm_Can_Data_T *pCanData,  bool format = false);
	virtual DVR_U8* GetPicData(int maintype, int subtype = 0);

protected:
	CAPTURE_TITLE_PARAM 	m_param;
	CTimeOverlay			*m_timeoverlay;
	char 					m_iconID[OSD_PIC_NUM];
	DVR_U64					m_time;
	vector<CAPTURE_TITLE_PARAM> osdparam;
};

class COSDTitleEx : public  COSDTitle
{
public:
    COSDTitleEx(void);
    virtual ~COSDTitleEx(void);
public:
	virtual int SetOverlay(Ofilm_Can_Data_T *pCanData, OSD_SRC_PARAM* srcparam, void* buffer, int size);
protected:
	virtual int  SetDasOverlay(Ofilm_Can_Data_T *pCanData, DVR_U8* pImage, int srcwidth, int x, int y);
	virtual int  PicOverlayFormat(int idx, OSD_SRC_PARAM* srcparam, Ofilm_Can_Data_T *pCanData,  bool format = false);
};

#endif//_OSD_TITILE_H_
