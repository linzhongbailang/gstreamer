#ifndef __FILE_DB_H__
#define __FILE_DB_H__
#include <glib.h>
#include "Object.h"

typedef enum _DB_CHECK_TYPE 
{
	DB_CHECK_TYPE_FOR_ADD,
    DB_CHECK_TYPE_FOR_SEARCH,
    DB_CHECK_TYPE_NUM
}DB_CHECK_TYPE;

typedef struct
{
    GList *pList;
    unsigned eFolderType;
    DVR_U64 ullTotalUsedSpace;
}FILEMAP_LIST_PRIVATE,*PFILEMAP_LIST_PRIVATE;

typedef struct
{
	int  (*CallBack)(void *pvParam1, void *pvParam2, void *pvParam3, void *pvContext);
	void *pInstance;
}FILEDB_CALLBACK,*PFILEDB_CALLBACK;

class CFileBase;
class CFileDB : public CObject
{
public:
    CFileDB(void);
    ~CFileDB(void);
public:
	DVR_RESULT Clear(PFILEMAP_LIST_PRIVATE pList);
	DVR_RESULT SearchFile(char *foldername, DVR_FOLDER_TYPE type, PFILEMAP_LIST_PRIVATE pList);
	DVR_RESULT AddItem(const char *szFileName, PFILEMAP_LIST_PRIVATE pList);
	DVR_RESULT DelItem(const char *szFileName, PFILEMAP_LIST_PRIVATE pList);
	DVR_RESULT GetInfo(const char *szFileName, PDVR_DB_IDXFILE idxfile, PFILEMAP_LIST_PRIVATE filelist);
	DVR_RESULT GetPosByName(const char *szFileName, unsigned  *puPos, PFILEMAP_LIST_PRIVATE pList);
	DVR_RESULT GetNameByPos(unsigned uRelPos, char *desFileName, PFILEMAP_LIST_PRIVATE filelist);
	DVR_RESULT GetTypeByName(const char *szFileName, unsigned& eFolderType, PFILEMAP_LIST_PRIVATE filelist);
	DVR_RESULT GetNextFile(const char *szFileName, char *desFileName, PFILEMAP_LIST_PRIVATE filelist);
	DVR_RESULT GetPrevFile(const char *szFileName, char *desFileName, PFILEMAP_LIST_PRIVATE filelist);
	DVR_RESULT SetFaildItem(const char *szFileName, unsigned eFolderType, PFILEMAP_LIST_PRIVATE filelist);
	DVR_RESULT SetFileTbale(void* pctx);
	DVR_U32	   GetFileIndex(const char *fileName);
	DVR_RESULT RegisterCallBack(void* pcallback);

protected:
	DVR_RESULT OnCheckState(PDVR_DB_IDXFILE idxfile);
	DVR_RESULT OnCheckMeta(PDVR_DB_IDXFILE idxfile);
	DVR_RESULT OnCheckItem(PDVR_DB_IDXFILE idxfile, DB_CHECK_TYPE type);
	DVR_RESULT OnCheckName(char *foldername, char *szFileName, DVR_FOLDER_TYPE type);
	DVR_RESULT OnAddItem(const char *szFileName, DB_CHECK_TYPE type, PFILEMAP_LIST_PRIVATE filelist);

private:
	PFILEDB_CALLBACK	m_pcallback;
	CFileBase*			m_filetbl;
};
#endif//__FILE_DB_H__
