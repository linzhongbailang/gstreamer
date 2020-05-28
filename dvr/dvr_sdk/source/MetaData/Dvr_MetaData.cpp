#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <windows.h>

#include <gst/gst.h>
#include "DVR_METADATA_INTFS.h"
#include "mp4_demux/MP4Demux.h"
#include "EasyExif.h"
#include "osa.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

int Dvr_Mp4Demux_Create(void **ppMP4DemuxHandle, const char *pFileName, bool loadall)
{
    void* pDemux = NULL;
    MP4Demux_OpenOptions opt;
    int ret = DVR_RES_SOK;

    if (ppMP4DemuxHandle == NULL || pFileName == NULL)
        return DVR_RES_EPOINTER;

    *ppMP4DemuxHandle = NULL;

    memset(&opt, 0x0, sizeof(opt));
    opt.pFilename = (char *)pFileName;
    opt.loadall   = loadall;

    MP4Demux_Create(&pDemux);
    if (!pDemux)
        return DVR_RES_EFAIL;

    ret = MP4Demux_Open(pDemux, &opt, sizeof(MP4Demux_OpenOptions));
    if (ret != 0)
        return DVR_RES_EFAIL;

    *ppMP4DemuxHandle = pDemux;

    return DVR_RES_SOK;
}

int Dvr_Mp4Demux_Destroy(void *pMP4DemuxHandle)
{
    MP4Demux_Close(pMP4DemuxHandle);
    MP4Demux_Release(pMP4DemuxHandle);

    return DVR_RES_SOK;
}

struct DVR_META_DATA
{
public:
    DVR_META_DATA()
    {
        m_bIsVideo = FALSE;
        m_pMp4Demux = NULL;
        ulItemNum = 0;
        memset(&m_tMetaDataItem, 0, sizeof(DVR_METADATA_ITEM));
    }
    ~DVR_META_DATA()
    {
        if (m_pMp4Demux != NULL)
        {
            Dvr_Mp4Demux_Destroy(m_pMp4Demux);
            m_pMp4Demux = NULL;
        }
    }

    gboolean m_bIsVideo;
    void *m_pMp4Demux;
    easyexif::EXIFInfo m_ExifInfo;
    DVR_METADATA_ITEM m_tMetaDataItem;
    unsigned long ulItemNum;
};

void* Dvr_MetaData_Create(void* pvFileName, unsigned int *pu32ItemNum, bool loadall)
{
    const char *filename = (const char *)pvFileName;
    int ret;

    if (pvFileName == NULL)
    {
        DPrint(DPRINT_ERR, "Filename is invalid.");
        return NULL;
    }

    if (pu32ItemNum == NULL)
        return NULL;

    *pu32ItemNum = 0;

    DVR_META_DATA* ptNewMetaData = new DVR_META_DATA;
    if (ptNewMetaData == NULL)
    {
        return NULL;
    }

    if (g_str_has_suffix(filename, "MP4") || g_str_has_suffix(filename, "mp4"))
    {
        ptNewMetaData->m_bIsVideo = TRUE;
    }
    else
    {
        ptNewMetaData->m_bIsVideo = FALSE;
    }

    if (ptNewMetaData->m_bIsVideo)
    {
        ret = Dvr_Mp4Demux_Create(&ptNewMetaData->m_pMp4Demux, filename, loadall);
        if (ret != DVR_RES_SOK)
        {
            goto fail;
        }
    }
    else
    {
        // Read the JPEG file into a buffer
        FILE *fp = fopen(filename, "rb");
        if (!fp) {
            DPrint(DPRINT_ERR, "Can't open file.\n");
            return NULL;
        }
        fseek(fp, 0, SEEK_END);
        unsigned long fsize = ftell(fp);
        rewind(fp);
        unsigned char *buf = new unsigned char[fsize];
        if (buf == NULL){
            DPrint(DPRINT_ERR, "out of memory\n");
            fclose(fp);
            return NULL;
        }

        if (fread(buf, 1, fsize, fp) != fsize) {
            DPrint(DPRINT_ERR, "Can't read file.\n");
            delete[] buf;
            fclose(fp);
            return NULL;
        }
        fclose(fp);

        // Parse EXIF
        int code = ptNewMetaData->m_ExifInfo.parseFrom(buf, fsize);
        delete[] buf;
        if (code) {
            DPrint(DPRINT_ERR, "Error parsing EXIF: code %d\n", code);
            return NULL;
        }
    }

    return ptNewMetaData;

fail:
    if (ptNewMetaData->m_bIsVideo && ptNewMetaData->m_pMp4Demux != NULL)
    {
        Dvr_Mp4Demux_Destroy(ptNewMetaData->m_pMp4Demux);
    }
    return NULL;
}

int Dvr_MetaData_Destroy(void *pvMetaDataHandle)
{
    if (pvMetaDataHandle == NULL)
        return -1;

    DVR_META_DATA* ptMetaData = (DVR_META_DATA*)pvMetaDataHandle;
    delete ptMetaData;

    return 0;
}

const DVR_METADATA_ITEM* Dvr_MetaData_GetDataByIndex(void* pvMetaDataHandle, unsigned int u32Index)
{
    return NULL;
}

const DVR_MEDIA_TRACK* Dvr_MetaData_GetTrackByIndex(void* pvMetaDataHandle, unsigned int u32Index)
{
    return NULL;
}

