#ifndef MP4COM_H_
#define MP4COM_H_

#define RETURN_ERROR(x) \
{\
	return x; \
}

#define ISO_SYNTAX_ERROR	-100

#ifdef TI_ARM
typedef struct _int64s
{
	int low;
	int hight;
} int64s;
#endif


/*
Mpeg4 File Author -- to support PacketVideo
*/
enum Mp4Author
{
	AuthorPacketVideo,
	AuthorPhillips,
	AuthorIntervideo,
	AuthorUnknown
};

/** Mp4Com types */
enum Mp4ComType
{
	MP4BOX, /** mpeg4 iso box type */
	MP4DESCR, /** mpeg4 iso descriptor type */
	MP4VE, /** mpeg4 visual element stream type */
	MP4FILE, /** mpeg4 iso file type */
	MP43GP, /** 3gp file type */
	MP4RIFF, /** riff file type */
	MP4AUDIOFRAMEHEADER, /** mpeg audio frame header, 11171-3  */
};

/** mpeg4 iso box types */
enum BoxType
{
	BOX_IVLD	= 'i'<<24|'v'<<16|'l'<<8|'d',
	BOX_CLIP	= 'c'<<24|'l'<<16|'i'<<8|'p',
	BOX_CRGN	= 'c'<<24|'r'<<16|'g'<<8|'n',
	BOX_UDTA	= 'u'<<24|'d'<<16|'t'<<8|'a',
	BOX_FTYP	= 'f'<<24|'t'<<16|'y'<<8|'p',
	BOX_UUID	= 'u'<<24|'u'<<16|'i'<<8|'d',
	BOX_MOOV	= 'm'<<24|'o'<<16|'o'<<8|'v',
	BOX_MVHD	= 'm'<<24|'v'<<16|'h'<<8|'d',
	BOX_IODS	= 'i'<<24|'o'<<16|'d'<<8|'s',
	BOX_TRAK	= 't'<<24|'r'<<16|'a'<<8|'k',
	BOX_TKHD	= 't'<<24|'k'<<16|'h'<<8|'d',
	BOX_TREF	= 't'<<24|'r'<<16|'e'<<8|'f',
	BOX_EDTS	= 'e'<<24|'d'<<16|'t'<<8|'s',
	BOX_ELST	= 'e'<<24|'l'<<16|'s'<<8|'t',
	BOX_MDIA	= 'm'<<24|'d'<<16|'i'<<8|'a',
	BOX_MDHD	= 'm'<<24|'d'<<16|'h'<<8|'d',
	BOX_HDLR	= 'h'<<24|'d'<<16|'l'<<8|'r',
	BOX_MINF	= 'm'<<24|'i'<<16|'n'<<8|'f',
	BOX_VMHD	= 'v'<<24|'m'<<16|'h'<<8|'d',
	BOX_SMHD	= 's'<<24|'m'<<16|'h'<<8|'d',
	BOX_NMHD	= 'n'<<24|'m'<<16|'h'<<8|'d',
	BOX_DINF	= 'd'<<24|'i'<<16|'n'<<8|'f',
	BOX_DREF	= 'd'<<24|'r'<<16|'e'<<8|'f',
	BOX_URL		= 'u'<<24|'r'<<16|'l'<<8|' ',
	BOX_STBL	= 's'<<24|'t'<<16|'b'<<8|'l',
	BOX_STSD	= 's'<<24|'t'<<16|'s'<<8|'d',
	BOX_STTS	= 's'<<24|'t'<<16|'t'<<8|'s',
	BOX_CTTS	= 'c'<<24|'t'<<16|'t'<<8|'s',
	BOX_STSC	= 's'<<24|'t'<<16|'s'<<8|'c',
	BOX_STSZ	= 's'<<24|'t'<<16|'s'<<8|'z',
	BOX_STCO	= 's'<<24|'t'<<16|'c'<<8|'o',
	BOX_CO64	= 'c'<<24|'o'<<16|'6'<<8|'4',
	BOX_STSS	= 's'<<24|'t'<<16|'s'<<8|'s',
	BOX_MVEX	= 'm'<<24|'v'<<16|'e'<<8|'x',
	BOX_MEHD	= 'm'<<24|'e'<<16|'h'<<8|'d',
	BOX_TREX	= 't'<<24|'r'<<16|'e'<<8|'x',
	BOX_MOOF	= 'm'<<24|'o'<<16|'o'<<8|'f',
	BOX_MFHD	= 'm'<<24|'f'<<16|'h'<<8|'d',
	BOX_TRAF	= 't'<<24|'r'<<16|'a'<<8|'f',
	BOX_TFHD	= 't'<<24|'f'<<16|'h'<<8|'d',
	BOX_TRUN	= 't'<<24|'r'<<16|'u'<<8|'n',
	BOX_MFRA	= 'm'<<24|'f'<<16|'r'<<8|'a',
	BOX_MDAT	= 'm'<<24|'d'<<16|'a'<<8|'t',
	BOX_DRM		= 'd'<<24|'r'<<16|'m'<<8|' ',
	BOX_MP4V	= 'm'<<24|'p'<<16|'4'<<8|'v',
	BOX_DX50	= 'D'<<24|'X'<<16|'5'<<8|'0',
	BOX_MP4A	= 'm'<<24|'p'<<16|'4'<<8|'a',
	BOX_S263	= 's'<<24|'2'<<16|'6'<<8|'3',
	BOX_D263	= 'd'<<24|'2'<<16|'6'<<8|'3',
	BOX_BITR	= 'b'<<24|'i'<<16|'t'<<8|'r',
	BOX_SAMR	= 's'<<24|'a'<<16|'m'<<8|'r',
	BOX_DAMR	= 'd'<<24|'a'<<16|'m'<<8|'r',
	BOX_TX3G	= 't'<<24|'x'<<16|'3'<<8|'g',
	BOX_FTAB	= 'f'<<24|'t'<<16|'a'<<8|'b',
	BOX_PASP	= 'p'<<24|'a'<<16|'s'<<8|'p',
	BOX_ESDS	= 'e'<<24|'s'<<16|'d'<<8|'s',
	BOX_GLBL	= 'g'<<24|'l'<<16|'b'<<8|'l',
	BOX_FREE	= 'f'<<24|'r'<<16|'e'<<8|'e',
	BOX_SKIP	= 's'<<24|'k'<<16|'i'<<8|'p',

