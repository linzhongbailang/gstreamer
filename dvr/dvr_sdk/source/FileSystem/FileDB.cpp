#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "osa_fs.h"
#include "FileDB.h"
#include "FileBase.h"
#include "DvrMutex.h"
#include "DVR_SDK_INTFS.h"
#include "DvrMutexLocker.h"
#include "RecorderUtils.h"
#include "DVR_SDK_DEF.h"
#include "DVR_METADATA_INTFS.h"

static gint TimeCompare(gconstpointer a, gconstpointer b)
{
	PDVR_DB_IDXFILE pItemA = (PDVR_DB_IDXFILE)a;
	PDVR_DB_IDXFILE pItemB = (PDVR_DB_IDXFILE)b;

    if(pItemA == NULL || pItemB == NULL)
        return 0;

    if(pItemA->mktime == pItemB->mktime)
        return 1;
    
    else if(pItemA->mktime > pItemB->mktime)
        return 1;
    
    else if(pItemA->mktime < pItemB->mktime)
        return -1;

    return 0;
}

static gint NameCompare(gconstpointer a, gconstpointer b)
{
	PDVR_DB_IDXFILE pItemA = (PDVR_DB_IDXFILE)a;
	PDVR_DB_IDXFILE pItemB = (PDVR_DB_IDXFILE)b;

    if(pItemA == NULL || pItemB == NULL)
        return 0;

    if (strcmp(pItemA->filename, pItemB->filename) == 0)
        return 0;

    return 1;
}

CFileDB::CFileDB(void)
{
    m_pcallback = NULL;
    m_filetbl   = NULL;
}

CFileDB::~CFileDB(void)
{
}

DVR_RESULT CFileDB::RegisterCallBack(void* pcallback)
{
    m_pcallback = (PFILEDB_CALLBACK)pcallback;
    return DVR_RES_SOK;
}

DVR_RESULT CFileDB::AddItem(const char *szFileName, PFILEMAP_LIST_PRIVATE filelist)
{
    DVR_RESULT ret = DVR_RES_EFAIL;
    if (szFileName == NULL || filelist == NULL || m_filetbl == NULL)
        return DVR_RES_EFAIL;

    ret = OnAddItem(szFileName, DB_CHECK_TYPE_FOR_ADD, filelist);
    if(ret != DVR_RES_SOK)
        return ret;

    if(filelist->eFolderType == DVR_FOLDER_TYPE_PHOTO)
        return DVR_RES_SOK;

    DVR_DB_TABLE table;
    memset(&table, 0, sizeof(DVR_DB_TABLE));
    table.type   = filelist->eFolderType;
    table.idf    = GetFileIndex(szFileName);
    BIT_SET(table.state, FS_STATE_FMT);
    BIT_SET(table.state, FS_STATE_READ);
    BIT_SET(table.state, FS_STATE_WRITE);
    m_filetbl->TableAdd(&table, true);
    return ret;
}

DVR_RESULT CFileDB::DelItem(const char *szFileName, PFILEMAP_LIST_PRIVATE filelist)
{
    DVR_DB_IDXFILE file;
    DVR_RESULT res = DVR_RES_EFAIL;
    if (szFileName == NULL || filelist == NULL || m_filetbl == NULL)
        return DVR_RES_EFAIL;

    memset(&file, 0, sizeof(DVR_DB_IDXFILE));
    strcpy(file.filename, szFileName);
    GList *find = g_list_find_custom(filelist->pList, &file, (GCompareFunc)NameCompare);
    if (find == NULL || find->data == NULL)
        return DVR_RES_EFAIL;

    PDVR_DB_IDXFILE idxfile = (PDVR_DB_IDXFILE)find->data;
    DPrint(DPRINT_ERR, "%s: before g_list_delete_link, list len:%d, totalsize:%lldKB!!!", __func__, g_list_length(filelist->pList), filelist->ullTotalUsedSpace);
    filelist->pList = g_list_delete_link(filelist->pList, find);
    filelist->ullTotalUsedSpace -= idxfile->length;
    g_free(idxfile);
    DPrint(DPRINT_ERR, "%s: after g_list_delete_link, list len:%d, totalsize:%lldKB!!!", __func__, g_list_length(filelist->pList), filelist->ullTotalUsedSpace);
    DVR_DB_TABLE table;
    memset(&table, 0, sizeof(DVR_DB_TABLE));
    table.type = filelist->eFolderType;
    table.idf  = GetFileIndex(szFileName);
    m_filetbl->TableDel(&table);
    return DVR_RES_SOK;
}

