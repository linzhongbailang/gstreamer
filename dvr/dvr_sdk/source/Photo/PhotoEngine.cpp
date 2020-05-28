#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "DVR_SDK_DEF.h"
#include "DVR_SDK_INTFS.h"
#include "PhotoEngine.h"
#include "BMP.h"
#include "OsdTitle.h"

#if _MSC_VER
#define snprintf _snprintf
#pragma warning(disable:4996)	// 4706: assignment within conditional expression
#endif

#define IMAGE_ORIG_WIDTH			1280
#define IMAGE_ORIG_HEIGHT			720

gdouble ms_time(void);

static PHOTO_OUT_MEMORY *GetMemory(int buf_size, unsigned int num_bufs)
{
	PHOTO_OUT_MEMORY *mem;

	mem = (PHOTO_OUT_MEMORY *)malloc(sizeof(PHOTO_OUT_MEMORY));
	mem->size = buf_size;
	mem->data = malloc(buf_size);

	return mem;
}

static void PutMemory(PHOTO_OUT_MEMORY *mem)
{
	if (mem)
	{
		if (mem->data)
			free(mem->data);
		free(mem);
	}
}

CPhotoEngine::CPhotoEngine(void *pParentEngine, CThumbNail *pTNHandle)
{
	m_pParentEngine = pParentEngine;

    memset(m_MediaFileName, 0, sizeof(m_MediaFileName));

    m_pImgBuf = (char *)malloc(IMAGE_ORIG_WIDTH * IMAGE_ORIG_HEIGHT * 3 / 2);
    if (m_pImgBuf != NULL)
    {
        memset(m_pImgBuf, 0, IMAGE_ORIG_WIDTH * IMAGE_ORIG_HEIGHT * 3 / 2);
    }

	m_pPhotoDir = NULL;
	m_pFileDescriptor = NULL;
    m_pExifData = NULL;
	photo_folder_max_file_index = 0;

	m_pEncoder = new Encoder_libjpeg(pTNHandle);
	if (m_pEncoder)
		m_pEncoder->SetCallback(GetMemory);
	m_pOut = NULL;

	memset(&m_MainJpeg, 0, sizeof(m_MainJpeg));
	memset(&m_TNJpeg, 0, sizeof(m_TNJpeg));

	m_pMainOutBuf = NULL;
	m_pTNOutBuf = NULL;
}

CPhotoEngine::~CPhotoEngine()
{
	m_pParentEngine = NULL;
	m_pFnDB_AddFile = NULL;

	if (m_pPhotoDir)
	{
		g_free(m_pPhotoDir);
		m_pPhotoDir = NULL;
	}

	if (m_pMainOutBuf)
	{
		free(m_pMainOutBuf);
		m_pMainOutBuf = NULL;
	}

	if (m_pTNOutBuf)
	{
		free(m_pTNOutBuf);
		m_pTNOutBuf = NULL;
	}

	if (m_pOut)
	{
		free(m_pOut);
		m_pOut = NULL;
	}

    if (m_pImgBuf != NULL)
    {
        free(m_pImgBuf);
        m_pImgBuf = NULL;
    }

	delete m_pEncoder;
}

gchar *CPhotoEngine::GetMainOutBuf()
{
	if (m_pMainOutBuf == NULL)
	{
		m_pMainOutBuf = (gchar *)malloc(m_pEncoder->OutBufSize());
	}

	return m_pMainOutBuf;
}

gchar *CPhotoEngine::GetTNOutBuf()
{
	if (m_pTNOutBuf == NULL)
	{
		m_pTNOutBuf = (gchar *)malloc(DVR_THUMBNAIL_WIDTH * DVR_THUMBNAIL_HEIGHT * 3);
		if (m_pTNOutBuf)
			memset(m_pTNOutBuf, 0, DVR_THUMBNAIL_WIDTH * DVR_THUMBNAIL_HEIGHT * 3);
	}

	return m_pTNOutBuf;
}

