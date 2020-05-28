

#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <direct.h>
#include <Windows.h>
#endif

#ifdef  __linux__
#include <sys/statfs.h>
#include <unistd.h>
#include <dirent.h>
#endif

#ifdef __TCS__
#include <unistd.h>
#endif

#include "OsdFile.h"

#ifndef WIN32
#define _O_BINARY 0
#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _O_CREAT O_CREAT
#define _O_TRUNC O_TRUNC
#define _O_RDWR O_RDWR
#define _S_IREAD (S_IRUSR | S_IRGRP | S_IROTH)
#define _S_IWRITE (S_IWUSR | S_IWGRP | S_IWOTH)
#define _open open
#define _access access
#define _stat stat
#endif

#include <string>

//////////////////////////////////////////////////////////////////////////////
///////// File find wrap
static int findnext(long handle, FileInfo* data);
static long findfirst(const char* fileName, FileInfo* data)
{
#ifdef WIN32
	_finddata_t finddata;
	long ret = _findfirst(fileName, &finddata);
	if(ret < 0)
	{
		return ret;
	}
	data->attrib = finddata.attrib;
	data->size = finddata.size;
	memcpy(data->name, finddata.name, 108 - 1);
	data->name[108 - 1] = '\0';
	data->time_write = finddata.time_write;
	data->time_access = finddata.time_access;
	data->time_create = finddata.time_create;
	return ret;
#elif defined(__linux__)
	char buf[128];
	DIR *directory;
	
	strcpy(buf, fileName);

	int len = strlen(fileName);

	if (buf[len - 1] == '*')
	{
		buf[len - 1] = '\0';
	}
	if ((directory = opendir(buf)))
	{
		if (findnext(reinterpret_cast<long>(directory), data) < 0)
		{
			closedir(directory);
			directory = 0;
			return -1;
		}
		return reinterpret_cast<long>(directory);
	}
	return -1;
#else
#error "findfirst not support!\n"
#endif
}

static int findnext(long handle, FileInfo* data)
{
#ifdef WIN32
	_finddata_t finddata;
	long ret = _findnext(handle, &finddata);
	if(ret < 0)
	{
		return ret;
	}
	data->attrib = finddata.attrib;
	data->size = finddata.size;
	memcpy(data->name, finddata.name, 108 - 1);
	data->name[108 - 1] = '\0';
	data->time_write = finddata.time_write;
	data->time_access = finddata.time_access;
	data->time_create = finddata.time_create;
	return ret;
#elif defined(__linux__)
	DIR *directory = reinterpret_cast<DIR *>(handle);

	if (directory)
	{
		struct dirent *dirent;

again:
		if ((dirent = readdir(directory)))
		{
			struct stat fileStatus;
			char fullName[260];
			if(dirent->d_reclen >= 260)
			{
				printf("findnext dir too long : %s\n", dirent->d_name);
				goto again;
			}
			sprintf(fullName, "%s/%s", data->name, dirent->d_name);
			(void)stat(fullName, &fileStatus);
			strcpy(data->name, dirent->d_name);
			data->size = fileStatus.st_size;	
			data->time_write = fileStatus.st_mtime;
			data->time_access = fileStatus.st_atime;
			data->time_create = fileStatus.st_ctime;
			data->attrib = 0;
			if (S_ISDIR(fileStatus.st_mode)) data->attrib |= CFile::directory;
			if (!(S_IWUSR & fileStatus.st_mode)) data->attrib |= CFile::readOnly;
			return 0;
		}
	}
	return -1;
#else
#error "findnext not support!"
#endif
}

static int findclose(long handle)
{
#ifdef WIN32
	return _findclose(handle);
#elif defined(__linux__)
	
	DIR *directory = reinterpret_cast<DIR *>(handle);

	assert(handle == reinterpret_cast<long>(directory));
	if (directory)
	{
		closedir(directory);
		return 0;
	}
	return -1;
#else
#error "findclose not support!"
#endif
}

#ifndef WIN32
static int _mkdir( const char *dirname)
{
	return mkdir((char *)dirname, _S_IREAD | _S_IWRITE);
}

static int _rmdir( const char *dirname)
{
	return rmdir((char *)dirname);
}
#endif

