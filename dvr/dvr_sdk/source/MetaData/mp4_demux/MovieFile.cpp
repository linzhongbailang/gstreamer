#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__) || defined(__ADS__)
#include <unistd.h>
#ifndef __ADS__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif
#endif

#include "Mp4Com.h"
#include "MovieFile.h"


void MovieFile::Initialize()
{
	if(m_pFileCtrl==0)
		return;

	if(m_cBuffer)
		delete m_cBuffer;
	m_cBuffer		= new char [MAX_MOVIE_BUFFER_LEN];
	if(NULL == m_cBuffer)
	{
		Reset();
	}
	m_pBufferBegin	= m_pBufferEnd = m_cBuffer;

	m_iFileSize = m_pFileCtrl->Size();
	m_pFileCtrl->Seek(0);
	FillBuffer();
}

MovieFile::MovieFile(FileCtrl * pFileCtrl)
{
	Reset();
	m_pFileCtrl = pFileCtrl;
	m_bDeleteFileCtrl = false;
	Initialize();
}

MovieFile::MovieFile(char * pathname)
{
	Reset();
	
	/** if there is no associated file */
	if(pathname==0)
		return;

	m_pFileCtrl = CreateFileCtrl();
	if(m_pFileCtrl)
	{
		m_pFileCtrl->Open(pathname, 0, FILECTRL_OPEN_READ);
		m_bDeleteFileCtrl = true;
	}

	Initialize();
}

MovieFile::~MovieFile()
{
	if(m_cBuffer)
		delete[] m_cBuffer;
	if(m_pFileCtrl && m_bDeleteFileCtrl)
		delete m_pFileCtrl;
	Reset();
}

void MovieFile::Reset()
{
	m_IoStatus = MOVIEFILE_SUCCESS;
	m_cBuffer		= 0;
	m_pBufferBegin	= 0;	
	m_pBufferEnd	= 0;	
	m_iFileSize		= 0;
	m_iFilePos		= 0;
	m_pFileCtrl		= 0;
	m_iRemainBits	= 0;
	m_iFilePos		= 0;
	m_bReadLittleEndian = false;
}

int MovieFile::FillBuffer(char *pBuf, unsigned int nFillSize)
{
	int size;

	size = m_pBufferEnd-m_pBufferBegin;

	if(pBuf==NULL && size==0 && nFillSize > MAX_MOVIE_BUFFER_LEN)
	{
		RETURN_ERROR(-1);
	}

	if(size==0)
		m_pBufferBegin = m_pBufferEnd = m_cBuffer;
	else if(m_pBufferBegin!=m_cBuffer)
	{	// caution this is expensive.
		memcpy(m_cBuffer,m_pBufferBegin,size);
		m_pBufferBegin = m_cBuffer;
		m_pBufferEnd = m_pBufferBegin+size;
	}

	int iLeft = m_iFileSize - m_iFilePos;
	if(iLeft < 0)
	{
		m_IoStatus = MOVIEFILE_CRITICAL_READERROR;
	}
	else if(iLeft == 0)
	{
		m_IoStatus = MOVIEFILE_EOF;
		RETURN_ERROR(-1);
	}
	if(NULL == m_pFileCtrl)
	{
		RETURN_ERROR(-1);
	}

	int nread = 0;
	if(size==0&&pBuf)
	{
		int ReadLen = iLeft > (int)nFillSize ? nFillSize : iLeft;
		nread = m_pFileCtrl->Read(pBuf, ReadLen);
		if(nread > 0)
		{			
			m_iFilePos += nread;
		}
		else
		{
			m_IoStatus = MOVIEFILE_EOF;
			RETURN_ERROR(-1);
		}
	}
	else
	{
		int ReadLen = nFillSize-(m_pBufferEnd-m_cBuffer);
		if (ReadLen > MAX_MOVIE_BUFFER_LEN || ReadLen <= 0)
		{
			m_IoStatus = MOVIEFILE_CRITICAL_READERROR;
			RETURN_ERROR(-1);
		}		
		ReadLen = iLeft > ReadLen ? ReadLen : iLeft;
				
		nread = m_pFileCtrl->Read(m_pBufferEnd, ReadLen);
		if(nread > 0)
		{
			m_pBufferEnd += nread;
			m_iFilePos += nread;
		}
		else
		{
			m_IoStatus = MOVIEFILE_EOF;
			RETURN_ERROR(-1);
		}		
	}
	return nread;
}

int MovieFile::GetC()
{
	return GetChar()&0xff;
}

