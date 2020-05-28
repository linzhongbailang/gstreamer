#include "dprint.h"
#include "DvrMutex.h"
#include "osa_fs.h"
#include "FileBase.h"
#include "osa_notifier.h"
#include "FileDBManger.h"
#include "DVR_SDK_DEF.h"
#include "DVR_SDK_INTFS.h"
#include "DvrMutexLocker.h"

IFileDBManger::IFileDBManger(OsaNotifier *notify)
{
    m_state     = false;
	m_mutex     = new DvrMutex();
    m_filedb    = new CFileDB();
    m_filetbl   = NULL;
    m_pNotifier = (OsaNotifier*)notify;
    
    memset(m_pMountpoint,     0, APP_MAX_FN_SIZE);
    memset(&m_ptfnCallBack,   0, sizeof(m_ptfnCallBack));
    memset(&m_NormalFileList, 0, sizeof(FILEMAP_LIST_PRIVATE));
    memset(&m_EventFileList,  0, sizeof(FILEMAP_LIST_PRIVATE));
    memset(&m_PhotoFileList,  0, sizeof(FILEMAP_LIST_PRIVATE));
    memset(&m_DasFileList,    0, sizeof(FILEMAP_LIST_PRIVATE));
    memset(&m_ErrorFileList,  0, sizeof(FILEMAP_LIST_PRIVATE));

    m_NormalFileList.eFolderType    = DVR_FOLDER_TYPE_NORMAL;
    m_EventFileList.eFolderType     = DVR_FOLDER_TYPE_EMERGENCY;
    m_PhotoFileList.eFolderType     = DVR_FOLDER_TYPE_PHOTO;
    m_DasFileList.eFolderType       = DVR_FOLDER_TYPE_DAS;

    int status = pthread_create(&m_pthread, NULL, DelThreadPro, this);
    if (status != 0)
        DPrint(DPRINT_ERR,"pthread_create() - Could not create thread [%d]\n", status);
	m_ptfnCallBack.CallBack  = DBCallBack;
	m_ptfnCallBack.pInstance = (void *)this;
    m_filedb->RegisterCallBack(&m_ptfnCallBack);

    pthread_mutex_init(&m_lock, NULL);
}

IFileDBManger::~IFileDBManger(void)
{
    Clear();
    pthread_mutex_destroy(&m_lock);
    pthread_join(m_pthread, NULL);
    FSDel(m_mutex);
    FSDel(m_filedb);
    FSDel(m_filetbl);
}

PFILEMAP_LIST_PRIVATE IFileDBManger::GetFileList(unsigned eFolderType)
{
    if (eFolderType == DVR_FOLDER_TYPE_NORMAL)
        return &m_NormalFileList;
    else if (eFolderType == DVR_FOLDER_TYPE_EMERGENCY)
        return &m_EventFileList;
    else if (eFolderType == DVR_FOLDER_TYPE_PHOTO)
        return &m_PhotoFileList;
    else if (eFolderType == DVR_FOLDER_TYPE_DAS)
        return &m_DasFileList;

    return &m_ErrorFileList;
}

DVR_RESULT IFileDBManger::GetFolderName(char *foldername, unsigned eFolderType)
{
    if(foldername == NULL)
        return DVR_RES_EFAIL;

    if (eFolderType == DVR_FOLDER_TYPE_NORMAL)
        return Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_NORMAL_FOLDER, foldername, APP_MAX_FN_SIZE, NULL);
    else if (eFolderType == DVR_FOLDER_TYPE_EMERGENCY)
        return Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_EMERGENCY_FOLDER, foldername, APP_MAX_FN_SIZE, NULL);
    else if (eFolderType == DVR_FOLDER_TYPE_PHOTO)
        return Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_PHOTO_FOLDER, foldername, APP_MAX_FN_SIZE, NULL);
    else if (eFolderType == DVR_FOLDER_TYPE_DAS)
        return Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_DAS_FOLDER, foldername, APP_MAX_FN_SIZE, NULL);

    return DVR_RES_EFAIL;
}

