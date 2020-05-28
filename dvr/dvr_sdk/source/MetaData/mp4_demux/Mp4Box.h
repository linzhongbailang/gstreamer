#ifndef MP4BOX_H_
#define MP4BOX_H_

#include "Mp4Com.h"
#include "MP4Demux.h"

#define BOXBUFLEN 255

class Vos;
class Mp4Navigator;
class Mp4File;

/** ISO MPEG4's BOX abstractor
*/
class Mp4Box : public Mp4Com
{
public:
	Mp4Box(BoxType aBoxType,int offset,int size, Mp4Box * aContainer) :
	  Mp4Com(MP4BOX),
		  m_BoxType(aBoxType), 
		  m_pContainer(aContainer) 
	  {
		  m_iBeginOffset=offset;
		  m_iSize=size;
		  m_bNotLoaded = false;
	  };
	  /** Load box and its child into memory.
	  */
	  virtual int 	Load(Mp4File * pMp4File) 
	  { 
		  m_bNotLoaded = true;
		  return 0; 
	  }
	  
	  virtual int 	LoadAll(Mp4File * pMp4File) 
	  { 
		  m_bNotLoaded = true;
		  return 0; 
	  }

	  /** Dump box to formatter.
	  */
	  virtual void 	Dump(Formatter* pFormatter) 
	  {
		  if(!pFormatter->UseDisplay() && m_bNotLoaded)
			  return;

		  if(m_bNotLoaded)
		  {
			  pFormatter->Print("<not_loaded/>");
		  }
	  };
	  /** Dump box's head information.
	  */
	  virtual void 	DumpHeader(Formatter* pFormatter)
	  {
		  if(!pFormatter->UseDisplay())
		  {
			  m_iBeginOffset = pFormatter->GetPos();
			  if(m_bNotLoaded)
				  return;
		  }

		  pFormatter->Print("<");
		  pFormatter->PrintInt((char *)&m_BoxType,4);
		  pFormatter->Print(" offset=\"%d\" size=\"%d\">\n",m_iBeginOffset,m_iSize);
		  if(!pFormatter->UseDisplay())
		  {
			  pFormatter->Put(0,(long)m_iSize);
			  pFormatter->Put(0,(long)m_BoxType);
		  }
		  Dump(pFormatter);
	  }
	  virtual void 	DumpFooter(Formatter* pFormatter)
	  {
		  if(!pFormatter->UseDisplay() && m_bNotLoaded)
			  return;

		  pFormatter->Print("</");
		  pFormatter->PrintInt((char *)&m_BoxType,4);
		  pFormatter->Print(">\n");
	  }
	  static int 		GetIntL(char * cBuf);
	  static int 		GetIntB(char * cBuf);
	  static short 	GetShortL(char * cBuf);
	  static short 	GetShortB(char * cBuf);
	  static long 	GetSizeOfInstance(Mp4File * pMp4File,int * count=0);
	  /** return box's container box
	  */
	  Mp4Box * 		GetContainer(BoxType aBoxType)
	  {
		  if(aBoxType==m_BoxType)
			  return this;
		  else if (m_pContainer)
			  return m_pContainer->GetContainer(aBoxType);
		  return 0;
	  }	
	  /** set box's container box
	  */
	  void SetContainer(Mp4Box* pContainer)
	  {
		  m_pContainer = pContainer;
	  }
	  /** set box's type 
	  */
	  BoxType GetBoxType() { return m_BoxType; };
	  /** set box's offset in the file 
	  */
	  int	GetBeginOffset() { return m_iBeginOffset; };
	  /** set box's size
	  */
	  virtual void SetSize(int iSize) 
	  {
		  m_iSize = iSize;
	  }

protected:
	~Mp4Box() 
	{
	}

	BoxType 				m_BoxType;	
	Mp4Box * 			m_pContainer;
};

/** File Type Box: has a function to support Kddi and Kddi Amc
*/
class FtypBox : public Mp4Box
{
public:
	FtypBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_FTYP,offset,size,aContainer)
	{
		m_compatible_brands = 0;
		m_n_compatible_brands = 0;
	}
	virtual ~FtypBox()
	{
		if(m_compatible_brands)
			delete [] m_compatible_brands;
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	int  MakeKddi();
	int  MakeKddiAmc();

protected:	
	long	m_major_brand;
	long	m_minor_version;
	long *m_compatible_brands;
	long 	m_n_compatible_brands;
};

class DscpBox : public Mp4Box
{
public:
	DscpBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_DSCP,offset,size,aContainer)
	{
	}
	virtual void Dump(Formatter* pFormatter)
	{
		pFormatter->Print("<info>DoCoMo's Mobile MP4 extension.</info>\n");
	}
};

class TitlBox : public Mp4Box
{
public:
	TitlBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_TITL,offset,size,aContainer)
	{
	}
	virtual void Dump(Formatter* pFormatter)
	{
		pFormatter->Print("<info>DoCoMo's Mobile MP4 extension.</info>\n");
	}
};

class ElstBox : public Mp4Box
{
public:
	ElstBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_ELST,offset,size,aContainer)
	{
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char  m_version;
	char  m_flags[3];
	long  m_entry_count;
	long  m_segment_duration;
	long  m_media_time;
	short m_media_rate_integer;
	short m_media_rate_fraction;
};


class EdtsBox : public Mp4Box
{
public:
	EdtsBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_EDTS,offset,size,aContainer)
	{
	}
	virtual int Load(Mp4File * pMp4File);
};

class WaveBox : public Mp4Box
{
public:
	WaveBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_EDTS,offset,size,aContainer)
	{
	}
	virtual int Load(Mp4File * pMp4File);
};

class AuthBox : public Mp4Box
{
public:
	AuthBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_AUTH,offset,size,aContainer)
	{
	}
	virtual void Dump(Formatter* pFormatter)
	{
		pFormatter->Print("<info>DoCoMo's Mobile MP4 extension.</info>\n");
	}
};

enum UuidTypes
{
	UUID_UNKNOWN,
	UUID_CPGD,
	UUID_MVML,
	UUID_ENCI,
	UUID_PROP,
};

/** User defined types : KDDI uses it.
*/
class UuidBox : public Mp4Box
{
public:
	UuidBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_UUID,offset,size,aContainer)
	{
		m_UuidType = UUID_UNKNOWN;
	}

	UuidBox * GetUuidBox();

	int 	Load(Mp4File * pMp4File);
	void 	Dump(Formatter* pFormatter);

	int GetUuidType()	{ return m_UuidType; }
	void SetUuidType(int iType);

	char m_usertype[16];
	char m_version;
	long m_flags;

	// KDDI cpgd UUID.
	long m_copy_guard_attribute;
	long m_limit_date;
	long m_limit_period;
	long m_limit_count;

	// KDDI mvml
	long m_permission;
	char m_rec_mode;
	long m_rec_date;

	// KDDI enci
	char m_device[8+1];
	char m_model[8+1];
	char m_encoder[8+1];
	char m_multiplexer[8+1];

protected:
	int  m_UuidType;
};

class FreeBox : public Mp4Box
{
public:
	FreeBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_FREE,offset,size,aContainer)
	{
	}