int CPhotoEngine::SetOsdTitle(DVR_U8* pImgBuf, PHOTO_INPUT_PARAM *pParam)
{
    if(pImgBuf == NULL || pParam == NULL || m_pParentEngine == NULL)
        return DVR_RES_EFAIL;

    OSD_SRC_PARAM srcparam;
    srcparam.type           = OSD_SRC_PIC;
    srcparam.imagewidth     = IMAGE_ORIG_WIDTH;
    srcparam.imageheight    = IMAGE_ORIG_HEIGHT;
    srcparam.data           = pImgBuf;
    srcparam.rectype        = OSD_SRC_REC_TYPE_NONE;
    Dvr_Sdk_Recorder_Get(DVR_RECORDER_PROP_DAS_RECTYPE, &srcparam.rectype, sizeof(int), NULL);

    Ofilm_Can_Data_T *pCanData = (Ofilm_Can_Data_T *)malloc(sizeof(Ofilm_Can_Data_T));
    memset(pCanData,0,sizeof(Ofilm_Can_Data_T));
    pCanData->vehicle_data.year = pParam->stVehInfo.TimeYear;
    pCanData->vehicle_data.month = pParam->stVehInfo.TimeMon;
    pCanData->vehicle_data.day = pParam->stVehInfo.TimeDay;
    pCanData->vehicle_data.hour = pParam->stVehInfo.TimeHour;
    pCanData->vehicle_data.minute = pParam->stVehInfo.TimeMin;
    pCanData->vehicle_data.second = pParam->stVehInfo.TimeSec;
    pCanData->vehicle_data.vehicle_speed = pParam->stVehInfo.VehicleSpeed;
	pCanData->vehicle_data.Vehicle_Speed_Validity = pParam->stVehInfo.VehicleSpeedValidity;
    pCanData->vehicle_data.vehicle_movement_state = pParam->stVehInfo.GearShiftPositon;
    pCanData->vehicle_data.Brake_Pedal_Position = pParam->stVehInfo.BrakePedalStatus;
    pCanData->vehicle_data.DriverBuckleSwitchStatus = pParam->stVehInfo.DriverBuckleSwitchStatus;
    pCanData->vehicle_data.Accelerator_Actual_Position = pParam->stVehInfo.AccePedalPosition;
    pCanData->vehicle_data.turn_signal = pParam->stVehInfo.TurnSignal;
    pCanData->vehicle_data.positioning_system_longitude = pParam->stVehInfo.GpsLongitude;
    pCanData->vehicle_data.positioning_system_latitude = pParam->stVehInfo.GpsLatitude;
    pCanData->vehicle_data.Lateral_Acceleration = pParam->stVehInfo.LateralAcceleration;
    pCanData->vehicle_data.Longitudinal_Acceleration = pParam->stVehInfo.LongitAcceleration;
	pCanData->vehicle_data.EmergencyLightstatus = pParam->stVehInfo.EmergencyLightstatus;
    pCanData->vehicle_data.APA_LSCAction = pParam->stVehInfo.APALSCAction;
    pCanData->vehicle_data.LAS_IACCTakeoverReq = pParam->stVehInfo.IACCTakeoverReq;
    pCanData->vehicle_data.ACC_TakeOverReq = pParam->stVehInfo.ACCTakeOverReq;
    pCanData->vehicle_data.ACC_AEBDecCtrlAvail = pParam->stVehInfo.AEBDecCtrlAvail;

    COSDTitle *pOsdTitie = new COSDTitle();
    pOsdTitie->SetOverlay(pCanData, &srcparam);

    free(pCanData);
    pCanData = NULL;
    delete pOsdTitie;
    pOsdTitie = NULL;
    return DVR_RES_SOK;
}

static guint get_photo_max_file_index(const char *dirname)
{
	GDir *dir = NULL;
	guint64 max_file_index = 0;
	guint64 file_index = 0;

	dir = g_dir_open(dirname, 0, NULL);
	if (dir)
	{
		const gchar *dir_ent;

		while ((dir_ent = g_dir_read_name(dir)))
		{
			if (!g_str_has_suffix(dir_ent, ".JPG"))
				continue;

			gchar *nptr = g_strrstr(dir_ent, "_");
			gchar *endptr;

			file_index = g_ascii_strtoull(nptr + 1, &endptr, 10);

			if (file_index > max_file_index)
			{
				max_file_index = file_index;
			}
		}

		g_dir_close(dir);
	}

	return (guint)max_file_index;
}


int CPhotoEngine::Encode(void)
{
	if (m_pEncoder)
	{
		m_pOut = m_pEncoder->Encode(&m_MainJpeg, &m_TNJpeg, m_pExifData);
	}

	return 0;
}

