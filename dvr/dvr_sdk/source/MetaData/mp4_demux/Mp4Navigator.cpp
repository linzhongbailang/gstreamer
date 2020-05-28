
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include "Mp4Com.h"
#include "Mp4File.h"
#include "Mp4Box.h"
#include "Mp4Navigator.h"
#include "Mp4Riff.h"

bool Mp4Navigator::Visit(Mp4Com *pCom)
{
	switch(pCom->GetType())
	{
	case MP4BOX:
		return Visit((Mp4Box *)pCom);
	case MP4DESCR:
		return Visit((Descr *)pCom);
	case MP4VE:
		return Visit((Mp4Ve *)pCom);
	default:
		//printf("Visit:Type:%d\n",pCom->GetType());
		break;
	}
	return true;
}

bool Mp4Navigator::VisitEnd(Mp4Com *pCom)
{
	//printf("Mp4Navigator::VisitEnd:%p,%d\n",pCom,pCom->GetType());
	switch(pCom->GetType())
	{
	case MP4BOX:
		return VisitEnd((Mp4Box *)pCom);
	case MP4DESCR:
		return VisitEnd((Descr *)pCom);
	case MP4VE:
		return VisitEnd((Mp4Ve *)pCom);
	default:
		//printf("VisitEnd:Type:%d\n",pCom->GetType());
		break;
	}
	return true;
}

bool Mp4XmlDumper::Visit(Mp4Com *pCom)
{
	//printf("MpXmlDumper::Visit:%p,%d\n",pCom,pCom->GetType());
	pCom->DumpHeader(&m_XmlFormatter);
	return true;
}

bool Mp4XmlDumper::VisitEnd(Mp4Com *pCom)
{
	//printf("MpXmlDumper::VisitEnd:%p,%d\n",pCom,pCom->GetType());
	pCom->DumpFooter(&m_XmlFormatter);
	return true;
}

bool Mp4Serializer::Visit(Mp4Com *pCom)
{
	if(!pCom->IsSerializable())
		return true;
	pCom->m_iOffsetOnFile = m_pFileFormatter->GetPos();
	pCom->DumpHeader(m_pFileFormatter);
	return true;
}

bool Mp4Serializer::VisitEnd(Mp4Com *pCom)
{
	if(!pCom->IsSerializable())
		return true;
	long lCurPos = m_pFileFormatter->GetPos();
	pCom->m_iSizeOnFile = lCurPos - pCom->m_iOffsetOnFile;
	pCom->DumpFooter(m_pFileFormatter);
	pCom->SetSize(pCom->m_iSizeOnFile);
	return true;
}

bool Mp4DsiExtractor::Visit(Descr *pDescr)
{
	DecoderSpecificInfo * pDSI = 0;
	if(pDescr->m_Tag==kDecSpecificInfoTag)
	{
		pDSI = (DecoderSpecificInfo *)pDescr;
		MdiaBox * pMdia = (MdiaBox *)pDSI->GetBoxContainer(BOX_MDIA);
		if(pMdia)
		{
			Mp4BoxFinder BoxFinder(BOX_HDLR);
			pMdia->Navigate(&BoxFinder);
			HdlrBox * pHdlr = (HdlrBox *)BoxFinder.GetBox();
			if(pHdlr)
			{
				if(pHdlr->m_handler_type==m_Handler)
				{
					m_iSize = pDSI->m_iSize;
					if(m_info)
						delete [] m_info;
					if(m_iSize > 0)
					{
						m_info = new char [m_iSize];
						if(NULL == m_info)
						{
							return false;
						}
						memcpy(m_info,pDSI->info,m_iSize);
					}
					return false;
				}
			}
		}
	}
	return true;
}

bool Mp4BoxFinder::Visit(Mp4Box *pBox)
{
	if(pBox->GetBoxType()==m_FindBox)
	{
		m_FoundBox = pBox;
		return false;
	}
	return true;
}

Mp4Box * Mp4BoxFinder::GetBox(BoxType aBoxType, Mp4Box * pBox)
{
	m_FoundBox 	= 0;
	m_FindBox 	= aBoxType;
	pBox->Navigate(this);
	return m_FoundBox;
}