protected:
};
class SkipBox : public Mp4Box
{
public:
	SkipBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_SKIP,offset,size,aContainer)
	{
	}
protected:
};

class MvhdBox : public Mp4Box
{
public:
	MvhdBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_MVHD,offset,size,aContainer)
	{
		//pFormatter->Print("MvhdBox.Create: %p\n",this);
	}
	~MvhdBox() 
	{
		//pFormatter->Print("MvhdBox.Destory: %p\n",this);
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 	m_version;
	char 	m_flags[3];
	unsigned long	m_creation_time;	
	unsigned long	m_modification_time;	
	long	m_timescale;	
	long	m_duration;	
	long	m_rate;	
	short	m_volume;	
	short	m_reserved1;	
	long	m_reserved2[2];	
	long	m_matrix[9];
	long	m_pre_defined[6];
	long	m_next_track_ID;
};

class TkhdBox : public Mp4Box
{
public:
	TkhdBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_TKHD,offset,size,aContainer)
	{
		//pFormatter->Print("TkhdBox.Create %p\n",this);
	}
	virtual ~TkhdBox()
	{
		//pFormatter->Print("TkhdBox.Destroy %p\n",this);
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 	m_version;
	char 	m_flags[3];
	unsigned long	m_creation_time;	
	unsigned long	m_modification_time;	
	long	m_track_ID;	
	long	m_reserved1;	
	unsigned long	m_duration;	
	long	m_reserved2[2];	
	long	m_pre_defined;
	short	m_volume;	
	short	m_reserved3;	
	long	m_matrix[9];
	long	m_width;
	long	m_height;
protected:
};

class MdhdBox : public Mp4Box
{
public:
	MdhdBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_MDHD,offset,size,aContainer)
	{
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 	m_version;
	char 	m_flags[3];
	unsigned long	m_creation_time;	
	unsigned long	m_modification_time;	
	unsigned long	m_timescale;	
	unsigned long	m_duration;	
	char	m_pad;	
	char	m_language[2];	
	short	m_pre_defined;
};

class HdlrBox : public Mp4Box
{
public:
	HdlrBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_HDLR,offset,size,aContainer)
	{
		m_name[0]=0;
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 	m_version;
	char 	m_flags[3];
	long	m_pre_defined;	
	long	m_handler_type;	
	char	m_reserved[12];
	char	m_name[2048];
};

class VmhdBox : public Mp4Box
{
public:
	VmhdBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_VMHD,offset,size,aContainer)
	{
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 	m_version;
	char 	m_flags[3];
	short m_graphicsmode;
	short m_opcolor[3];
};

class SmhdBox : public Mp4Box
{
public:
	SmhdBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_SMHD,offset,size,aContainer)
	{
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 	m_version;
	char 	m_flags[3];
	short m_balance;
	short m_reserved;
};

class NmhdBox : public Mp4Box
{
public:
	NmhdBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_NMHD,offset,size,aContainer)
	{
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 	m_version;
	char 	m_flags[3];
};

class UrlBox : public Mp4Box
{
public:
	UrlBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_URL,offset,size,aContainer)
	{
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 	m_version;
	char 	m_flags[3];
	char  m_location[2048];
};

class DrefBox : public Mp4Box
{
public:
	DrefBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_DREF,offset,size,aContainer)
	{
		m_data_entry = 0;
	}
	~DrefBox()
	{
		if(m_data_entry)
			delete [] m_data_entry;
		m_data_entry = 0;
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 	m_version;
	char 	m_flags[3];
	long	m_entry_count;
	UrlBox ** m_data_entry;
};

class DinfBox : public Mp4Box
{
public:
	DinfBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_DINF,offset,size,aContainer)
	{
		m_pDrefBox = 0;
	}
	virtual int Load(Mp4File * pMp4File);

protected:
	DrefBox *	m_pDrefBox;
};

class CBoxRecord 
{
public:
	short top;
	short left;
	short bottom;
	short right;
};

class CStyleRecord 
{
public:
	short startchar;
	short endchar;
	short font_id;
	char  face_style_flags;
	char  font_size;
	char  text_color_rgba[4];
};

class CFontRecord
{
public:

	CFontRecord() : font(0) {};
	~CFontRecord()
	{
		if(font)
			delete [] font;
	}

	short font_id;
	char  font_name_length;
	char* font;
};

class FtabBox : public Mp4Box
{
public:
	FtabBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_FTAB,offset,size,aContainer)
	{
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	short m_entry_count;
	CFontRecord m_FontRecord;
};

class Tx3gBox : public Mp4Box
{
public:
	Tx3gBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_TX3G,offset,size,aContainer),
		m_pFtabBox(0)
	{
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char	m_reserved[6];
	short	m_data_reference_index;
	long	m_displayflags;
	char	m_horizontal_justification;
	char	m_vertical_justification;
	char	m_background_color_rgba[4];

	CBoxRecord   m_BoxRecord;
	CStyleRecord m_StyleRecord;

	FtabBox * m_pFtabBox;

};

/**
Descriptor Abstraction.
*/
class Descr : public Mp4Com
{
public:
	Descr(eDescrTag aTag, int offset, int size, Descr * aContainer=0) :
	  Mp4Com(MP4DESCR),
		  m_Tag(aTag), 
		  m_iSize(size),
		  m_iBeginOffset(offset), 
		  m_pContainer(aContainer),
		  m_pBoxContainer(0)
	  {
	  };

	  virtual int  Load(Mp4File * pMp4File)=0;
	  virtual void Dump(Formatter* pFormatter) {};
	  virtual void DumpHeader(Formatter* pFormatter)
	  {
		  switch(m_Tag)
		  {
		  case kES_DescrTag:
			  pFormatter->Print("<ES_DescrTag ");
			  break;
		  case kDecoderConfigDescrTag:
			  pFormatter->Print("<DecoderConfigDescrTag ");
			  break;
		  case kDecSpecificInfoTag:
			  pFormatter->Print("<DecSpecificInfoTag ");
			  break;
		  case kSLConfigDescrTag:
			  pFormatter->Print("<SLConfigDescrTag ");
			  break;
		  default:
			  pFormatter->Print("<UnknownDescrTag Hex=\"0x%x ",m_Tag);
			  break;
		  }
		  pFormatter->Print(" offset=\"%d\" size=\"%d\">\n",m_iBeginOffset,m_iSize);
		  if(!pFormatter->UseDisplay())
		  {
			  pFormatter->Put(0,(char)m_Tag);
			  pFormatter->PutSizeOfInstance((long)m_iSize);
		  }
		  Dump(pFormatter);
	  }

	  virtual void 	DumpFooter(Formatter* pFormatter)
	  {
		  switch(m_Tag)
		  {
		  case kES_DescrTag:
			  pFormatter->Print("</ES_DescrTag>\n");
			  break;
		  case kDecoderConfigDescrTag:
			  pFormatter->Print("</DecoderConfigDescrTag>\n");
			  break;
		  case kDecSpecificInfoTag:
			  pFormatter->Print("</DecSpecificInfoTag>\n");
			  break;
		  case kSLConfigDescrTag:
			  pFormatter->Print("</SLConfigDescrTag>\n");
			  break;
		  default:
			  pFormatter->Print("</UnknownDescrTag>\n");
			  break;
		  }
	  }

