#ifndef MP4NAVIGATOR_H_
#define MP4NAVIGATOR_H_

class Mp4Box;
class Descr;
class Mp4Ve;
class Mp4Com;


/** Mp4Navigator will navigate the tree of Mp4Com nodes
*/
class Mp4Navigator
{
public:
	virtual ~Mp4Navigator() {} // (not needed if class becomes pure virtual)

	/** Visit Mp4Com node. 
	this will be followed by VisitEnd.
	*/
	virtual bool Visit(Mp4Com *pCom);
	/** Finish Visit Mp4Com node. 
	*/
	virtual bool VisitEnd(Mp4Com *pCom);
	/** Visit Mp4Box
	*/
	virtual bool Visit(Mp4Box *pBox) { return true; }
	/** Visit Descriptor
	*/
	virtual bool Visit(Descr *pDescr) { return true; }
	/** Visit Mpeg4 video element node.
	*/
	virtual bool Visit(Mp4Ve *pVe) { return true; }
	/** Finish Visit Mp4Box
	*/
	virtual bool VisitEnd(Mp4Box *pBox) { return true; }
	/** Finish Visit Descriptor
	*/
	virtual bool VisitEnd(Descr *pDescr) { return true; }
	/** Finish Visit Mpeg4 video element node.
	*/
	virtual bool VisitEnd(Mp4Ve *Ve) { return true; }
	/** Initialize Navigator specific data stuructures before start navigatino. 
	*/
	virtual void PrepareForNavigation() {}
};

/** Dumping Mp4Com trees to stdout as XML.
*/
class Mp4XmlDumper : public Mp4Navigator
{
public:
	virtual bool Visit(Mp4Com *pCom);
	virtual bool VisitEnd(Mp4Com *pCom);

	XmlFormatter m_XmlFormatter;
};

/** Dumping Mp4Com trees to 3gp file.
*/
class Mp4Serializer : public Mp4Navigator
{
public:
	virtual bool Visit(Mp4Com *pCom);
	virtual bool VisitEnd(Mp4Com *pCom);

	/** Set Output File: 
	if same file exits, it will overwrite the file.
	*/
	void SetOutput(char *pathname)	
	{
		if(m_pFileFormatter)
			delete m_pFileFormatter;
		m_pFileFormatter = new MpegFileFormatter(pathname);
		m_pFileFormatter->m_FileFormat = m_lFileFormat;
	}
	/** Set Output FileCtrl: 
	*/
	void SetOutput(FileCtrl *pFileCtrl)	
	{
		if(m_pFileFormatter)
			delete m_pFileFormatter;
		m_pFileFormatter = new MpegFileFormatter(pFileCtrl);
		if(NULL == m_pFileFormatter)
		{
			return;
		}
		m_pFileFormatter->m_FileFormat = m_lFileFormat;
	}

	Mp4Serializer() : Mp4Navigator()
	{
		m_pFileFormatter = 0;
		m_lFileFormat = 0;
	}
	~Mp4Serializer()
	{
		if(m_pFileFormatter)
			delete m_pFileFormatter;
		m_pFileFormatter = 0;
	}

	/** Set Output File Format: 
	*/
	void SetFileFormat(long lFileFormat)
	{
		if(m_pFileFormatter)
			m_pFileFormatter->m_FileFormat = lFileFormat;
		m_lFileFormat = lFileFormat;
	}
	virtual void PrepareForNavigation()
	{
		if(m_pFileFormatter)
			m_pFileFormatter->ResetCounters();
	}

	MpegFileFormatter * m_pFileFormatter;
protected:
	long m_lFileFormat;
};

/** Mpeg4 Decoder Specific Information Extractor
*/
class Mp4DsiExtractor : public Mp4Navigator
{
public:
	Mp4DsiExtractor(HandlerType aHandler) : 
	  m_iSize(0),
		  m_info(0),
		  m_Handler(aHandler)
	  {
	  };
	  virtual ~Mp4DsiExtractor()
	  {
		  if(m_info)
			  delete [] m_info;
		  m_info = 0;
	  }

