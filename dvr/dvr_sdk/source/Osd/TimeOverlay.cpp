#include <math.h>
#include "TimeOverlay.h"
#include "OsdTitle.h"

extern uint8_t fontstring18[OSD_FONT_TYPE_NUM][OSD_TIME_WIDTH*OSD_TIME_HEIGHT];
extern uint8_t fontstring20[OSD_FONT_TYPE_NUM][16*23];
extern uint8_t fontstring24[OSD_FONT_TYPE_NUM][16*28];
extern uint8_t fontstring24X23_W[24*23];
extern uint8_t fontstring24X28_W[24*28];

extern gdouble ms_time(void);
CTimeOverlay::CTimeOverlay(void)
{
    Init();
}

CTimeOverlay::~CTimeOverlay(void)
{
    if(m_timebuf != NULL)
    {
        free(m_timebuf);
        m_timebuf = NULL;
    }
}

int CTimeOverlay::Init(void)
{
    memset(&m_param,0,sizeof(CAPTURE_TITLE_PARAM));
    strcpy(m_string,"2013-10-11 11:11:11 N 00.00'00\" E 180.00'00\"");
    m_param.index     = 1;
    m_param.enable    = 1;
    m_param.x         = OSD_TIME_START_X(30);
    m_param.y         = OSD_TIME_START_Y;
    m_timebuf         = (DVR_U8*)malloc(DVR_IMG_REAL_WIDTH*OSD_HEIGHT);
    memset(m_timebuf,0,DVR_IMG_REAL_WIDTH*OSD_HEIGHT);
    m_buffsize        = 0;
    return DVR_RES_SOK;
}

int CTimeOverlay::SetOSDParm(CAPTURE_TITLE_PARAM* param)
{
    if(param == NULL)
        return -1;
    memcpy(&m_param,param,sizeof(CAPTURE_TITLE_PARAM));
    return DVR_RES_SOK;
}

int CTimeOverlay::GetOSDParm(CAPTURE_TITLE_PARAM* param)
{
    if(param == NULL)
        return DVR_RES_EFAIL;
    memcpy(param,&m_param,sizeof(CAPTURE_TITLE_PARAM));
    return DVR_RES_SOK;
}

DVR_U8* CTimeOverlay::GetFontData(int type)
{
    if(OSD_FONE_SIZE == 18)
        return &fontstring18[type][0];
    else if(OSD_FONE_SIZE == 20)
        return &fontstring20[type][0];
    else if(OSD_FONE_SIZE == 24)
        return &fontstring24[type][0];

    return NULL;
}

DVR_U8* CTimeOverlay::GetWData(int type)
{
    if(OSD_FONE_SIZE == 18)
        return &fontstring18[type][0];
    else if(OSD_FONE_SIZE == 20)
        return fontstring24X23_W;
    else if(OSD_FONE_SIZE == 24)
        return fontstring24X28_W;

    return NULL;
}

int CTimeOverlay::SetImage( DVR_U8* destdata, DVR_U8 *ysrc, int srcw, int h, int& off, int size, int type)
{   
    if(destdata == NULL || ysrc == NULL || m_timebuf == NULL)
        return DVR_RES_EFAIL;

    int startpos    = 0;
    ushort width    = 0;
    ushort height   = 0;
    if(type == OSD_TITLE_TIME)
    {
        startpos = OSD_TIME_START_Y;
        width    = OSD_TIME_WIDTH;
    }
    else if(type == OSD_TITLE_PIC)
    {
        startpos = OSD_PIC_START_Y;
        width    = OSD_PIC_HEIGHT;
    }

    DVR_U8 *srcbuf = ysrc + (h - startpos)*width;
    memcpy(destdata + h*srcw + off,srcbuf,size);
    memcpy(&m_timebuf[h*srcw + off],srcbuf,size);
    memcpy(&m_timebuf[h*srcw + (srcw>>1) + off],srcbuf,size);
    off += size;
    return DVR_RES_SOK;
}