	  Descr * 	GetContainer(eDescrTag aTag)
	  {
		  if(aTag==m_Tag)
			  return this;
		  else if (m_pContainer)
			  return m_pContainer->GetContainer(aTag);
		  return 0;
	  }	

	  /** return box container of decriptor if there is any */
	  Mp4Box * GetBoxContainer(BoxType aBoxType)
	  {
		  if (m_pBoxContainer)
			  return m_pBoxContainer->GetContainer(aBoxType);
		  else if(m_pContainer)
			  return m_pContainer->GetBoxContainer(aBoxType);
		  return 0;
	  }
	  /** set box container of decriptor */
	  void SetBoxContainer(Mp4Box * pBox)
	  {
		  m_pBoxContainer = pBox;
	  }

	  virtual void SetSize(int iSize) 
	  {
		  m_iSize = iSize-5;
	  }

	  eDescrTag 	m_Tag;	
	  int			m_iSize;

protected:
	Descr() : Mp4Com(MP4DESCR) {};

	int		m_iBeginOffset;
	Descr*	m_pContainer;
	Mp4Box*	m_pBoxContainer;
};

class RiffQcpFmt;
class DecoderSpecificInfo : public Descr
{
public:
	DecoderSpecificInfo(int offset, int size, Descr * aContainer=0) 
		: Descr(kDecSpecificInfoTag,offset,size,aContainer)
		,m_pVos(0)
	{
		info = 0;
		m_pVos = 0;
		m_pQcpFmt = 0;
	}
	~DecoderSpecificInfo();

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char * info;
	Vos *  m_pVos;
	RiffQcpFmt * m_pQcpFmt;
};


class DecoderConfigDescr : public Descr
{
public:
	DecoderConfigDescr(int offset, int size, Descr * aContainer=0) : Descr(kDecoderConfigDescrTag,offset,size,aContainer)
	{
		m_pDecoderSpecificInfo = 0;
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char m_objectTypeIndication;
	char m_streamType;
	char m_upStream;
	char m_reserved;
	long m_bufferSizeDB;
	long m_maxBitrate;
	long m_avgBitrate;

	DecoderSpecificInfo * m_pDecoderSpecificInfo;
};

class SLConfigDescr : public Descr
{
public:
	SLConfigDescr(int offset, int size, Descr * aContainer=0) : Descr(kSLConfigDescrTag,offset,size,aContainer)
	{
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char m_predefined;
	char m_useAccessUnitStartFlag;
	char m_useAccessUnitEndFlag;
	char m_useRandomAccessPointFlag;
	char m_hasRandomAccessUnitsOnlyFlag;
	char m_usePaddingFlag;
	char m_useTimeStampsFlag;
	char m_useIdleFlag;
	char m_durationFlag;
	long m_timeStampResolution;
	long m_OCRResolution;
	char m_timeStampLength;
	char m_OCRLength;
	char m_AU_Length;
	char m_instantBitrateLength;
	char m_degradationPriorityLength;
	char m_AU_seqNumLength;
	char m_apcketSeqNumLegnth;
	char m_reserved;
	long m_timeScale;
	short m_accessUnitDuration;
	short m_compositionUnitDuration;
#ifndef TI_ARM	
#if (!defined(__linux__) && _MSC_VER <= 1200) || (defined(ARMV4) && defined(_WIN32_WCE))
	__int64 m_startDecodingTimeStamp;
	__int64 m_startCompositionTimeStamp;
#else
	//	long long m_startDecodingTimeStamp;
	//	long long m_startCompositionTimeStamp;
	__int64 m_startDecodingTimeStamp;
	__int64 m_startCompositionTimeStamp;
#endif
#else
	int64s m_startDecodingTimeStamp;
	int64s m_startCompositionTimeStamp;
#endif	
};

class IPMP_Descriptor  : public Descr
{
public:
	IPMP_Descriptor(int offset, int size, Descr * aContainer=0) 
		: Descr(kIPMP_DescrTag,offset,size,aContainer),
		m_URLString(0),
		m_IPMP_data(0)
	{
	}
	~IPMP_Descriptor()
	{
		if(m_URLString)
			delete [] m_URLString;
		if(m_IPMP_data)
			delete [] m_IPMP_data;
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char	m_IPMP_DescriptorID;
	short	m_IPMPS_Type;
	char*	m_URLString;
	char*	m_IPMP_data;
};

/*
class KeyWordDescriptor  : public Descr
{
public:
KeyWordDescriptor(int offset, int size, Descr * aContainer=0) 
: Descr(kKeyWordDescrTag,offset,size,aContainer),
m_keyWordLength(0),
m_keyWord8(0),
m_keyWord16(0)
{
}
~KeyWordDescriptor()
{
int i;
if(m_keyWordLength)
delete [] m_keyWordLength;
if(m_keyWord8)
{
for(i=0;i<m_keyWordCount;i++)
delete [] m_keyWord8[i];
delete [] m_keyWord8;
}
if(m_keyWord16)
{
for(i=0;i<m_keyWordCount;i++)
delete [] m_keyWord16[i];
delete [] m_keyWord16;
}
}

virtual int Load(Mp4File * pMp4File);
virtual void Dump(Formatter* pFormatter);

long	m_languageCode;
char	m_isUTF8_string;
char	m_keyWordCount;
char *  m_keyWordLength;
char **	m_keyWord8;
short **m_keyWord16;
};

class ExpandedTextualDescriptor  : public Descr
{
public:
ExpandedTextualDescriptor(int offset, int size, Descr * aContainer=0) 
: Descr(kExpandedTextualDescriptor,offset,size,aContainer),
m_eventName8(0),
m_eventName16(0),
m_eventText8(0),
m_eventText16(0)
{
}
~ExpandedTextualDescriptor()
{
if(m_eventName8)
delete [] m_eventName8;
if(m_eventText8)
delete [] m_eventText8;
if(m_eventName16)
delete [] m_eventName16;
if(m_eventText16)
delete [] m_eventText16;
}

virtual int Load(Mp4File * pMp4File);
virtual void Dump(Formatter* pFormatter);

long	m_languageCode;
char	m_isUTF8_string;
char	m_itemCount
char*	m_itemDescriptionLength;
char*	m_itemDescription8;
short*	m_itemDescription16;
char*	m_itemLength; 
char*	m_itemText8;
short*	m_itemText16;
char	m_textLength;
long	m_nonItemTextLength;
char*	m_nonItemText8;
short*	m_nonItemText16;
};

class ShortTextualDescriptor  : public Descr
{
public:
ShortTextualDescriptor(int offset, int size, Descr * aContainer=0) 
: Descr(kShortTextualDescrTag,offset,size,aContainer),
m_eventName8(0),
m_eventName16(0),
m_eventText8(0),
m_eventText16(0)
{
}
~ShortTextualDescriptor()
{
if(m_eventName8)
delete [] m_eventName8;
if(m_eventText8)
delete [] m_eventText8;
if(m_eventName16)
delete [] m_eventName16;
if(m_eventText16)
delete [] m_eventText16;
}

virtual int Load(Mp4File * pMp4File);
virtual void Dump(Formatter* pFormatter);

long m_languageCode;
char m_isUTF8_string;
char m_nameLength;
char* m_eventName8;
short* m_eventName16;
char m_textLength;
char* m_eventText8; 
short* m_eventText16;
};

class RatingDescriptor  : public Descr
{
public:
RatingDescriptor(int offset, int size, Descr * aContainer=0) 
: Descr(kRatingDescrTag,offset,size,aContainer),
m_ratingInfo(0)
{
}
~RatingDescriptor()
{
if(m_ratingInfo)
delete [] m_ratingInfo;
}

virtual int Load(Mp4File * pMp4File);
virtual void Dump(Formatter* pFormatter);

long	m_ratingEntity;
short	m_ratingCriteria;
char*	m_ratingInfo;
};

class ContentClassificationDescriptor  : public Descr
{
public:
ContentClassificationDescriptor(int offset, int size, Descr * aContainer=0) 
: Descr(kContentClassificationDescrTag,offset,size,aContainer),
m_contentClassificationData(0)
{
}
~ContentClassificationDescriptor()
{
if(m_contentClassificationData)
delete [] m_contentClassificationData;
}

virtual int Load(Mp4File * pMp4File);
virtual void Dump(Formatter* pFormatter);

long	m_classificationEntity;
short	m_classificationTable;
char*	m_contentClassificationData;
};
*/
class InitialObjectDescriptor : public Descr
{
public:
	InitialObjectDescriptor(int offset, int size, Descr * aContainer=0) 
		: Descr(kInitialObjectDescrTag,offset,size,aContainer),
		m_URLstring(0)
	{
	}
	~InitialObjectDescriptor()
	{
		if(m_URLstring)
			delete [] m_URLstring;
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	short m_ObjectDescriptorID;
	char m_URL_Flag;
	unsigned char m_URLlength;
	char m_includeInlineProfileLevelFlag;
	char m_reserved;
	char m_ODProfileLevelIndication;
	char m_sceneProfileLevelIndication;
	char m_audioProfileLevelIndication;
	char m_visualProfileLevelIndication;
	char m_graphicsProfileLevelIndication;
	char m_URLLength;
	char * m_URLstring;
};


class ObjectDescriptor : public Descr
{
public:
	ObjectDescriptor(int offset, int size, Descr * aContainer=0) 
		: Descr(kObjectDescrTag,offset,size,aContainer),
		m_URLstring(0)
	{
	}
	~ObjectDescriptor()
	{
		if(m_URLstring)
			delete [] m_URLstring;
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);