DVR_RESULT IFileDBManger::AddItem(DVR_FILEMAP_META_ITEM *pszItem, unsigned eFolderType)
{
    DvrMutexLocker mutexLocker(m_mutex);
    if (pszItem == NULL || m_filedb == NULL)
        return DVR_RES_EFAIL;
    
    return m_filedb->AddItem(pszItem->szMediaFileName, GetFileList(eFolderType));
}

DVR_RESULT IFileDBManger::DelItem(const char *szFileName, unsigned eFolderType)
{
    DvrMutexLocker mutexLocker(m_mutex);
    if (szFileName == NULL || m_filedb == NULL)
        return DVR_RES_EFAIL;
   
    return m_filedb->DelItem(szFileName, GetFileList(eFolderType));
}

DVR_RESULT IFileDBManger::Clear(void)
{
    DvrMutexLocker mutexLocker(m_mutex);
    if (m_filedb == NULL)
        return DVR_RES_EFAIL;
    
    m_filedb->Clear(&m_NormalFileList);
    m_filedb->Clear(&m_EventFileList);
    m_filedb->Clear(&m_PhotoFileList);
    m_filedb->Clear(&m_DasFileList);
    m_filedb->Clear(&m_ErrorFileList);
    m_state                         = false;
    m_NormalFileList.eFolderType    = DVR_FOLDER_TYPE_NORMAL;
    m_EventFileList.eFolderType     = DVR_FOLDER_TYPE_EMERGENCY;
    m_PhotoFileList.eFolderType     = DVR_FOLDER_TYPE_PHOTO;
    m_DasFileList.eFolderType       = DVR_FOLDER_TYPE_DAS;
    return DVR_RES_SOK;
}

DVR_RESULT IFileDBManger::Mount(const char *szDirectory, const char *szDevice)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (szDirectory == NULL || m_filedb == NULL)
        return DVR_RES_EPOINTER;

    char *szDrive = (char *)szDirectory;
    //if(!strcmp(m_pMountpoint,szDrive))
    //    return DVR_RES_EPOINTER;

	DPrint(DPRINT_INFO, "IFileDBManger::Mount() %s szDevice %s\n",szDirectory,szDevice);
    FSDel(m_filetbl);
    m_filetbl = new CFileBase(szDrive, szDevice);
    m_filetbl->Open(szDrive);
    m_filedb->SetFileTbale(m_filetbl);
    strcpy(m_pMountpoint,szDrive);
	//DVR_DB_SCAN_STATE state = DVR_SCAN_STATE_COMPLETE;
	//OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_SCANSTATE, &state, sizeof(state),
	//	szDrive, (long)(strlen(szDrive) + 1), m_pNotifier);
    return DVR_RES_SOK;
}

DVR_RESULT IFileDBManger::UnMount(const char *szDirectory)
{
	DvrMutexLocker mutexLocker(m_mutex);
    char *szDrive = (char *)szDirectory;
    //if(!strcmp(m_pMountpoint,szDrive))
    //    return DVR_RES_EPOINTER;

	DPrint(DPRINT_INFO, "IFileDBManger::UnMount() %s\n", szDrive);
    FSDel(m_filetbl);
    return DVR_RES_SOK;
}

DVR_RESULT IFileDBManger::GetCountByType(DVR_U32& puCount, unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_mutex);

    puCount = 0;
    PFILEMAP_LIST_PRIVATE filelist = GetFileList(eFolderType);
    if(filelist)
        puCount = g_list_length(filelist->pList);

    return DVR_RES_SOK;
}

DVR_U64 IFileDBManger::GetUsedSpace(unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_mutex);
    PFILEMAP_LIST_PRIVATE filelist = GetFileList(eFolderType);

    if(filelist)
        return filelist->ullTotalUsedSpace;

    return 0;
}

DVR_RESULT IFileDBManger::GetNextName(const char *szFileName, DVR_FILEMAP_META_ITEM *pszItem, unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (szFileName == NULL || m_filedb == NULL)
        return DVR_RES_EPOINTER;

    memset(pszItem,0,sizeof(DVR_FILEMAP_META_ITEM));
    return m_filedb->GetNextFile(szFileName, pszItem->szMediaFileName, GetFileList(eFolderType));
}