// return chunk index [1..max chunk]
int Mp4TrackStream::GetChunkIdx(int iStreamOffset)
{
	int i=0;
	if(iStreamOffset >= m_StreamSize)
		RETURN_ERROR(-1);

	if ( (m_pChunk[m_iCurChunk].m_StreamOffset == iStreamOffset) && (iStreamOffset < m_pChunk[m_iCurChunk + 1].m_StreamOffset) )
	{
		i = m_iCurChunk + 1;
	}
	else if (iStreamOffset < m_pChunk[m_iCurChunk].m_StreamOffset)
	{
		for(i = m_iCurChunk; i >= 0; --i)
		{
			if(iStreamOffset >= m_pChunk[i].m_StreamOffset)
			{
				i = i + 1;
				break;
			}
		}
		if (i < 0)
		{
			i = m_iNumChunk;
		}
	}
	else  // iStreamOffset >= m_pChunk[m_iCurChunk].m_StreamOffset
	{
		for(i = m_iCurChunk; i < m_iNumChunk; ++i)
		{
			if(iStreamOffset < m_pChunk[i].m_StreamOffset)
			{
				break;
			}
		}
	}
	
	return i;
}

int Mp4TrackStream::ReadChunk(int iChunkIdx, char *buf, int iStreamOffset, int nRead)
{
	if(iChunkIdx > m_iNumChunk)
		return 0;
	Mp4Chunk * pChunk = m_pChunk+iChunkIdx-1;
	int iFileOffset = iStreamOffset-pChunk->m_StreamOffset+pChunk->m_BeginOffset;
	if(iFileOffset == pChunk->m_EndOffset)
	{
		m_iCurChunk++;
		if(m_iCurChunk > m_iNumChunk)
			return 0;
		pChunk = m_pChunk+m_iCurChunk-1;
		iFileOffset = pChunk->m_BeginOffset;
		m_CurStreamPos = pChunk->m_StreamOffset;
	}
	
	bool bLastOfChunk = false;
	if(iFileOffset+nRead > pChunk->m_EndOffset)
	{
		nRead = pChunk->m_EndOffset-iFileOffset;
		bLastOfChunk = true;
	}
#ifdef IAC_ENCRYPT	//sunstars IAC 3gp
	unsigned int pos = (iFileOffset-m_mdat_pos) & 0x07;
#endif	//end sunstars
	m_pMp4File->SetPos(iFileOffset);
	int iRead = m_pMp4File->GetBuffer(buf, nRead); 
	if(iRead >= nRead && bLastOfChunk)
		m_iCurChunk++;
#ifdef IAC_ENCRYPT	//sunstars IAC 3gp
	if(m_bEncrypt)
	{
		for(int i=0; i<iRead; i++)
		{
			buf[i] ^= m_password[pos];
			pos++;
			pos &= 0x7;
		}			
	}
#endif	//end sunstars
	return iRead;
}

int Mp4TrackStream::Read(char *buf, int nRead)
{
	if(m_iCurChunk > m_iNumChunk)
	{
		RETURN_ERROR(-1);
	}
	int rRead = 1;
	int iRemain = nRead;
	int iReaded = 0;
	while(iRemain > 0)
	{
		rRead = ReadChunk(m_iCurChunk, buf+iReaded, m_CurStreamPos, iRemain);
		if(rRead <=0)
			break;
		m_CurStreamPos += rRead;
		iReaded += rRead;
		iRemain 			-= rRead;
	}
	
	while(m_iCurSample < m_pStszBox->m_sample_count && m_pStszBox->GetSampleOffset(m_iCurSample+1) <= m_CurStreamPos)
	{
		m_iCurSample++;
	}
	return iReaded;
}

