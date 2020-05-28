#ifndef __FILE_BASE_H__
#define __FILE_BASE_H__
#include <glib.h>
#include "DVR_SDK_DEF.h"
#include "Object.h"

//#define		FS_DEBUG
#define		FS_DB_INDEX					"index"
#define		FS_DB_INDEX_NUM				1000
#define		FS_SECTOR_BITS				9
#define		FS_INDEX_BITS				4

//state
#define		FS_STATE_FMT				0
#define 	FS_STATE_READ				1		
#define		FS_STATE_WRITE				2
#define		FS_STATE_ERROR				3

//access
#define		FS_ACCESS_READ				0
#define		FS_ACCESS_WRITE				1
#define 	FS_ACCESS_LOCK        		2


#define		SECTORBITPOS				0x0B
#define		RESERVESECTORNUMPOS			0x0E
#define		SECTORTOTALNUMPOS			0x20
#define		FDTSECTORNUMPOS				0x24		
#define		ROOTDIRCLUSTERPOS			0x2C
#define		FATINFOSECTORPOS			0x30
#define		BACKUPDBRSECTORPOS			0x32
#define		CLUSTERSECTORNUMPOS			0x0d
#define		FILE_CONTENT_LEN			BIT(FS_INDEX_BITS)	//文件最大长度

typedef struct _tagDVR_DB_TABLE
{
	DVR_U16		   state;
	DVR_U16		   access;
	DVR_U16		   idx;
	DVR_U16		   type;
	DVR_U32		   idf;
	DVR_U32		   rsvd0[125];
}DVR_DB_TABLE,*PDVR_DB_TABLE;

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

class CFileBase: public CObject
{
public:
 	CFileBase(const char *szDirectory, const char* devname);
	virtual ~CFileBase(void);

public:
	int 	Open(const char *szDirectory);
	int		Close(void);
	int		Sync(void);
	int		Format(void);
	int		TableSetState(PDVR_DB_TABLE table);
	int		TableDel(PDVR_DB_TABLE table);
	int		TableAdd(PDVR_DB_TABLE table, bool sync = true);
	int		TableLock(PDVR_DB_TABLE table);
	int		TableUnLock(PDVR_DB_TABLE table);
	bool 	TableIsRebuild(void){return m_rebuild;};
	int 	GettblState(PDVR_DB_TABLE table, DVR_U16& state);
	int		GettblAccess(PDVR_DB_TABLE table, DVR_U16& access);
	DVR_U32 GetValidtblnum(void){return g_list_length(m_worklist);};
	int 	Dump(void);
	static	int Dump(PDVR_DB_TABLE index);

protected:
	int		OnGetTable(int idx, PDVR_DB_TABLE table);
	int		OnGetTableId(PDVR_DB_TABLE table);
	int		OnTableUpdate(int idx, PDVR_DB_TABLE table, bool sync = true);
	int		OnTableDel(int idx);
	int		OnGetFreeId(int& idx);
	int		OnUsedRemoveAt(int idx);
	int		OnGetWorkId(int& idx, PDVR_DB_TABLE table);
	int		OnCheckTable(void);
	int		OnTableRead (void* buffer, DVR_U64 sectorpos, uint sectornum);
	int		OnTableWrite(void* buffer, DVR_U64 sectorpos, uint sectornum);

protected:
	void	Init(void);
	int		Load(char* buffer);
	int		SetHideAtrr(void);
	int 	ReadSector(int fd, void* buffer, DVR_U32 startsector, DVR_U32 sectornum);
	int 	WriteSector(int fd, void* buffer, DVR_U32 startsector, DVR_U32 sectornum);
	DVR_U32	LittleToBig(char* data, DVR_U32 pos, DVR_U32 len);
	int     HideTblFile(int fd, PDBRINFO dbrinfo);
	int 	GetDbrData(int fd, PDBRINFO DBRinfo);

private:
	char 		m_pMountpoint[APP_MAX_FN_SIZE];
	char		m_devname[APP_MAX_FN_SIZE];
	char*		m_buffer;
	DVR_U32		m_totaltblnum;
	DVR_U32		m_size;
	int			m_fd;
	bool		m_rebuild;//yes or not newly created
	GList*		m_idlelist;
	GList*		m_worklist;
};
#endif//__FILE_BASE_H__

