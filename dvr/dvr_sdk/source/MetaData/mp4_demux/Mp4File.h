#ifndef MP4FILE_H_
#define MP4FILE_H_

#include "MovieFile.h"

#if 0
enum {
	AmcResolution96x80   = 1,
		AmcResolution128x96  = 2,
		AmcResolution176x144 = 3
};
#endif

/** Abstracted ISO Mpeg4 file
*/
class Mp4File : public MovieFile
{
public:
	Mp4File(char * pathname);
	Mp4File(FileCtrl * pFileCtrl);
	
	/** return mpeg4 box's size and type */
	BoxType	GetBoxHead(int * size);
	
	Mp4Author m_Author; /** author of the file. */
	
protected:
	Mp4File() {};
	
	BoxType	GetType();
	int		GetSize();
	unsigned long long		GetExtSize();
};

class MoovBox;
class FtypBox;
class MoofBox;
class TrakBox;
class Mp4Box;
class StblBox;
class UuidBox;

/** mpeg4 stream information structure */
class StreamInfo
{
public:
	/** duration in seconds */
	float	 	 m_fDuration;	
	/** video frame rate */
	float 	 m_fFrameRate;
	/** stream's avg bitrate */
	float 	 m_fBitRate;
	/** #samples */
	int		 m_iNumSamples;
	/** size of the stream */
	int		 m_iStreamSize;
	/** video width */
	int		 m_iVideoWidth;
	/** video height */
	int		 m_iVideoHeight;
	/** Track Head's timescale */
	int		 m_iTimeScale;
	/** duration in m_iTimeScale */
	int		 m_iDuration;	
	/** # channels of audio */
	int		 m_iNumChannels;
	/** bitsize of audio */
	int		 m_iBitSize;
	/** fequency of audio */
	int		 m_iFrequency;
	/** maximum bitrate */
	int		 m_iMaxBitRate;
	/** maximum sample size*/
	int		 m_iMaxSampleSize;
};

class Mp4TrackStream;
/** 3gp file, root of Mp4Com nodes tree */
class Mpeg3gpFile : public Mp4File, public Mp4Com
{
public:
	Mpeg3gpFile(char * pathname) 
		: Mp4File(pathname), 
		Mp4Com(MP43GP),
		m_pMoovBox(0), 
		m_pFtypBox(0), 
		m_pMoofBox(0),
#if 0
		m_lStartTime(0), 
		m_lStopTime(0),
		m_lStartPosition(0), 
		m_lStopPosition(0),
#endif
		m_lFileFormat(MP4_FILE_FORMAT),
		m_bForward(false),
		m_bRinger(false)
	{
		m_pUuidMvml = 0;
		m_pUuidEnci = 0;
		m_pUuidCpgd = 0;
	}
	
	Mpeg3gpFile(FileCtrl * pFileCtrl) 
		: Mp4File(pFileCtrl), 
		Mp4Com(MP43GP),
		m_pMoovBox(0), 
		m_pFtypBox(0), 
		m_pMoofBox(0),
#if 0
		m_lStartTime(0), 
		m_lStopTime(0),
		m_lStartPosition(0), 
		m_lStopPosition(0),
#endif
		m_lFileFormat(MP4_FILE_FORMAT),
		m_bForward(false),
		m_bRinger(false)
	{
		m_pUuidMvml = 0;
		m_pUuidEnci = 0;
		m_pUuidCpgd = 0;
	}
	
	~Mpeg3gpFile();
	
	/** load 3gp data strutures */
	int 	Load(bool bIsStreaming = false);
	int 	LoadAll(bool bIsStreaming = false);
	int 	LoadMoov(bool bIsStreaming = false);
	/** get stream information */
	int	GetStreamInfo(StreamInfo* pStreamInfo, HandlerType Handler);
	/** return stream's sample index at msTime time position */
	int	GetSampleIdx(HandlerType handler, int msTime);
	/** get Mp4TrackStream */
	Mp4TrackStream *	GetTrackStream(HandlerType handler, bool bEncrypt);
	
	/** determine if file has mpeg4 video */
	bool	IsMp4Video();
	/** determine if file has h.263 video */
	bool	HasH263();
	/** determine if file has h.264 video */
	bool	HasH264();
	/** determine if file has Amr video */
	bool	IsAmrAudio();
	/** determine if file has Alaw audio */
	bool	IsAlawAudio();
	/** determine if file has Ulaw audio */
	bool	IsUlawAudio();
	/** determine if file has Ima4 audio */
	bool	IsImaAudio();
	/** determine if file has mp3 audio */
	bool	IsMp3Audio();
	/** determine if file has alac audio */
	bool	IsAlacAudio();

