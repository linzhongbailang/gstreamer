#ifndef MP4AUDIO_H_
#define MP4AUDIO_H_

/** mpeg audio frame header defined in 11171-3 */
class Mp4AudioFrameHeader : public Mp4Com
{
public:
	Mp4AudioFrameHeader(int offset,int size) :
		Mp4Com(MP4AUDIOFRAMEHEADER)
		{
		m_bNotLoaded = true;
		m_iBeginOffset = offset;
		m_iSize = size;
		};
	virtual int 	Load(Mp4File * pMp4File);
	virtual void 	Dump(Formatter* pFormatter);

	/** sync syncword and load header information 
		 return -1 on failure
		 return start offset of header on success.
	*/
	int SeekAndLoad(Mp4File * pMp4File);

	/** 12 bits */
	unsigned short	syncword;
	/** 1 bits */
	unsigned char	id;
	/** 2 bits */
	unsigned char	layer;
	/** 1 bits */
	unsigned char	protection_bit;
	/** 4 bits */
	unsigned char 	bitrate_index;
	/** 2 bits */
	unsigned char 	sampling_frequency;
	/** 1 bits */
	unsigned char 	padding_bit;
	/** 1 bits */
	unsigned char	private_bit;
	/** 2 bits */
	unsigned char	mode;
	/** 2 bits */
	unsigned char	mode_extension;
	/** 1 bits */
	unsigned char	copyright;
	/** 1 bits */
	unsigned char	original_home;
	/** 2 bits */
	unsigned char	emphasis;
};

#endif