DVR_RESULT CFileDB::Clear(PFILEMAP_LIST_PRIVATE filelist)
{
	if(filelist == NULL)
        return DVR_RES_EFAIL;
    
    g_list_foreach(filelist->pList, (GFunc)free, NULL);
    g_list_free(filelist->pList);
	memset(filelist, 0, sizeof(FILEMAP_LIST_PRIVATE));
    return DVR_RES_SOK;
}

DVR_RESULT CFileDB::SetFileTbale(void* pctx)
{
    m_filetbl = (CFileBase*)pctx;
}

DVR_RESULT CFileDB::SetFaildItem(const char *szFileName, unsigned eFolderType, PFILEMAP_LIST_PRIVATE filelist)
{
    if(szFileName == NULL || filelist == NULL)
        return DVR_RES_EFAIL;

    struct stat result;
    stat(szFileName, &result);
    PDVR_DB_IDXFILE idxfile = (PDVR_DB_IDXFILE)g_malloc(sizeof(DVR_DB_IDXFILE));
    strcpy(idxfile->filename, szFileName);
    idxfile->length  = result.st_size >> 10;
    idxfile->efdtype = (DVR_FOLDER_TYPE)eFolderType;
    filelist->pList  = g_list_append(filelist->pList, idxfile);
    filelist->ullTotalUsedSpace += idxfile->length;
    return DVR_RES_SOK;
}

DVR_RESULT CFileDB::OnCheckState(PDVR_DB_IDXFILE idxfile)
{
    if(idxfile == NULL || m_filetbl == NULL)
        return DVR_RES_EFAIL;

    if(m_filetbl->TableIsRebuild())
        return DVR_RES_EFAIL;

    DVR_U16  state      = 0;
    DVR_U16  tblmask    = BIT(FS_STATE_FMT)|BIT(FS_STATE_READ)|BIT(FS_STATE_WRITE);

    DVR_DB_TABLE table;
    table.type   = idxfile->efdtype;
    table.idf    = GetFileIndex(idxfile->filename);
    DVR_RESULT ret = m_filetbl->GettblState(&table, state);
    if(ret != DVR_RES_SOK)
        return ret;
    
    if(state != tblmask)
        return DVR_RES_EFAIL;

    return DVR_RES_SOK;
}
DVR_RESULT CFileDB::OnCheckMeta(PDVR_DB_IDXFILE idxfile)
{
    if(idxfile == NULL)
        return DVR_RES_EFAIL;

    //Check that the file name is legitimate
    //......

    void *pMeta         = NULL;
    DVR_RESULT res      = DVR_RES_EFAIL;
    DVR_U32 nNumOfItem  = 0;
    DVR_MEDIA_INFO stMediaInfo;

    memset(&stMediaInfo, 0, sizeof(DVR_MEDIA_INFO));
    pMeta   = Dvr_MetaData_Create((void*)idxfile->filename, &nNumOfItem, true);
    res     = Dvr_MetaData_GetMediaInfo(pMeta, &stMediaInfo);

    const DVR_METADATA_ITEM *pItem = Dvr_MetaData_GetDataByType(pMeta, DVR_METADATA_TYPE_PICTURE);
    if (pItem != NULL)
    {
        int pngStartFlagLength = 8;
        int pngEndFlagLength = 12;
        unsigned int thumbnail_length = pItem->u32DataSize - pngStartFlagLength - pngEndFlagLength;
        if (thumbnail_length > DVR_THUMBNAIL_WIDTH*DVR_THUMBNAIL_HEIGHT*3)
        {
            DPrint(DPRINT_ERR,"thumbnail length too large[%d > %d]\n", thumbnail_length,DVR_THUMBNAIL_WIDTH*DVR_THUMBNAIL_HEIGHT*3);
            res = DVR_RES_EFAIL;
        }
    }
    Dvr_MetaData_Destroy(pMeta);
    if(stMediaInfo.u32Duration == 0)
    {
        DPrint(DPRINT_ERR,"szFileName %s  u32Duration %d faild(res=%d)!!!\n",idxfile->filename, stMediaInfo.u32Duration, res);
        res = DVR_RES_EFAIL;
    }

    if(res != DVR_RES_SOK && m_pcallback != NULL)
        m_pcallback->CallBack((void*)idxfile, &res, NULL, m_pcallback->pInstance);
    return res;
}