int CPhotoEngine::Set(PHOTO_PROP_ID ePropId, void *pPropData, int nPropSize)
{
	int ret = 0;

	switch (ePropId)
	{
	case PHOTO_PROP_DIR:
	{
		if (pPropData == NULL)
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		if(m_pPhotoDir != NULL)
		{
			g_free(m_pPhotoDir);
			m_pPhotoDir = NULL;
		}

		m_pPhotoDir = g_strdup((const char*)pPropData);
		photo_folder_max_file_index = get_photo_max_file_index(m_pPhotoDir) + 1;
	}
	break;

	case PHOTO_PROP_QUALITY:
	{
		if (pPropData == NULL || nPropSize != sizeof(int))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		int quality = *(int *)pPropData;

		if (m_pEncoder)
			m_pEncoder->SetQuality(quality);
	}
	break;

	case PHOTO_PROP_FORMAT:
	{
		if (pPropData == NULL || nPropSize != sizeof(PHOTO_FORMAT_SETTING))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		PHOTO_FORMAT_SETTING *pSet = (PHOTO_FORMAT_SETTING *)pPropData;

		if (m_pEncoder)
			m_pEncoder->SetFormat(pSet);
	}
	break;

	case PHOTO_PROP_CAMERA_EXIF:
	{
		if (pPropData == NULL || nPropSize != sizeof(CAMERA_EXIF))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		m_bHasExifData = TRUE;
		memcpy(&m_ExifData, pPropData, sizeof(CAMERA_EXIF));
	}
	break;

	default:
		break;
	}

	return ret;
}

int CPhotoEngine::Get(PHOTO_PROP_ID ePropId, void *pPropData, int nPropSize)
{
	int ret = 0;

	switch (ePropId)
	{
	case PHOTO_PROP_DIR:
		if (pPropData == NULL || nPropSize < (strlen(m_pPhotoDir)+1))
		{
			GST_ERROR("[Engine] (%d) Invalid Parameters!\n", __LINE__);
			return DVR_RES_EINVALIDARG;
		}

		memset(pPropData, 0, nPropSize);
		strcpy((char *)pPropData, m_pPhotoDir);

		break;

	case PHOTO_PROP_QUALITY:
		break;

	default:
		break;
	}

	return ret;
}

static gchar *create_time_format(const gchar * format)
{
#ifdef __linux__
	gint year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;	
	OSA_GetSystemTime(&year, &month, &day, &hour, &minute, &second);
	GDateTime *date = g_date_time_new_local(year, month, day, hour, minute, second);
	if(date == NULL)
		date = g_date_time_new_now_local();	
#else
	GDateTime *date = g_date_time_new_now_local();
#endif
	
	if (date == NULL)
		return NULL;
	
	gchar *sub_dst = g_date_time_format(date, format);
	g_date_time_unref(date);

	return sub_dst;
}

int CPhotoEngine::CreatePhotoFile(gchar *time, int direction, guint file_index, DVR_PHOTO_TYPE type)
{
	gchar *filename = NULL;
	const gchar *dir = NULL;
	const gchar *suffix = NULL;

	switch (direction)
	{
	case 0:
		dir = "F";//Front
		break;
	case 1:
		dir = "R";//Rear
		break;
	case 2:
		dir = "D";//Driver
		break;
	case 3:
		dir = "P";//Passenger
		break;
	default:
		break;
	}

	switch (type)
	{
	case DVR_PHOTO_TYPE_RAW:
		suffix = ".YUV";
		break;

	case DVR_PHOTO_TYPE_BMP:
		suffix = ".BMP";
		break;

	case DVR_PHOTO_TYPE_JPG:
		suffix = ".JPG";
		break;

	default:
		break;
	}

	gchar *sub_dst = NULL;
	gchar index[10] = { 0, };
	g_snprintf(index, sizeof(index), "%05d", file_index);
	sub_dst = g_strconcat("PHO_", time, "_", dir, "_", index, suffix, NULL);
	filename = g_strjoin(NULL, m_pPhotoDir, sub_dst, NULL);

	

    memset(m_MediaFileName, 0, sizeof(m_MediaFileName));
    strcpy(m_MediaFileName, filename);
	g_free(filename);

    g_snprintf(index, sizeof(index), "%05d", file_index);
    sub_dst = g_strconcat("THM_", time, "_", dir, "_", index, ".BMP", NULL);
    filename = g_strjoin(NULL, m_pPhotoDir, sub_dst, NULL);
    memset(m_TNFileName, 0, sizeof(m_TNFileName));
    strcpy(m_TNFileName, filename);
    g_free(filename);

	return DVR_RES_SOK;
}