int Mp4TrackStream::SeekToI(SEEKIDIRECTION direction, bool bIsStreaming)
{
    int i = 0;
	if(	m_pStssBox && (bIsStreaming || (m_VideoType != BOX_MP4V && m_VideoType != BOX_DIVX) && 
		m_pStssBox->m_entry_count > 10 &&
		m_pStssBox->m_sample_number[m_pStssBox->m_entry_count-1] > (m_pStszBox->m_sample_count-300)))
	{
		if(direction == SEEKI_FORWARD)
		{
			for(i=0;i<m_pStssBox->m_entry_count;i++)
			{
				if(m_pStssBox->m_sample_number[i] >= m_iCurSample+1)
				{
					SetSamplePosition(m_pStssBox->m_sample_number[i]);
					return m_iCurSample;
				}
			}
		}
		else // direction == SEEKI_BACKWARD
		{
			for(i=m_pStssBox->m_entry_count-1; i>=0; i--)
			{
				if(m_pStssBox->m_sample_number[i] <= m_iCurSample-1)
				{
					SetSamplePosition(m_pStssBox->m_sample_number[i]);
					return m_iCurSample;
				}
			}
		}
		return -1;
	}
	else
	{
		unsigned int uSamplesize = 0;
		unsigned int uSliceSize = 0;//h264 may have multi-slice
		unsigned int uSamplePos = 0;
		unsigned char buf[6];
		bool bVOP = false;
		unsigned long FrameType = 1;
		int CurrentStreamPos = 0;
		
		int iCurSample = m_iCurSample;
		memset(buf,0,sizeof(char) * 6);
		int ilastLoopSample = m_iCurSample;

		if(bIsStreaming)
			return -1;

		while (true)
		{
			m_iCurSample += direction;
			
			// Vincent 2007/10/25
			// Prevent  infinite loop in case direction is SEEKI_BACKWARD
			// because  m_iCurSample may be increased by function Read.
			if (ilastLoopSample <= m_iCurSample && direction == SEEKI_BACKWARD)
			{
				m_iCurSample = ilastLoopSample - 1;
			}

			ilastLoopSample = m_iCurSample;

			if(m_iCurSample > m_pStszBox->m_sample_count || m_iCurSample < 1)
			{
					return -1;
			}
			SetSamplePosition(m_iCurSample);
			//for h264 multislice, add by gavin
			CurrentStreamPos = m_CurStreamPos;
			uSamplesize = GetSampleSize(m_iCurSample);
			uSamplePos = m_pStszBox->GetSampleOffset(m_iCurSample);
			unsigned int uSampleOffset = 0;
			//add by gavin end

			int iRead = Read((char*)buf,6);
			if(iRead < 6)
			{
					return -1;
			}
			if (m_VideoType != BOX_AVC1)
			{
				bVOP= buf[0]==0 && buf[1]==0 && buf[2]==(char)1;
				FrameType = (buf[4]>>6)&0x3;			
				//printf("codec header[1]: %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
				if(bVOP)
				{
					if( (buf[3]==(unsigned char)0xB6 && FrameType == 0) ||	//I_VOP
						buf[3]==(unsigned char)0xB3 ||	//GOP					//follow perhaps needs to check inside
						buf[3]==(unsigned char)0xB0 ||							//visual_object_sequence_start_code
						buf[3]==(unsigned char)0xB5 ||							//visual_object_start_code
						(buf[3]<=(unsigned char)0x1f) ||	//video_object_start_code
						(buf[3]>=(unsigned char)0x20 && buf[3]<=(unsigned char)0x2f) )	//video_object_layer_start_code
					{
						break;
					}
				}
				else if(buf[0] == 0 && buf[1] == 0 && ((buf[2]&0xfc)==0x80) && ((buf[4]&0x2)==0) )  //h.263
				{
					break;
				}
			}
			else
			{
				while(uSampleOffset < uSamplesize)
				{
					int nBufLenSize = GetAvcBufferLenSize();
					uSliceSize = 0;
					for(int i = 0; i < nBufLenSize; i++)// little endian
					{
						if(i != 0)
						{
							uSliceSize <<= 8;
						}
						uSliceSize |= buf[i];
					}
					// Lin: H.264 seeking
					// One char: 0 XX YYYYY where XX is nal_ref_idc_mask, YYYYY is nal_unit_type

					const unsigned char nal_ref_idc_mask = 0x60; 
					const unsigned char nal_unit_type_mask = 0x1f;
					unsigned char cNalFirstByte = buf[nBufLenSize];
					if ((int)(cNalFirstByte&nal_unit_type_mask) == 5) // IDR frame
					{
						m_CurStreamPos = CurrentStreamPos;
						return m_iCurSample;
					}
					uSampleOffset+= uSliceSize + nBufLenSize;
					SetPos(uSamplePos + uSampleOffset);
					iRead = Read((char*)buf, 6);
					if(iRead<6)
					{
						return -1;
					}
				}
			}
		}
		m_CurStreamPos = CurrentStreamPos;
		return m_iCurSample;
	}
}

int Mp4TrackStream::ReadSample(char *buf, int nRead, unsigned int * pCts)
{
	int iSampleSize = m_pStszBox->GetSampleSize(m_iCurSample);
	int iSampleLocation = m_pStszBox->GetSampleOffset(m_iCurSample);
	if(iSampleSize<1 || iSampleLocation<0 )
	{
		return 0; // EOF ?
	}
	
	if(pCts && m_pSttsBox!=0)
	{
		*pCts = (unsigned int)GetSampleCtsOfMisc(m_iCurSample);
	}
	
	int iRead = 0;
	
	if (m_iRemains != 0)
	{
		iRead = m_iRemains>nRead ? nRead : m_iRemains;
		m_iRemains -= iRead;
	}
	else if(nRead < iSampleSize)
	{
		iRead = nRead;
		m_iRemains = iSampleSize - nRead;
	}
	else
	{
		Mp4BoxFinder BoxFinder(BOX_HDLR);
		AlawBox* pAlawBox = (AlawBox *)BoxFinder.GetBox(BOX_ALAW,m_pTrakBox);
		if(pAlawBox)
		{
			iRead = nRead;
		}
		else
		{
			iRead = iSampleSize;
		}
	}
	
	return Read(buf, iRead);
}