	  virtual bool Visit(Descr *pDescr);

	  /** size of m_info
	  */
	  int		m_iSize;
	  /** dsi information container.
	  */
	  char * 	m_info;
protected:
	Mp4DsiExtractor(){};

	HandlerType m_Handler;
	char m_profile_and_level_indication;
	char m_visual_object_verid;
	char m_visual_object_priority;
	char m_visual_object_type;
	char m_is_visual_object_identifier;
};

/** Find designated Mp4Box in Mp4Com tree.
*/
class Mp4BoxFinder : public Mp4Navigator
{
public:
	Mp4BoxFinder(BoxType aBoxType) : m_FindBox(aBoxType) {m_FoundBox = NULL;};
	virtual bool Visit(Mp4Box *pBox);
	/** return found box in the last search
	*/
	Mp4Box * GetBox() { return m_FoundBox; };
	/** search the aBoxType starting from pBox
	SUCCESS: return Mp4Box Pointer
	FAIL: return null
	*/
	Mp4Box * GetBox(BoxType aBoxType, Mp4Box * pBox);
protected:
	Mp4BoxFinder(){};
	BoxType m_FindBox;
	Mp4Box *	m_FoundBox;
};

class TrakBox;
/** return first track matched with the designated handler
*/
class Mp4TrackFinder : public Mp4Navigator
{
public:
	Mp4TrackFinder(HandlerType aHandler) : m_pFoundTrak(0)
	{
		m_Handler = aHandler;
	}
	virtual bool Visit(Mp4Box *pBox);
	/** return found track from the last search
	*/
	TrakBox * GetTrack() { return m_pFoundTrak; }
protected:
	Mp4TrackFinder() {};
	TrakBox *	m_pFoundTrak;
	HandlerType m_Handler;
};

/** Mpeg4 Chunk information. Mp4Stream internal use.
*/
class Mp4Chunk
{
public:
	Mp4Chunk() : 
	  m_BeginOffset(0),
		  m_EndOffset(0),
		  m_BeginSampleIdx(0),
		  m_EndSampleIdx(0),
		  m_StreamOffset(0)
	  {}

	  long m_BeginOffset;
	  long m_EndOffset;
	  long m_BeginSampleIdx;
	  long m_EndSampleIdx;
	  long m_StreamOffset;
};

typedef enum tagSEEKI_DIRECTION
{
	SEEKI_BACKWARD = -1,
	SEEKI_FORWARD = 1,
} SEEKIDIRECTION;

class StcoBox;
class StscBox;
class StszBox;
class SttsBox;
class StssBox;
class CttsBox;
class FileCtrl;
/** abstracted track stream in mp4 file
*/
class Mp4TrackStream : public Mp4Navigator
{
public:
	Mp4TrackStream(TrakBox * aTrack, Mp4File * aFile, bool bEncrypt ) : 
	  m_pTrakBox(aTrack), 
		  m_pMp4File(aFile),
		  m_pChunk(0),
		  m_iNumChunk(0),
		  m_CurStreamPos(0),
		  m_msStreamDuration(0),
		  m_TimeScale(1),
		  m_StreamSize(0),
		  m_iRemains(0)
	  {
#ifdef IAC_ENCRYPT		  //sunstars IAC 3gp
		  m_bEncrypt = bEncrypt;
#endif		  //end sunstars	 
		  // Lin: H.264 Seek
		  m_VideoType = (BoxType)0;

		  BuildFileMap();
		  m_iCurChunk = 1;
		  m_iCurSample = 1;

	  }
	  virtual ~Mp4TrackStream()
	  {
		  if(m_pChunk)
			  delete [] m_pChunk;	
		  m_pChunk = 0;
		  m_iNumChunk = 0;
	  }

#if 0
	  /** dump stream into pathname with header if specified : 
	  nWrite max writing
	  * size, nWrite=-1 -> write whole stream.
	  */
	  int 	Write(char *pahtname, bool bWithHeader,int nWrite=-1);
	  /** dump stream into fp with header if specified: 
	  nWrite max writing
	  * size, nWrite=-1 -> write whole stream.
	  */
	  int 	Write(FileCtrl *pFileCtrl, bool bWithHeader,int nWrite=-1);
	  /** write chunk which range contains in lNewChunkOffset.
	  */
	  int 	WriteChunk(FileCtrl *pFileCtrl, long lNewChunkOffset);
	  int ReadIFrameSample(char *buf, int nRead, long* pCts);
	  /** Dump stream thorugh pFormatter */
	  void Dump(Formatter* pFormatter);