int CTimeOverlay::IaccValueFormat(int idx, OSD_SRC_PARAM* srcparam, Ofilm_Can_Data_T *pCanData, int startpos, bool format)
{
    if(srcparam == NULL || pCanData == NULL)
        return DVR_RES_EFAIL;

    DVR_U8* pImage = srcparam->data;
    DVR_U8* destuv = srcparam->data + srcparam->imagewidth*srcparam->imageheight;
    if (pCanData == NULL || pImage == NULL || m_timebuf == NULL)
        return DVR_RES_EFAIL;

    float  smoothlateral= pCanData->vehicle_data.Lateral_Acceleration;
    float  smoothlongit = pCanData->vehicle_data.Longitudinal_Acceleration;
    int    lateralacc   = fabs(smoothlateral) * 100 + 5;
    int    longitacc    = fabs(smoothlongit) * 100 + 5;
    int    interval     = 20;
    int    space        = 16;
    int    offset       = 6;
    ushort picwitdh     = 0;
    ushort picheight    = 0;
    if(srcparam->type == OSD_SRC_PIC)
    {
        space    = 22;
        interval = 70;
    }

    for(int i = OSD_TIME_START_Y;i < OSD_TIME_END_Y;i++)
    {
        int off = startpos + interval;
        //X
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_X),srcparam->imagewidth,i,off,16);
        off += offset;
        //+/-
        int type = (smoothlateral > 0) ? OSD_FONT_TYPE_PLUS : OSD_FONT_TYPE_H;
        if(smoothlateral != 0.0f)
        {
            SetImage(pImage,GetFontData(type),srcparam->imagewidth,i,off);
            if(type == OSD_FONT_TYPE_PLUS)
                off += 3;
        }

        if(lateralacc/1000 != 0)
            SetImage(pImage,GetFontData(lateralacc/1000),srcparam->imagewidth,i,off);
        SetImage(pImage,GetFontData(lateralacc%1000/100),srcparam->imagewidth,i,off);
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_DOT),srcparam->imagewidth,i,off,4);
        SetImage(pImage,GetFontData(lateralacc%1000%100/10),srcparam->imagewidth,i,off);
        off += space + OSD_TIME_OFFSET_NUM;
        if(smoothlateral == 0.0f)
        {
            off += OSD_TIME_OFFSET_NUM * 2;
        }

        //Y
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_Y),srcparam->imagewidth,i,off,16);
        off += offset;
        //+/-
        type = (smoothlongit > 0) ? OSD_FONT_TYPE_PLUS : OSD_FONT_TYPE_H;
        if(smoothlongit != 0.0f)
        {
            SetImage(pImage,GetFontData(type),srcparam->imagewidth,i,off);
            if(type == OSD_FONT_TYPE_PLUS)
                off += 3;
        }

        if(longitacc/1000 != 0)
            SetImage(pImage,GetFontData(longitacc/1000),srcparam->imagewidth,i,off);
        SetImage(pImage,GetFontData(longitacc%1000/100),srcparam->imagewidth,i,off);
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_DOT),srcparam->imagewidth,i,off,4);
        SetImage(pImage,GetFontData(longitacc%1000%100/10),srcparam->imagewidth,i,off);
    }
    return DVR_RES_SOK;
}

