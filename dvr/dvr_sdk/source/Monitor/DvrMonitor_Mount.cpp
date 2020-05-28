/*===========================================================================*\
* Copyright 2003 O-Film Technologies, Inc., All Rights Reserved.
* O-Film Confidential
*
* DESCRIPTION:
*
* ABBREVIATIONS:
*   TODO: List of abbreviations used, or reference(s) to external document(s)
*
* TRACEABILITY INFO:
*   Design Document(s):
*     TODO: Update list of design document(s)
*
*   Requirements Document(s):
*     TODO: Update list of requirements document(s)
*
*   Applicable Standards (in order of precedence: highest first):
*
* DEVIATIONS FROM STANDARDS:
*   TODO: List of deviations from standards in this file, or
*   None.
*
\*===========================================================================*/

#ifdef __linux__

#include <windows.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <errno.h>
#include <string.h>

#include <dprint.h>

#include "DvrMonitor_Mount.h"
#include "osa_notifier.h"

#define USB_HUB_CLASS          9
#define USB_MASS_STORAGE_CLASS 8

static bool read_mbr(const char *dev, unsigned char *mbr);
static bool is_first_partition(const char *name);
static bool is_fat_partition(const char *path);

static const char * const VFAT_MOUNT_OPTION = "iocharset=utf8";
static const char * const NTFS_MOUNT_OPTION = "nls=utf8";

static const char * const EVENTS_DFL_POLL_MSECS = "/sys/module/block/parameters/events_dfl_poll_msecs";

DvrMonitorMount::DvrMonitorMount(OsaNotifier *notifier, void *sysCtrl)
{
	this->notifier = notifier;

	m_hubLevel = 0;

	m_fd = inotify_init();
	m_wd = inotify_add_watch(m_fd, "/dev", IN_CREATE | IN_DELETE);

	struct sockaddr_nl nls;
	memset(&nls, 0, sizeof(struct sockaddr_nl));

	nls.nl_family = AF_NETLINK;
	nls.nl_pid = getpid();
	nls.nl_groups = -1;
	m_pfd.events = POLLIN;
	m_pfd.fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	bind(m_pfd.fd, (const struct sockaddr *)&nls, sizeof(struct sockaddr_nl));

	m_sysCtrl = (DvrSystemControl *)sysCtrl;
	memset(m_szDevice, 0, sizeof(m_szDevice));
	memset(m_emmcDevice, 0, sizeof(m_emmcDevice));
	m_bHasMounted = false;
    m_bRemove     = false;
	m_bErrFormatPlugIn = false;
}

DvrMonitorMount::~DvrMonitorMount()
{
	StopMonitor();

	inotify_rm_watch(m_fd, m_wd);
	close(m_fd);
	close(m_pfd.fd);
}

static bool is_fat_partition(const char *path)
{
	bool bIsFat = false;
	char command[128];
	memset(command, 0, sizeof(command));

	snprintf(command, 128, "lsblk -lf %s | awk 'NR==2{print $2}'", path);
	
    FILE *fp = vpopen(command, "r");
	if(fp != NULL)
	{
	    char buffer[100];
	    char * line = NULL;
	    size_t len = 0;
	    ssize_t read;
	    if((read = getline(&line, &len, fp)) != -1) 
		{
			if(line != NULL)
			{
				if(!strncmp(line ,"dos", 3) || !strncmp(line ,"fat32", 5) || !strncmp(line ,"vfat", 4))
				{
					bIsFat = true;
				}
			}
	    }

		vpclose(fp);
	}
	else
	{
		char *errstr = strerror(errno);
		DPrint(DPRINT_ERR, "vpopen for command[%s] failed, reason[%s]!!!!", command, errstr);
	}

	return bIsFat;
}

static int FatfsCheck(const char* partition_dev)
{
	char cmdstring[128];
	memset(cmdstring, 0, sizeof(cmdstring));

	snprintf(cmdstring, 128, "dosfsck -a %s", partition_dev);
	
    FILE *fp = vpopen(cmdstring, "r");
	if(fp != NULL)
	{
		DPrint(DPRINT_INFO, "excute command [%s] successfully!!!", cmdstring);
		vpclose(fp);
	}
	else
	{
		char *errstr = strerror(errno);
		DPrint(DPRINT_ERR, "vpopen for command[%s] failed, reason[%s]!!!!", cmdstring, errstr);
	}

	return 0;
}

