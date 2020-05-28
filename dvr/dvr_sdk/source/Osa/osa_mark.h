#ifndef _OSA_SDCARD_MARK_H_
#define _OSA_SDCARD_MARK_H_
#include <osa.h>

#define	BIT(x)				(unsigned int)(1<<(x))
//#define		FS_DEBUG
#define		FS_SECTOR_BITS				9
#define		FS_INDEX_BITS				4



#define		SECTORBITPOS				0x0B
#define		RESERVESECTORNUMPOS			0x0E
#define		SECTORTOTALNUMPOS			0x20
#define		FDTSECTORNUMPOS				0x24		
#define		ROOTDIRCLUSTERPOS			0x2C
#define		FATINFOSECTORPOS			0x30
#define		BACKUPDBRSECTORPOS			0x32
#define		CLUSTERSECTORNUMPOS			0x0d
#define		FILE_CONTENT_LEN			BIT(FS_INDEX_BITS)	//文件最大长度

typedef struct _DBRINFO
{
	DVR_U16	sectorbit;			//每个扇区字节数			offset = 0x0b
	DVR_U16	reservesectornum;	//保留扇区数				offset = 0x0e
	DVR_U32	sectortotalnum; 	//总扇区数				offset = 0x20
	DVR_U32	fdtsectornum;		//fat表占用扇区数			offset = 0x24
	DVR_U32	rootdircluster; 	//根目录入口簇号			offset = 0x2c
	DVR_U16	fatinfosector;		//文件系统信息扇区号		offset = 0x30
	DVR_U16	backupdbrsector;	//备份扇区起始扇区号		offset = 0x32	
	DVR_U8	clustersectornum;	//每簇扇区数				offset = 0x0d
}DBRINFO, *PDBRINFO;

namespace DVR
{
	DVR_U32		OSA_MarkSDSpeed(char* devicename, char* mountpoint, DVR_SDSPEED_TYPE type);
	DVR_U32		OSA_GetSDSpeekType(char* devicename, char* mountpoint,  DVR_U32 *type);
	DVR_U32		OSA_DelMarkSDSpeekOK(char* devicename, char* mountpoint);
}

#endif /* _OSA_SDCARD_MARK_H_ */

