#include "FileBase.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include "DVR_SDK_INTFS.h"
#include "osa.h"

static gint IDCompare(gconstpointer a, gconstpointer b)
{
	int* pItemA = (int*)a;
	int* pItemB = (int*)b;

    if(pItemA == NULL || pItemB == NULL)
        return 0;

    if(*pItemA == *pItemB)
        return 1;
    
    else if(*pItemA > *pItemB)
        return 1;
    
    else if(*pItemA < *pItemB)
        return -1;

    return 0;
}

CFileBase::CFileBase(const char *szDirectory, const char* devname)
{
    m_worklist      = NULL;
    m_idlelist      = NULL;
    m_totaltblnum   = 0;
    m_fd            = -1;
    m_buffer        = NULL;
    m_rebuild       = false;
    strcpy(m_devname, devname);
    strcpy(m_pMountpoint, szDirectory);
    Init();
}

CFileBase::~CFileBase(void)
{
    Close();
}

void CFileBase::Init(void)
{
	DVR_U32 nCardTotalSpace = 0;
    char  filename[APP_MAX_FN_SIZE];
	Dvr_Sdk_StorageCard_Get(DVR_STORAGE_PROP_CARD_TOTALSPACE, &nCardTotalSpace, sizeof(DVR_U32), NULL);
    if(nCardTotalSpace < 16*1024*1024)
        m_totaltblnum = 300;
    else  if(nCardTotalSpace > 16*1024*1024 && nCardTotalSpace < 32*1024*1024)
        m_totaltblnum = 600;
    else  if(nCardTotalSpace > 32*1024*1024 && nCardTotalSpace < 64*1024*1024)
        m_totaltblnum = FS_DB_INDEX_NUM;
    else
        m_totaltblnum = FS_DB_INDEX_NUM;

    DPrint(DPRINT_ERR,"m_totaltblnum %d\n", m_totaltblnum);
    m_size  = sizeof(DVR_DB_TABLE)*m_totaltblnum;
}

int CFileBase::Load(char* buffer)
{
    if(m_fd < 0 ||buffer == NULL)
        return DVR_RES_EFAIL;

    int ret = ReadSector(m_fd, buffer, 0, m_totaltblnum);
    OnCheckTable();
    return ret;
}

int CFileBase::Open(const char *szDirectory)
{
    if(szDirectory == NULL)
        return DVR_RES_EFAIL;

    m_buffer    = (char *)malloc(m_size);
    memset(m_buffer, 0, m_size);

    char  filename[APP_MAX_FN_SIZE];
    sprintf(filename,"%s/%s",szDirectory,FS_DB_INDEX);
	if(0 == access(filename, F_OK))
    {
        SetHideAtrr();
        m_fd = open(filename,O_RDWR|O_NONBLOCK);
        if(m_fd < 0)
            DPrint(DPRINT_ERR,"filename %s\n",strerror(errno));

        Load(m_buffer);
        DPrint(DPRINT_ERR,"Open szDirectory [%s] tablename [%s]  m_fd = %d\n",szDirectory,filename,m_fd);
        return DVR_RES_SOK;
    }

    //Create and occupy space
    m_fd = open(filename,O_CREAT|O_RDWR|O_NONBLOCK);
    if(m_fd < 0)
    {
        DPrint(DPRINT_ERR,"devname %s\n",strerror(errno));
        return DVR_RES_EFAIL;
    }

    uint wsize  = 0;
    uint offset = 0;
    uint size   = m_totaltblnum;
    char *buf   = (char *)malloc(128*1024);
    memset(buf, 0x0, 128*1024);
    int count = 0;
    while(size > 0)
    {
        if(size > 256)
        {
            wsize = 256;
            int ret = WriteSector(m_fd, buf, offset, wsize);
            if(ret != DVR_RES_SOK)
            {
                close(m_fd);
                free(buf);
                DPrint(DPRINT_ERR,"filename %s m_totaltblnum %lld ret ========== %d\n",filename,m_totaltblnum,ret);
                return DVR_RES_EFAIL;
            }
        }
        else
        {
            wsize = size;
            int ret = WriteSector(m_fd, buf, offset, wsize);
            if(ret != DVR_RES_SOK)
            {
                close(m_fd);
                free(buf);
                DPrint(DPRINT_ERR,"filename %s m_totaltblnum %lld ret ========== %d\n",filename,m_totaltblnum,ret);
                return DVR_RES_EFAIL;
            }
        }
        size    -= wsize;
        offset  += wsize;
        count++;
    }
    fsync(m_fd);
    free(buf);
    DPrint(DPRINT_ERR,"count ===================== %d\n",count);
    int ret = SetHideAtrr();
    m_rebuild = true;
    Load(m_buffer);
    return ret;
}