static bool Get_EmmcDevice(char* emmcdevice)
{
	FILE *fp = NULL;
	int nSscanfNum = 0;
	char chDevicePath[PATH_MAX] = {0};
	char chMountPath[PATH_MAX] = {0};
	char chFsTypeName[PATH_MAX]= {0};
	char chBuffer[PATH_MAX] = {0};

    if(emmcdevice == NULL)
        return FALSE;
    
	fp = fopen("/proc/mounts","r");
	if (NULL == fp)
	{
		DPrint(DPRINT_ERR, "\n read  /proc/mounts  error! \n");
		return FALSE;
	}

	while(1)
	{
		memset(chBuffer, 0, sizeof(chBuffer));
		if(NULL == fgets(chBuffer, sizeof(chBuffer), fp))
			break;

		memset(chDevicePath, 0, sizeof(chDevicePath));
		memset(chMountPath, 0, sizeof(chMountPath));
		memset(chFsTypeName, 0, sizeof(chFsTypeName));
		
		nSscanfNum = sscanf(chBuffer, "%[^' '] %[^' '] %[^' ']", chDevicePath, chMountPath, chFsTypeName);
		if(3 != nSscanfNum)
			continue;

        if(strstr(chDevicePath, "/dev/mmcblk1"))
        {
            strcpy(emmcdevice,"/dev/mmcblk1p1");
            DPrint(DPRINT_INFO, "emmcdevice name =========%s\n", emmcdevice);
            return TRUE;
        }

        DPrint(DPRINT_INFO, "\nchDevicePath[%s], chMountPath[%s], chFstypeName[%s]\n", chDevicePath, chMountPath, chFsTypeName);
	}
	
	fclose(fp);

    return FALSE;
}

void DvrMonitorMount::StartMonitor(std::list<DVR_DEVICE> *pDevices)
{
	SetupSystemParameters();
	
	int devMountedCount = 0;

	regex_t udiskregex;
	regcomp(&udiskregex, "/dev/mmcblk1p1+?$", REG_EXTENDED | REG_NOSUB);

    
	DPrint(DPRINT_INFO, "DvrMonitorMount::StartMonitor enter\n");
	
	FILE *file = fopen("/proc/mounts", "r");
	if (file)
	{
		char line[PATH_MAX] = { 0 };
		while (fgets(line, PATH_MAX, file) != NULL)
		{
			char srcpath[PATH_MAX] = { 0 };
			char dstpath[PATH_MAX] = { 0 };
			char others[PATH_MAX] = { 0 };
			sscanf(line, "%s %s %s", srcpath, dstpath, others);

			if (regexec(&udiskregex, srcpath, 0, 0, 0) == 0)
			{
				if (access(srcpath, 0) == 0) {
					DVR_DEVICE device;
					memset(&device, 0, sizeof(device));
					memcpy(m_szDevice, srcpath, sizeof(srcpath));

					if(!strcmp(dstpath, SD_MOUNT_POINT))
					{
						device.eType = DVR_DEVICE_TYPE_USBHD;
						strcpy(device.szMountPoint, dstpath);
						strcpy(device.szDevicePath, srcpath);

						char szDevice[PATH_MAX] = { 0 };
						GetUSBHDDevice(srcpath, szDevice);
						device.intfClass = GetUSBInterfaceClass(szDevice);
						pDevices->push_back(device);

						devMountedCount++;
						
						m_bHasMounted = true;
                        printf("1--DvrMonitorMount---devMountedCount %d , m_bHasMounted = %d\n",devMountedCount,m_bHasMounted);
					}
				}
				else {
					umount(dstpath);
				}
			}
		}
		fclose(file);		
	}

	if(devMountedCount == 0)
	{
		mkdir("/media/data", 0777);

        DPrint(DPRINT_INFO, "DvrMonitorMount::StartMonitor devMountedCount=0\n");
        
		PurgeDeviceNode();

		char path[PATH_MAX] = { 0 };

		DIR *dir = opendir("/dev");
		struct dirent *dirent = NULL;
		while ((dirent = readdir(dir)) != NULL)
		{
			strcpy(path, "/dev/");
			strcat(path, dirent->d_name);

            
            
			if (regexec(&udiskregex, path, 0, 0, 0) == 0)
			{
				DVR_DEVICE device;
				memset(&device, 0, sizeof(device));
                
				char szDevice[PATH_MAX] = { 0 };
				GetUSBHDDevice(path, szDevice);
				device.intfClass = GetUSBInterfaceClass(szDevice);

                
                
                DPrint(DPRINT_INFO, "DvrMonitorMount::StartMonitor detect %s\n",path);
				if(!is_first_partition(dirent->d_name))
					continue;

				char dstpath[PATH_MAX] = { 0 };
				strcpy(dstpath, SD_MOUNT_POINT);
				mkdir(dstpath, 0777);

				if(is_fat_partition(path))
				{
					//FatfsCheck(path);
					if (mount(path, dstpath, "tfat", 0, 0) == 0)
					{			
					    
					    DPrint(DPRINT_INFO, "DvrMonitorMount::StartMonitor mount %s  %s \n",path,dstpath);
						device.eType = DVR_DEVICE_TYPE_USBHD;
						strcpy(device.szMountPoint, dstpath);
						strcpy(device.szDevicePath, path);
						pDevices->push_back(device);
						
						memcpy(m_szDevice, path, sizeof(path));
						m_bHasMounted = true;
                        printf("2--DvrMonitorMount---devMountedCount %d , m_bHasMounted = %d\n",devMountedCount,m_bHasMounted);
					}
					else
					{
						char *errstr = strerror(errno);
						rmdir(dstpath);

						int fd = open(path, O_RDONLY);
						if (fd == -1)
						{
							if (errno == ENXIO)
							{
								unlink(path);
							}
						}
						else
						{
							close(fd);
						}
					}
				}
				else
				{
				    if(Get_EmmcDevice(m_emmcDevice))
                    {
                        DVR_DEVICE device;
                        memset(&device, 0, sizeof(device));                        
						strcpy(m_szDevice, "/dev/mmcblk1p1");
                        
                        device.eType = DVR_DEVICE_TYPE_USBHD;
                        strcpy(device.szMountPoint, SD_MOUNT_POINT);
                        strcpy(device.szDevicePath, m_emmcDevice);
                    
                        char szDevice[PATH_MAX] = { 0 };
                        GetUSBHDDevice("/dev/mmcblk1p1", szDevice);
                        device.intfClass = GetUSBInterfaceClass(szDevice);
                        pDevices->push_back(device);
                    
                        devMountedCount++;
                        
                        m_bHasMounted = true;
                    }            
				    //first find emmc device and ignore
				    /*bool findemmc = strlen(m_emmcDevice) ? FALSE : Get_EmmcDevice(m_emmcDevice);
                    if(!findemmc)
                    {
                        DPrint(DPRINT_ERR, "UnRecognizable File System for path[%s]!!!!", path);
                        NotifyMountFailure(DVR_DEVICE_TYPE_USBHD, NULL, path);
                        m_bErrFormatPlugIn = true;
                    }*/
				}
			}
		}
		closedir(dir);
	}
	
	regfree(&udiskregex);

	DVR::OSA_tskCreate(&m_hThread, Monitor_MountTaskEntry, this);
	DVR::OSA_tskCreate(&m_hThread_TFCard, Monitor_MountTaskEntry_TFCard, this);
}