long Mp4TrackStream::SetPos(int streamPos)
{
	int iChunkIdx = GetChunkIdx(streamPos);
	if(iChunkIdx < 1 || iChunkIdx > m_iNumChunk)
		RETURN_ERROR(-1);
	Mp4Chunk * pChunk = m_pChunk+iChunkIdx-1;
	int iFileOffset = streamPos-pChunk->m_StreamOffset+pChunk->m_BeginOffset;
	if(m_pMp4File->SetPos(iFileOffset)<0)
		RETURN_ERROR(-1);
	m_CurStreamPos = streamPos;
	m_iCurChunk = iChunkIdx;
	return m_CurStreamPos;
}

long  Mp4TrackStream::SetSamplePosition(long iSample /* [1..maxSample] */)
{
	if(iSample < 1 || (m_pStszBox && iSample > m_pStszBox->m_sample_count))
		RETURN_ERROR(-1);
	int iStreamPosition = m_pStszBox->GetSampleOffset(iSample);
	if(iStreamPosition < 0)
		RETURN_ERROR(-1);
	if(SetPos(iStreamPosition) < 0)
		RETURN_ERROR(-1);
	
	m_iCurSample = iSample;
	return iStreamPosition;
}

Mp4DsiExtractor* Mp4TrackStream::GetDsi()
{
	if (m_pTrakBox == NULL)
		RETURN_ERROR(0);

	Mp4DsiExtractor* pDsiExtractor = NULL;
	Mp4BoxFinder BoxFinderHdlr(BOX_HDLR);
	m_pTrakBox->Navigate(&BoxFinderHdlr);
	HdlrBox* pHdlr = (HdlrBox*)BoxFinderHdlr.GetBox();
	if (pHdlr != NULL)
	{
		pDsiExtractor = new Mp4DsiExtractor((HandlerType)pHdlr->m_handler_type);
		if (pDsiExtractor == NULL)
			RETURN_ERROR(0);
		m_pTrakBox->Navigate(pDsiExtractor);
	}
	if (pDsiExtractor->m_info != NULL && pDsiExtractor->m_iSize > 0)
		return pDsiExtractor;

	Mp4BoxFinder BoxFinderGlbl(BOX_GLBL);
	m_pTrakBox->Navigate(&BoxFinderGlbl);
	GlblBox* pGlbl = (GlblBox*)BoxFinderGlbl.GetBox();
	if (pGlbl &&pGlbl->m_pGlblInfo != NULL && pGlbl->m_lInfoLen > 0)
	{
		pDsiExtractor->m_info = new char [pGlbl->m_lInfoLen];
		if (pDsiExtractor->m_info == NULL)
		{
			delete pDsiExtractor;
			RETURN_ERROR(0);
		}
		memcpy(pDsiExtractor->m_info, pGlbl->m_pGlblInfo, pGlbl->m_lInfoLen);
		pDsiExtractor->m_iSize = pGlbl->m_lInfoLen;
	}

	return pDsiExtractor;
}

long Mp4TrackStream::GetAvcBufferLenSize()
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	AvcBox *avcBox = (AvcBox *)BoxFinder.GetBox(BOX_AVC1,m_pTrakBox);
	if(avcBox == 0)
	{
		return 0;
	}
	return avcBox->lengthSizeMinusOne + 1;
}

long Mp4TrackStream::GetCTS(long lStreamPos)
{
	if(m_pStszBox==NULL)
	{
		Mp4BoxFinder BoxFinder(BOX_STCO);
		m_pStszBox = (StszBox *)BoxFinder.GetBox(BOX_STSZ,m_pTrakBox);
		if(m_pStszBox==0)
			RETURN_ERROR(0);
	}
	
	long lSampleIdx = m_pStszBox->GetSampleIdx(lStreamPos);
	
	if(m_pStssBox==NULL)
	{
		Mp4BoxFinder BoxFinder(BOX_STCO);
		m_pSttsBox = (SttsBox *)BoxFinder.GetBox(BOX_STTS,m_pTrakBox);
		if(m_pSttsBox==0)
			RETURN_ERROR(-1);
	}

	return m_pSttsBox->GetCTS(lSampleIdx);
}