int CFileBase::Close(void)
{
    FSFree(m_buffer);
    g_list_foreach(m_worklist, (GFunc)free, NULL);
    g_list_foreach(m_idlelist, (GFunc)free, NULL);
    g_list_free(m_worklist);
    g_list_free(m_idlelist);
    if(m_fd >= 0)
    {
		close(m_fd);
		m_fd = -1;
    }
    m_totaltblnum   = 0;
    m_rebuild       = false;
}

int CFileBase::Sync(void)
{
    if(m_fd < 0)
        return DVR_RES_EFAIL;
    
	fsync(m_fd);
    return DVR_RES_SOK;
}

int CFileBase::Format(void)
{
    char  filename[APP_MAX_FN_SIZE];
    sprintf(filename,"%s/%s",m_pMountpoint, FS_DB_INDEX);
	if(0 != access(filename, F_OK))
        return DVR_RES_SOK;

    remove(filename);
    return DVR_RES_SOK;
}

int CFileBase::TableAdd(PDVR_DB_TABLE table, bool sync)
{
    if(table == NULL)
    return DVR_RES_EFAIL;

    int idx = -1;
    int ret = OnGetFreeId(idx);
    if(ret != DVR_RES_SOK)
        return ret;

    ret = OnTableUpdate(idx, table, sync);
    if(ret != DVR_RES_SOK)
        return ret;

    int* id = (int*)g_malloc(sizeof(int));
    *id = idx;
    m_worklist = g_list_insert_sorted(m_worklist, id, (GCompareFunc)IDCompare);
    DPrint(DPRINT_ERR,"TableAdd------rebuild %d idx %d usednum %d freenum %d\n",m_rebuild,idx,g_list_length(m_worklist),g_list_length(m_idlelist));
#ifdef FS_DEBUG
    printf("<m_worklist>:");
    for(int i = 0; i < g_list_length(m_worklist);i++)
    {
        int* id = (int *)g_list_nth_data(m_worklist, i);
        printf("%d,",*id);
    }
    printf("\n");
#endif

    return DVR_RES_SOK;
}

int CFileBase::TableDel(PDVR_DB_TABLE table)
{
    if(table == NULL)
    return DVR_RES_EFAIL;

    int idx = -1;
    int ret = OnGetWorkId(idx, table);
    if(ret != DVR_RES_SOK)
        return ret;

    ret = OnTableDel(idx);
    if(ret != DVR_RES_SOK)
        return ret;

    int* id = (int*)g_malloc(sizeof(int));
    *id = idx;
    m_idlelist = g_list_append(m_idlelist, id);
    DPrint(DPRINT_ERR,"TableDel------rebuild %d idx %d usednum %d freenum %d\n",m_rebuild,idx,g_list_length(m_worklist),g_list_length(m_idlelist));
#ifdef FS_DEBUG
    printf("<m_idlelist>:");
    for(int i = 0; i < g_list_length(m_idlelist);i++)
    {
        int* id = (int *)g_list_nth_data(m_idlelist, i);
        printf("%d,",*id);
    }
    printf("\n");
#endif

    return DVR_RES_SOK;
}

int CFileBase::TableSetState(PDVR_DB_TABLE table)
{
    if(table == NULL)
    return DVR_RES_EFAIL;

    int idx = OnGetTableId(table);
    DVR_DB_TABLE tbl;
    OnGetTable(idx, &tbl);
    tbl.state |= table->state;
    return OnTableUpdate(idx, &tbl);
}

int CFileBase::TableLock(PDVR_DB_TABLE table)
{
    if(table == NULL)
        return DVR_RES_EFAIL;

    int idx = OnGetTableId(table);
    DVR_DB_TABLE tbl;
    memset(&tbl, 0 , sizeof(DVR_DB_TABLE));
    OnGetTable(idx, &tbl);
    BIT_SET(tbl.access,FS_ACCESS_LOCK);
    return OnTableUpdate(idx, &tbl);
}