char MovieFile::GetChar()
{
	int BufferLen = m_pBufferEnd - m_pBufferBegin;	
	if(BufferLen==0)
	{
		if(FillBuffer() < 0)
		{
			RETURN_ERROR(0);
		}
	}
	else if(BufferLen < 0)
	{
		m_IoStatus = MOVIEFILE_CRITICAL_READERROR;
		RETURN_ERROR(0);
	}
	if(m_pBufferBegin==m_pBufferEnd)
	{
		m_IoStatus = MOVIEFILE_CRITICAL_READERROR;
		RETURN_ERROR(0);
	}
	return *m_pBufferBegin++;
}

#ifndef TI_ARM
#if defined(__linux__)
long long MovieFile::Get64()
{
	char cBuf[8];
	long long ret=0;
	char *pRet = (char *)&ret;
	if(GetBuffer(cBuf, 8)!=8)
		RETURN_ERROR(-1);
	pRet[0] = cBuf[7];
	pRet[1] = cBuf[6];
	pRet[2] = cBuf[5];
	pRet[3] = cBuf[4];
	pRet[4] = cBuf[3];
	pRet[5] = cBuf[2];
	pRet[6] = cBuf[1];
	pRet[7] = cBuf[0];

	return ret;
}
#else
__int64 MovieFile::Get64()
{
	char cBuf[8];
	__int64 ret=0;
	char *pRet = (char *)&ret;
	if(GetBuffer(cBuf, 8)!=8)
		RETURN_ERROR(-1);
	pRet[0] = cBuf[7];
	pRet[1] = cBuf[6];
	pRet[2] = cBuf[5];
	pRet[3] = cBuf[4];
	pRet[4] = cBuf[3];
	pRet[5] = cBuf[2];
	pRet[6] = cBuf[1];
	pRet[7] = cBuf[0];

	return ret;
}
#endif
#else
int64s MovieFile::Get64()
{
	char cBuf[8];
	int64s ret = {0};

	char *pRet = (char *)&ret;
	if(GetBuffer(cBuf, 8)!=8)
	{
		int64s error = {-1, -1};
		return error;
	}
	pRet[0] = cBuf[7];
	pRet[1] = cBuf[6];
	pRet[2] = cBuf[5];
	pRet[3] = cBuf[4];
	pRet[4] = cBuf[3];
	pRet[5] = cBuf[2];
	pRet[6] = cBuf[1];
	pRet[7] = cBuf[0];

	return ret;
}
#endif		


int   MovieFile::Get32()
{
	char buf[4];
	if(GetBuffer(buf, 4)!=4)
		RETURN_ERROR(-1);
	return m_bReadLittleEndian?GetIntB(buf):GetIntL(buf);
}

int   MovieFile::Get24()
{
	char buf[4];
	memset(buf,0,4);
	if(m_bReadLittleEndian)
	{
		// do not convert into little endian format.
		if(GetBuffer(buf, 3)!=3)
			RETURN_ERROR(-1);
		GetIntB(buf);
	}
	if(GetBuffer(buf+1, 3)!=3)
		RETURN_ERROR(-1);
	return GetIntL(buf);
}

short MovieFile::Get16()
{
	char buf[2];
	if(GetBuffer(buf, 2)!=2)
		RETURN_ERROR(-1);
	return m_bReadLittleEndian?GetShortB(buf):GetShortL(buf);
}

int MovieFile::GetPos()
{
	return m_iFilePos-(m_pBufferEnd-m_pBufferBegin);
}


int MovieFile::SetPos(int pos)
{
	int curpos = GetPos();

	if(pos<curpos && m_pBufferBegin-m_cBuffer>curpos-pos)
	{
		m_pBufferBegin -= curpos-pos;
		return 0;
	}
	if(pos==curpos && pos<m_iFilePos)
		return 0;
	if(pos>curpos && pos<m_iFilePos)
	{
		m_pBufferBegin += pos-curpos;
		return 0;
	}
	m_pBufferEnd = m_pBufferBegin = m_cBuffer;
	if (pos > m_iFileSize)
	{
		return 0;
	}
	if(m_iFilePos!=pos)
	{
		m_iFilePos = pos;
		if(m_pFileCtrl->Seek(pos)<0)
			return -1;
	}

	//if(!m_pFileCtrl->IsStreaming())
	//	FillBuffer();
	return 0;
}

int MovieFile::GetFileSize()
{
	return m_iFileSize;
}