void DvrMonitorMount::StopMonitor()
{
	DVR::OSA_tskDelete(&m_hThread);
	DVR::OSA_tskDelete(&m_hThread_TFCard);
}

void DvrMonitorMount::NotifyFsCheck(DVR_DEVICE_TYPE devType, const char *mountPoint, const char *dev, int intfClass)
{
	DVR_DEVICE_EVENT deviceEvent = DVR_DEVICE_EVENT_FSCHECK;

	DVR_DEVICE device;
	memset(&device, 0, sizeof(device));

	device.eType = devType;
	if(mountPoint != NULL){
		strcpy(device.szMountPoint, mountPoint);
	}
	
	if (dev != NULL) {
		strcpy(device.szDevicePath, dev);
	}
	device.intfClass = intfClass;

	DPrint(DPRINT_ERR, "DvrMonitorMount::NotifyMount() %s has arrived\n", mountPoint);
	OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_DEVICEEVENT, (void *)&deviceEvent, (long)sizeof(deviceEvent),
		(void *)&device, (long)sizeof(device), notifier);

}

void DvrMonitorMount::NotifyMount(DVR_DEVICE_TYPE devType, const char *mountPoint, const char *dev, int intfClass)
{
	DVR_DEVICE_EVENT deviceEvent = DVR_DEVICE_EVENT_PLUGIN;

	DVR_DEVICE device;
	memset(&device, 0, sizeof(device));

	device.eType = devType;
	if(mountPoint != NULL){
		strcpy(device.szMountPoint, mountPoint);
	}
	
	if (dev != NULL) {
		strcpy(device.szDevicePath, dev);
	}
	device.intfClass = intfClass;

	DPrint(DPRINT_ERR, "DvrMonitorMount::NotifyMount() %s has arrived\n", mountPoint);
	OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_DEVICEEVENT, (void *)&deviceEvent, (long)sizeof(deviceEvent),
		(void *)&device, (long)sizeof(device), notifier);
}

