#include <stdio.h>
#include <stdlib.h>
#include "OsdTitle.h"
#include "TimeOverlay.h"
#include "dprint.h"

extern gdouble ms_time(void);
extern uint8_t picspeeddata[OSD_PIC_STATE_SPPED][108*36*3>>1];
extern uint8_t picdasdata[OSD_PIC_STATE_DAS][36*36*3>>1];
extern uint8_t picgeardata[OSD_PIC_STATE_GEAR][36*36*3>>1];
extern uint8_t picaccdata[OSD_PIC_STATE_ACC][36*36*3>>1];
extern uint8_t picbrakedata[OSD_PIC_STATE_BRAKE][36*36*3>>1];
extern uint8_t picleftdata[OSD_PIC_STATE_TURN_LEFT][36*36*3>>1];
extern uint8_t picrightdata[OSD_PIC_STATE_TURN_RIGHT][36*36*3>>1];
extern uint8_t picbuckledata[OSD_PIC_STATE_BUCKLE][36*36*3>>1];
extern uint8_t piclateraldata[28*20*3>>1];
extern uint8_t piclongitdata[20*28*3>>1];
extern uint8_t fontstring18[OSD_FONT_TYPE_NUM][OSD_TIME_WIDTH*OSD_TIME_HEIGHT];

COSDTitle::COSDTitle(void)
{
    m_timeoverlay   = NULL;
    m_time          = 0;
	memset(m_iconID,0,sizeof(m_iconID));
    memset(&m_param,0,sizeof(CAPTURE_TITLE_PARAM));
    m_timeoverlay = new CTimeOverlay();
}

COSDTitle::~COSDTitle(void)
{
    if(m_timeoverlay)
    {
        delete m_timeoverlay;
        m_timeoverlay = NULL;
    }
}

DVR_U8* COSDTitle::GetPicData(int maintype, int subtype)
{
    switch(maintype)
    {
#ifdef OSD_IACC
        case OSD_PIC_TYPE_DAS:
        return &picdasdata[subtype][0];
#endif
        case OSD_PIC_TYPE_SPPED:
        return &picspeeddata[subtype][0];
        case OSD_PIC_TYPE_GEAR:
        return &picgeardata[subtype][0];
        case OSD_PIC_TYPE_ACC:
        return &picaccdata[subtype][0];
        case OSD_PIC_TYPE_BRAKE:
        return &picbrakedata[subtype][0];
        case OSD_PIC_TYPE_TURN_LEFT:
        return &picleftdata[subtype][0];
        case OSD_PIC_TYPE_TURN_RIGHT:
        return &picrightdata[subtype][0];
        case OSD_PIC_TYPE_BUCKLE:
        return &picbuckledata[subtype][0];
        default:
        printf("error maintype %d subtype %d!!!!!!!!\n",maintype,subtype);
        break;
    }
    return NULL;
}

int COSDTitle::GetImageYPos(int idx, int srcw, int srch, int h, int x)
{
    if(idx == 0)
        return h * srcw + x;
    else if(idx == 1)
        return h * srcw + x + (srcw>>1);
    else if(idx == 2)
        return (h + (srch>>1)) * srcw + x;
    else if(idx == 3)
        return (h + (srch>>1)) * srcw + x + (srcw>>1);
    return 0;
}

int COSDTitle::GetImageUVPos(int idx, int srcw, int srch, int h)
{
    if(idx == 0)
        return h * srcw;
    else if(idx == 1)
        return h * srcw + (srcw>>1);
    else if(idx == 2)
        return (h + (srch>>2)) * srcw;
    else if(idx == 3)
        return (h + (srch>>2)) * srcw + (srcw>>1);
    return 0;
}

int COSDTitle::GetTitleWidth(int type, ushort* witdh, ushort* height)
{
    switch(type)
    {
        case OSD_PIC_TYPE_GEAR:
        case OSD_PIC_TYPE_ACC:
        case OSD_PIC_TYPE_BRAKE:
        case OSD_PIC_TYPE_BUCKLE:
        case OSD_PIC_TYPE_TURN_LEFT:
        case OSD_PIC_TYPE_TURN_RIGHT:
#ifdef OSD_IACC
        case OSD_PIC_TYPE_DAS:
#endif
        *witdh  = 36;
        *height = 36;
        break;
        case OSD_PIC_TYPE_SPPED:
        *witdh  = 108;
        *height = 36;
        break;
        default:
        printf("error type %d!!!!!!!!\n",type);
        break;
    }
    return DVR_RES_SOK;
}