DVR_RESULT IFileDBManger::GetPrevName(const char *szFileName, DVR_FILEMAP_META_ITEM *pszItem, unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (szFileName == NULL || m_filedb == NULL)
        return DVR_RES_EPOINTER;
   
   memset(pszItem,0,sizeof(DVR_FILEMAP_META_ITEM));
    return m_filedb->GetPrevFile(szFileName, pszItem->szMediaFileName, GetFileList(eFolderType));
}

DVR_RESULT IFileDBManger::GetPosByName(unsigned *puRelPos, const char *szFileName, unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (szFileName == NULL || m_filedb == NULL)
        return DVR_RES_EPOINTER;

    *puRelPos = 0;
    return m_filedb->GetPosByName(szFileName, puRelPos, GetFileList(eFolderType));
}

DVR_RESULT IFileDBManger::GetNameByPos(unsigned uRelPos, unsigned eFolderType, DVR_FILEMAP_META_ITEM *pszItem)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (pszItem == NULL || m_filedb == NULL)
        return DVR_RES_EPOINTER;

    memset(pszItem, 0, sizeof(DVR_FILEMAP_META_ITEM));
    return m_filedb->GetNameByPos(uRelPos, pszItem->szMediaFileName, GetFileList(eFolderType));
}

DVR_RESULT IFileDBManger::GetTypeByName(const char *pszDrive, char *szFileName, unsigned& eFolderType)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (szFileName == NULL || m_filedb == NULL)
        return DVR_RES_EPOINTER;

    eFolderType = 0;
    DVR_RESULT res = DVR_RES_EFAIL;
	for(int eType = DVR_FOLDER_TYPE_NORMAL; eType < DVR_FOLDER_TYPE_NUM; eType++)
    {
        res = m_filedb->GetTypeByName(szFileName, eFolderType, GetFileList(eType));
        if(res != DVR_RES_SOK)
            continue;
        break;
    }
    return res;
}

DVR_RESULT IFileDBManger::GetFileInfo(const char *szFileName, unsigned eFolderType, PDVR_DB_IDXFILE idxfile)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (szFileName == NULL || idxfile == NULL || m_filedb == NULL)
        return DVR_RES_EPOINTER;

    memset(idxfile, 0 , sizeof(DVR_DB_IDXFILE));
    return m_filedb->GetInfo(szFileName, idxfile, GetFileList(eFolderType));
}

DVR_RESULT IFileDBManger::ScanLibrary(const char *szDirectory)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (szDirectory == NULL || m_filedb == NULL)
        return DVR_RES_EPOINTER;

	for(int eFolderType = DVR_FOLDER_TYPE_NORMAL; eFolderType < DVR_FOLDER_TYPE_NUM; eFolderType++)
    {
        char foldername[APP_MAX_FN_SIZE] = { 0 };
        PFILEMAP_LIST_PRIVATE filelist = GetFileList(eFolderType);
        if(filelist == NULL || GetFolderName(foldername, eFolderType) != DVR_RES_SOK)
            continue;

        m_filedb->SearchFile(foldername, (DVR_FOLDER_TYPE)eFolderType, filelist);
    }
    DVR_U32 validnum = g_list_length(m_NormalFileList.pList) + g_list_length(m_EventFileList.pList) + g_list_length(m_DasFileList.pList);
    DBCreateTable(validnum);
    DPrint(DPRINT_ERR, "%s: IFileDBManger::ScanLibrary item count: [Normal: %d, %lldKB][Event: %d, %lldKB][Photo: %d, %lldKB][APA: %d, %lldKB][Failed: %d, %lldKB]\n",
        __func__, g_list_length(m_NormalFileList.pList), m_NormalFileList.ullTotalUsedSpace, 
        g_list_length(m_EventFileList.pList), m_EventFileList.ullTotalUsedSpace, 
        g_list_length(m_PhotoFileList.pList), m_PhotoFileList.ullTotalUsedSpace, 
        g_list_length(m_DasFileList.pList), m_DasFileList.ullTotalUsedSpace,
        g_list_length(m_ErrorFileList.pList),m_ErrorFileList.ullTotalUsedSpace);

    m_state = true;
    return DVR_RES_SOK;
}