void DvrMonitorMount::NotifyMountFailure(DVR_DEVICE_TYPE devType, const char *mountPoint, const char *dev)
{
	DVR_DEVICE_EVENT deviceEvent = DVR_DEVICE_EVENT_ERROR;

	DVR_DEVICE device;
	memset(&device, 0, sizeof(device));

	device.eType = DVR_DEVICE_TYPE_UNKNOWN;
	if(mountPoint != NULL){
		strcpy(device.szMountPoint, mountPoint);
		deviceEvent = DVR_DEVICE_EVENT_ERROR;
	}else{
		deviceEvent = DVR_DEVICE_EVENT_FORMAT_NOTSUPPORT;
	}
	if (dev != NULL) {
		strcpy(device.szDevicePath, dev);
	}

	DPrint(DPRINT_LOG, "DvrMonitorMount::NotifyMountFailure() %s\n", dev);
	OsaNotifier::PostNotification(DVR_NOTIFICATION_TYPE_DEVICEEVENT, (void *)&deviceEvent, (long)sizeof(deviceEvent),
		(void *)&device, (long)sizeof(device), notifier);
}

void DvrMonitorMount::NotifyUnMount(DVR_DEVICE_TYPE devType, const char *mountPoint, const char *dev)
{
	DVR_DEVICE_EVENT deviceEvent = DVR_DEVICE_EVENT_PLUGOUT;

	DVR_DEVICE device;
	memset(&device, 0, sizeof(device));
	device.eType = devType;

	if(mountPoint != NULL){
		strcpy(device.szMountPoint, mountPoint);
	}
	if (dev != NULL){
		strcpy(device.szDevicePath, dev);
	}

	OsaNotifier::SendNotification(DVR_NOTIFICATION_TYPE_DEVICEEVENT, (void *)&deviceEvent, (long)sizeof(deviceEvent),
		(void *)&device, (long)sizeof(device), notifier);
}

static bool read_mbr(const char *dev, unsigned char *mbr)
{
	char *name = (char *)malloc(100);
	const char *tmp = "/tmp/read_mbr_tmp";
	unlink(tmp);

	sprintf(name, "/sys/block/%s/dev", dev);

	FILE *file = fopen(name, "r");
	if (file == NULL) {
		DPrint(DPRINT_ERR, "read_mbr() open file %s failed! (%s)\n", name, strerror(errno));
		free(name);
		return false;
	}

	free(name);
	char *line = (char *)malloc(1024);
	fgets(line, 1024, file);

	int ma = 0;
	int mi = 0;
	sscanf(line, "%d:%d", &ma, &mi);
	free(line);
	fclose(file);

	DPrint(DPRINT_LOG, "%s: major = %d minor = %d\n", dev, ma, mi);
	mknod(tmp, S_IFBLK, makedev(ma, mi));

	file = fopen(tmp, "r");
	if (file == NULL) {
		DPrint(DPRINT_ERR, "read_mbr() open file %s failed! (%s)\n", tmp, strerror(errno));
		return false;
	}

	fread(mbr, 1, 512, file);

	fclose(file);

	unlink(tmp);

	return true;
}