int COSDTitle::SetPicOverlayParam(OSD_POINT* pPoint)
{
    int temp = 0;
    CAPTURE_TITLE_PARAM param;
    for(ushort i = 0;i < OSD_PIC_NUM;i++)
    {
        memset(&param,0,sizeof(CAPTURE_TITLE_PARAM));
        param.enable = true;
        param.index = i;
        param.x = pPoint->lX + temp;
        param.y = OSD_PIC_HEIGHT;
        GetTitleWidth(i,&param.width,&param.height);
        temp = pPoint->lX + param.width;
        param.raster = GetPicData(i,0);
        osdparam.push_back(param);
    }
    return DVR_RES_SOK;
}

int COSDTitle::GetOSDParam(uint idx, CAPTURE_TITLE_PARAM *param)
{
    for (vector<CAPTURE_TITLE_PARAM>::iterator it = osdparam.begin(); it != osdparam.end(); it++)
    {
        if(it->index != idx)
            continue;
        memcpy(param,&(*it),sizeof(CAPTURE_TITLE_PARAM));
    }
    return DVR_RES_SOK;
}

int COSDTitle::SetOSDParm(CAPTURE_TITLE_PARAM* param)
{
    if(param == NULL)
        return DVR_RES_EFAIL;
    memcpy(&m_param,param,sizeof(CAPTURE_TITLE_PARAM));
    return DVR_RES_SOK;
}

int COSDTitle::GetOSDParm(CAPTURE_TITLE_PARAM* param)
{
    if(param == NULL)
        return DVR_RES_EFAIL;
    memcpy(param,&m_param,sizeof(CAPTURE_TITLE_PARAM));
    return DVR_RES_SOK;
}

int COSDTitle::SpeedOverlay(DVR_U8* pImage, DVR_U16 vehicleSpeed, int x, int y, int pos)
{
    if(pImage == NULL)
        return DVR_RES_EFAIL;

    DVR_U8* ydata  = &fontstring18[OSD_FONT_TYPE_H][0];
    DVR_U8 *raster = GetPicData(OSD_PIC_TYPE_SPPED,m_iconID[OSD_PIC_TYPE_SPPED]);

    if(vehicleSpeed == 0x1FF)
    {
        if(x >= 14 && x < 24 && y >= 17 && y < 22)
            *pImage = 0x85;
        else if(x >= 28 && x < 38 && y >= 17 && y < 22)
            *pImage = 0x85;
        else if(x >= 42 && x < 52 && y >= 17 && y < 22)
            *pImage = 0x85;
        else
            *pImage = raster[pos];
        return DVR_RES_SOK;
    }
    
    if(x >= 14 && x < 25 && y >= 9 && y < 30)
    {
        ydata  = &fontstring18[vehicleSpeed/100][0];
        *pImage = ydata[(y - 9)*OSD_TIME_WIDTH + x - 14];
    }
    else if(x >= 25 && x < 36 && y >= 9 && y < 30)
    {
        ydata  = &fontstring18[vehicleSpeed%100/10][0];
        *pImage = ydata[(y - 9)*OSD_TIME_WIDTH + x - 25];
    }
    else if(x >= 36 && x < 47 && y >= 9 && y < 30)
    {
        ydata  = &fontstring18[vehicleSpeed%100%10][0];
        *pImage = ydata[(y - 9)*OSD_TIME_WIDTH + x - 36];
    }
    else
        *pImage = raster[pos];
    return DVR_RES_SOK;
}