int CFileBase::TableUnLock(PDVR_DB_TABLE table)
{
    if(table == NULL)
        return DVR_RES_EFAIL;

    int idx = OnGetTableId(table);
    DVR_DB_TABLE tbl;
    memset(&tbl, 0 , sizeof(DVR_DB_TABLE));
    OnGetTable(idx, &tbl);
    BIT_CLR(tbl.access,FS_ACCESS_LOCK);
    return OnTableUpdate(idx, &tbl);
}

int CFileBase::GettblState(PDVR_DB_TABLE table, DVR_U16& state)
{
    if(table == NULL)
        return DVR_RES_EFAIL;

    state = 0;
    int idx = OnGetTableId(table);
    DVR_DB_TABLE tbl;
    memset(&tbl, 0 , sizeof(DVR_DB_TABLE));
    int ret = OnGetTable(idx, &tbl);
    if(ret != DVR_RES_SOK)
        return ret;

    state = tbl.state;
    //Dump(&tbl);
    return DVR_RES_SOK;
}

int CFileBase::GettblAccess(PDVR_DB_TABLE table, DVR_U16& access)
{
    access = 0;
    if(table == NULL)
        return DVR_RES_EFAIL;

    int idx = OnGetTableId(table);
    DVR_DB_TABLE tbl;
    memset(&tbl, 0 , sizeof(DVR_DB_TABLE));
    int ret = OnGetTable(idx, &tbl);
    if(ret != DVR_RES_SOK)
        return ret;

    access = tbl.access;
    return DVR_RES_SOK;
}

int CFileBase::OnGetTable(int idx, PDVR_DB_TABLE table)
{
    if(idx < 0 || idx >= m_totaltblnum)
        return DVR_RES_EINVALIDARG;

    if(table == NULL || m_buffer == NULL)
        return DVR_RES_EFAIL;

    memcpy(table, m_buffer + idx*sizeof(DVR_DB_TABLE), sizeof(DVR_DB_TABLE));
    return DVR_RES_SOK;
}

int CFileBase::OnTableUpdate(int idx, PDVR_DB_TABLE table, bool sync)
{
    if(idx < 0 || idx >= m_totaltblnum)
        return DVR_RES_EINVALIDARG;

    if(table == NULL || m_buffer == NULL)
        return DVR_RES_EFAIL;

    PDVR_DB_TABLE buffer = (PDVR_DB_TABLE)(m_buffer + idx*sizeof(DVR_DB_TABLE));
    table->idx = idx;

    int ret = OnTableWrite(table, idx, 1);
    if(ret != DVR_RES_SOK)
        return ret;

    memcpy(buffer, table, sizeof(DVR_DB_TABLE));
    if(!sync)
        return ret;

    fsync(m_fd);
    return ret;
}

int CFileBase::OnTableDel(int idx)
{
    if(idx < 0 || idx >= m_totaltblnum)
        return DVR_RES_EINVALIDARG;

    if(m_buffer == NULL)
        return DVR_RES_EFAIL;

    PDVR_DB_TABLE table = (PDVR_DB_TABLE)(m_buffer + idx*sizeof(DVR_DB_TABLE));
    if(table == NULL)
        return DVR_RES_EFAIL;

    memset(table, 0, sizeof(DVR_DB_TABLE));
    int ret = OnTableWrite(table, idx, 1);
    if(ret != DVR_RES_SOK)
        return ret;
    fsync(m_fd);
    return DVR_RES_SOK;
}

int CFileBase::OnGetFreeId(int& idx)
{
    GList* find = g_list_first(m_idlelist);
    while(find)
    {
        int* index = (int*)find->data;
        if(index == NULL)
        {
            find = g_list_next(find);
            continue;
        }
        idx = *index;
        g_free(index);
        m_idlelist = g_list_delete_link(m_idlelist, find);
        return DVR_RES_SOK;
    }
    return DVR_RES_EFAIL;
}

int CFileBase::OnUsedRemoveAt(int idx)
{
    GList* find = g_list_first(m_worklist);
    while(find)
    {
        int* index = (int*)find->data;
        if(index == NULL || *index >= m_totaltblnum || *index != idx)
        {
            find = g_list_next(find);
            continue;
        }

        g_free(index);
        m_worklist = g_list_delete_link(m_worklist, find);
        return DVR_RES_SOK;
    }
    return DVR_RES_EFAIL;
}