static bool is_first_partition(const char *name)
{
#if 0

	regex_t mmc_nopartition;
	regcomp(&mmc_nopartition, "^mmcblk[0-9]$", REG_EXTENDED | REG_NOSUB);
#endif

	regex_t mmc_first_partition;
	regcomp(&mmc_first_partition, "^mmcblk1p1$", REG_EXTENDED | REG_NOSUB);

#if 0
	regex_t mmc_first_logical;
	regcomp(&mmc_first_logical, "^mmcblk[0-9]p5$", REG_EXTENDED | REG_NOSUB);

	regex_t udisk_nopartition;
	regcomp(&udisk_nopartition, "^sd[a-z]+$", REG_EXTENDED | REG_NOSUB);

	regex_t udisk_first_partition;
	regcomp(&udisk_first_partition, "^sd[a-z]+1$", REG_EXTENDED | REG_NOSUB);

	regex_t udisk_first_logical;
	regcomp(&udisk_first_logical, "^sd[a-z]+5$", REG_EXTENDED | REG_NOSUB);
#endif
	unsigned char mbr[512] = { 0 };

	int res = 0;

#if 0
	if (regexec(&mmc_nopartition, name, 0, 0, 0) == 0 ||
		regexec(&udisk_nopartition, name, 0, 0, 0) == 0) {

		if (read_mbr(name, mbr)) {
			//There is no partition in the disk
			if (*((int *)((unsigned char *)mbr + 0x01CA)) == 0 &&
				*((int *)((unsigned char *)mbr + 0x01DA)) == 0 &&
				*((int *)((unsigned char *)mbr + 0x01EA)) == 0 &&
				*((int *)((unsigned char *)mbr + 0x01FA)) == 0)
				res = 1;

			if (*((unsigned char *)mbr + 0x01BE) != 0 &&
				*((unsigned char *)mbr + 0x01BE) != 0x80 &&
				*((unsigned char *)mbr + 0x01CE) != 0 &&
				*((unsigned char *)mbr + 0x01CE) != 0x80 &&
				*((unsigned char *)mbr + 0x01DE) != 0 &&
				*((unsigned char *)mbr + 0x01DE) != 0x80 &&
				*((unsigned char *)mbr + 0x01EE) != 0 &&
				*((unsigned char *)mbr + 0x01EE) != 0x80)
				res = 1;
		}
	}
#endif

	if (regexec(&mmc_first_partition, name, 0, 0, 0) == 0) {

		char s[10] = { 0 };
		strcpy(s, name);
		s[strlen(s) - 1] = '\0';
		if (regexec(&mmc_first_partition, name, 0, 0, 0) == 0)
			s[strlen(s) - 1] = '\0';

		if (read_mbr(s, mbr)) {
			//The first partition is not an extended partition.
			if (*((unsigned char *)mbr + 0x01C2) != 0x05)
				res = 1;
		}
	}

#if 0
	if (regexec(&mmc_first_logical, name, 0, 0, 0) == 0 ||
		regexec(&udisk_first_logical, name, 0, 0, 0) == 0) {

		char s[10] = { 0 };
		strcpy(s, name);
		s[strlen(s) - 1] = '\0';
		if (regexec(&mmc_first_logical, name, 0, 0, 0) == 0)
			s[strlen(s) - 1] = '\0';

		if (read_mbr(s, mbr)) {
			//The first partition is an extended partition.
			if (*((unsigned char *)mbr + 0x01C2) == 0x05)
				res = 1;
		}
	}
#endif
	//regfree(&mmc_nopartition);
	regfree(&mmc_first_partition);
	//regfree(&mmc_first_logical);
	//regfree(&udisk_nopartition);
	//regfree(&udisk_first_partition);
	//regfree(&udisk_first_logical);

	return res;
}