const DVR_METADATA_ITEM* Dvr_MetaData_GetDataByType(void* pvMetaDataHandle, DVR_METADATA_TYPE eType)
{
    if (pvMetaDataHandle == NULL || eType == DVR_METADATA_TYPE_UNKNOWN)
        return NULL;
    DVR_META_DATA* ptMetaData = (DVR_META_DATA*)pvMetaDataHandle;

    if (ptMetaData->m_bIsVideo)
    {
        if (ptMetaData->m_pMp4Demux != NULL)
        {
            switch (eType)
            {
            case DVR_METADATA_TYPE_DESCRIPTION:
            {
                MP4Demux_UserData meta_data;
                MP4Demux_GetUserData(ptMetaData->m_pMp4Demux, &meta_data);

                ptMetaData->m_tMetaDataItem.eCodeType = DVR_METADATA_CODETYPE_UTF8;
                ptMetaData->m_tMetaDataItem.eType = DVR_METADATA_TYPE_DESCRIPTION;
                ptMetaData->m_tMetaDataItem.ps8Data = meta_data.description.szContent;
                ptMetaData->m_tMetaDataItem.u32DataSize = meta_data.description.nSize;
            }
            break;

            case DVR_METADATA_TYPE_PICTURE:
            {
                MP4Demux_UserData meta_data;
                MP4Demux_GetUserData(ptMetaData->m_pMp4Demux, &meta_data);

                ptMetaData->m_tMetaDataItem.eCodeType = DVR_METADATA_CODETYPE_BYTES;
                ptMetaData->m_tMetaDataItem.eType = DVR_METADATA_TYPE_PICTURE;
                ptMetaData->m_tMetaDataItem.ps8Data = meta_data.artwork.szContent;
                ptMetaData->m_tMetaDataItem.u32DataSize = meta_data.artwork.nSize;
            }
            break;

            default:
                return NULL;
            }
        }
    }
    else
    {
        switch (eType)
        {
        case DVR_METADATA_TYPE_DESCRIPTION:
        {
            ptMetaData->m_tMetaDataItem.eCodeType = DVR_METADATA_CODETYPE_UTF8;
            ptMetaData->m_tMetaDataItem.eType = DVR_METADATA_TYPE_DESCRIPTION;
            ptMetaData->m_tMetaDataItem.ps8Data = (char *)ptMetaData->m_ExifInfo.ImageDescription.c_str();
            ptMetaData->m_tMetaDataItem.u32DataSize = ptMetaData->m_ExifInfo.ImageDescription.size();
        }
        break;

        case DVR_METADATA_TYPE_APPLICATION:
        {
            ptMetaData->m_tMetaDataItem.eCodeType = DVR_METADATA_CODETYPE_UTF8;
            ptMetaData->m_tMetaDataItem.eType = DVR_METADATA_TYPE_APPLICATION;
            ptMetaData->m_tMetaDataItem.ps8Data = (char *)ptMetaData->m_ExifInfo.Software.c_str();
            ptMetaData->m_tMetaDataItem.u32DataSize = ptMetaData->m_ExifInfo.Software.size();
        }
        break;

        case DVR_METADATA_TYPE_ARTIST:
        {
            ptMetaData->m_tMetaDataItem.eCodeType = DVR_METADATA_CODETYPE_UTF8;
            ptMetaData->m_tMetaDataItem.eType = DVR_METADATA_TYPE_ARTIST;
            ptMetaData->m_tMetaDataItem.ps8Data = (char *)ptMetaData->m_ExifInfo.Artist.c_str();
            ptMetaData->m_tMetaDataItem.u32DataSize = ptMetaData->m_ExifInfo.Artist.size();
        }
        break;

        case DVR_METADATA_TYPE_COMMENT:
        {
            ptMetaData->m_tMetaDataItem.eCodeType = DVR_METADATA_CODETYPE_UTF8;
            ptMetaData->m_tMetaDataItem.eType = DVR_METADATA_TYPE_COMMENT;
            ptMetaData->m_tMetaDataItem.ps8Data = (char *)ptMetaData->m_ExifInfo.Comment.c_str();
            ptMetaData->m_tMetaDataItem.u32DataSize = ptMetaData->m_ExifInfo.Comment.size();
        }
        break;

        default:
            return NULL;
        }
    }

    return (const DVR_METADATA_ITEM*)&ptMetaData->m_tMetaDataItem;
}

int Dvr_MetaData_GetItemNum(void *pvMetaDataHandle)
{
    if (pvMetaDataHandle == NULL)
        return 0;
    DVR_META_DATA* ptMetaData = (DVR_META_DATA*)pvMetaDataHandle;
    return (int)ptMetaData->ulItemNum;
}

int Dvr_MetaData_GetMediaInfo(void* pvMetaDataHandle, DVR_MEDIA_INFO* pInfo)
{
    if (NULL == pvMetaDataHandle || NULL == pInfo)
    {
        return DVR_RES_EPOINTER;
    }

    DVR_META_DATA* ptMetaData = (DVR_META_DATA*)pvMetaDataHandle;
    if (ptMetaData->m_bIsVideo == TRUE)
    {
        if (ptMetaData->m_pMp4Demux != NULL)
        {
            MP4Demux_StreamInfo	info;
            MP4Demux_GetStreamInfo(ptMetaData->m_pMp4Demux, &info);

            pInfo->u32AudioTrackCount = 0;
            pInfo->u32VideoTrackCount = 1;
            pInfo->u32Duration = 1000 * info.dwVideoDuration / info.dwVideoTimeScale;
            pInfo->u32Seekable = TRUE;
            pInfo->u32BitRate = 0;
        }
    }
    else
    {
        //TODO
    }


    return DVR_RES_SOK;
}