	short m_ObjectDescriptorID;
	char m_URL_Flag;
	char m_reserved;
	unsigned char m_URLlength;
	char * m_URLstring;
};

class ES_Descr : public Descr
{
public:
	ES_Descr(int offset, int size, Descr * aContainer=0) : Descr(kES_DescrTag,offset,size,aContainer)
	{
		m_ES_ID = 1; // ??
		m_streamDependenceFlag = 0;
		m_URL_Flag = 0;
		m_OCRstreamFlag = 0;
		m_streamPriority = 16;
		m_dependsOn_ES_ID = 0;
		m_URLstring = 0;
		m_URLlength = 0;
		m_pDecoderConfigDescr = 0;	
		m_pSLConfigDescr = 0;	
	}
	~ES_Descr()
	{
		if(m_URLstring)
			delete [] m_URLstring;
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	short m_ES_ID;
	char m_streamDependenceFlag;
	char m_URL_Flag;
	char m_OCRstreamFlag;
	char m_streamPriority;
	short m_dependsOn_ES_ID;
	unsigned char m_URLlength;
	char * m_URLstring;
	short m_OCR_ES_ID;

	DecoderConfigDescr * m_pDecoderConfigDescr;	
	SLConfigDescr * m_pSLConfigDescr;	
};

class EsdsBox : public Mp4Box
{
public:
	EsdsBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_ESDS,offset,size,aContainer)
	{
		m_pES_Descr = 0;
		m_version = 0;
		m_flags[0] = m_flags[1] = m_flags[2] = 0;
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 	m_version;
	char 	m_flags[3];

	ES_Descr * m_pES_Descr;
};

class GlblBox : public Mp4Box
{
public:
	GlblBox(int offset, int size, Mp4Box* aContainer=0) : Mp4Box(BOX_GLBL, offset, size, aContainer)
	{
		m_pGlblInfo = NULL;
		m_lInfoLen = 0;
	}
	virtual ~GlblBox()
	{
		if (m_pGlblInfo != NULL)
			delete [] m_pGlblInfo;
	}
	virtual int Load(Mp4File * pMp4File);

	char* m_pGlblInfo;
	long  m_lInfoLen;
};

class UlawBox : public Mp4Box
{
public:
	UlawBox(int offset, int size, Mp4Box * aContainer=0);
	virtual int Load(Mp4File * pMp4File);
	char	m_reserved1[6];
	short m_data_reference_index;
	long	m_reserved2[2];
	short m_channelcount;
	short m_samplesize;
	short m_pre_defined;
	short m_reserved3;
	short m_sampleratehi;
	short m_sampleratelo;
};

class AlawBox : public Mp4Box
{
public:
	AlawBox(int offset, int size, Mp4Box * aContainer=0);
	virtual int Load(Mp4File * pMp4File);
	char	m_reserved1[6];
	short m_data_reference_index;
	long	m_reserved2[2];
	short m_channelcount;
	short m_samplesize;
	short m_pre_defined;
	short m_reserved3;
	short m_sampleratehi;
	short m_sampleratelo;
};

class AlacBox : public Mp4Box
{
public:
	AlacBox(int offset, int size, Mp4Box * aContainer=0);
	virtual int Load(Mp4File * pMp4File);
	char  m_extra_data[36];
};

class Ima4Box : public Mp4Box
{
public:
	Ima4Box(int offset, int size, Mp4Box * aContainer=0); 
	virtual int Load(Mp4File * pMp4File);
	char	m_reserved1[6];
	short m_data_reference_index;
	long	m_reserved2[2];
	short m_channelcount;
	short m_samplesize;
	short m_pre_defined;
	short m_reserved3;
	short m_sampleratehi;
	short m_sampleratelo;
};

class Mp3Box : public Mp4Box
{
public:
	Mp3Box(int offset, int size, Mp4Box * aContainer=0); 
	virtual int Load(Mp4File * pMp4File);
	char	m_reserved1[6];
	short m_data_reference_index;
	long	m_reserved2[2];
	short m_channelcount;
	short m_samplesize;
	short m_pre_defined;
	short m_reserved3;
	short m_sampleratehi;
	short m_sampleratelo;
};

class Mp4vBox : public Mp4Box
{
public:
	Mp4vBox(int offset, int size, Mp4Box * aContainer=0);

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);
	int GetObjectTypeIndication();