	BOX_PERF	= 'p'<<24|'e'<<16|'r'<<8|'f', // performer or artist 
	BOX_CPRT	= 'c'<<24|'p'<<16|'r'<<8|'t', // copyright -- MP4,DoCoMo
	BOX_AUTH	= 'a'<<24|'u'<<16|'t'<<8|'h', // author -- DoCoMo
	BOX_TITL	= 't'<<24|'i'<<16|'t'<<8|'l',	// title -- DoCoMo
	BOX_DSCP	= 'd'<<24|'s'<<16|'c'<<8|'p', // description -- DoCoMo

	// QuickTime Atoms
	BOX_ULAW	= 'u'<<24|'l'<<16|'a'<<8|'w', 
	BOX_ALAW	= 'a'<<24|'l'<<16|'a'<<8|'w', 
	BOX_H263	= 'h'<<24|'2'<<16|'6'<<8|'3',
	BOX_IMA4	= 'i'<<24|'m'<<16|'a'<<8|'4',
	BOX_AVC1	= 'a'<<24|'v'<<16|'c'<<8|'1',
	BOX_AVCC	= 'a'<<24|'v'<<16|'c'<<8|'C',
	BOX_ALAC	= 'a'<<24|'l'<<16|'a'<<8|'c',
	BOX_WAVE	= 'w'<<24|'a'<<16|'v'<<8|'e',	

	// mp3 audio Atoms
	BOX_MP3_	= '.'<<24|'m'<<16|'p'<<8|'3',
	// divx video Atom
	BOX_DIVX	= 'D'<<24|'I'<<16|'V'<<8|'X',
	// xvid video Atom
	BOX_XVID	= 'X'<<24|'V'<<16|'I'<<8|'D',

	//user data list

	BOX_META    = 'm'<<24|'e'<<16|'t'<<8|'a', // meta
	BOX_ILST    = 'i'<<24|'l'<<16|'s'<<8|'t', // meta
	BOX_COVR	= 'c'<<24|'o'<<16|'v'<<8|'r', // meta
    BOX__CVR    = 0xa9<<24|'c'<<16|'v'<<8|'r', //meta
	BOX_ART     = 0xa9<<24|'A'<<16|'R'<<8|'T', // meta
	BOX_WRT     = 0xa9<<24|'w'<<16|'r'<<8|'t', // meta
	BOX_GRP     = 0xa9<<24|'g'<<16|'r'<<8|'p', // meta
	BOX_ALB     = 0xa9<<24|'a'<<16|'l'<<8|'b', // meta
	BOX_TOO     = 0xa9<<24|'t'<<16|'o'<<8|'o', // meta
    BOX__DES    = 0xa9<<24|'d'<<16|'e'<<8|'s',

	BOX_CPY     = 0xa9<<24|'c'<<16|'p'<<8|'y', // Copyright statement
	BOX_PRF     = 0xa9<<24|'p'<<16|'r'<<8|'f', // Names of performers
	BOX_PRD     = 0xa9<<24|'p'<<16|'r'<<8|'d', // Name of producer
	BOX_NAM     = 0xa9<<24|'n'<<16|'a'<<8|'m', // Title of the content
	BOX_INF     = 0xa9<<24|'i'<<16|'n'<<8|'f', // Information about the movie

