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
#define		FILE_CONTENT_LEN			BIT(FS_INDEX_BITS)	//�ļ���󳤶�

typedef struct _DBRINFO
{
	DVR_U16	sectorbit;			//ÿ�������ֽ���			offset = 0x0b
	DVR_U16	reservesectornum;	//����������				offset = 0x0e
	DVR_U32	sectortotalnum; 	//��������				offset = 0x20
	DVR_U32	fdtsectornum;		//fat��ռ��������			offset = 0x24
	DVR_U32	rootdircluster; 	//��Ŀ¼��ڴغ�			offset = 0x2c
	DVR_U16	fatinfosector;		//�ļ�ϵͳ��Ϣ������		offset = 0x30
	DVR_U16	backupdbrsector;	//����������ʼ������		offset = 0x32	
	DVR_U8	clustersectornum;	//ÿ��������				offset = 0x0d
}DBRINFO, *PDBRINFO;

namespace DVR
{
	DVR_U32		OSA_MarkSDSpeed(char* devicename, char* mountpoint, DVR_SDSPEED_TYPE type);
	DVR_U32		OSA_GetSDSpeekType(char* devicename, char* mountpoint,  DVR_U32 *type);
	DVR_U32		OSA_DelMarkSDSpeekOK(char* devicename, char* mountpoint);
}

#endif /* _OSA_SDCARD_MARK_H_ */