long Mp4TrackStream::GetSampleCts(long lIdx)
{
	long cts = 0;
	if(m_pSttsBox==NULL)
	{
		Mp4BoxFinder BoxFinder(BOX_STCO);
		m_pSttsBox = (SttsBox *)BoxFinder.GetBox(BOX_STTS,m_pTrakBox);
		if(m_pSttsBox==0)
			RETURN_ERROR(-1);
	}

	// Vincent 2007/11/02 
	if(m_pSttsBox->m_entry_count == 0)
		RETURN_ERROR(-1);
	// End Vincent 2007/11/02
	cts = m_pSttsBox->GetCTS(lIdx);

	//check need revise by ctts
	if(m_pCttsBox!=NULL)
	{
		cts += m_pCttsBox->GetCTSoffset(lIdx);
	}
	return cts;
}

__int64 Mp4TrackStream::GetSampleCtsOfMisc(long lIdx)
{
	__int64 cts = GetSampleCts(lIdx);
	if(cts <= 0)
		return cts;
	cts = cts*1000/m_TimeScale;
	return cts;
}

long Mp4TrackStream::GetSampleSize(long lIdx)
{
	return m_pStszBox->GetSampleSize(lIdx);
}

long Mp4TrackStream::GetSamplePos(long lIdx)
{
	long streamPos = m_pStszBox->GetSampleOffset(lIdx);
	if(streamPos < 0)
		RETURN_ERROR(-1);
	int iChunkIdx = GetChunkIdx(streamPos);
	if(iChunkIdx < 1 || iChunkIdx > m_iNumChunk)
		RETURN_ERROR(-1);
	Mp4Chunk * pChunk = m_pChunk+iChunkIdx-1;
	long lFileOffset = streamPos-pChunk->m_StreamOffset+pChunk->m_BeginOffset;
	return lFileOffset;
}

long Mp4TrackStream::GetSampleIdx(long msTime)
{
	int iSampleIdx = 0; // [1..]
	
//	if(msTime < 0 || msTime > m_msStreamDuration)
//		RETURN_ERROR(-1);
	
	if(m_pStssBox==NULL)
	{
		Mp4BoxFinder BoxFinder(BOX_STCO);
		m_pSttsBox = (SttsBox *)BoxFinder.GetBox(BOX_STTS,m_pTrakBox);
		if(m_pSttsBox==0)
			RETURN_ERROR(-1);
	}
	
	int iMediaTime = (int)((__int64)msTime * m_TimeScale/1000.);
	iSampleIdx = m_pSttsBox->GetSampleIdx(iMediaTime);
	if(iSampleIdx<1)
		RETURN_ERROR(-1);
	
	return iSampleIdx;
}


//
// msTime : time in ms.
// return : stream position.
//
long Mp4TrackStream::GetPosition(long msTime)
{
	int iMediaTime = (int)((__int64)msTime * m_TimeScale/1000.);
	
	if(msTime < 0 || msTime > m_msStreamDuration)
		RETURN_ERROR(-1);
	
	if(m_pStssBox==NULL)
	{
		Mp4BoxFinder BoxFinder(BOX_STCO);
		m_pSttsBox = (SttsBox *)BoxFinder.GetBox(BOX_STTS,m_pTrakBox);
		if(m_pSttsBox==0)
			RETURN_ERROR(-1);
	}
	
	int iSyncSampleIdx = m_pSttsBox->GetSampleIdx(iMediaTime);
	if(iSyncSampleIdx < 1)
		RETURN_ERROR(-1);
	
	int iChunk = 0;
	for(iChunk=0;iChunk<m_iNumChunk;iChunk++)
	{
		if(m_pChunk[iChunk].m_BeginSampleIdx > iSyncSampleIdx)
			break;
	}
	iChunk--;
	if(iChunk < 0 || iChunk >= m_iNumChunk)
		RETURN_ERROR(-1);
	
	Mp4Chunk* pChunk = m_pChunk + iChunk;
	
	int iOffset = pChunk->m_StreamOffset;
	int Idx=0;
	for(Idx = pChunk->m_BeginSampleIdx; Idx < iSyncSampleIdx; Idx++)
		iOffset += m_pStszBox->GetSampleSize(Idx);
	
	return iOffset;
}