DVR_RESULT CFileDB::OnCheckItem(PDVR_DB_IDXFILE idxfile, DB_CHECK_TYPE type)
{
    if(idxfile == NULL || m_filetbl == NULL)
        return DVR_RES_EFAIL;
    
    int ret = DVR_RES_EFAIL;
    if(idxfile->length < DVR_THUMBNAIL_WIDTH*DVR_THUMBNAIL_HEIGHT*3>>10)
    {
        if(m_pcallback != NULL)
            m_pcallback->CallBack((void*)idxfile, &ret, NULL, m_pcallback->pInstance);
        return DVR_RES_EFAIL;
    }

    if(idxfile->eType != DVR_FILE_TYPE_VIDEO)
        return DVR_RES_SOK;

    if(type == DB_CHECK_TYPE_FOR_SEARCH)
    {
        ret = OnCheckState(idxfile);
        if(ret == DVR_RES_SOK)
            return ret;

        ret = OnCheckMeta(idxfile);
        if(ret != DVR_RES_SOK)
            return ret;

        if(m_filetbl->TableIsRebuild())
            return DVR_RES_SOK;

        DVR_DB_TABLE table;
        memset(&table, 0, sizeof(DVR_DB_TABLE));
        table.type   = idxfile->efdtype;
        table.idf    = GetFileIndex(idxfile->filename);
        BIT_SET(table.state, FS_STATE_FMT);
        BIT_SET(table.state, FS_STATE_READ);
        BIT_SET(table.state, FS_STATE_WRITE);
        m_filetbl->TableAdd(&table, true);
        DPrint(DPRINT_ERR,"OnCheckItem ------------addfile %s\n",idxfile->filename);
        chmod(idxfile->filename, 0666);
        return DVR_RES_SOK;
    }

    return OnCheckMeta(idxfile);
}

DVR_RESULT CFileDB::OnCheckName(char *foldername, char *szFileName, DVR_FOLDER_TYPE type)
{
    if(szFileName == NULL)
        return DVR_RES_EFAIL;

    int ret = DVR_RES_EFAIL;
    bool invaild = true;
    if(type == DVR_FOLDER_TYPE_NORMAL)
    {
        if(strncmp(szFileName, "NOR", 3) == 0)
            invaild = false;
    }
    else if(type == DVR_FOLDER_TYPE_EMERGENCY)
    {
        if(strncmp(szFileName, "EVT", 3) == 0 ||
            strncmp(szFileName, "CRS", 3) == 0 ||
            strncmp(szFileName, "AEB", 3) == 0)
            invaild = false;
    }
//#ifndef DVR_FEATURE_V302
    else if(type == DVR_FOLDER_TYPE_DAS)
    {
        if(strncmp(szFileName, "IAC", 3) == 0 ||
            strncmp(szFileName, "APA", 3) == 0 ||
            strncmp(szFileName, "ACC", 3) == 0)
            invaild = false;
    }
//#endif
    else
    {
        if(strncmp(szFileName, "PHO", 3) == 0)
            invaild = false;
    }

    if(!invaild)
        return DVR_RES_SOK;
    DVR_DB_IDXFILE idxfile;
    sprintf(idxfile.filename, "%s%s",foldername, szFileName);
    idxfile.efdtype = type;
    if(m_pcallback != NULL)
        m_pcallback->CallBack((void*)&idxfile, &ret, NULL, m_pcallback->pInstance);
    DPrint(DPRINT_ERR, "OnCheckName::Unrecognizable %s to deleted!!!\n", idxfile.filename);
    return ret;
}