DVR_RESULT IFileDBManger::SetFaildedItem(const char *szFileName, unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (szFileName == NULL || m_filedb == NULL)
        return DVR_RES_EPOINTER;

    pthread_mutex_lock(&m_lock);
    DVR_RESULT res = m_filedb->SetFaildItem(szFileName, eFolderType, &m_ErrorFileList);
    pthread_mutex_unlock(&m_lock);
    return res;
}

DVR_RESULT IFileDBManger::FileLock(const char *szFileName, unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (szFileName == NULL || m_filedb == NULL || m_filetbl == NULL)
        return DVR_RES_EPOINTER;

    DVR_DB_TABLE table;
    memset(&table, 0, sizeof(DVR_DB_TABLE));
    table.idf   = m_filedb->GetFileIndex(szFileName);
    table.type  = eFolderType;
    return m_filetbl->TableLock(&table);
}

DVR_RESULT IFileDBManger::FileUnLock(const char *szFileName, unsigned eFolderType)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (szFileName == NULL || m_filedb == NULL || m_filetbl == NULL)
        return DVR_RES_EPOINTER;

    DVR_DB_TABLE table;
    memset(&table, 0, sizeof(DVR_DB_TABLE));
    table.idf   = m_filedb->GetFileIndex(szFileName);
    table.type  = eFolderType;
    return m_filetbl->TableUnLock(&table);
}

DVR_RESULT IFileDBManger::GetFileAccess(const char *szFileName, unsigned eFolderType, DVR_U16& access)
{
	DvrMutexLocker mutexLocker(m_mutex);
    if (szFileName == NULL || m_filedb == NULL || m_filetbl == NULL || eFolderType == DVR_FOLDER_TYPE_PHOTO)
        return DVR_RES_EPOINTER;

    access = 0;
    DVR_DB_TABLE table;
    memset(&table, 0, sizeof(DVR_DB_TABLE));
    table.idf   = m_filedb->GetFileIndex(szFileName);
    table.type  = eFolderType;
    int ret = m_filetbl->GettblAccess(&table, access);
    //Added if not found in table
    if(ret == DVR_RES_EINVALIDARG)
    {
        BIT_SET(table.state, FS_STATE_FMT);
        BIT_SET(table.state, FS_STATE_READ);
        BIT_SET(table.state, FS_STATE_WRITE);
        ret = m_filetbl->TableAdd(&table, true);
        access = 0;
    }
    return ret;
}

int IFileDBManger::DBCallBack(void *pvParam1, void *pvParam2, void *pvParam3, void *pvContext)
{
	IFileDBManger *p        = (IFileDBManger *)pvContext;
    PDVR_DB_IDXFILE idxfile = (PDVR_DB_IDXFILE)pvParam1;
    int res                 = *(int *)pvParam2;

    if(p == NULL || idxfile == NULL || p->m_filedb == NULL)
        return DVR_RES_EPOINTER;

    if(res == DVR_RES_SOK)
        return res;

    pthread_mutex_lock(&p->m_lock);
    DPrint(DPRINT_ERR,"DBCallBack::failed szFileName === %s\n",idxfile->filename);
    res = p->m_filedb->SetFaildItem(idxfile->filename, idxfile->efdtype, &p->m_ErrorFileList);
    pthread_mutex_unlock(&p->m_lock);
    return res;
}