	char 	m_reserved1[6];
	short m_data_reference_index;
	short m_pre_defined1;
	short m_reserved2;
	long  m_reserved3[3];
	short	m_width;
	short	m_height;
	long	m_hor_resolution;
	long	m_vert_resolution;
	long	m_reserved4;
	short	m_pre_defined2;
	char	m_compressor_name[32+1];
	short	m_depth;
	short	m_pre_defined3;
	EsdsBox * m_pEsdsBox;
};

class DivxBox : public Mp4Box
{
public:
	DivxBox(int offset, int size, Mp4Box * aContainer=0);

	virtual int Load(Mp4File * pMp4File);

	char 	m_reserved1[6];
	short m_data_reference_index;
	short m_pre_defined1;
	short m_reserved2;
	long  m_reserved3[3];
	short	m_width;
	short	m_height;
	long	m_hor_resolution;
	long	m_vert_resolution;
	long	m_reserved4;
	short	m_pre_defined2;
	char	m_compressor_name[32+1];
	short	m_depth;
	short	m_pre_defined3;
	GlblBox* m_pGlblBox;
};

class XvidBox : public Mp4Box
{
public:
	XvidBox(int offset, int size, Mp4Box * aContainer=0);

	virtual int Load(Mp4File * pMp4File);

	char 	m_reserved1[6];
	short m_data_reference_index;
	short m_pre_defined1;
	short m_reserved2;
	long  m_reserved3[3];
	short	m_width;
	short	m_height;
	long	m_hor_resolution;
	long	m_vert_resolution;
	long	m_reserved4;
	short	m_pre_defined2;
	char	m_compressor_name[32+1];
	short	m_depth;
	short	m_pre_defined3;
};

class Mp4aBox : public Mp4Box
{
public:
	Mp4aBox(int offset, int size, Mp4Box * aContainer=0) ;

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);
	int GetObjectTypeIndication();

	char	m_reserved1[6];
	short m_data_reference_index;
	long	m_reserved2[2];
	short m_channelcount;
	short m_samplesize;
	short m_pre_defined;
	short m_reserved3;
	short m_sampleratehi;
	short m_sampleratelo;

	EsdsBox * m_pEsdsBox;
};

struct AVC_SPS_NAL
{
	unsigned short sequenceParameterSetLength;
	char* sequenceParameterSetNALUnit;
};
struct AVC_PPS_NAL
{
	unsigned short pictureParameterSetLength;
	char* pictureParameterSetNALUnit;
};
class AvcBox : public Mp4Box
{
public:
	AvcBox(int offset, int size, Mp4Box * aContainer=0);
	~AvcBox();

	virtual int Load(Mp4File * pMp4File);

	char 	m_reserved1[6];
	short m_data_reference_index;
	short m_pre_defined1;
	short m_reserved2;
	long  m_reserved3[3];
	short	m_width;
	short	m_height;
	long	m_hor_resolution;
	long	m_vert_resolution;
	long	m_reserved4;
	short	m_pre_defined2;
	char	m_compressor_name[32+1];
	short	m_depth;
	short	m_pre_defined3;

	//AVCC
	unsigned char configurationVersion;
	unsigned char AVCProfileIndication;
	unsigned char profile_compatibility;
	unsigned char AVCLevelIndication;
	unsigned char lengthSizeMinusOne;
	unsigned char numOfSequenceParameterSets;
	unsigned char numOfPictureParameterSets;

	AVC_SPS_NAL*	sps;
	AVC_PPS_NAL*	pps;
};

class BitrBox : public Mp4Box
{
public:
	BitrBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_BITR,offset,size,aContainer)
	{
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	long m_avg_bitrate;
	long m_max_bitrate;
};


class D263Box : public Mp4Box
{
public:
	D263Box(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_D263,offset,size,aContainer),
		m_pBitrBox(0)
	{
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	long  m_vendor;
	char  m_decoder_version;
	char  m_h263_level;
	char  m_h263_profile;

	BitrBox * m_pBitrBox;
};


class S263Box : public Mp4Box
{
public:
	S263Box(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_S263,offset,size,aContainer),
		m_pD263Box(0)
	{
		memset(m_reserved1, 0x0, 6);
		m_data_reference_index = 1;
		m_pre_defined1 = 0;
		m_reserved2 = 0;
		memset(m_reserved3, 0x0, sizeof(long)*3);
		m_width = 0;
		m_height = 0;
		m_hor_resolution = 0x00480000;
		m_vert_resolution = 0x00480000;
		m_reserved4 = 0;
		m_pre_defined2 = 1;
		memset(m_compressor_name,0x0, 33);
		m_depth = 24;
		m_pre_defined3 = -1;
		m_pD263Box = 0;
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char  m_reserved1[6];
	short m_data_reference_index;
	short m_pre_defined1;
	short m_reserved2;
	long  m_reserved3[3];
	short m_width;
	short m_height;
	long  m_hor_resolution;
	long  m_vert_resolution;
	long  m_reserved4;
	short m_pre_defined2;
	char  m_compressor_name[32+1];
	short m_depth;
	short m_pre_defined3;

	D263Box * m_pD263Box;
};



class DamrBox : public Mp4Box
{
public:
	DamrBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_DAMR,offset,size,aContainer)
	{
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	long 	m_type;
	long	m_vendor;
	char	m_decoder_version;
	short	m_mode_set;
	char	m_mode_change_period;
	char	m_frames_per_sample;
};

class SamrBox : public Mp4Box
{
public:
	SamrBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_SAMR,offset,size,aContainer)
	{
		m_pDamrBox = 0;
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char	m_reserved1[6];
	short	m_data_reference_index;
	long	m_reserved2[2];
	short m_channelcount;
	short m_samplesize;
	short m_pre_defined;
	short m_reserved3;
	short m_sampleratehi;
	short m_sampleratelo;

	DamrBox * m_pDamrBox;

};


class StsdBox : public Mp4Box
{
public:
	StsdBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_STSD,offset,size,aContainer)
	{
		m_SampleEntry = 0;
	}
	~StsdBox()
	{
		if(m_SampleEntry)
			delete [] m_SampleEntry;
		m_SampleEntry = 0;
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 	m_version;
	char 	m_flags[3];
	long	m_entry_count;

protected:
	Mp4Box ** m_SampleEntry;
};

struct tagSTTSBOX
{
	long sample_count;
	long sample_delta;
};
class SttsBox : public Mp4Box
{
public:
	SttsBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_STTS,offset,size,aContainer),
		//		m_sample_count(0),
		//		m_sample_delta(0)
		m_entry_prevsize(32),
		m_entry_size(0),
		m_entry(0),
		m_sample_starttime(0),
		m_sample_startidx(0),
		m_sample_delta(0)
	{
	}
	~SttsBox()
	{
		//		if(m_sample_count)
		//			delete [] m_sample_count;
		//		m_sample_count = 0;
		//		if(m_sample_delta)
		//			delete [] m_sample_delta;
		//		m_sample_delta = 0;
		if(m_entry)
			delete [] m_entry;
		m_entry = 0;
		if(m_sample_starttime)
			delete [] m_sample_starttime;
		m_sample_starttime = 0;
		if(m_sample_startidx)
			delete [] m_sample_startidx;
		m_sample_startidx = 0;
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);
	int GetSampleIdx(int iTime);
	long GetCTS(long lIdx);