int COSDTitle::SetDasOverlay(Ofilm_Can_Data_T *pCanData, DVR_U8* pImage, int srcwidth, int x, int y)
{
    if(pCanData == NULL || pImage == NULL || m_timeoverlay == NULL)
        return DVR_RES_EFAIL;

    int dasstate[OSD_PIC_STATE_DAS] = {0};
    if(pCanData->vehicle_data.APA_LSCAction == 0x1)
        dasstate[OSD_PIC_STATE_DAS_APA] = 1;

    int iacctoReq = pCanData->vehicle_data.LAS_IACCTakeoverReq;
    if(iacctoReq == 0x1 || iacctoReq == 0x2 || 
        iacctoReq == 0x3 || iacctoReq == 0x4 || iacctoReq == 0x6)
        dasstate[OSD_PIC_STATE_DAS_IACC] = 1;

    if(pCanData->vehicle_data.ACC_TakeOverReq == 0x1)
        dasstate[OSD_PIC_STATE_DAS_ACC] = 1;

    if(pCanData->vehicle_data.ACC_AEBDecCtrlAvail == 0x1)
      dasstate[OSD_PIC_STATE_DAS_AEB] = 1;

    DVR_U8 *OSDbuf  = m_timeoverlay->GetOSDBuf();
    if(OSDbuf == NULL)
        return DVR_RES_EFAIL;
        
    for(int type = OSD_PIC_STATE_DAS_APA;type < OSD_PIC_STATE_DAS;type++)
    {
        if(m_iconID[OSD_PIC_TYPE_DAS] == type)
            continue;

        if(!dasstate[type])
            continue;

        if(type == OSD_PIC_STATE_DAS_APA)
        {
            for(int i = 0;i < 10;i++)
            {
                memset(&pImage[(y + 6 + i) * srcwidth + x + 6], 0xeb, 11);
                memset(&OSDbuf[(y + 6 + i) * srcwidth + x + 6], 0xeb, 11);
                memset(&OSDbuf[(y + 6 + i) * srcwidth + (srcwidth>>1) + x + 6], 0xeb, 11);
            }
        }
        else if(type == OSD_PIC_STATE_DAS_AEB)
        {
            for(int i = 0;i < 10;i++)
            {
                memset(&pImage[(y + 6 + i) * srcwidth + x + 20], 0xeb, 11);
                memset(&OSDbuf[(y + 6 + i) * srcwidth + x + 20], 0xeb, 11);
                memset(&OSDbuf[(y + 6 + i) * srcwidth + (srcwidth>>1) + x + 20], 0xeb, 11);
            }
        }
        else if(type == OSD_PIC_STATE_DAS_IACC)
        {
            for(int i = 0;i < 10;i++)
            {
                memset(&pImage[(y + 20 + i) * srcwidth + x + 6], 0xeb, 11);
                memset(&OSDbuf[(y + 20 + i) * srcwidth + x + 6], 0xeb, 11);
                memset(&OSDbuf[(y + 20 + i) * srcwidth + (srcwidth>>1) + x + 6], 0xeb, 11);
            }
        }
        else if(type == OSD_PIC_STATE_DAS_ACC)
        {
            for(int i = 0;i < 10;i++)
            {
                memset(&pImage[(y + 20 + i) * srcwidth + x + 20], 0xeb, 11);
                memset(&OSDbuf[(y + 20 + i) * srcwidth + x + 20], 0xeb, 11);
                memset(&OSDbuf[(y + 20 + i) * srcwidth + (srcwidth>>1) + x + 20], 0xeb, 11);
            }
        }
    } 
    /*static int flag = 0;
    if(!flag)
    {
        FILE *fp_out=fopen("output.yuv","wb+");
        fwrite(pImage,1,2048*1280*3>>1,fp_out);
        fclose(fp_out);
        flag = 1;
    }*/

    return DVR_RES_SOK;
}

int COSDTitle::ConvertGearType(int pos)
{
    switch(pos)
    {
        case 0://p
        return OSD_PIC_STATE_GEAR_P;
        case 1://r
        return OSD_PIC_STATE_GEAR_R;
        case 2://n
        return OSD_PIC_STATE_GEAR_N;
        case 3://d
        return OSD_PIC_STATE_GEAR_D;
        case 4://m
        return OSD_PIC_STATE_GEAR_M;
        case 5://i
        return OSD_PIC_STATE_GEAR_I;
        default:
        {
            printf("Error type %d\n",pos);
            return OSD_PIC_STATE_GEAR_D;
        }
    }

    return OSD_PIC_STATE_GEAR_I;
}

int COSDTitle::ConvertAccType(int pos)
{
    if(pos == 0 || pos >= 100)
        return OSD_PIC_STATE_ACC_W;

    int state[5] = {OSD_PIC_STATE_ACC_1,OSD_PIC_STATE_ACC_2,OSD_PIC_STATE_ACC_3,OSD_PIC_STATE_ACC_4,OSD_PIC_STATE_ACC_5};
    return state[pos / 20];
}