int CFileBase::OnGetWorkId(int& idx, PDVR_DB_TABLE table)
{
    int tblidx = OnGetTableId(table);
    if(tblidx < 0 || tblidx >= m_totaltblnum)
        return DVR_RES_EINVALIDARG;

    int ret = OnUsedRemoveAt(tblidx);
    if(ret != DVR_RES_SOK)
        return ret;

    idx = tblidx;
    return DVR_RES_SOK;
}

int CFileBase::OnGetTableId(PDVR_DB_TABLE table)
{
    if(table == NULL || m_buffer == NULL)
        return DVR_RES_EFAIL;

    GList* find = g_list_first(m_worklist);
    while(find)
    {
        int* index = (int*)find->data;
        if(index == NULL || *index >= m_totaltblnum)
        {
            find = g_list_next(find);
            continue;
        }
        int idx = *index;
        PDVR_DB_TABLE tbl = (PDVR_DB_TABLE)(m_buffer + idx*sizeof(DVR_DB_TABLE));
        if(tbl == NULL || !BIT_ISSET(tbl->state, FS_STATE_FMT))
        {
            find = g_list_next(find);
            continue;
        }

        if(tbl->idf == table->idf && tbl->type == table->type)
            return idx;

        find = g_list_next(find);
    }

    return DVR_RES_EFAIL;
}

int CFileBase::OnCheckTable(void)
{
    if(m_buffer == NULL)
        return DVR_RES_EFAIL;

    for(int idx = 0;idx < m_totaltblnum;idx++)
    {
        PDVR_DB_TABLE table = (PDVR_DB_TABLE)(m_buffer + idx*sizeof(DVR_DB_TABLE));
        if(table == NULL)
            continue;

        if(BIT_ISSET(table->state, FS_STATE_FMT))
        {
            int* id = (int*)g_malloc(sizeof(int));
            *id = idx;
            m_worklist = g_list_insert_sorted(m_worklist, id, (GCompareFunc)IDCompare);
            continue;
        }

        int* id = (int*)g_malloc(sizeof(int));
        *id = idx;
        m_idlelist = g_list_insert_sorted(m_idlelist, id, (GCompareFunc)IDCompare);
    }
    DPrint(DPRINT_ERR, "%s: CFileBase::rebuild %d used %d free %d\n",
        __func__,m_rebuild,g_list_length(m_worklist),g_list_length(m_idlelist));
#ifdef FS_DEBUG
        
        printf("<m_worklist>:");
        for(int i = 0; i < g_list_length(m_worklist);i++)
        {
            int* id = (int *)g_list_nth_data(m_worklist, i);
            printf("%d,",*id);
        }
        printf("\n");
        printf("<m_idlelist>:");
        for(int i = 0; i < g_list_length(m_idlelist);i++)
        {
            int* id = (int *)g_list_nth_data(m_idlelist, i);
            printf("%d,",*id);
        }
        printf("\n");
#endif
    return DVR_RES_SOK;
}

int CFileBase::OnTableRead(void* buffer, DVR_U64 sectorpos, uint sectornum)
{
    if(m_fd < 0)
        return DVR_RES_EFAIL;

    return ReadSector(m_fd, buffer, sectorpos, sectornum);
}

int CFileBase::OnTableWrite(void* buffer, DVR_U64 sectorpos, uint sectornum)
{
    if(m_fd < 0)
        return DVR_RES_EFAIL;
    return WriteSector(m_fd, buffer, sectorpos, sectornum);    
}

int	CFileBase::ReadSector(int fd, void* buffer, DVR_U32 startsector, DVR_U32 sectornum)
{
	if (buffer == NULL || sectornum == 0)
		return DVR_RES_EFAIL;

	if (fd < 0)
		return DVR_RES_EFAIL;

	DVR_U16	secbit      = FS_SECTOR_BITS;
	DVR_U64	startpos    = startsector;
	DVR_U64 pos         = startpos << secbit;
	DVR_U32	size        = sectornum << secbit;
	int 	ret;

	ret = lseek64(fd, pos, SEEK_SET);
    if (ret < 0)
    {
        DPrint(DPRINT_ERR,"lseek64 %lld %s\n",pos, strerror(errno));
        return DVR_RES_EFAIL;
    }

	ret = read(fd, buffer, size);
    if (ret != (int)size)
	{
		DPrint(DPRINT_ERR,"ReadSector:pos = %lld, sectornum = %d, buffer = %p, ret = %d  %s\n", pos, sectornum, buffer, ret,strerror(errno));
		return DVR_RES_EFAIL;
	}
    return DVR_RES_SOK;
}