//
// return first matched track.
// TODO: handle multiple tracks.
//
bool Mp4TrackFinder::Visit(Mp4Box *pBox)
{
	if(m_pFoundTrak)
		return false;
	
	if(pBox->GetBoxType()==BOX_HDLR)
	{
		HdlrBox * pHdlr = (HdlrBox *)pBox;
		if(pHdlr->m_handler_type==m_Handler)
		{
			m_pFoundTrak = (TrakBox *)pHdlr->GetContainer(BOX_TRAK);
			if(m_pFoundTrak && m_Handler==HANDLER_VIDEO)
			{
				Mp4BoxFinder BoxFinder(BOX_MP4V);
				Mp4vBox * pMp4vBox = (Mp4vBox *)BoxFinder.GetBox(BOX_MP4V,m_pFoundTrak);
				if(pMp4vBox)
				{
					int iObjectType = pMp4vBox->GetObjectTypeIndication();
					switch(iObjectType)
					{
					case 0x20:	//MPEG4
					case 0x60:	//MPEG2
					case 0x61:	//MPEG2
					case 0xc2:	//PV
					case 0x6a:  //MPEG1
					case 0x6c:  //JPEG 
						return false;
					}
				}
				else
				{
					S263Box * pS263Box = (S263Box *)BoxFinder.GetBox(BOX_S263,m_pFoundTrak);
					if(pS263Box)
						return false;
					AvcBox* pAvcBox = (AvcBox*)BoxFinder.GetBox(BOX_AVC1,m_pFoundTrak);
					if(pAvcBox)
						return false;
					DivxBox* pDivxBox = (DivxBox*)BoxFinder.GetBox(BOX_DIVX,m_pFoundTrak);
					if(pDivxBox)
						return false;
					XvidBox* pXvidBox = (XvidBox*)BoxFinder.GetBox(BOX_XVID,m_pFoundTrak);
					if(pXvidBox)
						return false;
					DivxBox* pDx50Box = (DivxBox*)BoxFinder.GetBox(BOX_DX50,m_pFoundTrak);
					if(pDx50Box)
						return false;
				}
				m_pFoundTrak = NULL; // keep search.
			}
			else
				return false;
		}
	}
	return true;
}

