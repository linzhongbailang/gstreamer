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
#include "osa_mark.h"

static int	ReadSector(int fd, void* buffer, DVR_U32 startsector, DVR_U32 sectornum)
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

static int WriteSector(int fd, void* buffer, DVR_U32 startsector, DVR_U32 sectornum)
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

static DVR_U32 LittleToBig(char* data, DVR_U32 pos, DVR_U32 len)
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

static int GetDbrData(int fd, PDBRINFO DBRinfo)
{
	char* buffer = (char*)malloc(1024);	
	if(buffer == NULL)
		return DVR_RES_EFAIL;

	memset(DBRinfo, 0, sizeof(DBRINFO));
    //读0、1号扇区信息
    int ret = ReadSector(fd, buffer, 0, 2);
    if(ret != DVR_RES_SOK)
    {
        free(buffer);
        return DVR_RES_EFAIL;
    }
	DBRinfo->sectorbit 			= LittleToBig(buffer, SECTORBITPOS, 		2);
	DBRinfo->reservesectornum 	= LittleToBig(buffer, RESERVESECTORNUMPOS, 	2);
	DBRinfo->sectortotalnum 	= LittleToBig(buffer, SECTORTOTALNUMPOS, 	4);
	DBRinfo->fdtsectornum 		= LittleToBig(buffer, FDTSECTORNUMPOS, 		4);
	DBRinfo->rootdircluster 	= LittleToBig(buffer, ROOTDIRCLUSTERPOS, 	4);
	DBRinfo->fatinfosector 		= LittleToBig(buffer, FATINFOSECTORPOS, 	2);
	DBRinfo->backupdbrsector 	= LittleToBig(buffer, BACKUPDBRSECTORPOS,	2);
	DBRinfo->clustersectornum 	= LittleToBig(buffer, CLUSTERSECTORNUMPOS,	1);
#ifdef FS_DEBUG
	DPrint(DPRINT_ERR,"<========================FAT32_DBR========================>\n");
	DPrint(DPRINT_ERR,"\t\tsectorbit = %d, reservesectornum  = %d\n", DBRinfo->sectorbit, DBRinfo->reservesectornum);
	DPrint(DPRINT_ERR,"\t\tfdtsectornum   	= %d, rootdircluster = %d\n", DBRinfo->fdtsectornum, DBRinfo->rootdircluster);
	DPrint(DPRINT_ERR,"\t\tfatinfosector  	= %d, backupdbrsector %d\n",DBRinfo->fatinfosector, DBRinfo->backupdbrsector);
	DPrint(DPRINT_ERR,"\t\tclustersectornum = %d, sectortotalnum = %d\n",DBRinfo->clustersectornum, DBRinfo->sectortotalnum);
	DPrint(DPRINT_ERR,"\n");
    DPrint(DPRINT_ERR,"\t\t");
    for(int i = 0; i < 16;i++)
    {
        DPrint(DPRINT_ERR,"buffer[%x] = 0x%x",0x34 + i, buffer[0x34 + i]);
    }
#endif
    free(buffer);
	return DVR_RES_SOK;
}

static int MarkType(int fd, DVR_SDSPEED_TYPE type)
{
	char* buffer = (char*)malloc(512);	
	if(buffer == NULL)
		return DVR_RES_EFAIL;

    //读0号扇区信息
    int ret = ReadSector(fd, buffer, 0, 1);
    if(ret != DVR_RES_SOK)
    {
        free(buffer);
        return DVR_RES_EFAIL;
    }
    //从0x34字节偏移处开始的16个字节为保留字段，此处作为SD卡写速度的标志
    if(type == DVR_SD_SPEED_NORMAL)
    {
        for(int i = 0;i < 16;i++)
        {
            buffer[0x34 + i] = 0x7D;
        }
    }
    else if(type == DVR_SD_SPEED_BAD)
    {
        for(int i = 0;i < 16;i++)
        {
            buffer[0x34 + i] = 0xFF;
        }
    }
    else
    {
        free(buffer);
        return DVR_RES_SOK;
    }

    WriteSector(fd, buffer, 0, 1);
    fsync(fd);
    free(buffer);
	return DVR_RES_SOK;
}