int CPhotoEngine::ClosePhotoFile()
{
	m_pFileDescriptor = fopen(m_MediaFileName, "wb");
	if (m_pFileDescriptor == NULL)
	{
		GST_ERROR("open file %s failed", m_MediaFileName);
		return DVR_RES_EFAIL;
	}
	
	if (m_pFileDescriptor)
	{
		if (m_pOut)
		{
			fwrite(m_pOut->data, 1, m_pOut->size, m_pFileDescriptor);
			fclose(m_pFileDescriptor);
			PutMemory(m_pOut);
			m_pOut = NULL;
		}

		m_pFileDescriptor = NULL;
	}

    /*BMPIMAGE bmpimg;
    bmpimg.channels = 3;
    bmpimg.data = (unsigned char *)m_TNJpeg.dst; 
    bmpimg.width = m_TNJpeg.out_width;
    bmpimg.height = m_TNJpeg.out_height;
    WriteBMP(m_TNFileName, &bmpimg, BIT24);*/

	if (m_pFnDB_AddFile != NULL)
	{
        m_pFnDB_AddFile(m_MediaFileName, m_TNFileName, m_pParentEngine);
	}

	return DVR_RES_SOK;
}

void CPhotoEngine::InitParam(int index, PHOTO_INPUT_PARAM *pParam, DVR_PHOTO_TYPE type)
{
	switch (type)
	{
	case DVR_PHOTO_TYPE_RAW:
	{
		m_MainJpeg.src = m_pImgBuf;
		m_MainJpeg.in_width = pParam->width;
		m_MainJpeg.in_height = pParam->height;
		m_MainJpeg.src_size = pParam->width * pParam->height * 3 / 2;
		m_MainJpeg.dst = m_pImgBuf;
		m_MainJpeg.out_width = pParam->width;
		m_MainJpeg.out_height = pParam->height;
		m_MainJpeg.dst_size = m_MainJpeg.src_size;
		m_MainJpeg.out_format = DVR_PHOTO_TYPE_RAW;
		m_MainJpeg.jpeg_size = m_MainJpeg.src_size;
		m_MainJpeg.valid = FALSE;
	}
	break;

	case DVR_PHOTO_TYPE_BMP:
		break;

	case DVR_PHOTO_TYPE_JPG:
	{
		int ret = 0;

		m_MainJpeg.src = m_pImgBuf;
		m_MainJpeg.in_width = pParam->width;
		m_MainJpeg.in_height = pParam->height;
		m_MainJpeg.src_size = m_pEncoder->InBufSize();
		m_MainJpeg.dst = GetMainOutBuf();
		m_MainJpeg.out_width = pParam->width;
		m_MainJpeg.out_height = pParam->height;
		m_MainJpeg.dst_size = m_pEncoder->OutBufSize();
		m_MainJpeg.out_format = DVR_PHOTO_TYPE_JPG;
		m_MainJpeg.jpeg_size = 0;
		m_MainJpeg.valid = TRUE;

        m_TNJpeg.src = m_pImgBuf;
        m_TNJpeg.src_size = m_pEncoder->InBufSize();
        m_TNJpeg.dst = GetTNOutBuf();
        m_TNJpeg.dst_size = DVR_THUMBNAIL_WIDTH * DVR_THUMBNAIL_HEIGHT * 3;
        m_TNJpeg.in_width = pParam->width;
        m_TNJpeg.in_height = pParam->height;
        m_TNJpeg.out_width = DVR_THUMBNAIL_WIDTH;
        m_TNJpeg.out_height = DVR_THUMBNAIL_HEIGHT;
        m_TNJpeg.valid = TRUE;
        m_TNJpeg.jpeg_size = 0;
        m_TNJpeg.out_format = DVR_PHOTO_TYPE_RAW;

		if (m_pExifData != NULL)
		{
			if (0 == ret)
			{
                GDateTime *date = g_date_time_new_local(pParam->stVehInfo.TimeYear + 2013,
                    pParam->stVehInfo.TimeMon + 1, 
                    pParam->stVehInfo.TimeDay + 1, 
                    pParam->stVehInfo.TimeHour, 
                    pParam->stVehInfo.TimeMin, 
                    pParam->stVehInfo.TimeSec);
				if(date != NULL)
				{
					gchar *sub_dst = g_date_time_format(date, "%Y-%m-%d-%H-%M-%S");
					ret = m_pExifData->insertElement("DateTime", sub_dst);
					g_free(sub_dst);
				}
			}
						
			if (0 == ret)
			{
				char temp_value[5];
				snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", photo_folder_max_file_index);
				ret = m_pExifData->insertElement("Artist", temp_value);
			}

			if (0 == ret)
			{
				char temp_value[5];
				snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", pParam->width);
				ret = m_pExifData->insertElement(TAG_IMAGE_WIDTH, temp_value);
			}

			if (0 == ret)
			{
				char temp_value[5];
				snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", pParam->height);
				ret = m_pExifData->insertElement(TAG_IMAGE_LENGTH, temp_value);
			}

            if (0 == ret)
            {
                char temp_value[5];
                snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", pParam->stVehInfo.VehicleSpeed);
                ret = m_pExifData->insertElement("CustomRendered", temp_value);
            }

            if (0 == ret)
            {
                char temp_value[5];
                snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", pParam->stVehInfo.GearShiftPositon);
                ret = m_pExifData->insertElement("ExposureMode", temp_value);
            }

            if (0 == ret)
            {
                char temp_value[5];
                snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", pParam->stVehInfo.BrakePedalStatus);
                ret = m_pExifData->insertElement("WhiteBalance", temp_value);
            }

            if (0 == ret)
            {
                char temp_value[5];
                snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", pParam->stVehInfo.DriverBuckleSwitchStatus);
                ret = m_pExifData->insertElement("Contrast", temp_value);
            }

            if (0 == ret)
            {
                char temp_value[5];
                snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", pParam->stVehInfo.AccePedalPosition);
                ret = m_pExifData->insertElement("Saturation", temp_value);
            }

            if (0 == ret)
            {
                char temp_value[5];
                snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", pParam->stVehInfo.TurnSignal);
                ret = m_pExifData->insertElement("Sharpness", temp_value);
            }

            if (0 == ret)
            {
                char temp_value[64];
                snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", pParam->stVehInfo.GpsLongitude);
                ret = m_pExifData->insertElement("Make", temp_value);
            }
            if (0 == ret)
            {
                char temp_value[64];
                snprintf(temp_value, sizeof(temp_value) / sizeof(char), "%u", pParam->stVehInfo.GpsLatitude);
                ret = m_pExifData->insertElement("Model", temp_value);
            }
		}	
	}
	break;

	default:
		break;
	}
}