int COSDTitle::ConvertDasType(int type)
{
    int picstate = OSD_PIC_STATE_DAS_BLACK;

    switch(type)
    {
        case OSD_SRC_REC_TYPE_NONE:
        {
            picstate = OSD_PIC_STATE_DAS_BLACK;
            break;
        }
        case OSD_SRC_REC_TYPE_APA:
        {
            picstate = OSD_PIC_STATE_DAS_APA;
            break;
        }
        case OSD_SRC_REC_TYPE_IACC:
        {
            picstate = OSD_PIC_STATE_DAS_IACC;
            break;
        }
        case OSD_SRC_REC_TYPE_ACC:
        {
            picstate = OSD_PIC_STATE_DAS_ACC;
            break;
        }
        case OSD_SRC_REC_TYPE_AEB:
        {
            picstate = OSD_PIC_STATE_DAS_AEB;
            break;
        }
        default:
        {
            printf("Error type %d\n",type);
            break;
        }
    }

    return picstate;
}

bool COSDTitle::CheckVehicleData(Ofilm_Can_Data_T *pCanData, int type, bool force)
{
	unsigned char GearShiftPositon;
	unsigned char BrakePedalStatus;
	unsigned char DriverBuckleSwitchStatus;
	unsigned char AccePedalPosition;
	unsigned char LeftTurnLampStatus;
	unsigned char RightTurnLampStatus;
	
    if (pCanData == NULL)
        return false;

	GearShiftPositon = pCanData->vehicle_data.vehicle_movement_state;
	BrakePedalStatus = pCanData->vehicle_data.Brake_Pedal_Position;
	DriverBuckleSwitchStatus = pCanData->vehicle_data.DriverBuckleSwitchStatus;
	AccePedalPosition = pCanData->vehicle_data.Accelerator_Actual_Position;		
	if(pCanData->vehicle_data.turn_signal == 1)
	{
		LeftTurnLampStatus = 1;
		RightTurnLampStatus = 0;
	}
	else if(pCanData->vehicle_data.turn_signal == 2)
	{
		LeftTurnLampStatus = 0;
		RightTurnLampStatus = 1;
	}
	else if(pCanData->vehicle_data.EmergencyLightstatus == 1)
	{
		LeftTurnLampStatus = 1;
		RightTurnLampStatus = 1;
	}
	else
	{
		LeftTurnLampStatus = 0;
		RightTurnLampStatus = 0;
	}

    if(GearShiftPositon < 5)
    {
        m_iconID[OSD_PIC_TYPE_GEAR] = ConvertGearType(GearShiftPositon);
    }
#if 0	
    else if(GearShiftPositon < 6)
    {
        //d
        m_iconID[OSD_PIC_TYPE_GEAR] = OSD_PIC_STATE_GEAR_D;//3;
    }
    else if(GearShiftPositon == 7)
    {
        //m
        m_iconID[OSD_PIC_TYPE_GEAR] = OSD_PIC_STATE_GEAR_M;//4;
    }
#endif
    else
    {  
        //invalid
        m_iconID[OSD_PIC_TYPE_GEAR] = OSD_PIC_STATE_GEAR_I;//5;
    }

    //if(vehicle_data.BrakePedalStatus == 0 || vehicle_data.BrakePedalStatus == 3)
    //{
    //    m_iconID[OSD_PIC_TYPE_BRAKE]    = OSD_PIC_STATE_BRAKE_W;
    //}
    //else if(vehicle_data.BrakePedalStatus == 1)
    //{
    //    m_iconID[OSD_PIC_TYPE_BRAKE]    = OSD_PIC_STATE_BRAKE_R;
    //}
    //else
    //{
    //    m_iconID[OSD_PIC_TYPE_BRAKE]    = OSD_PIC_STATE_BRAKE_W;
    //}

    m_iconID[OSD_PIC_TYPE_SPPED]        = OSD_PIC_STATE_SPPED_W;

	if(AccePedalPosition==0)
		m_iconID[OSD_PIC_TYPE_ACC] = OSD_PIC_STATE_ACC_W;
	else if(AccePedalPosition<19)
		m_iconID[OSD_PIC_TYPE_ACC] = OSD_PIC_STATE_ACC_1;
	else if(AccePedalPosition<39)
		m_iconID[OSD_PIC_TYPE_ACC] = OSD_PIC_STATE_ACC_2;
	else if(AccePedalPosition<59)
		m_iconID[OSD_PIC_TYPE_ACC] = OSD_PIC_STATE_ACC_3;
	else if(AccePedalPosition<79)
		m_iconID[OSD_PIC_TYPE_ACC] = OSD_PIC_STATE_ACC_4;
	else
		m_iconID[OSD_PIC_TYPE_ACC] = OSD_PIC_STATE_ACC_5;
	
    m_iconID[OSD_PIC_TYPE_BRAKE]        = (BrakePedalStatus >= 1) ? OSD_PIC_STATE_BRAKE_R: OSD_PIC_STATE_BRAKE_W;//vehicle_data.BrakePedalStatus;
    m_iconID[OSD_PIC_TYPE_TURN_LEFT]    = (LeftTurnLampStatus >= 1) ? OSD_PIC_STATE_TURN_LEFT_GREEN : OSD_PIC_STATE_TURN_LEFT_W;//vehicle_data.LeftTurnLampStatus;
    m_iconID[OSD_PIC_TYPE_TURN_RIGHT]   = (RightTurnLampStatus >= 1) ? OSD_PIC_STATE_TURN_RIGHT_GREEN : OSD_PIC_STATE_TURN_RIGHT_W;//vehicle_data.RightTurnLampStatus;
    m_iconID[OSD_PIC_TYPE_BUCKLE]       = (DriverBuckleSwitchStatus > 0) ? OSD_PIC_STATE_BUCKLE_R : OSD_PIC_STATE_BUCKLE_GREEN; 

#ifdef OSD_IACC
    m_iconID[OSD_PIC_TYPE_DAS]          = ConvertDasType(type);
#endif
    return true;
}