static int MarkClear(int fd)
{
	char* buffer = (char*)malloc(512);	
	if(buffer == NULL)
		return DVR_RES_EFAIL;

    //读0号扇区信息
    int ret = ReadSector(fd, buffer, 0, 1);
    if(ret != DVR_RES_SOK)
    {
        free(buffer);
        return DVR_RES_EFAIL;
    }
    //从0x34字节偏移处开始的16个字节为保留字段，此处作为SD卡写速度的标志
    memset(&buffer[0x34], 0x0, 16);
    WriteSector(fd, buffer, 0, 1);
    fsync(fd);
    free(buffer);
	return DVR_RES_SOK;
}

static int GetMarkType(int fd, DVR_U32 *type)
{
	char* buffer = (char*)malloc(512);
	if(buffer == NULL)
		return DVR_RES_EFAIL;

    //读0号扇区信息
    int ret = ReadSector(fd, buffer, 0, 1);
    if(ret != DVR_RES_SOK)
    {
        free(buffer);
        return DVR_RES_EFAIL;
    }
    //从0x34字节偏移处开始的16个字节为保留字段，此处作为SD卡写速度的标志
    int speednormal = 0;
    int speedbad = 0;
    for(int i = 0;i < 16;i++)
    {
        if(buffer[0x34 + i] == 0x7D)
            speednormal++;
        else if(buffer[0x34 + i] == 0xFF)
            speedbad++;
    }
    //由于SD插入电脑后可能提示扫描并修复，如果用户选择扫描并修复后，这里偏移的第14字节可能会被修改。
    if(speednormal >= 10)
        *type = DVR_SD_SPEED_NORMAL;
    else if(speedbad >= 10)
        *type = DVR_SD_SPEED_BAD;
    else
        *type = DVR_SD_SPEED_NONE;

    free(buffer);
    return DVR_RES_SOK;
}

static int SetMark(char* devicename, char* mountpoint, DVR_SDSPEED_TYPE type)
{
    int ret = 0;
    DBRINFO DBRinfo;
    
    if(devicename == NULL || mountpoint == NULL)
        return DVR_RES_EFAIL;

    DPrint(DPRINT_ERR,"SetMark type %d umount  %s %s\n",type,devicename,mountpoint );
    if(umount2(mountpoint, MNT_FORCE) != 0)
        DPrint(DPRINT_ERR,"umount2 %s\n",strerror(errno));

    int fd = open(devicename, O_RDWR|O_SYNC|O_NONBLOCK);
    if(fd < 0)
    {
        DPrint(DPRINT_ERR,"devicename %s %s\n",devicename, strerror(errno));
        mount(devicename, mountpoint, "tfat", 0, 0);
        DPrint(DPRINT_ERR,"SetMark %s mount  %s %s\n",devicename,mountpoint );
        return DVR_RES_EFAIL;
    }

    /*ret = GetDbrData(fd, &DBRinfo);
    if(ret < 0)
    {
        DPrint(DPRINT_ERR,"GetDbrData failded !!!!\n");
        goto end;
    }*/
    
    ret = MarkType(fd, type);
    if(ret < 0)
    {
        DPrint(DPRINT_ERR,"FindIndex failded !!!!\n");
        goto end;
    }
end:
    close(fd);
    if(ret < 0)
        DPrint(DPRINT_ERR,"SetMark %s\n",strerror(errno));
    DPrint(DPRINT_ERR,"SetMark type=%d mount  %s %s\n",type,devicename,mountpoint );
    if(mount(devicename, mountpoint, "tfat", 0, 0) != 0)
    {
        DPrint(DPRINT_ERR,"mount %s\n",strerror(errno));
    }
    return ret;
}