	BOX_REQ     = 0xa9<<24|'r'<<16|'e'<<8|'q', // Special hardware and software requirements
	BOX_ENC     = 0xa9<<24|'e'<<16|'n'<<8|'c', // 

};

/** mpeg4 track handlers */
enum HandlerType
{
	HANDLER_ALL		= 0,
	HANDLER_VIDEO	= 'v'<<24|'i'<<16|'d'<<8|'e',
	HANDLER_AUDIO	= 's'<<24|'o'<<16|'u'<<8|'n',
	HANDLER_TEXT	= 't'<<24|'e'<<16|'x'<<8|'t',
};

enum eDescrTag
{
	kObjectDescrTag				= 0x01,
	kInitialObjectDescrTag		= 0x02,
	kES_DescrTag				= 0x03,
	kDecoderConfigDescrTag		= 0x04,
	kDecSpecificInfoTag			= 0x05,
	kSLConfigDescrTag			= 0x06,
	kContentIdentDescrTag		= 0x07,
	kSupplContentIdentDescrTag	= 0x08,
	kIPI_DescrPointerTag		= 0x09,
	kIPMP_DescrPointerTag		= 0x0A,
	kIPMP_DescrTag				= 0x0B,
	kQoS_DescrTag				= 0x0C,
	kContentClassificationDescrTag = 0x40,
	kKeyWordDescrTag			= 0x41,
	kRatingDescrTag				= 0x42,
	kLanguageDescrTag			= 0x43,
	kShortTextualDescrTag		= 0x44,
	kExpandedTextualDescrTag	= 0x45,
	kContentCreatorNameDescrTag	= 0x46,
	kContentCreationDateDescrTag= 0x47,
	kOCICreatorNameDescrTag		= 0x48,
	kOCICreationDateDescrTag	= 0x49,
	kSmpteCameraPositionDescrTag= 0x4A,
};

#if 0
enum eObjectType
{
	kVisualIso14496_2 = 0x20,
	kAudioIso14496_3 = 0x40,
};

/** elementary stream type
*/
enum eStreamType
{
	kVisualStream = 0x04,
	kAudioStream = 0x05
};
#endif

/** determine File Format of 3gp file.
*/
enum
{
	MP4_FILE_FORMAT,	// generic mp4 file format -- default
	KDDI_3GP_FILE_FORMAT,
	KDDI_AMC_FILE_FORMAT,
	DOCOMO_3GP_FILE_FORMAT,
};

#define MAX_NUM_CHILDREN		128
#define MAX_TRACK_NUM			256

#define ExtDescrTagStartRange	0x60
#define ExtDescrTagEndRange		0xFE
#define OCIDescrTagStartRange	0x40
#define OCIDescrTagEndRange		0x5F

/** Dump Navigator uses Formatter.
*/
class Formatter
{
public:
	Formatter() { m_bConvertToBig = true; }
	/** put long */
	virtual void Put(char* szFormat,long iVal)=0;
	/** put short */
	virtual void Put(char* szFormat,short iVal)=0;
	/** put char */
	virtual void Put(char * szFormat, char cVal)=0;
	/** put 24 bits */
	virtual void Put24(char * szFormat, long cVal)=0;
	/** put string */
	virtual void Put(char * szFormat, char *  String)=0;
#ifndef TI_ARM	
#if defined(__linux__)
	/** put 64 bits */
	virtual void Put(char * szFormat, long long iVal)=0;
#else
	/** put 64 bits */
	virtual void Put(char * szFormat, __int64 iVal)=0;
#endif
#else
	virtual void Put(char * szFormat, int64s iVal)=0;
#endif
	/** print to stdout */
	virtual void Put(char * szFormat)
	{
		Print(szFormat);
	}
	/** put charater stream as it is */
	virtual void PutStream(char * buf, int size) {}
	/** generate sizeofinstance structure */
	virtual void PutSizeOfInstance(long soi)
	{
		return;
	}
	/** print to stdout */
	virtual void Print(char *szFormat, ...);
	/** print integer as character string */
	virtual void PrintInt(char *string, int size);
	/** determine if the formatter will use stdout */
	virtual bool UseDisplay()=0;
	/** return current reading/wrting position */
	virtual long GetPos() { return 0; };
	/** return next track id -- for 3gp formatter */
	virtual long GetNextTrackId() { return 1; };
	/** determine endianess converstion need */
	void ConvertToBig(bool bEnable)
	{
		m_bConvertToBig = bEnable;
	}

public:
	bool m_bConvertToBig;
	long m_FileFormat;
};

