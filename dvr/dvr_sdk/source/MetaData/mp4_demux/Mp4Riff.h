#ifndef MP4RIFF_H_
#define MP4RIFF_H_

#include "Mp4Com.h"

/** qcp-file = riff-qlcm fmt vrat [labl] [offs] data [cnfg]
*/
enum RiffType
{
	RIFF_RIFF = 'R'<<24|'I'<<16|'F'<<8|'F',
		RIFF_FMT  = 'f'<<24|'m'<<16|'t'<<8|' ',
		RIFF_VRAT = 'v'<<24|'r'<<16|'a'<<8|'t',
		RIFF_LABL = 'l'<<24|'a'<<16|'b'<<8|'l',
		RIFF_OFFS = 'o'<<24|'f'<<16|'f'<<8|'s',
		RIFF_DATA = 'd'<<24|'a'<<16|'t'<<8|'a',
		RIFF_CNFG = 'c'<<24|'n'<<16|'f'<<8|'g',
		RIFF_TEXT = 't'<<24|'e'<<16|'x'<<8|'t',
		RIFF_QLCM = 'Q'<<24|'L'<<16|'C'<<8|'M',
		RIFF_WAVE = 'W'<<24|'A'<<16|'V'<<8|'E',
};

class Mp4File;
/** Riff File abstraction
*/
class Mp4Riff : public Mp4Com
{
public:
	Mp4Riff(int aRiffType,int offset,int size,Mp4Riff * aContainer) :
	  Mp4Com(MP4RIFF),
		  ChunkId(aRiffType), 
		  m_pContainer(aContainer) 
	  {
		  m_bNotLoaded = true;
		  m_iBeginOffset = offset;
		  m_iSize = size;
	  };
	  
	  /** Load box and its child into memory.
	  */
	  virtual int 	Load(Mp4File * pMp4File);
	  virtual void 	Dump(Formatter* pFormatter);
	  virtual void 	DumpHeader(Formatter* pFormatter)
	  {
		  if(!pFormatter->UseDisplay())
		  {
			  m_iBeginOffset = pFormatter->GetPos();
			  if(m_bNotLoaded)
				  return;
		  }
		  pFormatter->Print("<");
		  pFormatter->PrintInt((char *)&ChunkId,4);
		  pFormatter->Print(" offset=\"%d\" size=\"%d\">\n",m_iBeginOffset,m_iSize);
		  pFormatter->Print("<chunksize val=\"%d\"/>\n",ChunkSize);
		  if(!pFormatter->UseDisplay())
		  {
			  pFormatter->ConvertToBig(true);
			  pFormatter->Put(0,(long)ChunkId);
			  pFormatter->ConvertToBig(false);
			  pFormatter->Put(0,(long)ChunkSize);
		  }
		  Dump(pFormatter);
	  }
	  
	  virtual void 	DumpFooter(Formatter* pFormatter)
	  {
		  if(!pFormatter->UseDisplay() && m_bNotLoaded)
			  return;
		  
		  pFormatter->Print("</");
		  pFormatter->PrintInt((char *)&ChunkId,4);
		  pFormatter->Print(">\n");
	  }
	  static int GetChunkIdNSize(Mp4File * pMp4File,int *pId, int * pSize);
	  /** set node size and chunk size */
	  void SetSize(int iSize);
	  
	  long ChunkId;	
	  long ChunkSize;
	  
protected:
	Mp4Riff * m_pContainer;
};

/** riff-wave fmt */
class RiffFmt : public Mp4Riff
{
public:
	RiffFmt(int offset, int size, Mp4Riff * aContainer=0)
		: Mp4Riff(RIFF_FMT,offset,size,aContainer)
	{
	}
	virtual int 	Load(Mp4File * pMp4File);
	virtual void 	Dump(Formatter* pFormatter);
	
	/** PCM=1 other values indicate some form of compression */
	short	AudioFormat;
	/** Mono=1, Stereo=2 */
	short	NumChannels;
	/** 8000 ... */
	long	SampleRate;
	/** = SampleRate * NumChannels * BitsPerSample/8 */
	long	ByteRate;
	/** Number of bytes for one sample, NumChannels * BitsPerSample/8 */
	short	BlockAlign;
	/** 8, 16, ... */
	short	BitsPerSample;
};

/** riff-qlcm's fmt - RFC 3625 */
class RiffQcpFmt : public Mp4Riff
{
public:
	RiffQcpFmt(int offset, int size, Mp4Riff * aContainer=0)
		: Mp4Riff(RIFF_FMT,offset,size,aContainer)
	{
	}
	virtual int 	Load(Mp4File * pMp4File);
	virtual void 	Dump(Formatter* pFormatter);
	