	char 	m_version;
	char 	m_flags[3];
	//	long	* m_sample_count;
	//	long	* m_sample_delta;
	//sunstars read optimize
	long	m_entry_prevsize; //@xsh
	long	m_entry_size; //@xsh
	long	m_entry_count;
	tagSTTSBOX	*m_entry;
	long	m_sample_num;
	long  m_total_delta;

protected:
	long	* m_sample_starttime;
	long	* m_sample_startidx;
	long	m_sample_delta;
};

class CttsBox : public Mp4Box
{
public:
	CttsBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_CTTS,offset,size,aContainer)
	{
		m_sample_count = 0;
		m_sample_compo_offset = 0;
	}
	~CttsBox()
	{
		if(m_sample_count)
			delete [] m_sample_count;
		m_sample_count = 0;
		if(m_sample_compo_offset)
			delete [] m_sample_compo_offset;
		if(m_sample_offset)
			delete [] m_sample_offset;
		m_sample_compo_offset = 0;
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);
	long GetCTSoffset(long lIdx);
	char 	m_version;
	char 	m_flags[3];
	long	m_entry_count;
	long	* m_sample_count;
	long	* m_sample_compo_offset;
	long	m_sample_num;
	long	* m_sample_offset;
};

struct tagSTSCBOX
{
	long first_chunk;
	long samples_per_chunk;
	long sample_description_index;
};
class StscBox : public Mp4Box
{
public:
	StscBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_STSC,offset,size,aContainer)
	{
		//		m_first_chunk = 0;	
		//		m_samples_per_chunk = 0;	
		//		m_sample_description_index = 0;	
		m_entry = 0;
	}

	~StscBox()
	{
		//		if(m_first_chunk)
		//			delete [] m_first_chunk;
		//		m_first_chunk = 0;	
		//		if(m_samples_per_chunk)
		//			delete [] m_samples_per_chunk;
		//		m_samples_per_chunk = 0;	
		//		if(m_sample_description_index)
		//			delete [] m_sample_description_index;
		//		m_sample_description_index = 0;	
		if(m_entry)
			delete [] m_entry;
		m_entry = 0;
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 		m_version;
	char 		m_flags[3];
	long		m_entry_count;
	//	long * 	m_first_chunk;	
	//	long * 	m_samples_per_chunk;	
	//	long * 	m_sample_description_index;	
	//  sunstars read optimize
	tagSTSCBOX* m_entry;
};

class StszBox : public Mp4Box
{
public:
	StszBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_STSZ,offset,size,aContainer),
		m_lTotalSampleSize(0)
	{
		m_entry_size = 0;
		m_entry_offset = 0;
		m_lMaxSampleSize = 0;
	}
	~StszBox()
	{
		if(m_entry_size)
			delete [] m_entry_size;
		m_entry_size = 0;
		if(m_entry_offset)
			delete [] m_entry_offset;
		m_entry_offset = 0;
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	long		GetTotalSampleSize()
	{
		return m_lTotalSampleSize;
	}		
	long		GetMaxSampleSize()
	{
		return m_lMaxSampleSize;
	}	

	// SampleIdx[1..m_sample_count]
	long		GetSampleSize(int SampleIdx)
	{
		if(SampleIdx < 1)
			SampleIdx = 1;
		else if(SampleIdx > m_sample_count)
			return 0;
		return m_sample_size!=0?m_sample_size : m_entry_size[SampleIdx-1];
	}
	
	long		SetSampleSize(long SampleSize, long MaxSampleSize = 0);

	long GetSampleOffset(int SampleIdx)
	{
		if(SampleIdx < 1)
			SampleIdx = 1;
		else if(SampleIdx > m_sample_count)
			return 0;
		return m_sample_size!=0?m_sample_size*(SampleIdx-1) : m_entry_offset[SampleIdx-1];
	}

	long	GetSampleIdx(long lSampleLocation);

	char 		m_version;
	char 		m_flags[3];
	long		m_sample_size;
	long		m_sample_count;
	long * 	m_entry_size;	
	long *	m_entry_offset; //stream offset
protected:
	long		m_lTotalSampleSize;
	long		m_lMaxSampleSize;
};

class StcoBox : public Mp4Box
{
public:
	StcoBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_STCO,offset,size,aContainer)
	{
		m_chunk_offset = 0;
		m_isCo64 = false;
	}
	~StcoBox()
	{
		if(m_chunk_offset)
			delete [] m_chunk_offset;
		m_chunk_offset = 0;
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);
	virtual void SetCo64(bool isCo64);
	char 		m_version;
	char 		m_flags[3];
	long		m_entry_count;
	unsigned long long	*	m_chunk_offset;
	bool		m_isCo64;
};

class StssBox : public Mp4Box
{
public:
	StssBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_STSS,offset,size,aContainer)
	{
		m_sample_number = 0;
	}
	~StssBox()
	{
		if(m_sample_number)
			delete [] m_sample_number;
		m_sample_number = 0;
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);
	int 	GetClosestSyncSampleIdx(int iSampleIdx);
	int 	GetSafeSyncSampleIdx(int iSampleIdx);

	char 		m_version;
	char 		m_flags[3];
	long		m_entry_count;
	long *	m_sample_number;
};


class StblBox : public Mp4Box
{
public:
	StblBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_STBL,offset,size,aContainer)
	{
		m_pStsdBox = 0;
		m_pSttsBox = 0;
		m_pStscBox = 0;
		m_pStszBox = 0;
		m_pStcoBox = 0;
		m_pStssBox = 0;
		m_pCttsBox = 0;
	}

	virtual int Load(Mp4File * pMp4File);

	StsdBox * 	m_pStsdBox;
	SttsBox * 	m_pSttsBox;
	StscBox * 	m_pStscBox;
	StszBox * 	m_pStszBox;
	StcoBox * 	m_pStcoBox;
	StssBox * 	m_pStssBox;
	CttsBox * 	m_pCttsBox;

};

class MinfBox : public Mp4Box
{
public:
	MinfBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_MINF,offset,size,aContainer)
	{
		m_pVmhdBox = 0;
		m_pSmhdBox = 0;
		m_pNmhdBox = 0;
		m_pDinfBox = 0;
		m_pStblBox = 0;
		m_pHdlrBox = 0;
	}

	virtual int Load(Mp4File * pMp4File);

	VmhdBox *	m_pVmhdBox;
	SmhdBox *	m_pSmhdBox;
	NmhdBox *	m_pNmhdBox;
	DinfBox *	m_pDinfBox;
	StblBox *	m_pStblBox;
	HdlrBox *	m_pHdlrBox;
};

class MdiaBox : public Mp4Box
{
public:
	MdiaBox(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_MDIA,offset,size,aContainer)
	{
		m_pMdhdBox = 0;
		m_pHdlrBox = 0;
		m_pMinfBox = 0;
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	MdhdBox *	m_pMdhdBox;
	HdlrBox *	m_pHdlrBox;
	MinfBox *	m_pMinfBox;
};

class TrexBox : public Mp4Box
{
public:
	TrexBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_TREX,offset,size,aContainer)
	{
	}
	~TrexBox()
	{
	}
	virtual int  Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char	m_version;
	char 	m_flags[3];
	long	m_track_ID;
	long	m_default_sample_description_index;
	long	m_default_sample_duration;
	long	m_default_sample_size;
	long	m_default_sample_flags;
};