int COSDTitle::PicOverlayFormat(int idx, OSD_SRC_PARAM* srcparam, Ofilm_Can_Data_T *pCanData, bool format)
{
    if(pCanData == NULL || srcparam == NULL || m_timeoverlay == NULL)
        return DVR_RES_EFAIL;

    ushort i,j,k            = 0;
    ushort pos              = 0;
    
    ushort startoff         = 0;
    int    offset           = 0;
    ushort imgwidth         = 0;
    ushort imgheight        = 0;
    int    addroffset       = 0;
    DVR_U16 VehicleSpeed    = 0;
    CAPTURE_TITLE_PARAM param;
    ushort x0,y0,x1,y1 = 0;

#ifdef OSD_IACC
    x0 = OSD_PIC_START_X(90);
    x1 = OSD_PIC_END_X(90);
    if(srcparam->type == OSD_SRC_PIC)
    {
        x0 = OSD_PIC_START_X(330);
        x1 = OSD_PIC_END_X(330);
    }
    GetTitleWidth(OSD_PIC_TYPE_DAS,&imgwidth,&imgheight);
    startoff = x0 + imgwidth;
#else
    x0 = OSD_PIC_START_X(84);
    x1 = OSD_PIC_END_X(84);
    if(srcparam->type == OSD_SRC_PIC)
    {
        x0 = OSD_PIC_START_X(274);
        x1 = OSD_PIC_END_X(274);
    }
    startoff = x0;
#endif
    y0 = OSD_PIC_START_Y;
    y1 = OSD_PIC_END_Y;
    DVR_U8 *pImage  = srcparam->data;
    DVR_U8 *destUV  = pImage + srcparam->imagewidth*srcparam->imageheight;
    DVR_U8 *OSDbuf  = m_timeoverlay->GetOSDBuf();
    memset(&param,0,sizeof(CAPTURE_TITLE_PARAM));
    if(format && idx == 1)
    {
        memcpy(pImage,OSDbuf,srcparam->imagewidth*OSD_HEIGHT);
        goto UV;
    }

    if(!format || idx != 0)
        goto UV;

	if(pCanData->vehicle_data.Vehicle_Speed_Validity == 1)
	{
		if(pCanData->vehicle_data.vehicle_speed > 0.0f)
			VehicleSpeed = (DVR_U16)pCanData->vehicle_data.vehicle_speed;
		else
			VehicleSpeed = 0;
	}
	else
	{
		VehicleSpeed = 0x1FF; //invalid value
	}

    for(i = y0;i < y1;i++)
    {        
        offset = x0;
        GetTitleWidth(OSD_PIC_TYPE_SPPED,&imgwidth,&imgheight);
        for(j = startoff;j < startoff + imgwidth;j++)
        {
            pos = (i - y0)*imgwidth + j - startoff;
            addroffset = GetImageYPos(idx,srcparam->imagewidth,srcparam->imageheight,i,j);
            SpeedOverlay(&pImage[addroffset],VehicleSpeed,j - startoff,i - y0,pos);
        }
        int startpos = GetImageYPos(idx,srcparam->imagewidth,srcparam->imageheight,i,startoff);
        memcpy(OSDbuf + i * srcparam->imagewidth + startoff,&pImage[startpos],imgwidth);
        memcpy(OSDbuf + i * srcparam->imagewidth + (srcparam->imagewidth>>1) + startoff,&pImage[startpos],imgwidth);
        for(k = 0;k < OSD_PIC_NUM;k++)
        {
            GetTitleWidth(k, &imgwidth, &imgheight);
            if(k == OSD_PIC_TYPE_SPPED)
            {
                offset += imgwidth;
                continue;
            }
            param.raster  = GetPicData(k,m_iconID[k]);
            m_timeoverlay->SetImage(pImage,param.raster,srcparam->imagewidth,i,offset,imgwidth,OSD_TITLE_PIC);
        }
    }
#ifdef OSD_IACC
    SetDasOverlay(pCanData, pImage, srcparam->imagewidth, x0, y0);
#endif
UV:
    for(i = (y0>>1); i< (y1>>1); i++)
    {
        offset = 0;
        for(k = 0;k < OSD_PIC_NUM;k++)
        {
            GetTitleWidth(k,&imgwidth,&imgheight);
            param.raster = GetPicData(k,m_iconID[k]);
            DVR_U8* srcUV  = param.raster + imgwidth * OSD_PIC_HEIGHT;
            int off = GetImageUVPos(idx,srcparam->imagewidth,srcparam->imageheight,i) + x0 + offset;
            memcpy(destUV + off, srcUV + (i - (y0>>1))*imgwidth, imgwidth);
            offset += imgwidth;
        }
    }
    return DVR_RES_SOK;
}

