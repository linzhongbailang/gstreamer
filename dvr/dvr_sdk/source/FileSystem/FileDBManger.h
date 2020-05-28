#ifndef __FILE_DBMANGER_H__
#define __FILE_DBMANGER_H__
#include <pthread.h>
#include "FileDB.h"

class DvrMutex;
class CFileBase;
class IFileDBManger : public CObject
{
public:
    IFileDBManger(OsaNotifier *notify);
    ~IFileDBManger(void);

public:
	DVR_RESULT AddItem(DVR_FILEMAP_META_ITEM *pszItem, unsigned eFolderType);
	DVR_RESULT DelItem(const char *szFileName, unsigned eFolderType);
	DVR_RESULT SetFaildedItem(const char *szFileName, unsigned eFolderType);
	DVR_U64	   GetUsedSpace(unsigned eFolderType);
	DVR_RESULT GetCountByType(DVR_U32& puCount, unsigned eFolderType);
	DVR_RESULT GetFileInfo(const char *szFileName, unsigned eFolderType, PDVR_DB_IDXFILE idxfile);
	DVR_RESULT GetPosByName(unsigned *puRelPos, const char *szFileName, unsigned eFolderType);
	DVR_RESULT GetTypeByName(const char *pszDrive, char *szFileName, unsigned& eFolderType);
	DVR_RESULT GetNameByPos(unsigned uRelPos, unsigned eFolderType, DVR_FILEMAP_META_ITEM *pszItem);
	DVR_RESULT GetNextName(const char *szFileName, DVR_FILEMAP_META_ITEM *pszItem, unsigned eFolderType);
	DVR_RESULT GetPrevName(const char *szFileName, DVR_FILEMAP_META_ITEM *pszItem, unsigned eFolderType);
	DVR_RESULT ScanLibrary(const char *szDirectory);
	DVR_RESULT Mount(const char *szDirectory, const char *szDevice);
	DVR_RESULT UnMount(const char *szDirectory);
	DVR_RESULT Clear(void);
	
public:
	DVR_RESULT FileLock(const char *szFileName, unsigned eFolderType);
	DVR_RESULT FileUnLock(const char *szFileName, unsigned eFolderType);
	DVR_RESULT GetFileAccess(const char *szFileName, unsigned eFolderType, DVR_U16& access);
	
protected:
    static void*	DelThreadPro(void* pParam);
    static void*	TblThreadPro(void* pParam);
	DVR_RESULT 		DBCreateTable(DVR_U32 validnum);
	static int 		DBCallBack(void *pvParam1, void *pvParam2, void *pvParam3, void *pvContext);
	DVR_RESULT  	GetFolderName(char *foldername, unsigned eFolderType);
	PFILEMAP_LIST_PRIVATE GetFileList(unsigned eFolderType);

private:
	bool		m_state;
	DvrMutex	*m_mutex;
	CFileDB		*m_filedb;
	CFileBase	*m_filetbl;
	OsaNotifier *m_pNotifier;
    pthread_t 	m_pthread;
	char 		m_pMountpoint[APP_MAX_FN_SIZE];
    pthread_mutex_t 		m_lock;
	FILEDB_CALLBACK  		m_ptfnCallBack;
    FILEMAP_LIST_PRIVATE  	m_NormalFileList;
    FILEMAP_LIST_PRIVATE  	m_EventFileList;
    FILEMAP_LIST_PRIVATE  	m_PhotoFileList;
    FILEMAP_LIST_PRIVATE  	m_DasFileList;
    FILEMAP_LIST_PRIVATE  	m_ErrorFileList;
};
#endif//__FILE_DBMANGER_H__