class MehdBox : public Mp4Box
{
public:
	MehdBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_MEHD,offset,size,aContainer)
	{
	}
	~MehdBox()
	{
	}

	virtual int  Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char	m_version;
	char 	m_flags[3];
	long	m_fragment_duration;
};


class MvexBox : public Mp4Box
{
public:
	MvexBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_MVEX,offset,size,aContainer),
		m_pMehdBox(0), m_pTrexBox(0)
	{
	}

	virtual int  Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter) {};

	MehdBox*	m_pMehdBox;
	TrexBox*	m_pTrexBox;
};


class CprtBox : public Mp4Box
{
public:
	CprtBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_CPRT,offset,size,aContainer),
		m_language(0), m_notice(0)
	{
	}
	~CprtBox()
	{
		if(m_notice)
			delete [] m_notice;
	}

	virtual int  Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char	m_version;
	char 	m_flags[3];
	long	m_language; // const bit(1)=0, unsigned int(5)[3]
	char*	m_notice;
};

class IlstChildBox : public Mp4Box
{
public:
	IlstChildBox(BoxType aBoxType, int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(aBoxType,offset,size,aContainer),
		m_language(0), m_content(0),m_codes(0),
		m_boxtype(aBoxType),m_size(0)
	{
	
	}

	~IlstChildBox()
	{
		if(m_content)
			delete [] m_content;
	}
	virtual void Dump(Formatter* pFormatter)
	{
		pFormatter->Print("<info>DoCoMo's Mobile MP4 extension.</info>\n");
	}

	virtual int  Load(Mp4File * pMp4File);

	BoxType m_boxtype;
	int   m_size;     //
	long	m_language; //
	char*	m_content;
	int     m_codes;
};

class IlstBox : public Mp4Box
{
public:
	IlstBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_ILST,offset,size,aContainer),
        m_pCpyBox(0), m_pPrfBox(0), m_pPrdBox(0), m_pNamBox(0), m_pInfBox(0), m_pAlbBox(0), m_pCanDataBox(0), m_pCovrBox(0), m_pDescBox(0)
	{
	}
	 
	virtual int  Load(Mp4File * pMp4File);
	virtual int  LoadIlstCvr(Mp4File * pMp4File);

	IlstChildBox*     m_pCpyBox;
	IlstChildBox*     m_pPrfBox;
	IlstChildBox*     m_pPrdBox;
	IlstChildBox*     m_pNamBox;
	IlstChildBox*     m_pInfBox;
	IlstChildBox*     m_pAlbBox;
	IlstChildBox*     m_pCanDataBox;
    IlstChildBox*     m_pCovrBox;
    IlstChildBox*     m_pDescBox;
};

class MetaBox : public Mp4Box
{
public:
	MetaBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_META,offset,size,aContainer),
		m_pIlstBox(0)
	{
	}
	
	virtual int  Load(Mp4File * pMp4File);
	virtual int  LoadMetaIlst(Mp4File * pMp4File);

	IlstBox*     m_pIlstBox;

};

class UdtaSubBox : public Mp4Box
{
public:
	UdtaSubBox(BoxType aBoxType, int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(aBoxType,offset,size,aContainer),
		m_language(0), m_notice(0),m_codes(0),m_size(0)
	{
	}
	~UdtaSubBox()
	{
		if(m_notice)
			delete [] m_notice;
	}

	virtual int  Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char	m_version;
	char 	m_flags[3];
	long	m_language; // const bit(1)=0, unsigned int(5)[3]
	char*	m_notice;
	int     m_codes;
	int     m_size;
};

class UdtaChildBox : public Mp4Box
{
public:
	UdtaChildBox(BoxType aBoxType, int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(aBoxType,offset,size,aContainer),
		m_language(0), m_content(0),m_codes(0),
		m_boxtype(aBoxType),m_size(0)
	{
	
	}

	~UdtaChildBox()
	{
		if(m_content)
			delete [] m_content;
	}
	virtual void Dump(Formatter* pFormatter)
	{
		pFormatter->Print("<info>DoCoMo's Mobile MP4 extension.</info>\n");
	}

	virtual int  Load(Mp4File * pMp4File);

	BoxType m_boxtype;
	short   m_size;     //
	long	m_language; //
	char*	m_content;
	int     m_codes;
};

class UdtaBox : public Mp4Box
{
public:
	UdtaBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_UDTA,offset,size,aContainer),
		m_pCprtBox(0),m_pPerfBox(0),m_pAuthBox(0),m_pTitlBox(0),m_pDscpBox(0),
		m_pMetaBox(0),
		m_pCpyBox(0),m_pPrfBox(0),m_pPrdBox(0),m_pNamBox(0),m_pInfBox(0)
	{
	}
	 
	virtual int  Load(Mp4File * pMp4File);
	virtual int  LoadUdtaMeta(Mp4File * pMp4File);
	MetaBox* m_pMetaBox;

	UdtaSubBox*	m_pCprtBox;
	UdtaSubBox*	m_pPerfBox;
	UdtaSubBox*	m_pAuthBox;
	UdtaSubBox*	m_pTitlBox;
	UdtaSubBox*	m_pDscpBox;

	UdtaChildBox*     m_pCpyBox;
	UdtaChildBox*     m_pPrfBox;
	UdtaChildBox*     m_pPrdBox;
	UdtaChildBox*     m_pNamBox;
	UdtaChildBox*     m_pInfBox;

};

class TrakBox : public Mp4Box
{
public:
	TrakBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_TRAK,offset,size,aContainer),
		m_pTkhdBox(0),
		m_pMdiaBox(0),
		m_pUdtaBox(0),
		m_pSource(0)
	{
	}

	virtual int 	Load(Mp4File * pMp4File);
	long				GetStreamSize();
	Mp4File *		GetSourceFile() { return m_pSource; }
	void 				SetSourceFile(Mp4File * pSource) { m_pSource=pSource; }

	TkhdBox 	* m_pTkhdBox;
	MdiaBox 	* m_pMdiaBox;
	UdtaBox 	* m_pUdtaBox;
protected:
	Mp4File * m_pSource;
};

class IodsBox : public Mp4Box
{
public:
	IodsBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_IODS,offset,size,aContainer)
	{
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 	m_version;
	char 	m_flags[3];

	ES_Descr * m_pES_Descr;
};

class DrmBox : public Mp4Box
{
public:
	DrmBox(int offset, int size, Mp4Box * aContainer);
	~DrmBox()
	{
		if(m_data)
			delete [] m_data;
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	void NotifyFileSize(int iFileSize);

	char*	m_data;
	long	m_data_len;
	bool  m_bRinger;
	bool	m_bForward;
	int 	m_iFileSize;
};

#define MAX_TRACK_NUM 256
class MoovBox : public Mp4Box
{
public:
	MoovBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_MOOV,offset,size,aContainer)
		,m_pMvhdBox(0)
		,m_pMvexBox(0)
		,m_pDrmBox(0)
		,m_pUdtaBox(0)
	{
		int i=0;
		m_iNextTrak = 0;
		for(i=0;i<MAX_TRACK_NUM;i++)
			m_pTrakBox[i] = 0;
	}