int MovieFile::GetBuffer(char * pBuf, int size)
{
	int bufsize, transfer, maxsize;

	maxsize = m_iFileSize-GetPos();
	if(size > maxsize)
		size = maxsize;
#if 0
	for(transfer=size;transfer>0;)
	{
		bufsize = m_pBufferEnd - m_pBufferBegin;
		if(transfer<=bufsize)
		{
			memcpy(pBuf, m_pBufferBegin, transfer);
			m_pBufferBegin += transfer;
			transfer = 0;
		}
		else
		{
			memcpy(pBuf, m_pBufferBegin, bufsize);
			pBuf += bufsize;
			transfer -= bufsize;
			m_pBufferBegin += bufsize;
			if(transfer)
			{
				int nBufferLoad = transfer > MAX_MOVIE_BUFFER_LEN ? MAX_MOVIE_BUFFER_LEN: transfer;
				if(FillBuffer(NULL, nBufferLoad) < 0)
				{
					RETURN_ERROR(0);
				}
			}
		}
	}
#else
	transfer = size;
	bufsize = m_pBufferEnd - m_pBufferBegin;
	if(transfer<=bufsize)
	{
		memcpy(pBuf, m_pBufferBegin, transfer);
		m_pBufferBegin += transfer;
	}
	else
	{
		memcpy(pBuf, m_pBufferBegin, bufsize);
		m_pBufferBegin += bufsize;
		transfer -= bufsize;
		pBuf += bufsize;

		if(transfer<DEFAULT_LOAD_BUFFER_LEN)
		{
			//for the small size, FillBuffer first to read bigger chunk of data from file
			FillBuffer();
			bufsize = m_pBufferEnd - m_pBufferBegin;
			if(bufsize<transfer)
			{
				size = (size-transfer)+bufsize;
				transfer = bufsize;
			}			
			memcpy(pBuf, m_pBufferBegin, transfer);
			m_pBufferBegin += transfer;	
		}
		else
		{			
			if(FillBuffer(pBuf, transfer) < 0)
			{
				RETURN_ERROR(0);
			}
		}
	}
#endif

	if(m_pBufferBegin > m_pBufferEnd)
	{
		fflush(stdout);
		RETURN_ERROR(-1);
	}
	return size;
}

int MovieFile::GetIntB(char * cBuf)
{
	unsigned char *us = reinterpret_cast<unsigned char *>(cBuf);
	return us[3]<<24|us[2]<<16|us[1]<<8|us[0];
}

int MovieFile::GetIntL(char * cBuf)
{
	unsigned char *us = reinterpret_cast<unsigned char *>(cBuf);
	return us[0]<<24|us[1]<<16|us[2]<<8|us[3];
}

short MovieFile::GetShortL(char * cBuf)
{
	unsigned char *us = reinterpret_cast<unsigned char *>(cBuf);
	return us[0]<<8|us[1];
}

short MovieFile::GetShortB(char * cBuf)
{
	unsigned char *us = reinterpret_cast<unsigned char *>(cBuf);
	return us[1]<<8|us[0];
}

int MovieFile::GetBits(int iBits)
{
	static const unsigned char bitmask_tab[] =
	{
		0x0,
		0x80,
		0xc0,
		0xe0,
		0xf0,
		0xf8,
		0xfc,
		0xfe,
		0xff,
	};
	int iRet = 0;
	int iRequireBits = iBits;
	unsigned char cVal=0;

	do {
		if(m_iRemainBits >= iRequireBits)
		{
			iRet <<= iRequireBits;
			cVal  = (m_CurChar & bitmask_tab[iRequireBits])&0xff;
			if(iRequireBits < 8)
				cVal >>= (8-iRequireBits);
			iRet |= cVal&0xff;
			m_iRemainBits -= iRequireBits;
			m_CurChar <<= iRequireBits;
			iRequireBits = 0;
			break;
		}
		else if(m_iRemainBits > 0)
		{
			iRet <<= m_iRemainBits;
			cVal = (m_CurChar & bitmask_tab[(unsigned char)m_iRemainBits])&0xff;
			if(m_iRemainBits < 8)
				cVal >>= (8-m_iRemainBits);
			iRet |= cVal&0xff;
			iRequireBits -= m_iRemainBits;
			m_iRemainBits = 0;
		}
		if(m_iRemainBits==0)
		{
			m_CurChar = (unsigned char)GetChar();
			if(m_IoStatus<0)
				RETURN_ERROR(-1);
			m_iRemainBits = 8;
		}
	} 
	while(iRequireBits > 0);

	return iRet;
}

int MovieFile::NextBits(int iBits)
{
	int iRemainBits =	m_iRemainBits;
	unsigned char	uChar = m_CurChar;
	int iCurPos = GetPos();
	int iNextBits = GetBits(iBits);

	if(iNextBits < 0 || m_IoStatus<0)
		RETURN_ERROR(-1);

	SetPos(iCurPos);	// slow - achung
	m_CurChar = uChar;
	iRemainBits = m_iRemainBits;
	return iNextBits;
}

int MovieFile::NextStartCode()
{
	if(m_iRemainBits<8)
		m_iRemainBits = 0; // might have 8 bits stranded, leave there.
	return GetStartCode();
}