int CFileBase::WriteSector(int fd, void* buffer, DVR_U32 startsector, DVR_U32 sectornum)
{
	if (buffer == NULL || sectornum == 0)
		return DVR_RES_EFAIL;

	if (fd < 0)
		return DVR_RES_EFAIL;

	DVR_U16	secbit		= FS_SECTOR_BITS;
	DVR_U64	startpos	= startsector;
	DVR_U64 pos 		= startpos << secbit;
	DVR_U32	size		= sectornum << secbit;
	DVR_U32	ret         = 0;
	int		errcnt 		= 3;

	while (errcnt--)
	{
		lseek64(fd, pos, SEEK_SET);
		ret = write(fd, buffer, size);
		if (ret != size)
			continue;

		break;
	}

    if (ret != (int)size)
	{
		DPrint(DPRINT_ERR,"WriteSector:pos = %lld, sectornum = %d, buffer = %p, ret = %d  %s\n", pos, sectornum, buffer, ret, strerror(errno));
		return DVR_RES_EFAIL;
	}

    return DVR_RES_SOK;
}

DVR_U32 CFileBase::LittleToBig(char* data, DVR_U32 pos, DVR_U32 len)
{
	DVR_U32 ret = 0;
	switch(len)
	{
		case 1:
			ret = data[pos];
			break;
		case 2:
			ret = (data[pos+1] << 8) + data[pos];
			break;
		case 4:
			ret = (data[pos+3] << 24) + (data[pos+2] << 16) + (data[pos+1] << 8) + data[pos];
			break;
		default:
			break;
	}
	return ret;
}

int CFileBase::GetDbrData(int fd, PDBRINFO DBRinfo)
{
	char* buffer = (char*)malloc(1024);	
	if(buffer == NULL)
		return DVR_RES_EFAIL;

	memset(DBRinfo, 0, sizeof(DBRINFO));
    //¶Á0¡¢1ºÅÉÈÇøÐÅÏ¢
    ReadSector(fd, buffer, 0, 2);
	DBRinfo->sectorbit 			= LittleToBig(buffer, SECTORBITPOS, 		2);
	DBRinfo->reservesectornum 	= LittleToBig(buffer, RESERVESECTORNUMPOS, 	2);
	DBRinfo->sectortotalnum 	= LittleToBig(buffer, SECTORTOTALNUMPOS, 	4);
	DBRinfo->fdtsectornum 		= LittleToBig(buffer, FDTSECTORNUMPOS, 		4);
	DBRinfo->rootdircluster 	= LittleToBig(buffer, ROOTDIRCLUSTERPOS, 	4);
	DBRinfo->fatinfosector 		= LittleToBig(buffer, FATINFOSECTORPOS, 	2);
	DBRinfo->backupdbrsector 	= LittleToBig(buffer, BACKUPDBRSECTORPOS,	2);
	DBRinfo->clustersectornum 	= LittleToBig(buffer, CLUSTERSECTORNUMPOS,	1);
#ifdef FS_DEBUG
	printf("<========================FAT32_DBR========================>\n");
	printf("\t\tsectorbit = %d, reservesectornum  = %d\n", DBRinfo->sectorbit, DBRinfo->reservesectornum);
	printf("\t\tfdtsectornum   	= %d, rootdircluster = %d\n", DBRinfo->fdtsectornum, DBRinfo->rootdircluster);
	printf("\t\tfatinfosector  	= %d, backupdbrsector %d\n",DBRinfo->fatinfosector, DBRinfo->backupdbrsector);
	printf("\t\tclustersectornum = %d, sectortotalnum = %d\n",DBRinfo->clustersectornum, DBRinfo->sectortotalnum);
	printf("\n");
#endif
    free(buffer);
	return DVR_RES_SOK;
}