void DvrMonitorMount::Monitor_MountTaskEntry(void *ptr)
{
	DvrMonitorMount *p = (DvrMonitorMount *)ptr;

	char buffer[PATH_MAX] = { 0 };
	int len = 0;
	while ((len = read(p->m_fd, buffer, PATH_MAX)) > 0)
	{
		struct inotify_event *event = (struct inotify_event *)buffer;
		char *offset = buffer;
		while ((char *)event - buffer < len)
		{
			char srcpath[PATH_MAX] = { 0 };
			strcpy(srcpath, "/dev/");
			strcat(srcpath, event->name);
			char dstpath[PATH_MAX] = { 0 };
			strcpy(dstpath, SD_MOUNT_POINT);

			regex_t udiskregex;
			regcomp(&udiskregex, "^mmcblk1p1+?$", REG_EXTENDED | REG_NOSUB);

			//regex_t udisk_nopartition;
			//regcomp(&udisk_nopartition, "^mmcblk[0-9]+$", REG_EXTENDED | REG_NOSUB);

			if (regexec(&udiskregex, event->name, 0, 0, 0) == 0)
			{
				//p->PurgeMountPoint();
				DVR_DEVICE_TYPE devType = DVR_DEVICE_TYPE_USBHD;

                DPrint(DPRINT_ERR, "Monitor_MountTaskEntry regexec event->name=%s event->mask=0x%x  !!!!",event->name,event->mask);
				if (event->mask & IN_CREATE) {
					int fd = open(srcpath, O_RDONLY);
					if (fd < 0) {
						DPrint(DPRINT_LOG, "Monitor_MountTaskEntry() cannot open file %s (%s)\n", srcpath, strerror(errno));
						goto next_event;
					}
					close(fd);

					//if (regexec(&udisk_nopartition, event->name, 0, 0, 0) == 0)
					//{
					//	goto next_event;
					//}

					char szDevice[PATH_MAX] = { 0 };
					int intfClass;

					if (devType == DVR_DEVICE_TYPE_USBHD) {
						p->GetUSBHDDevice(srcpath, szDevice);
						intfClass = p->GetUSBInterfaceClass(szDevice);
					}

					mkdir(dstpath, 0777);
					if(is_fat_partition(srcpath))
					{
						p->NotifyFsCheck(devType, dstpath, srcpath, intfClass);
						
						//FatfsCheck(srcpath);
						int count = 0;
                        while(p->m_bHasMounted && count != 3)
                        {
                            if(strcmp(p->m_szDevice, srcpath) != 0)
                            {
                                DPrint(DPRINT_ERR, "The old device need to umount m_szDevice[%s] path[%s]!!!!", p->m_szDevice, srcpath);
                                p->m_bHasMounted = false;
                                OSA_Sleep(300);
                            }
                            OSA_Sleep(300);
                            DPrint(DPRINT_ERR, "Waitting Device umount OK (count %d)!!!!",count);
                            count++;
                        }
                        
						if (mount(srcpath, dstpath, "tfat", 0, 0) == 0) {
                            
							DPrint(DPRINT_ERR, "Monitor_MountTaskEntry mount srcpath:%s, dstpath:%s, devType:%d successfully\n", srcpath, dstpath, devType);
							p->NotifyMount(devType, dstpath, srcpath, intfClass);

							memset(p->m_szDevice, 0, sizeof(p->m_szDevice));
							memcpy(p->m_szDevice, srcpath, sizeof(srcpath));
                            p->m_bRemove = false;
							p->m_bHasMounted = true;
						}
						else {
							char *errstr = strerror(errno);
							DPrint(DPRINT_ERR, "Monitor_MountTaskEntry mount failed: srcpath:%s, dstpath:%s, errstr:%s\n", srcpath, dstpath, errstr);
							p->NotifyMountFailure(devType, dstpath, srcpath);
						}
					}
					else
					{
						DPrint(DPRINT_ERR, "UnRecognizable File System for path[%s]!!!!", srcpath);
						p->NotifyMountFailure(devType, NULL, srcpath);
						p->m_bErrFormatPlugIn = true;
					}
				}
                else if(event->mask & IN_DELETE)
                {
                    p->m_bRemove = true;
                    DPrint(DPRINT_ERR, "Monitor_MountTaskEntry remove path[%s]!!!!", srcpath);
                }
#if 0				
				else if(event->mask & IN_DELETE)
				{
					if (regexec(&udisk_nopartition, event->name, 0, 0, 0) == 0)
					{
						goto next_event;
					}
					
					char mount_point[PATH_MAX] = { 0 };					
					p->m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_ROOT_FOLDER, mount_point, PATH_MAX, NULL);
					
					if(umount2(mount_point, MNT_FORCE) == 0)
					{
						DPrint(DPRINT_ERR, "Monitor_MountTaskEntry unmount srcpath:%s, dstpath:%s, devType:%d\n", srcpath, mount_point, devType);
						p->m_bHasMounted = false;
					}
					else
					{						
						char *errstr = strerror(errno);
						DPrint(DPRINT_ERR, "Monitor_MountTaskEntry umount failed: %s %s\n", mount_point, errstr);
					}
				
					p->NotifyUnMount(devType, mount_point, mount_point);
				}
#endif				
			}
		next_event:
			regfree(&udiskregex);
			//regfree(&udisk_nopartition);

			int tmp_len = sizeof(struct inotify_event) + event->len;
			event = (struct inotify_event *)(offset + tmp_len);
			offset += tmp_len;
		}
	}

	return;
}