int Mp4TrackStream::BuildFileMap()
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	m_pStcoBox = (StcoBox *)BoxFinder.GetBox(BOX_STCO,m_pTrakBox);
	m_pStscBox = (StscBox *)BoxFinder.GetBox(BOX_STSC,m_pTrakBox);
	m_pStszBox = (StszBox *)BoxFinder.GetBox(BOX_STSZ,m_pTrakBox);
	m_pSttsBox = (SttsBox *)BoxFinder.GetBox(BOX_STTS,m_pTrakBox);
	m_pStssBox = (StssBox *)BoxFinder.GetBox(BOX_STSS,m_pTrakBox);
	m_pCttsBox = (CttsBox *)BoxFinder.GetBox(BOX_CTTS,m_pTrakBox);
	
	if(m_pStszBox==0 || m_pStcoBox==0 || m_pStscBox==0 || m_pSttsBox==0)
		RETURN_ERROR(-1);
	if(m_pStcoBox->m_entry_count < 1)
		RETURN_ERROR(-1);
	
	MdhdBox *pMdhdBox = (MdhdBox *)BoxFinder.GetBox(BOX_MDHD,m_pTrakBox);
	if(pMdhdBox==0)
		RETURN_ERROR(-1);
	if (BoxFinder.GetBox(BOX_MP4V,m_pTrakBox))
		m_VideoType = BOX_MP4V;
	if (BoxFinder.GetBox(BOX_DIVX,m_pTrakBox))
		m_VideoType = BOX_DIVX;
	else if (BoxFinder.GetBox(BOX_S263,m_pTrakBox))
		m_VideoType = BOX_S263;
	else if (BoxFinder.GetBox(BOX_AVC1,m_pTrakBox))
		m_VideoType = BOX_AVC1;
	else if (BoxFinder.GetBox(BOX_XVID,m_pTrakBox))
		m_VideoType = BOX_XVID;

	AlawBox* pAlawBox = (AlawBox *)BoxFinder.GetBox(BOX_ALAW,m_pTrakBox);
	if(pAlawBox)
	{
		int max_samplePerChunk = 0;
		for(int i = 0; i < m_pStscBox->m_entry_count; i++)
		{
			if (max_samplePerChunk < m_pStscBox->m_entry[i].samples_per_chunk)
			{
				max_samplePerChunk = m_pStscBox->m_entry[i].samples_per_chunk;
			}
			
		}
		m_pStszBox->SetSampleSize(pAlawBox->m_channelcount*(8>>3), max_samplePerChunk*pAlawBox->m_channelcount*(8>>3));// 8 means bitpersample for alaw
	}

	UlawBox* pUlawBox = (UlawBox *)BoxFinder.GetBox(BOX_ULAW,m_pTrakBox);
	if(pUlawBox)
	{
		int max_samplePerChunk = 0;
		for(int i = 0; i < m_pStscBox->m_entry_count; i++)
		{
			if (max_samplePerChunk < m_pStscBox->m_entry[i].samples_per_chunk)
			{
				max_samplePerChunk = m_pStscBox->m_entry[i].samples_per_chunk;
			}

		}
		m_pStszBox->SetSampleSize(pUlawBox->m_channelcount*(8>>3), max_samplePerChunk*pUlawBox->m_channelcount*(8>>3));// 8 means bitpersample for alaw
	}
	
	m_TimeScale 		= pMdhdBox->m_timescale;	
	m_msStreamDuration 	= (int)((__int64)pMdhdBox->m_duration*1000./m_TimeScale);	
	
	m_iNumChunk = m_pStcoBox->m_entry_count;
	m_iCurChunk = 1;
	if(m_pChunk)
		delete [] m_pChunk;
	m_pChunk		= new Mp4Chunk[m_iNumChunk];
	if(NULL == m_pChunk)
	{
		RETURN_ERROR(-2);
	}
	Mp4Chunk * 	pChunk=0;
	int 			iStscIdx=0;
	int 			iChunkIdx=1;
	int 			iSampleIdx = 1;
	int 			Idx=1;
	int			iChunkSize=0;
	
	m_StreamSize = 0;
	m_lMaxSampleSize = 0;
	for(iChunkIdx=1,iStscIdx=0;iChunkIdx<m_iNumChunk+1;iChunkIdx++)
	{
		if(iStscIdx<m_pStscBox->m_entry_count)
		{
			if(m_pStscBox->m_entry[iStscIdx].first_chunk<1)
			{
				m_pStscBox->m_entry[iStscIdx].first_chunk = 1;
			}
			if(iChunkIdx>=m_pStscBox->m_entry[iStscIdx].first_chunk)
				iStscIdx++;
		}

		pChunk 					= &m_pChunk[iChunkIdx-1];
		pChunk->m_StreamOffset= m_StreamSize;
		pChunk->m_BeginOffset= m_pStcoBox->m_chunk_offset[iChunkIdx-1];
#ifdef IAC_ENCRYPT		//sunstars IAC 3gp
		if(m_bEncrypt)
			pChunk->m_BeginOffset += 8;
#endif		//End sunstars 
		pChunk->m_BeginSampleIdx = iSampleIdx;
		iSampleIdx			+=		m_pStscBox->m_entry[iStscIdx-1].samples_per_chunk;
		pChunk->m_EndSampleIdx = iSampleIdx-1;
		pChunk->m_EndOffset = pChunk->m_BeginOffset;
		
		int size;
		for(Idx=pChunk->m_BeginSampleIdx,iChunkSize=0;Idx<pChunk->m_EndSampleIdx+1;Idx++)
		{
			size = m_pStszBox->GetSampleSize(Idx);
			iChunkSize += size;
			if(m_lMaxSampleSize < size)
			{
				m_lMaxSampleSize = size;
			}
		}
		pChunk->m_EndOffset = pChunk->m_BeginOffset + iChunkSize;
		m_StreamSize += iChunkSize;
	}
	
	return 0;
}
#if 0
#define MAX_STREAM_READ 8192
int Mp4TrackStream::Write(FileCtrl * pFileCtrl, bool bWithHeader, int nWrite)
{
	char buf[MAX_STREAM_READ];
	unsigned long  TotWrite = nWrite > 0 ? nWrite : m_StreamSize; 
	if(bWithHeader && m_pTrakBox)
	{
		Mp4BoxFinder BoxFinder(BOX_HDLR);
		m_pTrakBox->Navigate(&BoxFinder);
		HdlrBox * pHdlr = (HdlrBox *)BoxFinder.GetBox();
		if(pHdlr==0)
			RETURN_ERROR(-1);
		
		if((HandlerType)pHdlr->m_handler_type==HANDLER_AUDIO)
		{
			// temporary hack.
			pFileCtrl->Write("#!AMR\n", 6);
		}
		else
		{
			Mp4DsiExtractor* pDsiExtractor = GetDsi();
			if(pDsiExtractor->m_iSize > 0)
			{
				if((int)pFileCtrl->Write(pDsiExtractor->m_info, pDsiExtractor->m_iSize)!=
					pDsiExtractor->m_iSize)
				{
					if(pDsiExtractor)
						delete pDsiExtractor;
					RETURN_ERROR(-1);
				}
			}
			if(pDsiExtractor)
				delete pDsiExtractor;
		}
	}
	
	size_t nRead = 0, nTot=0, iCount=0, nRemainWrite=TotWrite;
	
	do {
		if(nRemainWrite > MAX_STREAM_READ)
			nRead = MAX_STREAM_READ;
		else
			nRead = nRemainWrite;
		if(nRead < 1)
			break;
		nRead = Read(buf, nRead);
		if(nRead < 1)
			break;
		if(pFileCtrl->Write(buf, nRead)!=(int)nRead)
		{
			//			DBG(("write failure: nRead: %d\n",nRead));
			break;
		}
		iCount++;
		nTot 				+= nRead;
		nRemainWrite 	-= nRead;
	} while(nRead > 0);
	
	return nTot;
}

