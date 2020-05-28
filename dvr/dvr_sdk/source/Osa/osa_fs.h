#ifndef _OSA_FS_H_
#define _OSA_FS_H_

#include <osa.h>

/*
typedef struct  
{

}FS_STAT;
*/

typedef enum
{
	OSA_FS_FMT_INVALID = -1,
	OSA_FS_FMT_FAT16 = 0,
	OSA_FS_FMT_FAT32 = 1,
	OSA_FS_FMT_EXTFAT = 2
}FS_FMT;

namespace DVR
{
	DVR_U32 OSA_FS_GetFreeSpace(const char *szDrive);
	DVR_U32 OSA_FS_GetTotalSpace(const char *szDrive);
	DVR_U32 OSA_FS_GetUsedSpace(const char *szDrive);
	DVR_RESULT OSA_MkDir(const char *dirName);
	DVR_RESULT OSA_FileCopy(char *srcFn, char *dstFn);
	DVR_RESULT OSA_FileMove(char *srcFn, char *dstFn);
	DVR_RESULT OSA_FileDel(char *filename);
	DVR_RESULT OSA_CardFormat(char *szDevicePath, char *szMountPoint);
    DVR_U32 OSA_FS_CheckUsedSpace(const char *szFilePath, DVR_BOOL *pCardPlugOut);
	DVR_RESULT OSA_GetFileStatInfo(const char* fileName, DVR_U64 *pullFileSize, DVR_U64 *pullModifyTime);
}

#endif