int COSDTitle::SetOverlay(Ofilm_Can_Data_T *pCanData, OSD_SRC_PARAM* srcparam)
{
    if(pCanData == NULL || srcparam == NULL)
        return DVR_RES_EFAIL;

    int timeend     = 0;
    int imagenum    = (srcparam->type == OSD_SRC_PIC) ? 1 : 4;
    bool force      = true;//(srcparam->type == OSD_SRC_PIC) ? true : false;
    bool format     = CheckVehicleData(pCanData, srcparam->rectype, force);
    for(int idx = 0;idx < imagenum;idx++)
    {
        if(m_timeoverlay != NULL)
            m_timeoverlay->TimeOverlayFormat(idx,srcparam,pCanData,timeend,format);

        //double start = ms_time();
        PicOverlayFormat(idx,srcparam,pCanData,format);
        //double end = ms_time();
        //if(format && idx == 0)
        //    DPrint(DPRINT_INFO, "PicOverlayFormat(%p type %d) takes %lf ms\n",this,srcparam->type,end-start);
    }
    return DVR_RES_SOK;
}

COSDTitleEx::COSDTitleEx(void)
{
    if(m_timeoverlay != NULL)
    {
       delete m_timeoverlay;
       m_timeoverlay = new CTimeOverlayEx;
    }
}

COSDTitleEx::~COSDTitleEx(void)
{
    if(m_timeoverlay != NULL)
    {
       delete m_timeoverlay;
       m_timeoverlay = NULL;
    }
}