int CTimeOverlay::TimeOverlayFormat(int idx, OSD_SRC_PARAM* srcparam, Ofilm_Can_Data_T *pCanData, int& timeend, bool format)
{
	unsigned char GpsLongitude_IsEast;
    unsigned short GpsLongitude_Deg;
    unsigned short GpsLongitude_Min;
    unsigned short GpsLongitude_Sec;

    unsigned char GpsLatitude_IsNorth;
    unsigned short GpsLatitude_Deg;
    unsigned short GpsLatitude_Min;
    unsigned short GpsLatitude_Sec;

    if(srcparam == NULL || pCanData == NULL)
        return DVR_RES_EFAIL;

    DVR_U8* pImage = srcparam->data;
    DVR_U8* destuv = srcparam->data + srcparam->imagewidth*srcparam->imageheight;
    if (pCanData == NULL || pImage == NULL || m_timebuf == NULL)
        return DVR_RES_EFAIL;

    if(!format || idx != 0)
    {
        if(idx%2 == 0)
        {
            memcpy(pImage + (idx>>1)*srcparam->imagewidth*(srcparam->imageheight>>1),m_timebuf,srcparam->imagewidth*OSD_HEIGHT);
            memset(destuv + (idx>>1)*srcparam->imagewidth*(srcparam->imageheight>>2),0x80,srcparam->imagewidth*(OSD_HEIGHT>>1));
        }
        return DVR_RES_SOK;
    }
    //clear buffer
    memset(m_timebuf,0,DVR_IMG_REAL_WIDTH*OSD_HEIGHT);
    //clear zero Y
    memset(pImage,0,srcparam->imagewidth*OSD_HEIGHT);
    //clear zero UV
    memset(destuv,0x80,srcparam->imagewidth*(OSD_HEIGHT>>1));
    //struct tm *t;
    //time_t tt;
    //time(&tt);
    //t = localtime(&tt);
    //printf("TimeOverlayFormat::%4d-%02d-%02d %02d:%02d:%02d\n", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    int offset = 0;
    int year    = pCanData->vehicle_data.year + 2013;
    int month   = pCanData->vehicle_data.month;
    int day     = pCanData->vehicle_data.day;
    int hour    = pCanData->vehicle_data.hour;
    int minute  = pCanData->vehicle_data.minute;
    int second  = pCanData->vehicle_data.second;
#ifdef OSD_IACC
    int startpos = OSD_TIME_START_X(30);
    if(srcparam->type == OSD_SRC_PIC)
        startpos = OSD_TIME_START_X(50);
#else
    int startpos = OSD_TIME_START_X(30);
    if(srcparam->type == OSD_SRC_PIC)
        startpos = OSD_TIME_START_X(100);
#endif
    float GpsLongitude = (float)pCanData->vehicle_data.positioning_system_longitude * 0.000001f - 268.435455f;
    float GpsLatitude = (float)pCanData->vehicle_data.positioning_system_latitude * 0.000001f - 134.217727f;
    float absGpsLongitude = fabs(GpsLongitude);
    float absGpsLatitude = fabs(GpsLatitude); 

	absGpsLongitude = (absGpsLongitude > 180.0f) ? 180.0f : absGpsLongitude;
	absGpsLatitude = (absGpsLatitude > 90.0f) ? 90.0f : absGpsLatitude;

    GpsLongitude_IsEast = (GpsLongitude >= 0) ? 1 : 0;
    GpsLongitude_Deg = (uint16_t)absGpsLongitude;
    GpsLongitude_Min = (uint16_t)((float)((float)absGpsLongitude - GpsLongitude_Deg) * 60);
    GpsLongitude_Sec = (uint16_t)((((float)((float)absGpsLongitude - GpsLongitude_Deg) * 60) - GpsLongitude_Min) * 60)  ;

    GpsLatitude_IsNorth = (GpsLatitude >= 0) ? 1 : 0;
    GpsLatitude_Deg = (uint16_t)absGpsLatitude;
    GpsLatitude_Min = (uint16_t)((float)((float)absGpsLatitude - GpsLatitude_Deg) * 60);
    GpsLatitude_Sec = (uint16_t)((((float)((float)absGpsLatitude - GpsLatitude_Deg) * 60) - GpsLatitude_Min) * 60)  ;





	//double start = ms_time();
    for(int i = OSD_TIME_START_Y;i < OSD_TIME_END_Y;i++)
    {
        offset = startpos;
        SetImage(pImage,GetFontData(year/1000),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(year%1000/100),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(year%1000%100/10),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(year%1000%100%10),srcparam->imagewidth,i,offset);

        //2013-
        offset += 3;
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_H),srcparam->imagewidth,i,offset,8);

        SetImage(pImage,GetFontData(month/10),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(month%10),srcparam->imagewidth,i,offset);

        //2013-10-
        offset += 3;
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_H),srcparam->imagewidth,i,offset,8);

        SetImage(pImage,GetFontData(day/10),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(day%10),srcparam->imagewidth,i,offset);

        //2013-10-10
        offset += 7;
        SetImage(pImage,GetFontData(hour/10),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(hour%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_M),srcparam->imagewidth,i,offset,6);

        //2013-10-10 10:10
        SetImage(pImage,GetFontData(minute/10),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(minute%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10:
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_M),srcparam->imagewidth,i,offset,6);

        //2013-10-10 10:10:10
        SetImage(pImage,GetFontData(second/10),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(second%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10:10   S
#ifdef OSD_IACC
        if(srcparam->type == OSD_SRC_PIC)
            offset += 40;
        else
            offset += 15;
#else
        offset += 50;
#endif
        int direction = GpsLatitude_IsNorth ? OSD_FONT_TYPE_N : OSD_FONT_TYPE_S;
        SetImage(pImage,GetFontData(direction),srcparam->imagewidth,i,offset,16);

        //2013-10-10 10:10:10   S10
        offset += 4;
        SetImage(pImage,GetFontData(GpsLatitude_Deg/10),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(GpsLatitude_Deg%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10:10   S 10.
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_DOT),srcparam->imagewidth,i,offset,5);

        //2013-10-10 10:10:10   S 10.10
        SetImage(pImage,GetFontData(GpsLatitude_Min/10),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(GpsLatitude_Min%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10:10   S 10.10'
        offset += 2;
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_COMMA),srcparam->imagewidth,i,offset,3);

        //2013-10-10 10:10:10   S 10.10'10
        SetImage(pImage,GetFontData(GpsLatitude_Sec/10),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(GpsLatitude_Sec%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10:10   S 10.10'10"
        offset += 2;
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_DQM),srcparam->imagewidth,i,offset,6);

        //2013-10-10 10:10:10   S 10.10'10" W
#ifdef OSD_IACC
        int move = 20;
        if(srcparam->type == OSD_SRC_PIC)
            offset += 40;
        else
            offset += 15;
#else
        int move = 24;
        offset += 50;
#endif
        direction = GpsLongitude_IsEast ? OSD_FONT_TYPE_E : OSD_FONT_TYPE_W;
        if(direction == OSD_FONT_TYPE_E)
            SetImage(pImage,GetFontData(direction),srcparam->imagewidth,i,offset,16);
        else if(direction == OSD_FONT_TYPE_W)
        {
            DVR_U8 *srcbuf = GetWData(OSD_FONT_TYPE_W) + (i - OSD_TIME_START_Y)*24;
            memcpy(pImage + i*srcparam->imagewidth + offset,srcbuf,move);
            memcpy(&m_timebuf[i*srcparam->imagewidth + offset],srcbuf,move);
            memcpy(&m_timebuf[i*srcparam->imagewidth + (srcparam->imagewidth>>1) + offset],srcbuf,move);
            offset += move;
        }

        //2013-10-10 10:10 S 10.10'10" W 180
        offset += 4;
        if(GpsLongitude_Deg/100 != 0)
            SetImage(pImage,GetFontData(GpsLongitude_Deg/100),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(GpsLongitude_Deg%100/10),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(GpsLongitude_Deg%100%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10 S 10.10'10" W 180.
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_DOT),srcparam->imagewidth,i,offset,5);

        //2013-10-10 10:10 S 10.10'10" W 180.10
        SetImage(pImage,GetFontData(GpsLongitude_Min/10),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(GpsLongitude_Min%10),srcparam->imagewidth,i,offset);
        
        //2013-10-10 10:10 S 10.10'10" W 180.10'
        offset += 2;
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_COMMA),srcparam->imagewidth,i,offset,3);

        //2013-10-10 10:10 S 10.10'10" W 180.10'10
        SetImage(pImage,GetFontData(GpsLongitude_Sec/10),srcparam->imagewidth,i,offset);
        SetImage(pImage,GetFontData(GpsLongitude_Sec%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10 S 10.10'10" W 180.10'10"
        offset += 2;
        SetImage(pImage,GetFontData(OSD_FONT_TYPE_DQM),srcparam->imagewidth,i,offset,6);
    }
    timeend = offset;
    #ifdef OSD_IACC
    //IaccValueFormat(idx, srcparam, pCanData, offset, format);
    #endif
	//double end = ms_time();
	//DPrint(DPRINT_INFO, "TimeOverlayFormat(type %d) takes %lf ms\n",srcparam->type, end-start);
    return DVR_RES_SOK;
}