	/** get object type indicator of stream */
	int   GetObjectTypeIndication(HandlerType Handler);
	int GetAudioObjectType(HandlerType Handler);
	/** return whole duration of playback and return value's precision */
	long  GetDuration(long * pPrecision);
	/** return the first matched box */
	Mp4Box* GetBox(BoxType aBoxType);
	/** get trakbox pointer */
	TrakBox * GetTrack(HandlerType Handler);

#ifdef MP4_MUX
	/** remove track from the 3gp tree */
	void	DeleteTrack(HandlerType Handler);
#endif

#if 0
	/** specify start and stop of all streaming in milli second */
	int	SetSegment(int iFrom, int iTo);
	/** dump the specified stream into output file */
	int 	Extract(HandlerType handler, char * output, bool bWithHeader=false);
	/** dump the specified stream into fp file */
	int 	Extract(HandlerType handler, FileCtrl * fp, bool bWithHeader=false);
	/** dump the specified stream's STSZ into output file */
	int 	DumpStsz(HandlerType handler, char * output);
	/** add track to the 3gp tree */
	void	AddTrack(TrakBox* pTrakBox);
	/** write 3gp tree to pathname */
	int   Write(char *pathanme);
	/** write 3gp tree using filectrl */
	int   Write(FileCtrl *pFileCtrl);

	/** get trakbox pointer and remove it from the tree */
	TrakBox * ExtractTrack(HandlerType Handler); 
	
	/** build mpeg4 video element stream's Stbl */
	StblBox * BuildMp4vStbl(float fFrameRate);
	/** build AAC element stream's Stbl */
	StblBox * BuildAacStbl(char * szfile, unsigned char * pDsi, int iDsiSize, int & iSamplingRate);
	/** build AMR element stream's Stbl */
	StblBox * BuildAmrStbl(int iMode);
	/** build AMR element stream's Stbl */
	StblBox * BuildQcelpStbl(char * pathname, unsigned char octet0);
	/** build MP3 element stream's Stbl */
	StblBox * BuildMp3Stbl(int & iSamplingRate);
	
	/** set drm settings for DoCoMo and KDDI */
	void SetDrm(bool bForward, bool bRinger);
	
	/** set file format settings for DoCoMo and KDDI */
	void SetFileFormat(long FileFormat);
	void SetKddiAmcRecMode(char cRecMode);
#endif

	MoovBox * m_pMoovBox;
	FtypBox * m_pFtypBox;
	MoofBox * m_pMoofBox;
	
	/** to support .AMC file format properly */
	UuidBox * m_pUuidMvml;
	UuidBox * m_pUuidEnci;
	UuidBox * m_pUuidCpgd;
	
	unsigned int m_mdat_pos;

protected:
	Mpeg3gpFile() : Mp4Com(MP43GP) {};
	
	void	DumpHeader(Formatter* pFormatter);
	void	DumpFooter(Formatter* pFormatter);

#if 0
	int 	UpdateSystemInfo();
	char	m_cKddiAmcRecMode;
	long	m_lStartTime;
	long	m_lStopTime;
	long	m_lStartPosition;
	long	m_lStopPosition;
#endif	
	long	m_lFileFormat;
	bool	m_bForward;
	bool	m_bRinger;
};


/* BufferFile allows Mp4File operations on buffer */
class BufferFile : public Mp4File
{
public:
	BufferFile(char * pBuf, int size) : Mp4File((char *)0)
	{
		m_cBuffer 		= pBuf;
		m_pBufferBegin = m_cBuffer;	
		m_pBufferEnd 	= m_cBuffer + size;	
		m_iFileSize 	= size;
	}
	~BufferFile()
	{
		Reset(); // to protect free m_cBuffer from MovieFile class.
	}
	
	// generic file operations.
	virtual int GetPos()
	{
		return m_pBufferBegin - m_cBuffer;
	}
	virtual int SetPos(int pos)
	{
		if(pos < 0 || pos > m_iFileSize)
			RETURN_ERROR(-1);
		m_pBufferBegin = m_cBuffer + pos;
		m_iRemainBits	= 0;
		m_CurChar		= 0;
		return 0;
	}
	virtual int GetFileSize()
	{
		return m_iFileSize;
	}
	
private:
	BufferFile();
	
	virtual int FillBuffer(char *pBuf = NULL, unsigned int nFillSize = DEFAULT_LOAD_BUFFER_LEN)
	{
		m_IoStatus = MOVIEFILE_EOF;
		RETURN_ERROR(-1);
	}
};

#if MP4_MUX

/** RIFF file: to support wave and qcelp file formats. */
class RiffRiff;
class RiffFile : public Mp4File, public Mp4Com
{
public:
	RiffFile(char * pathname) 
		: Mp4File(pathname), 
		Mp4Com(MP4RIFF)
	{
		m_pRiffRiff = 0;
	}
	
	/** load riff data strutures */
	int 	Load();
	/** make pcm file into riff wave file */
	int MakeWave(char *pcmfile, char *wavefile,int iSamplingRate,int iChannels);
	/** write RIFF tree to pathname */
	int   Write(char *pathanme);
	/** write DATA chunk to pathname */
	int   WriteData(char *pathanme);
	
	void DumpHeader(Formatter* pFormatter);
	void DumpFooter(Formatter* pFormatter);
	
	RiffRiff * m_pRiffRiff;
};

#endif

#endif