int COSDTitleEx::PicOverlayFormat(int idx, OSD_SRC_PARAM* srcparam, Ofilm_Can_Data_T *pCanData, bool format)
{
    if(pCanData == NULL || srcparam == NULL || m_timeoverlay == NULL)
        return DVR_RES_EFAIL;

    ushort i,j,k            = 0;
    ushort pos              = 0;
    
    ushort startoff         = 0;
    int    offset           = 0;
    ushort imgwidth         = 0;
    ushort imgheight        = 0;
    int    addroffset       = 0;
    DVR_U16 VehicleSpeed    = 0;
    CAPTURE_TITLE_PARAM param;
    ushort x0,y0,x1,y1 = 0;

#ifdef OSD_IACC
    x0 = OSD_PIC_START_X(90);
    x1 = OSD_PIC_END_X(90);
    if(srcparam->type == OSD_SRC_PIC)
    {
        x0 = OSD_PIC_START_X(330);
        x1 = OSD_PIC_END_X(330);
    }
    GetTitleWidth(OSD_PIC_TYPE_DAS,&imgwidth,&imgheight);
    startoff = x0 + imgwidth;
#else
    x0 = OSD_PIC_START_X(84);
    x1 = OSD_PIC_END_X(84);
    if(srcparam->type == OSD_SRC_PIC)
    {
        x0 = OSD_PIC_START_X(274);
        x1 = OSD_PIC_END_X(274);
    }
    startoff = x0;
#endif
    y0 = OSD_PIC_START_Y;
    y1 = OSD_PIC_END_Y;
    DVR_U8 *pImage  = m_timeoverlay->GetOSDBuf();
    DVR_U8 *destUV  = pImage + srcparam->imagewidth*srcparam->imageheight;
    memset(&param,0,sizeof(CAPTURE_TITLE_PARAM));

	if(pCanData->vehicle_data.Vehicle_Speed_Validity == 0)
	{
		if(pCanData->vehicle_data.vehicle_speed > 0.0f)
			VehicleSpeed = pCanData->vehicle_data.vehicle_speed;
		else
			VehicleSpeed = 0;

		if(VehicleSpeed>254) VehicleSpeed=254;
	}
	else
	{
		VehicleSpeed = 0x1FF; //invalid value
	}

    for(i = y0;i < y1;i++)
    {        
        offset = x0;
        GetTitleWidth(OSD_PIC_TYPE_SPPED,&imgwidth,&imgheight);
        for(j = startoff;j < startoff + imgwidth;j++)
        {
            pos = (i - y0)*imgwidth + j - startoff;
            addroffset = GetImageYPos(idx,srcparam->imagewidth,srcparam->imageheight,i,j);
            SpeedOverlay(&pImage[addroffset],VehicleSpeed,j - startoff,i - y0,pos);
        }
        int startpos = GetImageYPos(idx,srcparam->imagewidth,srcparam->imageheight,i,startoff);
       for(k = 0;k < OSD_PIC_NUM;k++)
        {
            GetTitleWidth(k, &imgwidth, &imgheight);
            if(k == OSD_PIC_TYPE_SPPED)
            {
                offset += imgwidth;
                continue;
            }
            param.raster  = GetPicData(k,m_iconID[k]);
            m_timeoverlay->SetImage(param.raster,srcparam->imagewidth,i,offset,imgwidth,OSD_TITLE_PIC);
        }
    }
#ifdef OSD_IACC
    SetDasOverlay(pCanData, pImage, srcparam->imagewidth, x0, y0);
#endif
UV:
    for(i = (y0>>1); i< (y1>>1); i++)
    {
        offset = 0;
        for(k = 0;k < OSD_PIC_NUM;k++)
        {
            GetTitleWidth(k,&imgwidth,&imgheight);
            param.raster = GetPicData(k,m_iconID[k]);
            DVR_U8* srcUV  = param.raster + imgwidth * OSD_PIC_HEIGHT;
            int off = GetImageUVPos(idx,srcparam->imagewidth,srcparam->imageheight,i) + x0 + offset;
            memcpy(destUV + off, srcUV + (i - (y0>>1))*imgwidth, imgwidth);
            //memcpy(destUV + off + (srcparam->imagewidth>>1),srcUV + (i - (y0>>1))*imgwidth, imgwidth);
            offset += imgwidth;
        }
    }
    return DVR_RES_SOK;
}