DVR_RESULT CFileDB::OnAddItem(const char *szFileName, DB_CHECK_TYPE type, PFILEMAP_LIST_PRIVATE filelist)
{
    if(szFileName == NULL || filelist == NULL || m_filetbl == NULL)
        return DVR_RES_EFAIL;

    struct stat result;
    DVR_FILE_TYPE  eType;
    if(stat(szFileName, &result) < 0)
    {
        DPrint(DPRINT_ERR,"stat %s faild!!!-----%s\n",szFileName,strerror(errno));
        return DVR_RES_EFAIL;
    }

    if(g_str_has_suffix(szFileName, "MP4") || g_str_has_suffix(szFileName, "mp4"))
        eType = DVR_FILE_TYPE_VIDEO;
    else if(g_str_has_suffix(szFileName, "JPG") || g_str_has_suffix(szFileName, "jpg"))
        eType = DVR_FILE_TYPE_IMAGE;
    else
    {
        DPrint(DPRINT_ERR,"Unrecognizable file type faild!!!-----%s\n",szFileName);
        return DVR_RES_EFAIL;
    }

    DVR_DB_IDXFILE ctxfile;
    memset(&ctxfile, 0, sizeof(DVR_DB_IDXFILE));
    ctxfile.eType       = eType;
    ctxfile.mktime      = result.st_mtime;
    ctxfile.efdtype     = (DVR_FOLDER_TYPE)filelist->eFolderType;
    ctxfile.length      = result.st_size >> 10;
    strcpy(ctxfile.filename, szFileName);

    //printf("Mode:szFileName%s  %lo (octal) %d\n",szFileName,(unsigned long)result.st_mode,result.st_mode & S_IXUSR);
    DVR_RESULT ret = OnCheckItem(&ctxfile, type);
    if(ret != DVR_RES_SOK)
        return ret;

    //printf("<lsw> result.st_mtime = %ld d_name = %s\n",result.st_mtime,szFileName);
    //struct tm *p = localtime(&result.st_mtime);
    //printf("%4d-%2d-%2d %2d:%2d:%d\n", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec);
    //printf("%s", ctime(&result.st_mtime));

    PDVR_DB_IDXFILE idxfile = (PDVR_DB_IDXFILE)g_malloc(sizeof(DVR_DB_IDXFILE));
    memcpy(idxfile, &ctxfile, sizeof(DVR_DB_IDXFILE));
    filelist->pList = g_list_insert_sorted(filelist->pList, idxfile, (GCompareFunc)TimeCompare);
    filelist->ullTotalUsedSpace += idxfile->length;
    return DVR_RES_SOK;
}

DVR_RESULT CFileDB::GetInfo(const char *szFileName, PDVR_DB_IDXFILE idxfile, PFILEMAP_LIST_PRIVATE filelist)
{
    if(szFileName == NULL || idxfile == NULL || filelist == NULL)
        return DVR_RES_EFAIL;
    
    DVR_DB_IDXFILE file;
    memset(&file, 0, sizeof(DVR_DB_IDXFILE));
    strcpy(file.filename, szFileName);
    GList *find = g_list_find_custom(filelist->pList, &file, (GCompareFunc)NameCompare);
    if (find == NULL || find->data == NULL)
        return DVR_RES_EFAIL;

    memcpy(idxfile, find->data, sizeof(DVR_DB_IDXFILE));
    return DVR_RES_SOK;
}




DVR_RESULT CFileDB::GetNextFile(const char *szFileName, char *desFileName, PFILEMAP_LIST_PRIVATE filelist)
{
    if(szFileName == NULL || desFileName == NULL || filelist == NULL)
        return DVR_RES_EFAIL;
    
    DVR_DB_IDXFILE file;
    memset(&file, 0, sizeof(DVR_DB_IDXFILE));
    strcpy(file.filename, szFileName);
    GList *find = g_list_find_custom(filelist->pList, &file, (GCompareFunc)NameCompare);
    if (find == NULL || find->data == NULL)
        return DVR_RES_EFAIL;

    find = g_list_next(find);
    if (find == NULL || find->data == NULL)
        return DVR_RES_EFAIL;

    PDVR_DB_IDXFILE idxfile = (PDVR_DB_IDXFILE)find->data;
    strcpy(desFileName, idxfile->filename);
    DPrint(DPRINT_ERR,"GetNextFile:: %s\n",idxfile->filename);
    return DVR_RES_SOK;
}

DVR_RESULT CFileDB::GetPrevFile(const char *szFileName, char *desFileName, PFILEMAP_LIST_PRIVATE filelist)
{
    if(szFileName == NULL || desFileName == NULL || filelist == NULL)
        return DVR_RES_EFAIL;
    
    DVR_DB_IDXFILE file;
    memset(&file, 0, sizeof(DVR_DB_IDXFILE));
    strcpy(file.filename, szFileName);
    GList *find = g_list_find_custom(filelist->pList, &file, (GCompareFunc)NameCompare);
    if (find == NULL || find->data == NULL)
        return DVR_RES_EFAIL;

    find = g_list_previous(find);
    if (find == NULL || find->data == NULL)
        return DVR_RES_EFAIL;

    PDVR_DB_IDXFILE idxfile = (PDVR_DB_IDXFILE)find->data;
    strcpy(desFileName, idxfile->filename);
    DPrint(DPRINT_ERR,"GetPrevFile:: %s\n",idxfile->filename);
    return DVR_RES_SOK;
}