int Mp4TrackStream::Write(char *pathname, bool bWithHeader, int nWrite)
{
	FileCtrl * pFileCtrl = CreateFileCtrl();
	
	if(pFileCtrl->Open(pathname, 0, FILECTRL_OPEN_WRITE)!=0)
	{
	RETURN_ERROR(0);
	}
	
	int iRet = Write(pFileCtrl, bWithHeader, nWrite);
	
	pFileCtrl->Close();
	delete pFileCtrl;
	
	return iRet;
}

int Mp4TrackStream::WriteChunk(FileCtrl *pFileCtrl, long lNewChunkOffset)
{
	int iChunkIdx, iStreamOffset;
	
	iStreamOffset = m_CurStreamPos;
	
	iChunkIdx = GetChunkIdx(iStreamOffset);
	if(iChunkIdx < 0)
	{
		//DBG(("return error\n"));
		RETURN_ERROR(-1);
	}
	
	Mp4Chunk * pChunk = m_pChunk+iChunkIdx-1;
	int iChunkSize = pChunk->m_EndOffset - pChunk->m_BeginOffset;
	int iTotalWritten=0;
	
	iTotalWritten = Write(pFileCtrl, false /* no extra header required */, iChunkSize);
	
	//printf("%d(%d)=?%d\n",iChunkIdx,iChunkSize,iTotalWritten);
	
	Mp4BoxFinder BoxFinder(BOX_HDLR);
	StcoBox* pStcoBox = (StcoBox *)BoxFinder.GetBox(BOX_STCO,m_pTrakBox);
	if(pStcoBox && lNewChunkOffset>-1)
	{
		pStcoBox->m_chunk_offset[iChunkIdx-1] = lNewChunkOffset;
	}
	
	return iTotalWritten;
}


void Mp4TrackStream::Dump(Formatter* pFormatter)
{
	int i;
	pFormatter->Print("m_StreamSize: %d\n",m_StreamSize);
	for(i=0;i<m_iNumChunk;i++)
	{
		pFormatter->Print("Chunk[%d]:Offset(%d,%d)=%d,Sample(%d,%d),StrmOffset(%d)\n",
			i+1,
			m_pChunk[i].m_BeginOffset,
			m_pChunk[i].m_EndOffset,
			m_pChunk[i].m_EndOffset-m_pChunk[i].m_BeginOffset,
			m_pChunk[i].m_BeginSampleIdx,
			m_pChunk[i].m_EndSampleIdx,
			m_pChunk[i].m_StreamOffset);
	}
}


int Mp4TrackStream::DumpStsz(char *output)
{
	char buf[256];
	FileCtrl * pFileCtrl = CreateFileCtrl();
	int i;
	
	//DBG(("DumpStsz:%s\n", output));
	if(m_pStszBox==0)
		RETURN_ERROR(-1);
	if(pFileCtrl->Open(output, 0, FILECTRL_OPEN_WRITE)!=0)
		RETURN_ERROR(-1);
	for(i=0;i<m_pStszBox->m_sample_count;i++)
	{
		sprintf(buf, "%d %ld\n", i,
			m_pStszBox->m_sample_size>0?
			m_pStszBox->m_sample_size:
								m_pStszBox->m_entry_size[i]);
								pFileCtrl->Write(buf, strlen(buf));
	}
	pFileCtrl->Close();
	delete pFileCtrl;
	return 0;
}



#endif