int COSDTitleEx::SetDasOverlay(Ofilm_Can_Data_T *pCanData, DVR_U8* pImage, int srcwidth, int x, int y)
{
    if(pCanData == NULL || pImage == NULL)
        return DVR_RES_EFAIL;

    int dasstate[OSD_PIC_STATE_DAS] = {0};
    //if(pCanData->vehicle_data.APA_LSCAction == 0x1)
    //    dasstate[OSD_PIC_STATE_DAS_APA] = 1;

    //int iacctoReq = pCanData->vehicle_data.LAS_IACCTakeoverReq;
    //if(iacctoReq == 0x1 || iacctoReq == 0x2 || 
    //    iacctoReq == 0x3 || iacctoReq == 0x4 || iacctoReq == 0x6)
     //   dasstate[OSD_PIC_STATE_DAS_IACC] = 1;

    if(pCanData->vehicle_data.ACC_TakeOverReq == 0x1)
        dasstate[OSD_PIC_STATE_DAS_ACC] = 1;

    if(pCanData->vehicle_data.ACC_AEBDecCtrlAvail == 0x1)
      dasstate[OSD_PIC_STATE_DAS_AEB] = 1;

    for(int type = OSD_PIC_STATE_DAS_APA;type < OSD_PIC_STATE_DAS;type++)
    {
            if(m_iconID[OSD_PIC_TYPE_DAS] == type)
                continue;
    
            if(!dasstate[type])
                continue;
    
            if(type == OSD_PIC_STATE_DAS_APA)
            {
                for(int i = 0;i < 10;i++)
                {
                    memset(&pImage[(y + 6 + i) * srcwidth + x + 6], 0xeb, 11);
                }
            }
            else if(type == OSD_PIC_STATE_DAS_AEB)
            {
                for(int i = 0;i < 10;i++)
                {
                    memset(&pImage[(y + 6 + i) * srcwidth + x + 20], 0xeb, 11);
                }
            }
            else if(type == OSD_PIC_STATE_DAS_IACC)
            {
                for(int i = 0;i < 10;i++)
                {
                    memset(&pImage[(y + 20 + i) * srcwidth + x + 6], 0xeb, 11);
                }
            }
            else if(type == OSD_PIC_STATE_DAS_ACC)
            {
                for(int i = 0;i < 10;i++)
                {
                    memset(&pImage[(y + 20 + i) * srcwidth + x + 20], 0xeb, 11);
                }
            }
        }
    /*static int flag = 0;
    if(!flag)
    {
        FILE *fp_out=fopen("output.yuv","wb+");
        fwrite(pImage,1,2048*1280*3>>1,fp_out);
        fclose(fp_out);
        flag = 1;
    }*/

    return DVR_RES_SOK;
}


int COSDTitleEx::SetOverlay(Ofilm_Can_Data_T *pCanData, OSD_SRC_PARAM* srcparam, void* buffer, int size)
{
    if(pCanData == NULL || srcparam == NULL)
        return DVR_RES_EFAIL;

#if 0	
	static int count=0;

	count++;
	if(count%20==0) 
		DPrint(DPRINT_ERR,"turn_signal:%d EmergencyLightstatus:%d GearShiftPositon:%d BrakePedalStatus:%d DriverBuckleSwitchStatus:%d AccePedalPosition:%d  (%d-%d-%d %d:%d:%d) speed(%d %f) ADAS(%d %d)!!!\n",
			pCanData->vehicle_data.turn_signal,pCanData->vehicle_data.EmergencyLightstatus,pCanData->vehicle_data.vehicle_movement_state,
			pCanData->vehicle_data.Brake_Pedal_Position,pCanData->vehicle_data.DriverBuckleSwitchStatus,pCanData->vehicle_data.Accelerator_Actual_Position,
			pCanData->vehicle_data.year ,pCanData->vehicle_data.month,pCanData->vehicle_data.day,pCanData->vehicle_data.hour,pCanData->vehicle_data.minute, pCanData->vehicle_data.second,
			pCanData->vehicle_data.Vehicle_Speed_Validity,pCanData->vehicle_data.vehicle_speed,
			pCanData->vehicle_data.LAS_IACCTakeoverReq,pCanData->vehicle_data.ACC_AEBDecCtrlAvail
		);
#endif
	
    int timeend     = 0;
    int imagenum    = 1;
    bool force      = true;//(srcparam->type == OSD_SRC_PIC) ? true : false;
    bool format     = CheckVehicleData(pCanData, srcparam->rectype, force);
    for(int idx = 0;idx < imagenum;idx++)
    {
        if(m_timeoverlay != NULL)
        {
            m_timeoverlay->SetOSDBuf(buffer, size);
            m_timeoverlay->TimeOverlayFormat(idx,srcparam,pCanData,timeend,format);
        }

        //double start = ms_time();
        PicOverlayFormat(idx,srcparam,pCanData,format);
        //double end = ms_time();
        //if(format && idx == 0)
        //    DPrint(DPRINT_INFO, "PicOverlayFormat(%p type %d) takes %lf ms\n",this,srcparam->type,end-start);
    }
    return DVR_RES_SOK;
}