	/** major version number of QCP format
	set as "2"
	*/
	char Major;
	/** set to "0" */
	char Minor;
	/** UINT32 UINT16 UNINT16 8OCTET */
	char CodecGuid[17];
	/** QCELP-13K: 1 or 2, EVRC, and SMV: 1 */
	short CodecVersion;
	/** 8OCTET */
	char CodecName[81];
	/** average data rate in bits per second */
	short AverageBps;
	/** largest possible packet size */
	short PacketSize;
	/** # samples encoded in every packet */
	short BlockSize;
	/** speech sampling rate */
	short SamplingRate;
	/** number of bits for each sample */
	short SamplingSize;
	/** # possible rates */
	long  NumRates;
	/** Rate-map-table {rate-size,rate-octet}[8] */
	char RateSize[8];
	char RateOctet[8];
};

/** actual data, pcm, qcelp, etc. */
class RiffData : public Mp4Riff
{
public:
	RiffData(int offset, int size, Mp4Riff * aContainer=0)
		: Mp4Riff(RIFF_DATA,offset,size,aContainer)
	{
		m_pcmfile[0] = 0;
	}
	/** do not load/dump this node */
	virtual int 	Load(Mp4File * pMp4File)
	{
		ChunkSize = m_iSize - 8;
		m_bNotLoaded = true;
		return 0;
	}
	virtual void 	Dump(Formatter* pFormatter);
	int SetPcmFile(char * pcmfile)
	{
		m_bNotLoaded = false;
		strcpy(m_pcmfile, pcmfile);
		return 0;
	}
	
	/** write riffdata into a file */
	int Write(Mp4File * pMp4File, char *pathname);
protected:
	char m_pcmfile[256];
};

/** QCELP's VRAT */
class RiffVrat : public Mp4Riff
{
public:
	RiffVrat(int offset, int size, Mp4Riff * aContainer=0)
		: Mp4Riff(RIFF_VRAT,offset,size,aContainer)
	{
	}
	virtual int 	Load(Mp4File * pMp4File);
	virtual void 	Dump(Formatter* pFormatter);
	
	long VarRateFlag;
	long SizeInPackets;
};

/** RIFF labl class */
class RiffLabl : public Mp4Riff
{
public:
	RiffLabl(int offset, int size, Mp4Riff * aContainer=0)
		: Mp4Riff(RIFF_LABL,offset,size,aContainer)
	{
	}
	virtual int 	Load(Mp4File * pMp4File);
	virtual void 	Dump(Formatter* pFormatter);
	
	/* 48 octects */
	char Label[48+1]; 
};

/** RIFF offs class */
class RiffOffs : public Mp4Riff
{
public:
	RiffOffs(int offset, int size, Mp4Riff * aContainer=0)
		: Mp4Riff(RIFF_OFFS,offset,size,aContainer),
		Offset(0)
	{
	}
	~RiffOffs()
	{
		if(Offset)
			delete [] Offset;
	}
	virtual int 	Load(Mp4File * pMp4File);
	virtual void 	Dump(Formatter* pFormatter);
	
	long StepSize;
	long NumOffsets;
	long * Offset;
};

/** RIFF cnfg class */
class RiffCnfg : public Mp4Riff
{
public:
	RiffCnfg(int offset, int size, Mp4Riff * aContainer=0)
		: Mp4Riff(RIFF_CNFG,offset,size,aContainer)
	{
	}
	virtual int 	Load(Mp4File * pMp4File);
	virtual void 	Dump(Formatter* pFormatter);
	
	/** config is a bitmapped configruation word, for application use */
	short Config;
};

/** RIFF text class */
class RiffText : public Mp4Riff
{
public:
	RiffText(int offset, int size, Mp4Riff * aContainer=0)
		: Mp4Riff(RIFF_TEXT,offset,size,aContainer)
	{
	}
	virtual int 	Load(Mp4File * pMp4File);
	virtual void 	Dump(Formatter* pFormatter);
	
	char Text[4096];
};

/** RIFF chunk descriptor */
class RiffRiff : public Mp4Riff
{
public:
	RiffRiff(int offset,int size,Mp4Riff * aContainer=0)
		: Mp4Riff(RIFF_RIFF,offset,size,aContainer)
	{
		m_pRiffFmt = 0;
		m_pRiffQcpFmt = 0;
		m_pRiffData = 0;
	}
	virtual int 	Load(Mp4File * pMp4File);
	virtual void 	Dump(Formatter* pFormatter);
	
	long Format;
	
	RiffFmt * m_pRiffFmt;
	RiffQcpFmt * m_pRiffQcpFmt;
	RiffData * m_pRiffData;
};

#endif