	virtual int Load(Mp4File * pMp4File);
	virtual int LoadMoovUdta(Mp4File * pMp4File);
	virtual int LoadMvhd(Mp4File * pMp4File);
	virtual int GetUdtaData(MP4Demux_UserData* pUserData);
	virtual int GetCanData(char *buf, unsigned int *frameSize);
	MvhdBox *	m_pMvhdBox;
	TrakBox *	m_pTrakBox[MAX_TRACK_NUM];
	MvexBox *   m_pMvexBox;
	DrmBox *    m_pDrmBox;
	int			m_iNextTrak;
	UdtaBox *   m_pUdtaBox;
};

class TfhdBox : public Mp4Box
{
public:
	TfhdBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_TFHD,offset,size,aContainer)
	{
	}

	virtual int  Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char	m_version;
	char 	m_flags[3];
	long	m_track_ID;
#ifndef TI_ARM	
#if (!defined(__linux__) && _MSC_VER <= 1200) || (defined(ARMV4) && defined(_WIN32_WCE))
	__int64 m_base_data_offset;
#else
	//	long long m_base_data_offset;
	__int64 m_base_data_offset;
#endif
#else
	int64s m_base_data_offset;
#endif

	long	m_sample_description_index;
	long	m_default_sample_duration;
	long	m_default_sample_size;
	long	m_default_sample_flags;
};

class TrunBox : public Mp4Box
{
public:
	TrunBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_TRUN,offset,size,aContainer)
	{
	}

	virtual int  Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char	m_version;
	char 	m_flags[3];
	long	m_sample_count;
	long	m_data_offset;
	long	m_first_sample_flags;
	long	m_sample_duration;
	long	m_sample_size;
	long	m_sample_flags;
	long	m_sample_composition_time_offset;
	long	m_sample_degradation_priority;
};

class TrafBox : public Mp4Box
{
public:
	TrafBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_TRAF,offset,size,aContainer),
		m_pTfhdBox(0), m_pTrunBox(0)
	{
	}

	virtual int  Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter) {}

	TfhdBox*	m_pTfhdBox;
	TrunBox*	m_pTrunBox;
};

class MfhdBox : public Mp4Box
{
public:
	MfhdBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_MFHD,offset,size,aContainer)
	{
	}

	virtual int  Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char	m_version;
	char 	m_flags[3];
	long	m_sequence_number;
};

class MoofBox : public Mp4Box
{
public:
	MoofBox(int offset, int size, Mp4Box * aContainer=0) 
		: Mp4Box(BOX_MOOF,offset,size,aContainer),
		m_pMfhdBox(0), m_pTrafBox(0)
	{
	}

	virtual int  Load(Mp4File * pMp4File);

	MfhdBox*	m_pMfhdBox;
	TrafBox*	m_pTrafBox;
};

#ifdef MP4_MUX

template<class Type>
class BOXBUF
{
public:
	BOXBUF()
	{
		pBuf = new Type[BOXBUFLEN];
		pNext = NULL;
	}
	~BOXBUF()
	{
		delete []pBuf;
		pBuf = NULL;
	}
	Type*			pBuf;
	BOXBUF*			pNext;
};

template<class Type>
class CBoxBuf
{
public:
	CBoxBuf()
	{
		pBufHead = pBufNode = NULL;
		iBufNum = 1;
		iBufIndx = 0;
		ResetIterater();
	}

	virtual ~CBoxBuf()
	{
		ReleaseBuf();
	}

	void ReleaseBuf()
	{
		BOXBUF<Type>* pTmpNode = NULL;
		while (pBufHead != NULL)
		{
			pTmpNode = pBufHead->pNext;
			delete pBufHead;
			pBufHead = pTmpNode;
		}
		pBufNode = pBufHead = NULL;
		ResetIterater();
	}

	void AddElement(Type element)
	{
		if (pBufNode == NULL)
		{
			ReleaseBuf();
			pBufNode = new BOXBUF<Type>();
			pBufHead = pBufNode;
			iBufNum = 1;
			iBufIndx = 0;	
		}

		if (iBufIndx == BOXBUFLEN)
		{
			pBufNode->pNext = new BOXBUF<Type>();
			pBufNode = pBufNode->pNext;
			++iBufNum;
			iBufIndx = 0;
		}
		pBufNode->pBuf[iBufIndx++] = element;
	}

	void ResetIterater()
	{
		pIterBuf = pBufHead;
		iIterIndx = 0;
	}

	bool IterateElement(Type* pElement)
	{
		if (iIterIndx == BOXBUFLEN && pIterBuf != NULL)
		{
			pIterBuf = pIterBuf->pNext;
			iIterIndx = 0;
		}
		if (pIterBuf == NULL)
			return false;
		*pElement = pIterBuf->pBuf[iIterIndx++];
		return true;
	}

	Type* GetPtrAt(int indx)
	{
		if (indx < 0 || pBufHead == NULL)
			return NULL;
		int iBufNum = indx / BOXBUFLEN;
		BOXBUF<Type>* pCurBuf = pBufHead;
		for (int i = 0; i < iBufNum, pCurBuf != NULL; ++i)
			pCurBuf = pCurBuf->pNext;
		if (pCurBuf == NULL)
			return NULL;
		return (pCurBuf->pBuf) + (indx % BOXBUFLEN) - 1;
	}

protected:
	BOXBUF<Type>*		pBufHead;
	BOXBUF<Type>*		pBufNode;
	int					iBufNum;
	int					iBufIndx;

	BOXBUF<Type>*		pIterBuf;
	int					iIterIndx;
};


class StszBoxMem : public Mp4Box, public CBoxBuf<long>
{
public:
	StszBoxMem(int offset, int size, Mp4Box * aContainer=0);
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 		m_version;
	char 		m_flags[3];
	long		m_sample_size;
	long		m_sample_count;
protected:
	long		m_lTotalSampleSize;
};

class StssBoxMem : public Mp4Box, public CBoxBuf<long>
{
public:
	StssBoxMem(int offset, int size, Mp4Box * aContainer=0);
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 		m_version;
	char 		m_flags[3];
	long		m_entry_count;


};

class StcoBoxMem : public Mp4Box, public CBoxBuf<long>
{
public:
	StcoBoxMem(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_STCO,offset,size,aContainer)
	{
	}

	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 		m_version;
	char 		m_flags[3];
	long		m_entry_count;
};

class StscBoxMem : public Mp4Box
{
public:
	StscBoxMem(int offset, int size, Mp4Box * aContainer=0) : Mp4Box(BOX_STSC,offset,size,aContainer)
	{
	}
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);

	char 		m_version;
	char 		m_flags[3];
	long		m_entry_count;


	CBoxBuf<long> m_FirstChunk;
	CBoxBuf<long> m_SampsPerChk;
	CBoxBuf<long> m_SampDscIndx;
};

#endif //MP4_MUX used

#endif