/** generate Xml output to stdout.
*/
class XmlFormatter : public Formatter
{
public:
	virtual void Put(char* szFormat,long iVal);
	virtual void Put(char* szFormat,short iVal);
	virtual void Put(char * szFormat, char cVal);
	virtual void Put24(char * szFormat, long cVal);
	virtual void Put(char * szFormat, char *  String);
#ifndef TI_ARM	
#if defined(__linux__)
	virtual void Put(char * szFormat, long long iVal);
#else
	virtual void Put(char * szFormat, __int64 iVal);
#endif
#else
	virtual void Put(char * szFormat, int64s iVal);
#endif
	virtual bool UseDisplay() { return true; }
};

class FileCtrl;
/** save it to file.
formatter has 3gp file specfific function and DoCoMo and KDDI specific
functions.
*/
class MpegFileFormatter : public Formatter
{
public:
	virtual void Put(char* szFormat,long iVal);
	virtual void Put(char* szFormat,short iVal);
	virtual void Put(char * szFormat, char cVal);
	virtual void Put24(char * szFormat, long cVal);
	virtual void Put(char * szFormat, char *  String);
#ifndef TI_ARM	
#if defined(__linux__)
	virtual void Put(char * szFormat, long long iVal);
#else
	virtual void Put(char * szFormat, __int64 iVal);
#endif
#else
	virtual void Put(char * szFormat, int64s iVal);
#endif    
	/** put charater stream as it is */
	virtual void  PutStream(char * buf, int size);
	/** write size in sizeofinstance format. */
	virtual void  PutSizeOfInstance(long soi);
	/** disable stdout print */
	virtual bool  UseDisplay() { return false; }
	/** get current write position */
	virtual long  GetPos();

	MpegFileFormatter(FileCtrl *pFileCtrl);
	MpegFileFormatter(char *pathname);
	virtual ~MpegFileFormatter();

	/** Open output file of the formatter */
	int Open(char *pathname);

	/** return file pointer of 3gpformatter's output */
	FileCtrl * GetFileCtrl()
	{
		return m_pFileCtrl;
	}
	virtual long GetNextTrackId()
	{ 
		return (++m_NextTrackId); 
	};
	/** reset trackid counters */
	void ResetCounters()
	{
		m_NextTrackId = 0;
	}

protected:
	virtual void PutBuf(char *string, int size);
	MpegFileFormatter() {};
	long 	m_NextTrackId;
	FileCtrl * m_pFileCtrl;
};

class Mp4Navigator;
/** Root class of all mpeg4 navigatable object.(server of Mp4Navigator)
*/
class Mp4Com
{
public:
	/** Navigate Mp4Com tree */
	bool Navigate(Mp4Navigator* pNavigator);
	/** Dump Header information */
	virtual void DumpHeader(Formatter* pFormatter);
	/** Dump Footer information */
	virtual void DumpFooter(Formatter* pFormatter);
	/** Dump Node information */
	virtual void Dump(Formatter* pFormatter){};
	/** return Mp4ComType */
	Mp4ComType   GetType()
	{
		return m_Type;
	}
	/** if navigate fail then stop naviagtion */
	void EnableStopNavigation(bool bEnable)
	{
		m_bStopNavigateIfFail = bEnable;
	}
	/** set size of node */
	virtual void SetSize(int iSize) { return; }
	/** remove son from this node  
	if(bDelete) delete pSon.
	*/
	void Remove(Mp4Com* pSon, bool bDelete=true);
	/** Adopt son to this node */
	void Adopt(Mp4Com* pSon);
	/** dump loaded structure into XML -- stdout */
	void DumpXml();
	/** return Mp4Com node's size */
	int GetSize() { return m_iSize; }
	/** makr loaded flag */
	void SetLoaded(bool bLoaded)
	{
		m_bNotLoaded = !bLoaded;
	}

	/** for serialization. 3gp mux use these. */
	int m_iSizeOnFile;
	/** for serialization. 3gp mux use these. */
	int m_iOffsetOnFile;
	/** start position in the file */
	int m_iBeginOffset;
	/** make node serializable */
	void MakeSerializable(bool bEnable)
	{
		m_bSerializable = bEnable;
	}
	bool IsSerializable() { return m_bSerializable; }
protected:
	Mp4Com(Mp4ComType aType) 
		: m_iSizeOnFile(0),
		m_iOffsetOnFile(0),
		m_ulNumChildren(0),
		m_bSerializable(true),
		m_Type(aType),
		m_bStopNavigateIfFail(true)
	{
	}
	virtual ~Mp4Com()
	{
		ReleaseChildren();
	}

	Mp4Com* m_Children[MAX_NUM_CHILDREN];
	unsigned long m_ulNumChildren;	

	bool m_bNotLoaded;
	int m_iSize;
	bool m_bSerializable;

private:
	void ReleaseChildren();
	Mp4ComType m_Type;
	bool m_bStopNavigateIfFail;
};

#endif