void DvrMonitorMount::Monitor_MountTaskEntry_TFCard(void *ptr)
{
	DvrMonitorMount *p = (DvrMonitorMount *)ptr;
	while(1)
	{
		if(!p->m_bHasMounted)
		{
			OSA_Sleep(50);//sleep 50ms
			goto end;
		}

		//if(strcmp(p->m_emmcDevice,"/dev/mmcblk1p1") == 0)
		{
			if(access("/dev/mmcblk1p1", F_OK) != 0)
			{
				DPrint(DPRINT_ERR, "Monitor_MountTaskEntry_TFCard try to umount 1   m_bHasMounted %d \n",p->m_bHasMounted);
				p->m_bRemove = true;
			}
		}
		//else if(access("/dev/mmcblk0p1", F_OK) != 0)
		///{
        //    DPrint(DPRINT_ERR, "Monitor_MountTaskEntry_TFCard try to umount 2 p->m_bHasMounted %d\n", p->m_bHasMounted);
		//	p->m_bRemove = true;
		//}

		if(p->m_bRemove && p->m_bHasMounted)	
		{
			char mount_point[PATH_MAX] = { 0 }; 				
			DVR_BOOL enable = 0;
			
			p->m_sysCtrl->StorageCard_Get(DVR_STORAGE_PROP_ROOT_FOLDER, mount_point, PATH_MAX, NULL);
			p->m_sysCtrl->MonitorStorage_Set(DVR_MONITOR_STORAGE_PROP_ENABLE_NORMAL_DETECT, &enable, sizeof(DVR_BOOL));
			p->NotifyUnMount(DVR_DEVICE_TYPE_USBHD, mount_point, mount_point);

			while(1)
			{
				DPrint(DPRINT_ERR, "Monitor_MountTaskEntry_TFCard try to umount %s\n", mount_point);
				
				if(umount2(mount_point, MNT_FORCE) == 0)
				{
					DPrint(DPRINT_ERR, "Monitor_MountTaskEntry_TFCard unmount srcpath:%s, dstpath:%s successfully\n", p->m_szDevice, mount_point);
					p->m_bHasMounted = false;
                    p->m_bRemove = false;
					break;
				}
				else
				{						
					char *errstr = strerror(errno);
					DPrint(DPRINT_ERR, "Monitor_MountTaskEntry_TFCard umount failed: %s %s\n", mount_point, errstr);
                    if(errno == EINVAL || errno == ENOENT)
                    {
                        p->m_bHasMounted = false;
                        p->m_bRemove = false;
                        break;
                    }
				}
				
				OSA_Sleep(50);//sleep 50ms
			}
		}
end:
		if(p->m_bRemove && p->m_bErrFormatPlugIn)
		{
			p->NotifyUnMount(DVR_DEVICE_TYPE_UNKNOWN, NULL, NULL);
            p->m_bRemove = false;
			p->m_bErrFormatPlugIn = false;
		}
		OSA_Sleep(20);//sleep 20ms
	}
}

void DvrMonitorMount::PurgeMountPoint()
{
	regex_t udiskregex;
	regcomp(&udiskregex, "/dev/sd[a-z]+[0-9]?$", REG_EXTENDED | REG_NOSUB);

	FILE *file = fopen("/proc/mounts", "r");
	if (file)
	{
		char line[PATH_MAX] = { 0 };
		while (fgets(line, PATH_MAX, file) != NULL) {
			char srcpath[PATH_MAX] = { 0 };
			char dstpath[PATH_MAX] = { 0 };
			char others[PATH_MAX] = { 0 };
			sscanf(line, "%s %s %s", srcpath, dstpath, others);

			if (regexec(&udiskregex, srcpath, 0, 0, 0) == 0)
			{
				bool valid = false;
				FILE *file = fopen(srcpath, "r");
				if (file != NULL)
				{
					char c;
					if (fread(&c, 1, 1, file) == 1)
						valid = true;

					fclose(file);
				}

				if (!valid) {
					umount(dstpath);
				}
			}
		}
	}
	fclose(file);

	regfree(&udiskregex);
}

void DvrMonitorMount::PurgeDeviceNode()
{
	char path[PATH_MAX] = { 0 };

	regex_t udiskregex;
	regcomp(&udiskregex, "mmcblk1p+[0-9]?$", REG_EXTENDED | REG_NOSUB);

	DIR *dir = opendir("/dev");
	struct dirent *dirent = NULL;
	while ((dirent = readdir(dir)) != NULL)
	{
		strcpy(path, "/dev/");
		strcat(path, dirent->d_name);

		if (regexec(&udiskregex, dirent->d_name, 0, 0, 0) == 0) {
			int fd = open(path, O_RDONLY);
			char c;
			int res = read(fd, &c, 1);
			close(fd);

			if (res < 0) {
				unlink(path);
			}
		}
	}

	closedir(dir);

	char devnode[PATH_MAX] = { 0 };

	dir = opendir("/sys/block");
	while ((dirent = readdir(dir)) != NULL) {
		strcpy(path, "/sys/block/");
		strcat(path, dirent->d_name);

		if (regexec(&udiskregex, dirent->d_name, 0, 0, 0) == 0) {

			strcat(path, "/dev");
			FILE *fdev = fopen(path, "r");
			if (fdev) {
				int ma = 0;
				int mi = 0;
				fscanf(fdev, "%d:%d", &ma, &mi);

				path[strlen(path) - 4] = '\0';
				DPrint(DPRINT_LOG, "DvrMonitorMount::PurgeDeviceNode() %s: major = %d minor = %d\n", path, ma, mi);

				strcpy(devnode, "/dev/");
				strcat(devnode, dirent->d_name);
				mknod(devnode, S_IFBLK, makedev(ma, mi));
			}
			fclose(fdev);

			DIR *subdir = opendir(path);
			struct dirent *subdirent = NULL;
			while ((subdirent = readdir(subdir)) != NULL) {
				if (strncmp(subdirent->d_name, dirent->d_name, strlen(dirent->d_name)) == 0) {
					char subpath[PATH_MAX] = { 0 };
					strcpy(subpath, path);
					strcat(subpath, "/");
					strcat(subpath, subdirent->d_name);
					strcat(subpath, "/dev");

					FILE *fdev = fopen(subpath, "r");
					if (fdev) {
						int ma = 0;
						int mi = 0;
						fscanf(fdev, "%d:%d", &ma, &mi);

						DPrint(DPRINT_LOG, "DvrMonitorMount::PurgeDeviceNode() %s: major = %d minor = %d\n", subpath, ma, mi);

						strcpy(devnode, "/dev/");
						strcat(devnode, subdirent->d_name);
						mknod(devnode, S_IFBLK, makedev(ma, mi));
					}

					fclose(fdev);
				}
			}

			closedir(subdir);

		}
	}

	regfree(&udiskregex);

	closedir(dir);
}