DVR_RESULT CFileDB::GetPosByName(const char *szFileName, unsigned *puPos, PFILEMAP_LIST_PRIVATE filelist)
{
    if(szFileName == NULL || filelist == NULL)
        return DVR_RES_EFAIL;
    
    DVR_DB_IDXFILE file;
    memset(&file, 0, sizeof(DVR_DB_IDXFILE));
    strcpy(file.filename, szFileName);
    GList *find = g_list_find_custom(filelist->pList, &file, (GCompareFunc)NameCompare);
    if (find == NULL ||find->data == NULL)
        return DVR_RES_EFAIL;

    *puPos = g_list_index(filelist->pList, find->data);
    DPrint(DPRINT_ERR,"GetPosByName =============== %d\n",*puPos);
    return DVR_RES_SOK;
}

DVR_RESULT CFileDB::GetNameByPos(unsigned uRelPos, char *desFileName, PFILEMAP_LIST_PRIVATE filelist)
{
    if(desFileName == NULL || filelist == NULL)
        return DVR_RES_EFAIL;
    
    if (uRelPos >= g_list_length(filelist->pList))
    {
        DPrint(DPRINT_ERR, "%s: Invalid uRelPos[%d], uCount[%d]!\n", __func__, uRelPos, g_list_length(filelist->pList));
        return DVR_RES_EFAIL;
    }

    PDVR_DB_IDXFILE idxfile = (PDVR_DB_IDXFILE)g_list_nth_data(filelist->pList, uRelPos);
    if (idxfile == NULL)
        return DVR_RES_EFAIL;

    strncpy(desFileName, idxfile->filename, APP_MAX_FN_SIZE);
    return DVR_RES_SOK;
}

DVR_RESULT CFileDB::GetTypeByName(const char *szFileName, unsigned& eFolderType, PFILEMAP_LIST_PRIVATE filelist)
{
    if(szFileName == NULL || filelist == NULL)
        return DVR_RES_EFAIL;
    
    DVR_DB_IDXFILE file;
    memset(&file,0,sizeof(DVR_DB_IDXFILE));
    strcpy(file.filename, szFileName);
    GList *find = g_list_find_custom(filelist->pList, &file, (GCompareFunc)NameCompare);
    if (find == NULL ||find->data == NULL)
        return DVR_RES_EFAIL;

    PDVR_DB_IDXFILE idxfile = (PDVR_DB_IDXFILE)find->data;
    eFolderType = idxfile->efdtype;
    return DVR_RES_SOK;
}

DVR_U32 CFileDB::GetFileIndex(const char *fileName)
{
    char *nptr = strrchr((char *)fileName, '_');
    char *endptr;
    DVR_U32 file_index = 0;

    if (nptr)
        file_index = strtoull(nptr + 1, &endptr, 10);

    return file_index;
}

DVR_RESULT CFileDB::SearchFile(char *foldername, DVR_FOLDER_TYPE type, PFILEMAP_LIST_PRIVATE filelist)
{
    if(foldername == NULL || filelist == NULL)
        return DVR_RES_EFAIL;

	struct dirent *pDirent = NULL;
	if(0 != access(foldername, F_OK))
        return DVR_RES_EFAIL;

	DIR *pDir = opendir(foldername);
	//printf("<lsw> strName = %s\n", foldername);
	if(pDir == NULL)
	{
		DPrint(DPRINT_ERR,"<lsw> NULL == pDir foldername %s\n",foldername);
		return DVR_RES_EFAIL;
	}

    while((pDirent = readdir(pDir)) != NULL)
	{
		if(pDirent->d_name[0] == '.')
			continue;

        if(OnCheckName(foldername, pDirent->d_name, type))
			continue;

        char filename[APP_MAX_FN_SIZE];
        sprintf(filename, "%s%s",foldername, pDirent->d_name);
        OnAddItem(filename, DB_CHECK_TYPE_FOR_SEARCH, filelist);
    }
	closedir(pDir);
#ifdef FS_DEBUG
    for(int i = 0; i < g_list_length(filelist->pList);i++)
    {
        PDVR_DB_IDXFILE idxfile = (PDVR_DB_IDXFILE)g_list_nth_data(filelist->pList, i);
		DPrint(DPRINT_ERR,"<%d> idxfile->filename %s\n",i, idxfile->filename);
    }
#endif
    return DVR_RES_SOK;
}


