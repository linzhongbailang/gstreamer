
#include <osa_fs.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#else
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif

DVR_U32 DVR::OSA_FS_GetFreeSpace(const char *szDrive)
{
	GFileInfo *fileinfo;
	guint64 free = 0;

	GFile *file = g_file_new_for_path(szDrive);
    if (file != NULL)
    {
        fileinfo = g_file_query_filesystem_info(file, G_FILE_ATTRIBUTE_FILESYSTEM_FREE, NULL, NULL);
        if (fileinfo != NULL)
        {
            free = g_file_info_get_attribute_uint64(fileinfo, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
        }
    }

    if (file != NULL)
    {
        g_object_unref(file);
    }
    if (fileinfo != NULL)
    {
        g_object_unref(fileinfo);
    }

	return (DVR_U32)(free / 1024);
}

DVR_U32 DVR::OSA_FS_GetTotalSpace(const char *szDrive)
{
	GFileInfo *fileinfo;
	guint64 total = 0;

    GFile *file = g_file_new_for_path(szDrive);
    if (file != NULL)
    {
        fileinfo = g_file_query_filesystem_info(file, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE, NULL, NULL);
        if (fileinfo != NULL)
        {
            total = g_file_info_get_attribute_uint64(fileinfo, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE);
        }
    }

    if (file != NULL)
    {
        g_object_unref(file);
    }
    if (fileinfo != NULL)
    {
        g_object_unref(fileinfo);
    }

	return (DVR_U32)(total / 1024);
}

DVR_U32 DVR::OSA_FS_GetUsedSpace(const char *szDrive)
{
	GFileInfo *fileinfo;
	guint64 used = 0;

    GFile *file = g_file_new_for_path(szDrive);
    if (file != NULL)
    {
        fileinfo = g_file_query_filesystem_info(file, G_FILE_ATTRIBUTE_FILESYSTEM_USED, NULL, NULL);
        if (fileinfo != NULL)
        {
            used = g_file_info_get_attribute_uint64(fileinfo, G_FILE_ATTRIBUTE_FILESYSTEM_USED);
        }
    }

    if (file != NULL)
    {
        g_object_unref(file);
    }
    if (fileinfo != NULL)
    {
        g_object_unref(fileinfo);
    }

	return (DVR_U32)(used / 1024);
}

DVR_RESULT DVR::OSA_FileCopy(char *srcFn, char *dstFn)
{
	int res = DVR_RES_SOK;

	GError *error = NULL;

	GFile *source_file = g_file_new_for_path(srcFn);
	GFile *dest_file = g_file_new_for_path(dstFn);

    if (!g_file_query_exists(dest_file, NULL))
    {
        g_file_copy(source_file, dest_file, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error);
        if (error)
        {
            res = DVR_RES_EFAIL;
        }
    }

	g_object_unref(source_file);
	g_object_unref(dest_file);

	return res;
}

DVR_RESULT DVR::OSA_FileMove(char *srcFn, char *dstFn)
{
	int res = DVR_RES_SOK;
	gboolean ret = TRUE;
	GError *error = NULL;

	GFile *source_file = g_file_new_for_path(srcFn);
	GFile *dest_file = g_file_new_for_path(dstFn);

    if (!g_file_query_exists(dest_file, NULL))
    {
    	ret = g_file_move(source_file, dest_file, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &error);
		if(!ret || error)
    	{
    	    printf("OSA_FileMove ------ %s\n",strerror(errno));
			res = DVR_RES_EFAIL;
		}
    }

	g_object_unref(source_file);
	g_object_unref(dest_file);

	return res;
}

DVR_RESULT DVR::OSA_FileDel(char *filename)
{
	int res = DVR_RES_SOK;
	GError *error = NULL;

	GFile *file = g_file_new_for_path(filename);
	if(file != NULL)
	{
		g_file_delete(file, NULL, &error);
		if (error)
		{
			res = DVR_RES_EFAIL;
			DPrint(DPRINT_INFO, "OSA_FileDel: Failed to Delete [%s]", filename);
		}
		else
		{
			DPrint(DPRINT_INFO, "OSA_FileDel: Delete [%s] successfully", filename);
		}
		
		g_object_unref(file);
	}
	else
	{
		res = DVR_RES_EFAIL;
	}

	return res;
}

DVR_RESULT DVR::OSA_MkDir(const char *dirName)
{
	int res = DVR_RES_SOK;
	
	g_mkdir_with_parents(dirName, 0755);
	
	return res;
}

DVR_RESULT DVR::OSA_GetFileStatInfo(const char* fileName, DVR_U64 *pullFileSize, DVR_U64 *pullModifyTime)
{
    GStatBuf stat_buf;
	
	if (fileName == NULL)
		return DVR_RES_EINVALIDARG;

	if (g_lstat(fileName, &stat_buf) == -1)
	{
		if(pullFileSize != NULL)
			*pullFileSize = 0;

		if(pullModifyTime != NULL)
			*pullModifyTime = 0;
		
		return DVR_RES_EFAIL;
	}

	if(pullFileSize != NULL)
		*pullFileSize = (stat_buf.st_size >> 10);

	if(pullModifyTime != NULL)
		*pullModifyTime = stat_buf.st_mtim.tv_sec;

	return DVR_RES_SOK;
}


namespace DVR
{
    class CheckSpace
    {
    public:
        CheckSpace(const char *filepath, DVR_BOOL *pCardPlugOut)
        {
            m_KBytes = 0;
			m_pCardPlugOut = pCardPlugOut;
            memset(m_FilePath, 0, sizeof(m_FilePath));
            strcpy(m_FilePath, filepath);
            Check(filepath);
            AddBytes(4096); // Add the folder itself
        }

        DVR_U32 GetKBytes()
        {
            return m_KBytes;
        }

    private:
        DVR_U32 m_KBytes; //Byte
        DVR_BOOL *m_pCardPlugOut;
        char m_FilePath[128];

        void AddBytes(DVR_U32 bytes)
        {
            m_KBytes += bytes/1024;
        }

        void Check(const char *dirPath)
        {
            GDir *dir = NULL;
            GStatBuf statbuf;
			DVR_U32 st_size = 0;

            dir = g_dir_open(dirPath, 0, NULL);
            if (dir)
            {
                const gchar *dir_ent;
                while ((dir_ent = g_dir_read_name(dir)))
                {
    				gchar destfile[APP_MAX_FN_SIZE] = {0};
					strncpy(destfile, dirPath, APP_MAX_FN_SIZE);    
                	g_strlcat(destfile, dir_ent, APP_MAX_FN_SIZE);
					
                    g_lstat(destfile, &statbuf);
                    if (S_ISDIR(statbuf.st_mode))
                    {
                        if (strcmp(".", destfile) == 0 ||
                            strcmp("..", destfile) == 0)
                        {
                            continue;
                        }

						st_size = (DVR_U32)statbuf.st_size;
                        AddBytes(st_size);
                        Check(destfile);
                    }
                    else
                    {
                   	 	st_size = (DVR_U32)statbuf.st_size;
                        AddBytes(st_size);
                    }

					if(m_pCardPlugOut != NULL)
					{
						if(*m_pCardPlugOut == TRUE)
						{
							break;
						}
					}
                }

                g_dir_close(dir);
            }
        }
    };
}

DVR_U32 DVR::OSA_FS_CheckUsedSpace(const char *szFilePath, DVR_BOOL *pCardPlugOut)
{
    if (szFilePath == NULL)
        return DVR_RES_EPOINTER;

    CheckSpace cs = CheckSpace(szFilePath, pCardPlugOut);

    return (DVR_U32)(cs.GetKBytes());
}

#ifdef WIN32

#include <windows.h>
#include <ShlObj.h>

typedef DWORD(WINAPI *PFNSHFORMATDRIVE)(HWND hwnd, UINT drive, UINT fmtID, UINT options);

DVR_RESULT DVR::OSA_CardFormat(char *szDevicePath, char *szMountPoint)
{
	int res = DVR_RES_SOK;

	WCHAR cDisk = L'F';

	HINSTANCE hDll = LoadLibraryW(L"Shell32.dll");
	if (hDll == NULL)
		return DVR_RES_EFAIL;
	PFNSHFORMATDRIVE pFnSHFormatDrive = (PFNSHFORMATDRIVE)GetProcAddress(hDll, "SHFormatDrive");
	if (pFnSHFormatDrive == NULL)
	{
		FreeLibrary(hDll);
		return DVR_RES_EFAIL;
	}   
	//pFnSHFormatDrive(NULL,cDisk-L'A',SHFMT_ID_DEFAULT,0);  
	FreeLibrary(hDll); 

	return res;
}

#else

#include<sys/types.h>
#include<sys/wait.h>
#include <sys/mount.h>
#include <regex.h>
#include <errno.h>
#include <osa_mark.h>

//#define MKDOSFS_PATH	"/usr/sbin/mkdosfs"
#define MKDOSFS_PATH   "/usr/sbin/mkfatfs"



DVR_RESULT DVR::OSA_CardFormat(char *szDevicePath, char *szMountPoint)
{
	int res = DVR_RES_SOK;

    DPrint(DPRINT_INFO,"enter osa_cardformat \n");
	if(szDevicePath == NULL)
	{
		DPrint(DPRINT_ERR, "OSA_CardFormat: Invalid devpath");
		return DVR_RES_EFAIL;
	}
    DVR_U32 type = DVR_SD_SPEED_NONE;
    DVR::OSA_GetSDSpeekType(szDevicePath, szMountPoint, &type);
	if(szMountPoint != NULL)
	{
		// if dev has been mounted, umount
		int umount_ret = 0;
		umount_ret = umount2(szMountPoint, MNT_FORCE);
		DPrint(DPRINT_INFO,"umount point %s \n",szMountPoint);
		if (umount_ret != 0) 
		{
			char *errstr = strerror(errno);
			DPrint(DPRINT_ERR, "OSA_CardFormat: umount failed: %s %s\n", szMountPoint, errstr);
			return DVR_RES_EFAIL;
		}
	}

	pid_t pid = fork();
	if(pid == -1)
	{
		DPrint(DPRINT_INFO, "OSA_CardFormat: fork error\n");
		res = DVR_RES_EFAIL;
	}
	else if(pid == 0)
	{
		// mkdosfs
		char *args[4];

		args[0] = MKDOSFS_PATH;
		args[1] = szDevicePath;
		args[2] = NULL;
		args[3] = NULL;

		if(execv(args[0], args) == -1)
		{
			res = DVR_RES_EFAIL;
		}

		exit(res);
	}

	waitpid(pid, &res, 0);

	int mount_ret = 0;
    
	if(szMountPoint != NULL)
	{
		mount_ret = mount(szDevicePath, szMountPoint, "tfat", 0, 0);
		DPrint(DPRINT_INFO, "OSA_CardFormat: mount srcpath:%s, dstpath:%s successfully\n", szDevicePath, szMountPoint);
	}
	else
	{
		mount_ret = mount(szDevicePath, SD_MOUNT_POINT, "tfat", 0, 0);
		DPrint(DPRINT_INFO, "OSA_CardFormat: mount srcpath:%s, dstpath:%s successfully\n", szDevicePath,SD_MOUNT_POINT);
	}

	if(mount_ret != 0)
	{
		char *errstr = strerror(errno);
		DPrint(DPRINT_ERR, "OSA_CardFormat: mount failed: srcpath:%s, dstpath:%s, errstr:%s\n", szDevicePath, szMountPoint, errstr);		
	}
    else
    {
        if(type != DVR_SD_SPEED_NONE)
            DVR::OSA_MarkSDSpeed(szDevicePath, szMountPoint, (DVR_SDSPEED_TYPE)type);
    }

	return res;
}

#endif