void DvrMonitorMount::EnumUSBDevice(std::list<DVR_DEVICE> *pDevices)
{
	char *path = (char *)malloc(1024);
	memset(path, 0, 1024);

	DIR *dir = opendir("/sys/bus/usb/devices");
	
	struct dirent *dirent = NULL;
	while ((dirent = readdir(dir)) != NULL) {
		if (strcmp(dirent->d_name, ".") == 0 ||
			strcmp(dirent->d_name, "..") == 0)
			continue;

		const char *controller = "usb";
		if (strncmp(dirent->d_name, controller, strlen(controller)) == 0)
			continue;

		char *colon = strchr(dirent->d_name, ':');
		if (colon && *(colon - 1) != '0') {
			int intfClass = GetUSBInterfaceClass(dirent->d_name);
			
			DVR_DEVICE device;
			memset(&device, 0, sizeof(device));
			device.eType = DVR_DEVICE_TYPE_USBHD;
			device.intfClass = intfClass;
			strcpy(device.szDevicePath, dirent->d_name);

			pDevices->push_back(device);
		}
	}

	closedir(dir);
	free(path);
}

int DvrMonitorMount::GetUSBInterfaceClass(const char *intf)
{
	char *path = (char *)malloc(PATH_MAX);
	memset(path, 0, PATH_MAX);

	sprintf(path, "/sys/bus/usb/devices/%s/bInterfaceClass", intf);
	FILE *file = fopen(path, "r");

	int intfClass = 0;
	if (file) {
		fscanf(file, "%d", &intfClass);
		fclose(file);
	}

	DPrint(DPRINT_LOG, "DvrMonitorMount::GetUSBInterfaceClass() USB interface %s class 0x%x\n",
		intf, intfClass);
	free(path);

	return intfClass;
}

void DvrMonitorMount::GetUSBHDDevice(const char *mountPoint, char *dev)
{
	char *path = (char *)malloc(PATH_MAX);
	memset(path, 0, PATH_MAX);
	char *devpath = (char *)malloc(PATH_MAX);
	memset(devpath, 0, PATH_MAX);

	const char *slash = strrchr(mountPoint, '/');
	slash++;

	sprintf(path, "/sys/block/%s", slash);

	for (unsigned i = strlen(path) - 1; i >= 0; i--) {
		if (path[i] >= '0' && path[i] <= '9')
			path[i] = '\0';
		else
			break;
	}

	readlink(path, devpath, PATH_MAX);
	char *colon = strchr(devpath, ':');
	if (colon) {
		char *slash = strchr(colon, '/');
		*slash = '\0';
		slash = strrchr(devpath, '/');
		if (slash) {
			slash++;
			strcpy(dev, slash);
		}
	}

	free(path);
	free(devpath);
}

void DvrMonitorMount::SetupSystemParameters()
{
	int fd = open(EVENTS_DFL_POLL_MSECS, O_RDWR);
	if (fd < 0) {
		DPrint(DPRINT_ERR, "DvrMonitorMount::SetupSystemParamters() failed to open file %s (%s)\n", EVENTS_DFL_POLL_MSECS, strerror(errno));
		return;
	}

	const char *delay = "500";
	if (write(fd, delay, strlen(delay)) < 0) {
		DPrint(DPRINT_ERR, "DvrMonitorMount::SetupSystemParamters() failed to write to file %s (%s)\n", EVENTS_DFL_POLL_MSECS, strerror(errno));
	}

	close(fd);
}

#endif