//Compatible with old systems
//The validnum is not zero and the table is newly created to indicate that
//the old system record card was inserted
DVR_RESULT  IFileDBManger::DBCreateTable(DVR_U32 validnum)
{
    if (m_filedb == NULL || m_filetbl == NULL || validnum == 0)
        return DVR_RES_EPOINTER;

    //1.The table has been created to check for a match
    if(m_filetbl->GetValidtblnum() == validnum)
        return DVR_RES_SOK;

    //2.check the table is newly created
    if(!m_filetbl->TableIsRebuild())
        return DVR_RES_SOK;

    //table mismatch re-creation to avoid data loss caused by power failure during creation
    if(!m_filetbl->TableIsRebuild() && m_filetbl->GetValidtblnum() != validnum)
    {
        m_filetbl->Close();
        m_filetbl->Format();
        m_filetbl->Open(m_pMountpoint);
    }
    pthread_t 	thread;
    DVR_RESULT status = pthread_create(&thread, NULL, TblThreadPro, this);
    if (status != 0)
        DPrint(DPRINT_ERR,"pthread_create() - Could not create thread [%d]\n", status);
    pthread_detach(thread);
    return status;
}

void* IFileDBManger::TblThreadPro(void* pParam)
{
    IFileDBManger *pThis = (IFileDBManger *)pParam;
    if(pThis == NULL || pThis->m_filedb == NULL || pThis->m_filetbl == NULL)
        return NULL;

    //Old system files need to create tables
    DPrint(DPRINT_ERR,"TblThreadPro-------------------start\n");
	for(int eFolderType = DVR_FOLDER_TYPE_NORMAL; eFolderType < DVR_FOLDER_TYPE_NUM; eFolderType++)
    {
        if(eFolderType == DVR_FOLDER_TYPE_PHOTO)
            continue;

        char foldername[APP_MAX_FN_SIZE] = { 0 };
        PFILEMAP_LIST_PRIVATE filelist = pThis->GetFileList(eFolderType);
        if(filelist == NULL || pThis->GetFolderName(foldername, eFolderType) != DVR_RES_SOK)
            continue;

        pThis->m_mutex->Lock();
        GList* find = g_list_first(filelist->pList);
        while(find)
        {
            PDVR_DB_IDXFILE idxfile = (PDVR_DB_IDXFILE)find->data;
            if(idxfile == NULL)
            {
                find = g_list_next(find);
                continue;
            }
            DVR_DB_TABLE table;
            memset(&table, 0, sizeof(DVR_DB_TABLE));
            table.type  = filelist->eFolderType;
            table.idf   = pThis->m_filedb->GetFileIndex(idxfile->filename);
            BIT_SET(table.state, FS_STATE_FMT);
            BIT_SET(table.state, FS_STATE_READ);
            BIT_SET(table.state, FS_STATE_WRITE);
            pThis->m_filetbl->TableAdd(&table, false);
            find = g_list_next(find);
        }
        pThis->m_mutex->UnLock();
        usleep(20*1000);
    }
    pThis->m_filetbl->Sync();
    DPrint(DPRINT_ERR,"TblThreadPro-------------------over\n");
    //pThis->m_filetbl->Dump();
    return NULL;
}

void* IFileDBManger::DelThreadPro(void* pParam)
{
    IFileDBManger *pThis = (IFileDBManger *)pParam;
    if(pThis == NULL || pThis->m_filedb == NULL)
        return NULL;

    for (;;)
    {
        while(g_list_length(pThis->m_ErrorFileList.pList) == 0 || !pThis->m_state)
        {
            usleep(50*1000);
        }

        pthread_mutex_lock(&pThis->m_lock);
		GList* find = g_list_first(pThis->m_ErrorFileList.pList);
        if(find == NULL)
        {
            pthread_mutex_unlock(&pThis->m_lock);
            usleep(50*1000);
            continue;
        }

		PDVR_DB_IDXFILE idxfile = (PDVR_DB_IDXFILE)find->data;
		if(idxfile == NULL)
        {
            pthread_mutex_unlock(&pThis->m_lock);
            usleep(50*1000);
            continue;
        }
        char filename[APP_MAX_FN_SIZE];
        int eFolderType = idxfile->efdtype;
        strncpy(filename, idxfile->filename, APP_MAX_FN_SIZE);
        pThis->m_ErrorFileList.pList = g_list_delete_link(pThis->m_ErrorFileList.pList, find);
        g_free(idxfile);
        pthread_mutex_unlock(&pThis->m_lock);

        pThis->m_filedb->DelItem(filename, pThis->GetFileList(eFolderType));
        DVR::OSA_FileDel(filename);
    }
    return NULL;
}