CTimeOverlayEx::CTimeOverlayEx(void)
{
    if(m_timebuf != NULL)
    {
        free(m_timebuf);
        m_timebuf = NULL;
    }
}

CTimeOverlayEx::~CTimeOverlayEx(void)
{
    m_timebuf = NULL;
}

int CTimeOverlayEx::SetImage(DVR_U8 *ysrc, int srcw, int h, int& off, int size, int type)
{   
    if(ysrc == NULL || m_timebuf == NULL)
        return DVR_RES_EFAIL;

    int startpos    = 0;
    ushort width    = 0;
    ushort height   = 0;
    if(type == OSD_TITLE_TIME)
    {
        startpos = OSD_TIME_START_Y;
        width    = OSD_TIME_WIDTH;
    }
    else if(type == OSD_TITLE_PIC)
    {
        startpos = OSD_PIC_START_Y;
        width    = OSD_PIC_HEIGHT;
    }

    DVR_U8 *srcbuf = ysrc + (h - startpos)*width;
    memcpy(&m_timebuf[h*srcw + off],srcbuf,size);
    off += size;
    return DVR_RES_SOK;
}


int CTimeOverlayEx::TimeOverlayFormat(int idx, OSD_SRC_PARAM* srcparam, Ofilm_Can_Data_T *pCanData, int& timeend, bool format)
{
	unsigned char GpsLongitude_IsEast;
    unsigned short GpsLongitude_Deg;
    unsigned short GpsLongitude_Min;
    unsigned short GpsLongitude_Sec;

    unsigned char GpsLatitude_IsNorth;
    unsigned short GpsLatitude_Deg;
    unsigned short GpsLatitude_Min;
    unsigned short GpsLatitude_Sec;

    if(srcparam == NULL || pCanData == NULL)
        return DVR_RES_EFAIL;

    if (pCanData == NULL || m_timebuf == NULL)
        return DVR_RES_EFAIL;

    //clear Y
    memset(m_timebuf,0,srcparam->imagewidth*srcparam->imageheight);

    //clear zero UV
    memset(m_timebuf + srcparam->imagewidth*srcparam->imageheight,0x80,srcparam->imagewidth*(srcparam->imageheight>>1));
    //struct tm *t;
    //time_t tt;
    //time(&tt);
    //t = localtime(&tt);
    //printf("TimeOverlayFormat::%4d-%02d-%02d %02d:%02d:%02d\n", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    int offset = 0;
    int year    = pCanData->vehicle_data.year + 2013;
    int month   = pCanData->vehicle_data.month;
    int day     = pCanData->vehicle_data.day;
    int hour    = pCanData->vehicle_data.hour;
    int minute  = pCanData->vehicle_data.minute;
    int second  = pCanData->vehicle_data.second;
#ifdef OSD_IACC
    int startpos = OSD_TIME_START_X(30);
    if(srcparam->type == OSD_SRC_PIC)
        startpos = OSD_TIME_START_X(50);
#else
    int startpos = OSD_TIME_START_X(30);
    if(srcparam->type == OSD_SRC_PIC)
        startpos = OSD_TIME_START_X(100);
#endif
    float GpsLongitude = (float)pCanData->vehicle_data.positioning_system_longitude * 0.000001f - 268.435455f;
    float GpsLatitude = (float)pCanData->vehicle_data.positioning_system_latitude * 0.000001f - 134.217727f;
    float absGpsLongitude = fabs(GpsLongitude);
    float absGpsLatitude = fabs(GpsLatitude); 

	absGpsLongitude = (absGpsLongitude > 180.0f) ? 180.0f : absGpsLongitude;
	absGpsLatitude = (absGpsLatitude > 90.0f) ? 90.0f : absGpsLatitude;

    GpsLongitude_IsEast = (GpsLongitude >= 0) ? 1 : 0;
    GpsLongitude_Deg = (uint16_t)absGpsLongitude;
    GpsLongitude_Min = (uint16_t)((float)((float)absGpsLongitude - GpsLongitude_Deg) * 60);
    GpsLongitude_Sec = (uint16_t)((((float)((float)absGpsLongitude - GpsLongitude_Deg) * 60) - GpsLongitude_Min) * 60)  ;

    GpsLatitude_IsNorth = (GpsLatitude >= 0) ? 1 : 0;
    GpsLatitude_Deg = (uint16_t)absGpsLatitude;
    GpsLatitude_Min = (uint16_t)((float)((float)absGpsLatitude - GpsLatitude_Deg) * 60);
    GpsLatitude_Sec = (uint16_t)((((float)((float)absGpsLatitude - GpsLatitude_Deg) * 60) - GpsLatitude_Min) * 60)  ;

#if 0	
	static int count=0;
	count++;
	if(count%20==0) 
		DPrint(DPRINT_ERR,"111   N:%d %d.%d'%d'' E:%d %d.%d'%d'' \n",
			GpsLatitude_IsNorth,GpsLatitude_Deg,GpsLatitude_Min,GpsLatitude_Sec,
			GpsLongitude_IsEast,GpsLongitude_Deg,GpsLongitude_Min,GpsLongitude_Sec
	);
#endif


	//double start = ms_time();
    for(int i = OSD_TIME_START_Y;i < OSD_TIME_END_Y;i++)
    {
        offset = startpos;
        SetImage(GetFontData(year/1000),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(year%1000/100),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(year%1000%100/10),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(year%1000%100%10),srcparam->imagewidth,i,offset);

        //2013-
        offset += 3;
        SetImage(GetFontData(OSD_FONT_TYPE_H),srcparam->imagewidth,i,offset,8);

        SetImage(GetFontData(month/10),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(month%10),srcparam->imagewidth,i,offset);

        //2013-10-
        offset += 3;
        SetImage(GetFontData(OSD_FONT_TYPE_H),srcparam->imagewidth,i,offset,8);

        SetImage(GetFontData(day/10),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(day%10),srcparam->imagewidth,i,offset);

        //2013-10-10
        offset += 7;
        SetImage(GetFontData(hour/10),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(hour%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:
        SetImage(GetFontData(OSD_FONT_TYPE_M),srcparam->imagewidth,i,offset,6);

        //2013-10-10 10:10
        SetImage(GetFontData(minute/10),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(minute%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10:
        SetImage(GetFontData(OSD_FONT_TYPE_M),srcparam->imagewidth,i,offset,6);

        //2013-10-10 10:10:10
        SetImage(GetFontData(second/10),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(second%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10:10   S
#ifdef OSD_IACC
        if(srcparam->type == OSD_SRC_PIC)
            offset += 40;
        else
            offset += 15;
#else
        offset += 50;
#endif
        int direction = GpsLatitude_IsNorth ? OSD_FONT_TYPE_N : OSD_FONT_TYPE_S;
        SetImage(GetFontData(direction),srcparam->imagewidth,i,offset,16);

        //2013-10-10 10:10:10   S10
        offset += 4;
        SetImage(GetFontData(GpsLatitude_Deg/10),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(GpsLatitude_Deg%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10:10   S 10.
        SetImage(GetFontData(OSD_FONT_TYPE_DOT),srcparam->imagewidth,i,offset,5);

        //2013-10-10 10:10:10   S 10.10
        SetImage(GetFontData(GpsLatitude_Min/10),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(GpsLatitude_Min%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10:10   S 10.10'
        offset += 2;
        SetImage(GetFontData(OSD_FONT_TYPE_COMMA),srcparam->imagewidth,i,offset,3);

        //2013-10-10 10:10:10   S 10.10'10
        SetImage(GetFontData(GpsLatitude_Sec/10),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(GpsLatitude_Sec%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10:10   S 10.10'10"
        offset += 2;
        SetImage(GetFontData(OSD_FONT_TYPE_DQM),srcparam->imagewidth,i,offset,6);

        //2013-10-10 10:10:10   S 10.10'10" W
#ifdef OSD_IACC
        int move = 20;
        if(srcparam->type == OSD_SRC_PIC)
            offset += 40;
        else
            offset += 15;
#else
        int move = 24;
        offset += 50;
#endif
        direction = GpsLongitude_IsEast ? OSD_FONT_TYPE_E : OSD_FONT_TYPE_W;

#if 1
        if(direction == OSD_FONT_TYPE_E)
            SetImage(GetFontData(direction),srcparam->imagewidth,i,offset,16);
        else if(direction == OSD_FONT_TYPE_W)
        {
            DVR_U8 *srcbuf = GetWData(OSD_FONT_TYPE_W) + (i - OSD_TIME_START_Y)*24;
            memcpy(&m_timebuf[i*srcparam->imagewidth + offset],srcbuf,move);
            memcpy(&m_timebuf[i*srcparam->imagewidth + (srcparam->imagewidth>>1) + offset],srcbuf,move);
            offset += move;
        }
#else
		SetImage(GetFontData(direction),srcparam->imagewidth,i,offset,16);
#endif
        //2013-10-10 10:10 S 10.10'10" W 180
        offset += 4;
        if(GpsLongitude_Deg/100 != 0)
            SetImage(GetFontData(GpsLongitude_Deg/100),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(GpsLongitude_Deg%100/10),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(GpsLongitude_Deg%100%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10 S 10.10'10" W 180.
        SetImage(GetFontData(OSD_FONT_TYPE_DOT),srcparam->imagewidth,i,offset,5);

        //2013-10-10 10:10 S 10.10'10" W 180.10
        SetImage(GetFontData(GpsLongitude_Min/10),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(GpsLongitude_Min%10),srcparam->imagewidth,i,offset);
        
        //2013-10-10 10:10 S 10.10'10" W 180.10'
        offset += 2;
        SetImage(GetFontData(OSD_FONT_TYPE_COMMA),srcparam->imagewidth,i,offset,3);

        //2013-10-10 10:10 S 10.10'10" W 180.10'10
        SetImage(GetFontData(GpsLongitude_Sec/10),srcparam->imagewidth,i,offset);
        SetImage(GetFontData(GpsLongitude_Sec%10),srcparam->imagewidth,i,offset);

        //2013-10-10 10:10 S 10.10'10" W 180.10'10"
        offset += 2;
        SetImage(GetFontData(OSD_FONT_TYPE_DQM),srcparam->imagewidth,i,offset,6);
    }
    timeend = offset;
    #ifdef OSD_IACC
    //IaccValueFormat(idx, srcparam, pCanData, offset, format);
    #endif
	//double end = ms_time();
	//DPrint(DPRINT_INFO, "TimeOverlayFormat(type %d) takes %lf ms\n",srcparam->type, end-start);
    return DVR_RES_SOK;
}