static int DelMark(char* devicename, char* mountpoint)
{
    int ret = 0;
    DBRINFO DBRinfo;

    if(devicename == NULL || mountpoint == NULL)
        return DVR_RES_EFAIL;

    if(umount2(mountpoint, MNT_FORCE) != 0)
        DPrint(DPRINT_ERR,"umount2 %s\n",strerror(errno));

    int fd = open(devicename, O_RDWR|O_SYNC|O_NONBLOCK);
    if(fd < 0)
    {
        DPrint(DPRINT_ERR,"devicename %s %s\n",devicename, strerror(errno));
        mount(devicename, mountpoint, "tfat", 0, 0);
        return DVR_RES_EFAIL;
    }

    ret = GetDbrData(fd, &DBRinfo);
    if(ret < 0)
    {
        DPrint(DPRINT_ERR,"GetDbrData failded !!!!\n");
        goto end;
    }
    
    ret = MarkClear(fd);
    if(ret < 0)
    {
        DPrint(DPRINT_ERR,"MarkClear failded !!!!\n");
        goto end;
    }
end:
    close(fd);
    DPrint(DPRINT_ERR,"DelMark   mount %s  %s\n",devicename,mountpoint);
    if(mount(devicename, mountpoint, "tfat", 0, 0) != 0)
    {
        DPrint(DPRINT_ERR,"mount %s\n",strerror(errno));
    }
    return ret;
}

static int GetMark(char* devicename, char* mountpoint, DVR_U32 *type)
{
    int ret = 0;
    DBRINFO DBRinfo;

    if(devicename == NULL || mountpoint == NULL || type == NULL)
        return DVR_RES_EFAIL;

    if(umount2(mountpoint, MNT_FORCE) != 0)
        DPrint(DPRINT_ERR,"umount2 %s\n",strerror(errno));

    DPrint(DPRINT_ERR,"GetMark   umount %s  %s\n",devicename,mountpoint);

    int fd = open(devicename, O_RDWR|O_SYNC|O_NONBLOCK);
    if(fd < 0)
    {
        DPrint(DPRINT_ERR,"devicename %s %s\n",devicename, strerror(errno));
        DPrint(DPRINT_ERR,"GetMark 1   mount %s  %s\n",devicename,mountpoint);
        mount(devicename, mountpoint, "tfat", 0, 0);
        return DVR_RES_EFAIL;
    }

    ret = GetDbrData(fd, &DBRinfo);
    if(ret < 0)
    {
        DPrint(DPRINT_ERR,"GetDbrData failded !!!!\n");
        goto end;
    }
    
    ret = GetMarkType(fd, type);
    if(ret < 0)
    {
        DPrint(DPRINT_ERR,"GetMarkType failded !!!!\n");
        goto end;
    }
end:
    close(fd);
    if(mount(devicename, mountpoint, "tfat", 0, 0) != 0)
    {
        DPrint(DPRINT_ERR,"mount %s\n",strerror(errno));
    }
    DPrint(DPRINT_ERR,"GetMark 2   mount  %s  %s  type %d\n",devicename,mountpoint,*type);
    return ret;
}

DVR_U32 DVR::OSA_MarkSDSpeed(char* devicename, char* mountpoint, DVR_SDSPEED_TYPE type)
{
    if(devicename == NULL || mountpoint == NULL)
        return DVR_RES_EFAIL;
    return SetMark(devicename, mountpoint, type);
}

DVR_U32 DVR::OSA_GetSDSpeekType(char* devicename, char* mountpoint,  DVR_U32 *type)
{
    if(devicename == NULL || mountpoint == NULL || type == NULL)
        return DVR_RES_EFAIL;
    return GetMark(devicename, mountpoint, type);
}

DVR_U32 DVR::OSA_DelMarkSDSpeekOK(char* devicename, char* mountpoint)
{
    if(devicename == NULL || mountpoint == NULL)
        return DVR_RES_EFAIL;
    return DelMark(devicename, mountpoint);
}