static int _statfs(const char *path, DVR_U64* userFreeBytes, DVR_U64* totalBytes, DVR_U64* totalFreeBytes)
{
#ifdef WIN32
	GetDiskFreeSpaceEx((LPCWSTR)path,
		(PULARGE_INTEGER)userFreeBytes,
		(PULARGE_INTEGER)totalBytes,
		(PULARGE_INTEGER)totalFreeBytes); 
#elif defined(__GNUC__)
	struct statfs info;

	*userFreeBytes = 0;
	*totalBytes = 0;
	*totalFreeBytes = 0;
	if (statfs(path, &info) == 0)
	{
		*userFreeBytes = (DVR_U64)(info.f_bsize) * info.f_bavail;
		*totalBytes = (DVR_U64)(info.f_bsize) * info.f_blocks;
		*totalFreeBytes = (DVR_U64)(info.f_bsize) * info.f_bfree;
	}
#endif
	return 0;
}

static int __stat( const char *path, FileInfo * info)
{
	int ret;
	struct _stat s;

	ret = _stat(path, &s);
	if(ret != 0)
	{
		return ret;
	}

	strcpy(info->name, path);
	info->attrib = s.st_mode;
	info->time_write = s.st_mtime;
	info->time_access = s.st_atime;
	info->time_create = s.st_ctime;
	info->size = s.st_size;

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
///////   various io entries
static const int fsMaxOptNum = 32;
static char fsPaths[fsMaxOptNum][32] = {};
static FSOperations fsOpts[fsMaxOptNum] = 
{
	{
		fopen,
			fclose,
			fread,
			fwrite,
			fflush,
			fseek,
			ftell,
			fgets,
			fputs,
			rename,
			remove,
			findfirst,
			findnext,
			findclose,
			_mkdir,
			_rmdir,
			_statfs,
			_access,
			__stat,
	},
};

void hookFS(const char* path, const FSOperations* opts)
{
	int i;

	for(i = 1; i < fsMaxOptNum; i++)
	{
		if(opts)
		{
			if(strlen(fsPaths[i]) == 0)
			{
				strcpy(fsPaths[i], path);
				fsOpts[i] = *opts;
				break;
			}
		}
		else
		{
			if(strcmp(path, fsPaths[i]) == 0)
			{
				strcpy(fsPaths[i], "");
				break;
			}
		}
	}

}

static FSOperations* findOpts(const char* pFileName)
{
	int i;

	for(i = 1; i < fsMaxOptNum; i++)
	{
		int len = strlen(fsPaths[i]);
		if(len != 0 && strncmp(pFileName, fsPaths[i], len) == 0)
		{
			break;
		}
	}

	if(i == fsMaxOptNum)
	{
		i = 0;
	}
	return &fsOpts[i];
}

///////////////////////////////////////////////////////////////////////////////
///////   CFile implement
CFile::CFile()
{
	m_internal = new FileInternal;
	m_internal->m_file = NULL;
	m_internal->m_buffer = NULL;
	m_internal->m_length = 0;
	m_internal->m_position = 0;
	m_internal->m_opt = &fsOpts[0];
}

CFile::~CFile()
{
	if(m_internal->m_file)
	{
		Close();
	}
	if(m_internal->m_buffer)
	{
		UnLoad();
	}
	delete m_internal;
}

bool CFile::Open(const char* pFileName, unsigned int dwFlags /* = modeReadWrite */)
{
	// 关闭已经打开的文件，起到容错的作用
	if(m_internal->m_file)
	{
		Close();
	}
	if(m_internal->m_buffer)
	{
		UnLoad();
	}

	int oflag = _O_BINARY;
	const char* mode = "";
	int fd;

	m_internal->m_opt = findOpts(pFileName);
	if(m_internal->m_opt != &fsOpts[0])
	{
		switch (dwFlags & 0xf) 
		{
		case modeRead:
			mode = "rb";
			break;
		case modeWrite:
			if (dwFlags & modeNoTruncate)
			{
				mode = "ab";
			}
			else
			{
				mode = "wb";
			}
			break;
		case modeReadWrite:
			if(dwFlags & modeCreate)
			{
				mode = "wb+";
			}
			else
			{
				mode = "rb+";
			}
		}
		m_internal->m_file = m_internal->m_opt->fopen(pFileName, mode);
		if(!m_internal->m_file)
		{
			return false;
		}

		goto init;
	}

	switch (dwFlags & 0xf) 
	{
	case modeRead:
		oflag |= _O_RDONLY;
		break;
	case modeWrite:
		oflag |= _O_WRONLY;
		break;
	case modeReadWrite:
		oflag |= _O_RDWR;
		break;
	default:
		break;
	}

	if(dwFlags & modeCreate)
	{
		oflag |= _O_CREAT;
		if(!(dwFlags & modeNoTruncate))
		{
			oflag |= _O_TRUNC;
		}
	}

	if(dwFlags & osNoBuffer)
	{
	}

	fd = _open(pFileName, oflag, _S_IREAD | _S_IWRITE);

	if(fd == -1)
	{
		return false;
	}

	switch (dwFlags & 0xf) 
	{
	case modeRead:
		mode = "rb";
		break;
	case modeWrite:
		if (dwFlags & modeNoTruncate)
		{
			mode = "ab";
		}
		else
		{
			mode = "wb";
		}
		break;
	case modeReadWrite:
		if(dwFlags & modeCreate)
		{
			mode = "wb+";
		}
		else
		{
			mode = "rb+";
		}
	}

	m_internal->m_file = fdopen(fd, mode);

	if(!m_internal->m_file)
	{
		return false;
	}
	
	setvbuf(m_internal->m_file, NULL, _IOFBF, 16384); 

init:
	long pos = m_internal->m_opt->ftell(m_internal->m_file);
	m_internal->m_position = pos < 0 ? 0 : pos;

	FileInfo info;

	if(m_internal->m_opt->stat(pFileName, &info) == 0)
	{
		m_internal->m_length = (unsigned int)info.size;
	}
	else
	{
		m_internal->m_length = 0; // 有可能还没有创建起来
	}

	return true;
}

bool CFile::IsOpened()
{
	return m_internal->m_file != 0;
}

void CFile::Close()
{
	if(!m_internal->m_file)
	{
		return;
	}

	m_internal->m_opt->fclose(m_internal->m_file);

	m_internal->m_file = NULL;
}

unsigned char * CFile::Load(const char* pFileName)
{
	unsigned int ret;

	assert(!m_internal->m_buffer);
	if(Open(pFileName, modeRead | modeNoTruncate))
	{
		if(m_internal->m_length)
		{
			m_internal->m_buffer = new unsigned char[m_internal->m_length];
			if(m_internal->m_buffer)
			{
				Seek(0, begin);
				ret = Read(m_internal->m_buffer, m_internal->m_length);
				assert(ret == m_internal->m_length);
			}
		}
	}
	return m_internal->m_buffer;
}

void CFile::UnLoad()
{
	if(m_internal->m_buffer)
	{
		Close();

		delete []m_internal->m_buffer;
		m_internal->m_buffer = NULL;
	}
}

unsigned int CFile::Read(void *pBuffer, unsigned int dwCount)
{
	if(!m_internal->m_file)
	{
		return 0;
	}

	size_t len = m_internal->m_opt->fread(pBuffer, 1, dwCount, m_internal->m_file);
	m_internal->m_position += len;

	return len;
}

unsigned int CFile::Write(void *pBuffer, unsigned int dwCount)
{
	if(!m_internal->m_file)
	{
		return 0;
	}

	size_t len = m_internal->m_opt->fwrite(pBuffer, 1, dwCount, m_internal->m_file);
	m_internal->m_position += len;

	if(m_internal->m_length < m_internal->m_position)
	{
		m_internal->m_length = m_internal->m_position;
	}

	return len;
}

void CFile::Flush()
{
	if(!m_internal->m_file)
	{
		return;
	}
	m_internal->m_opt->fflush(m_internal->m_file);
#ifdef __linux__
	fsync(fileno(m_internal->m_file));
#endif
}

unsigned int CFile::Seek(long lOff, unsigned int nFrom)
{
	if(!m_internal->m_file)
	{
		return 0;
	}
	int origin = 0;
	unsigned int position = 0;

	switch(nFrom){
	case begin:
		position = lOff;
		origin = SEEK_SET;
		break;
	case current:
		position = m_internal->m_position + lOff;
		origin = SEEK_CUR;
		break;
	case end:
		position = m_internal->m_length + lOff;
		origin = SEEK_END;
		break;
	}

	// 如果文件偏移一样，直接返回，提高效率
	if(position == m_internal->m_position)
	{
		return position;
	}

	if(m_internal->m_opt->fseek(m_internal->m_file, lOff, origin) >= 0)
	{
		int pos = m_internal->m_opt->ftell(m_internal->m_file);
		m_internal->m_position = pos < 0 ? 0 : pos;

		return m_internal->m_position;
	};

	return 0;
}

unsigned int CFile::GetPosition()
{
	if(!m_internal->m_file)
	{
		return 0;
	}

	return m_internal->m_position;
}

unsigned int CFile::GetLength()
{
	if(!m_internal->m_file)
	{
		return 0;
	}

	return m_internal->m_length;
}

char * CFile::Gets(char *s, int size)
{
	if(!m_internal->m_file)
	{
		return 0;
	}

	char* p = m_internal->m_opt->fgets(s, size, m_internal->m_file);
	if(p)
	{
		m_internal->m_position += strlen(p);
	}

	return p;
}

int CFile::Puts(const char *s)
{
	if(!m_internal->m_file)
	{
		return 0;
	}

	int ret = m_internal->m_opt->fputs(s, m_internal->m_file);
	if(ret != EOF)
	{
		m_internal->m_position += strlen(s);
	}

	return ret;
}

bool CFile::Rename(const char* oldName, const char* newName)
{
	FSOperations* opt = findOpts(oldName);

	return (opt->rename(oldName, newName) == 0);
}

bool CFile::Remove(const char* fileName)
{
	FSOperations* opt = findOpts(fileName);

	return (opt->remove(fileName) == 0);
}

bool CFile::MakeDirectory(const char* dirName)
{
	FSOperations* opt = findOpts(dirName);

	return(opt->mkdir(dirName) == 0);
}

bool CFile::RemoveDirectory(const char* dirName)
{
	FSOperations* opt = findOpts(dirName);

	return (opt->rmdir(dirName) == 0);
}

bool CFile::StatFS(const char* path, DVR_U64* userFreeBytes, DVR_U64* totalBytes, DVR_U64* totalFreeBytes)
{
	FSOperations* opt = findOpts(path);

	return (opt->statfs(path, userFreeBytes, totalBytes, totalFreeBytes) == 0);
}

bool CFile::Access(const char* path, int mode)
{
	//printf("1313line[%d]path[%s]\n",__LINE__, path);
	//int ret;
	FSOperations* opt = findOpts(path);
	////printf("1313line[%d]path[%s]mode[%d]\n",__LINE__, path, mode);
	//ret = opt->access(path, mode);
	//ret = access(path, mode);
	//printf("1313line[%d]path[%s]ret[%d]\n",__LINE__, path, ret);
	//return (ret == 0);
	return (opt->access(path, mode) == 0);
}

bool CFile::Stat(const char* path, FileInfo* info)
{
	FSOperations* opt = findOpts(path);

	return (opt->stat(path, info) == 0);
}

///////////////////////////////////////////////////////////////////////////////
///////   CFileFind implement
CFileFind::CFileFind()
{
	m_internal = new FileFindInternal;
	m_internal->m_opt = &fsOpts[0];
	m_internal->m_handle = -1;
}

CFileFind::~CFileFind()
{
	close();
	delete m_internal;
}

bool CFileFind::findFile(const char* fileName)
{
	// 关闭已经打开的句柄，起到容错的作用
	close();
	
	const char* p = fileName + strlen(fileName);
	while(*p != '/' && p != fileName)
	{
		p--;
	}
	m_internal->m_path.assign(fileName, p - fileName + 1);

	m_internal->m_opt = findOpts(fileName);
	strcpy(m_internal->m_fileInfo.name, m_internal->m_path.c_str());
	return ((m_internal->m_handle = m_internal->m_opt->findfirst(fileName, &m_internal->m_fileInfo)) != -1);
}

bool CFileFind::findNextFile()
{
	strcpy(m_internal->m_fileInfo.name, m_internal->m_path.c_str());
	return (m_internal->m_opt->findnext(m_internal->m_handle, &m_internal->m_fileInfo) != -1);
}

void CFileFind::close()
{
	if(m_internal->m_handle < 0)
	{
		return;
	}
	m_internal->m_opt->findclose(m_internal->m_handle);
	m_internal->m_handle = -1;
}

unsigned int CFileFind::getLength()
{
	return (uint)m_internal->m_fileInfo.size;
}

std::string CFileFind::getFileName()
{
	return m_internal->m_fileInfo.name;
}

std::string CFileFind::getFilePath()
{
	return m_internal->m_path + m_internal->m_fileInfo.name;
}

bool CFileFind::isReadOnly()
{
	return ((m_internal->m_fileInfo.attrib & CFile::readOnly) != 0);
}

bool CFileFind::isDirectory()
{
	return ((m_internal->m_fileInfo.attrib & CFile::directory) != 0);
}

bool CFileFind::isHidden()
{
	return ((m_internal->m_fileInfo.attrib & CFile::hidden) != 0);
}

bool CFileFind::isNormal()
{
	return (m_internal->m_fileInfo.attrib == CFile::normal);
}