int MovieFile::GetStartCode()
{	 // can be made more efficient and currently unaligned
	int code;

	code = GetBits(8)<<24 | GetBits(8)<<16 | GetBits(8)<<8 | GetBits(8);

	if(m_IoStatus<0)
		RETURN_ERROR(-1);

	while((code&0xffffff00)!=0x00000100)
	{
		if(GetPos()>=m_iFileSize)
			return -1;
		code = code<<8 | GetBits(8);
		if(m_IoStatus<0)
			RETURN_ERROR(-1);
	}
	return code&0xff;;
}

#if 0
// return enum picture type.
int MovieFile::SearchNextVop(int & iPos)
{
	int PictureType = UNKNOWN_VOP;
	int code;

	code = GetC()<<24 | GetC()<<16 | GetC()<<8 | GetC();

	if(m_IoStatus<0)
	{
		RETURN_ERROR(-1);
	}

	while((code&0xffffffff)!=0x000001B6)
	{
		if(GetPos()>=m_iFileSize)
		{
			RETURN_ERROR(-1);
		}
		code = code<<8 | GetC();

		if(m_IoStatus<0)
		{
			RETURN_ERROR(-1);
		}
	}


	iPos = GetPos()-4;
	PictureType = (GetC()>>6)&0x3;

	return PictureType;
}
#endif

/** stdio io file ctrl */
class StdioFileCtrl : public FileCtrl
{
public:
	StdioFileCtrl() : m_fp(0)
	{
	}
	virtual ~StdioFileCtrl()
	{
#ifndef TI_ARM	
		if(m_fp)
			fclose(m_fp);
		m_fp = 0;
#endif		
	}
	virtual int	Open(char * pathname, int filesize, int mode);
	virtual int Close();
	virtual int Read(char * buf, int nbytes);
	virtual int Write(char * buf, int nbytes);
	virtual int Seek(int pos);
	virtual int Tell();
	virtual int Size();

protected:
	FILE * m_fp;
};

#ifndef TI_ARM
int StdioFileCtrl::Open(char * pathname, int filesize, int mode)
{
	Close();
	if(mode&FILECTRL_OPEN_READ)
		m_fp = fopen(pathname, "rb");
	else if(mode&FILECTRL_OPEN_WRITE)
		m_fp = fopen(pathname, "wb");
	return 0;
}

int StdioFileCtrl::Close()
{
	if(m_fp)
	{
		fclose(m_fp);
		m_fp = 0;
	}
	return 0;
}

int StdioFileCtrl::Read(char * buf, int nbytes)
{
	if(m_fp==0 || buf==0 || nbytes < 1)
		return -1;

	return fread((unsigned char *)buf, 1, nbytes, m_fp);
}

int StdioFileCtrl::Write(char * buf, int nbytes)
{
	if(m_fp==0 || buf==0 || nbytes < 1)
		return -1;

	return fwrite((unsigned char *)buf, 1, nbytes, m_fp);
}

int StdioFileCtrl::Seek(int pos)
{
	if(m_fp==0 || pos < 0)
		return -1;

	return fseek(m_fp, pos, SEEK_SET);
}

int StdioFileCtrl::Tell()
{
	if(m_fp==0)
		return -1;

	return ftell(m_fp);
}

int StdioFileCtrl::Size()
{
	if(m_fp==0)
		return -1;
	int cur_pos = ftell(m_fp);
	fseek(m_fp, 0, SEEK_END);
	int file_size = ftell(m_fp);
	fseek(m_fp, cur_pos, SEEK_SET);
	return file_size;
}

FileCtrl * CreateFileCtrl()
{
	return new StdioFileCtrl;
}

#else

int StdioFileCtrl::Open(char * pathname, int mode)
{
	Close();
	return 0;
}

int StdioFileCtrl::Close()
{
	if(m_fp)
	{
		m_fp = 0;
	}
	return 0;
}

int StdioFileCtrl::Read(char * buf, int nbytes)
{
	if(m_fp==0 || buf==0 || nbytes < 1)
		return -1;

	return -1; 
}

int StdioFileCtrl::Write(char * buf, int nbytes)
{
	if(m_fp==0 || buf==0 || nbytes < 1)
		return -1;

	return -1; 
}

int StdioFileCtrl::Seek(int pos)
{
	if(m_fp==0 || pos < 0)
		return -1;

	return -1; 
}

int StdioFileCtrl::Tell()
{
	if(m_fp==0)
		return -1;

	return -1; 
}

int StdioFileCtrl::Size()
{
	if(m_fp==0)
		return -1;

	return -1; 
}

FileCtrl * CreateFileCtrl()
{
	return new StdioFileCtrl;
}

#endif