int CPhotoEngine::Photo_Process(DVR_U32 param1, DVR_U32 param2, void *pContext)
{
	PHOTO_INPUT_PARAM *pParam = (PHOTO_INPUT_PARAM *)param1;
	CPhotoEngine *pThis = (CPhotoEngine *)pContext;
	DVR_PHOTO_TYPE type = (DVR_PHOTO_TYPE)pParam->eType;
    int eIndex = pParam->eIndex;
	DVR_RESULT res = DVR_RES_SOK;

	void *exif_data = NULL;
	ExifElementsTable * exifTable = NULL;

	const gchar *format = "%Y%m%d_%H%M%S";
	gchar *pTime = create_time_format(format);
    if (pThis->m_pImgBuf)
    {
        memcpy(pThis->m_pImgBuf, pParam->pImageBuf[eIndex], pThis->m_pEncoder->InBufSize());
        if(pParam->width == IMAGE_ORIG_WIDTH)
        {
            pThis->SetOsdTitle((DVR_U8*)pThis->m_pImgBuf,pParam);
        }
    }

	gdouble start = ms_time();

    pThis->m_pExifData = new ExifElementsTable;

	res = pThis->CreatePhotoFile(pTime, eIndex, pThis->photo_folder_max_file_index, type);
	if (res == 0)
	{
		pThis->InitParam(eIndex, pParam, type);
		pThis->Encode();
		if (pThis->m_MainJpeg.jpeg_size <= 0)
		{
			res = DVR_RES_EFAIL;
		}
	}
	pThis->ClosePhotoFile();

    delete pThis->m_pExifData;
    pThis->m_pExifData = NULL;
	
	gdouble end = ms_time();
	DPrint(DPRINT_INFO, "1 picture encoding takes %f ms\n", end-start);

	if(res == DVR_RES_SOK)
		pThis->photo_folder_max_file_index += 1;

	if (pTime)
	{
		g_free(pTime);
		pTime = NULL;
	}
	
	return res;
}

int CPhotoEngine::Photo_Return(void *pContext)
{
	DPrint(DPRINT_INFO, "take photo done\n");
	return DVR_RES_SOK;
}