	  /** Dump Stsz box into the output file */
	  int  DumpStsz(char * output);
#endif  
	  /** read stream data into buffer upto nRead
	  */
	  int 	Read(char *buf, int nRead);
	  /** read stream data into buffer upto min(nRead,sample size)
	  */
	  int 	ReadSample(char *buf, int nRead, unsigned int * pCts=0);

	  /** locate sample reading position to iSample
	  iSample: [1..maxSample]
	  */
	  int SeekToI(SEEKIDIRECTION direction, bool bIsStreaming=0);

	  long  SetSamplePosition(long iSample);
	  /**  return the sample index read lastly.
	  return value: [1..maxSample]
	  */
	  int   GetCurrentSampleIdx() 
	  {
		  return m_iCurSample;
	  }
	  void   SetCurrentSampleIdx(long lIdx) 
	  {
		  m_iCurSample = lIdx;
	  }

	  long  GetSampleCts(long lIdx);
	  long long  GetSampleCtsOfMisc(long lIdx);
	  long  GetSampleSize(long lIdx);
	  long  GetSamplePos(long lIdx);

	  /** return max sample size in this stream.
	  */
	  long	GetMaxSampleSize()
	  {
		  return m_lMaxSampleSize;
	  }

	  /** return reading pointer
	  */
	  int 	GetPos()
	  {
		  return m_CurStreamPos;
	  }

	  /** return stream position in byte corresponding milli second time arg.
	  */
	  long	GetPosition(long msTime);

	  /** return stream's sample index corresponding milli second time arg.
	  */
	  long	GetSampleIdx(long msTime);
	  /** return stream's sync sample index corresponding milli second time arg.
	  only valid for video elementary stream.
	  */

	  long	GetCTS(long lStreamPos);
	  /** return Composition Time Stamp of give sample index.
	  */

	  long 	SetPos(int streamPos);
	  /** return whole stream size in bytes.
	  */

	  int 	GetStreamSize()
	  {
		  return m_StreamSize;
	  }

	  long 	GetAvcBufferLenSize();
	  /** Get Decoder Specific Information (DSI) */
	  Mp4DsiExtractor* GetDsi();

#ifdef IAC_ENCRYPT	  //sunstars IAC 3gp
	  void EnableEncrypt(bool bEnable, char* password, unsigned int mdatpos)
	  {
		  m_bEncrypt = bEnable;
		  m_mdat_pos = mdatpos;
		  memcpy(m_password, password, 8);
	  };
protected:
	bool m_bEncrypt;
	unsigned char m_password[8];
	unsigned int m_mdat_pos;
#endif	//end sunstars


public:
	int BuildFileMap();
	Mp4TrackStream() {};

	int ReadChunk(int iChunkIdx, char *buf, int iStreamOffset, int nRead);
	int GetChunkIdx(int iStreamOffset);


	Mp4File *	m_pMp4File;
	Mp4Chunk *	m_pChunk;
	StcoBox	*	m_pStcoBox;
	StscBox	*	m_pStscBox;
	SttsBox	*	m_pSttsBox;
	StszBox	*	m_pStszBox;
	StssBox *	m_pStssBox;
	CttsBox *	m_pCttsBox;
	// Lin: H.264 Seek
	BoxType	m_VideoType;

	int			m_iNumChunk;
	unsigned int			m_StreamSize;
	int			m_msStreamDuration;
	int			m_TimeScale;
	long		m_lMaxSampleSize;
	int	m_iCurChunk;
	int m_iCurSample;
	int	m_CurStreamPos;
	int m_iRemains;

public:
	TrakBox *	m_pTrakBox;
};

#endif