int CFileBase::HideTblFile(int fd, PDBRINFO dbrinfo)
{
    int		sectornum   = 256 << (FS_SECTOR_BITS - FS_INDEX_BITS);
    int     size        = sectornum << FS_SECTOR_BITS;
    char* 	buffer	    = (char*)malloc(size);

    if(buffer == NULL)
    	return DVR_RES_EFAIL;

    if(dbrinfo == NULL)
    {
        free(buffer);
    	return DVR_RES_EFAIL;
    }

    memset(buffer, 0, size);
    DVR_U32 pos = dbrinfo->reservesectornum + dbrinfo->fdtsectornum*2;
    int ret = ReadSector(fd, buffer, pos, sectornum);
    if(ret != DVR_RES_SOK)
    {
        free(buffer);
        DPrint(DPRINT_ERR,"HideTblFile %s\n",strerror(errno));
        return ret;
    }
    for(int i = 0;i < size;i += FILE_CONTENT_LEN)
    {
    	if(buffer[i] == 'I' && buffer[i+1] == 'N' &&
           buffer[i+2] == 'D' && buffer[i+3] == 'E' && 
           buffer[i+4] == 'X' && buffer[i+5] == 0x20)
         {
            //for(int j = 0;j < FILE_CONTENT_LEN;j++)
            //{
            //    printf("buffer[%d] = 0x%x %c\n",j,buffer[i + j],buffer[i +j]);
            //}
            DPrint(DPRINT_ERR,"HideTblFile curattr (0x%x)\n",buffer[i+11]);
            //hide file
            if(buffer[i+11] == 0x22)
                break;

            buffer[i+11] = 0x22;
            DVR_U32 sector = i >> FS_SECTOR_BITS;
            ret = WriteSector(fd,buffer + (sector << FS_SECTOR_BITS), pos + sector, 1);
            fsync(fd);
            DPrint(DPRINT_ERR,"WriteSector (pos %d---sector %d) attr 0x%x ret %d\n",pos,sector,buffer[i+11],ret);
            break;
         }
    }
    free(buffer);
    return ret;
}

int CFileBase::SetHideAtrr(void)
{
	int ret = 0;
	DBRINFO DBRinfo;
    
    memset(&DBRinfo,0,sizeof(DBRINFO));
    if(access(m_devname, F_OK) != 0)
        return DVR_RES_EFAIL;

    if(umount2(m_pMountpoint, MNT_FORCE) != 0)
        DPrint(DPRINT_ERR,"umount2 %s\n",strerror(errno));

    int fd = open(m_devname, O_RDWR|O_SYNC|O_NONBLOCK);
    if(fd < 0)
    {
        DPrint(DPRINT_ERR,"devname %s\n",strerror(errno));
        mount(m_devname, m_pMountpoint, "tfat", 0, 0);
        return DVR_RES_EFAIL;
    }

    ret = GetDbrData(fd, &DBRinfo);
	if(ret < 0)
	{
        DPrint(DPRINT_ERR,"GetDbrData failded !!!!\n");
        goto end;
	}
    ret = HideTblFile(fd, &DBRinfo);
    if(ret < 0)
    {
        DPrint(DPRINT_ERR,"FindIndex failded !!!!\n");
        goto end;
    }
end:
    close(fd);
    if(ret < 0)
        DPrint(DPRINT_ERR,"SetHideAtrr %s\n",strerror(errno));

    if(mount(m_devname, m_pMountpoint, "tfat", 0, 0) != 0)
    {
        DPrint(DPRINT_ERR,"mount %s\n",strerror(errno));
    }
    return ret;
}

int CFileBase::Dump(void)
{
    if(m_buffer == NULL)
        return DVR_RES_EFAIL;

    for(int idx = 0;idx < m_totaltblnum;idx++)
    {
        PDVR_DB_TABLE table = (PDVR_DB_TABLE)(m_buffer + idx*sizeof(DVR_DB_TABLE));
        if(table == NULL)
            continue;

        Dump(table);
    }
    return DVR_RES_SOK;
}

int CFileBase::Dump(PDVR_DB_TABLE table)
{
    if(table == NULL)
        return DVR_RES_EFAIL;
	DPrint(DPRINT_INFO,"<========================TABLE(id=%d)========================>\n", table->idx);
	DPrint(DPRINT_INFO,"\t\tidf %d  state  = 0x%08x, access = 0x%08x, type = %d\n", table->idf, table->state, table->access, table->type);
	DPrint(DPRINT_INFO,"\n\n");
    return DVR_RES_SOK;
}
