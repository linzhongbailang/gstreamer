#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include "Mp4Com.h"
#include "Mp4File.h"
#include "Mp4Box.h"
#include "Mp4Navigator.h"
#include "Mp4Riff.h"
#include "Mp4Ve.h"

#define  VIDEO_CHUNK_TIME	0.25
#define  AMR_CHUNK_TIME    0.1
#define  AAC_CHUNK_TIME    0.25
#define  QCELP_CHUNK_TIME  0.2
#define  MP3_CHUNK_TIME   	0.1

enum Mode { MR475 = 0,
MR515,
MR59,
MR67,
MR74,
MR795,
MR102,
MR122,
MRDTX,
N_MODES     /* number of (SPC) modes */
};

enum Quality
{
	Q_LOWEST = 0,
	Q_LOW,
	Q_MEDIUM,
	Q_GOOD,
	Q_HIGH,
	Q_BEST,
};

Mp4File::Mp4File(char * pathname) : MovieFile(pathname)
{
}

Mp4File::Mp4File(FileCtrl * pFileCtrl) : MovieFile(pFileCtrl)
{
}


Mpeg3gpFile::~Mpeg3gpFile()
{
	m_pMoovBox = 0;
	m_pFtypBox = 0;
	m_pMoofBox = 0;
}

BoxType	Mp4File::GetBoxHead(int * size)
{
	*size = GetSize();
	return GetType();
}

BoxType	Mp4File::GetType()
{
	char cType[4];
	if(GetBuffer(cType,4)!=4)
		RETURN_ERROR(BOX_IVLD);
	return (BoxType)Mp4Box::GetIntL(cType);
}

long  Mpeg3gpFile::GetDuration(long * pPrecision)
{
	if(m_pMoovBox==0 || pPrecision==0)
		RETURN_ERROR(0);
	*pPrecision = m_pMoovBox->m_pMvhdBox->m_timescale;
	return m_pMoovBox->m_pMvhdBox->m_duration;
}

int	Mp4File::GetSize()
{
	char cSize[4];
	if(GetBuffer(cSize,4)!=4)
		RETURN_ERROR(0);
	int size = Mp4Box::GetIntL(cSize);
	return size;
}

unsigned long long	Mp4File::GetExtSize()
{
	unsigned char cSize[8];
	if(GetBuffer((char*)cSize,8)!=8)
		RETURN_ERROR(0);
	unsigned long long size = 0;
	for(int i = 0; i < 8; i++)
	{
		size = size << 8;
		size |= cSize[i];
	}
	return size;
}

int Mpeg3gpFile::LoadMoov(bool bIsStreaming)
{
    m_mdat_pos = 0; 
    int iOffset = 0;
    int iSize=0;
    int iSumSize=0;
    int iBoxType=0;
    MoovBox * pBox = NULL;
    int ftyptype = 0;
    int freetype = 1;
    int moovtype = 2;
    int typemask = 0;

    if(m_IoStatus<0)
        RETURN_ERROR(-1); 

    if(m_pMoovBox || m_pFtypBox)
        RETURN_ERROR(ISO_SYNTAX_ERROR);

    while(iOffset+8<m_iFileSize)
    {
        SetPos(iOffset);
        iSize    = 0;
        iBoxType  = GetBoxHead(&iSize);
        if(iBoxType == 0 || iSize <= 0)
        {
            if (iSize == 0 && iBoxType == BOX_MDAT)
                return 0;
            if(typemask & ((1<<ftyptype)|(1<<freetype)|(1<<moovtype)))
                return 0;

            RETURN_ERROR(ISO_SYNTAX_ERROR);
        }

        if(iSize == 1)
        {
            unsigned long long  llsize = GetExtSize();
            if(llsize> 0xFFFFFFFF)
            {
                //todo with larger size??need update more variant 's type from int to long long
                RETURN_ERROR(E_NOTIMPL);
            }
            iSize = (int)llsize;
        }
        
        iSumSize += iSize;
        switch(iBoxType)
        {
            case BOX_FTYP:
            typemask |= 1<<ftyptype;
            break;
            case BOX_FREE:
            typemask |= 1<<freetype;
            break;
            case BOX_MOOV:
            {
                if(m_pMoovBox)
                    RETURN_ERROR(ISO_SYNTAX_ERROR);
                pBox = m_pMoovBox = new MoovBox(iOffset, iSize);
                typemask |= 1<<moovtype;
            }
            break;
            default:
                break;
        }
        if(pBox)
        {
            Adopt(pBox);
            int ret = pBox->LoadMvhd(this);
            if(ret!=0)
                RETURN_ERROR(-1);
        }

        iOffset+=iSize;

        if(bIsStreaming)
        {
            if(m_pMoovBox && m_pFtypBox)
                break;
        }

        if(m_pMoovBox && iBoxType == BOX_MOOV)
            break;
    }

    if(iSumSize != iOffset)
    {
        RETURN_ERROR(ISO_SYNTAX_ERROR);
    }

    return 0;
}

int Mpeg3gpFile::Load(bool bIsStreaming)
{
	m_mdat_pos = 0;	
	int iOffset = 0;
	int iSize=0;
	int iSumSize=0;
	int iBoxType=0;
	MoovBox * pBox = NULL;
    int ftyptype = 0;
    int freetype = 1;
    int moovtype = 2;
    int typemask = 0;

	if(m_IoStatus<0)
		RETURN_ERROR(-1); 

	if(m_pMoovBox || m_pFtypBox)
		RETURN_ERROR(ISO_SYNTAX_ERROR);

	while(iOffset+8<m_iFileSize)
	{
		SetPos(iOffset);
		iSize 	 = 0;
		iBoxType  = GetBoxHead(&iSize);
		if(iBoxType == 0 || iSize <= 0)
        {
            if (iSize == 0 && iBoxType == BOX_MDAT)
                return 0;
            if(typemask & ((1<<ftyptype)|(1<<freetype)|(1<<moovtype)))
                return 0;

            RETURN_ERROR(ISO_SYNTAX_ERROR);
        }

		if(iSize == 1)
		{
			unsigned long long  llsize = GetExtSize();
			if(llsize> 0xFFFFFFFF)
			{
				//todo with larger size??need update more variant 's type from int to long long
				RETURN_ERROR(E_NOTIMPL);
			}
			iSize = (int)llsize;
		}
		
		iSumSize += iSize;
		switch(iBoxType)
        {
            case BOX_FTYP:
            typemask |= 1<<ftyptype;
            break;
            case BOX_FREE:
            typemask |= 1<<freetype;
            break;
            case BOX_MOOV:
            {
                if(m_pMoovBox)
                    RETURN_ERROR(ISO_SYNTAX_ERROR);
                pBox = m_pMoovBox = new MoovBox(iOffset, iSize);
                typemask |= 1<<moovtype;
            }
            break;
            default:
                break;
        }
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->LoadMoovUdta(this);
			if(ret!=0)
				RETURN_ERROR(-1);
		}

		iOffset+=iSize;

		if(bIsStreaming)
		{
			if(m_pMoovBox && m_pFtypBox)
				break;
		}

        if(m_pMoovBox && iBoxType == BOX_MOOV)
            break;
	}

	if(iSumSize != iOffset)
	{
		RETURN_ERROR(ISO_SYNTAX_ERROR);
	}

	return 0;
}

int Mpeg3gpFile::LoadAll(bool bIsStreaming)
{
	m_mdat_pos = 0;	
	int iOffset = 0;
	int iSize=0;
	int iSumSize=0;
	int iBoxType=0;
	Mp4Box * pBox = NULL;
    int ftyptype = 0;
    int freetype = 1;
    int moovtype = 2;
    int typemask = 0;

	if(m_IoStatus<0)
		RETURN_ERROR(-1); 

	if(m_pMoovBox || m_pFtypBox)
		RETURN_ERROR(ISO_SYNTAX_ERROR);

	while(iOffset+8<m_iFileSize)
	{
		SetPos(iOffset);
		iSize 	 = 0;
		iBoxType  = GetBoxHead(&iSize);
		if(iBoxType == 0 || iSize <= 0)
        {
            if (iSize == 0 && iBoxType == BOX_MDAT)
                return 0;
            if(typemask & ((1<<ftyptype)|(1<<freetype)|(1<<moovtype)))
                return 0;
            RETURN_ERROR(ISO_SYNTAX_ERROR);
        }

		
		if(iSize == 1)
		{
			unsigned long long  llsize = GetExtSize();
			if(llsize> 0xFFFFFFFF)
			{
				//todo with larger size??need update more variant 's type from int to long long
				RETURN_ERROR(E_NOTIMPL);
			}
			iSize = (int)llsize;
		}
		
		iSumSize += iSize;
		switch(iBoxType)
		{
#ifdef IAC_ENCRYPT			//sunstars IAC 3gp
		case BOX_MDAT:
			m_mdat_pos = iOffset + 8 ;
			pBox = new Mp4Box((BoxType)iBoxType, iOffset, iSize, 0);
			break;		
#endif			//sunstars IAC 3gp end
		case BOX_MOOV:
			{
				// to support KDDI amc properly we insert 3 major uuid boxes before
				// moovbox
				UuidBox * pUuid;
				if(m_pUuidCpgd==0)
				{
					pUuid = new UuidBox(0,10);
					if (pUuid != NULL)
					{
						pUuid->SetUuidType(UUID_CPGD);
						pUuid->m_copy_guard_attribute = 0;
						pUuid->m_limit_date = 0;
						pUuid->m_limit_period = 0;
						pUuid->m_limit_count = 0;
						pUuid->SetLoaded(false);
						m_pUuidCpgd = pUuid;
						Adopt(pUuid);
					}
				}
				if(m_pMoovBox)
					RETURN_ERROR(ISO_SYNTAX_ERROR);
				pBox = m_pMoovBox = new MoovBox(iOffset, iSize);
                typemask |= 1<<moovtype;
			}
			break;
		case BOX_FTYP:
			{
				if(m_pFtypBox)
					RETURN_ERROR(ISO_SYNTAX_ERROR);
				pBox = m_pFtypBox = new FtypBox(iOffset, iSize);
                typemask |= 1<<ftyptype;
			}
			break;
		case BOX_MOOF:
			{
				if(m_pMoofBox)
					RETURN_ERROR(ISO_SYNTAX_ERROR);
				pBox = m_pMoofBox = new MoofBox(iOffset, iSize);
			}
			break;
		case BOX_FREE:
			{
				pBox = new FreeBox(iOffset, iSize);
                typemask |= 1<<freetype;
			}
			break;
		case BOX_SKIP:
			{
				pBox = new SkipBox(iOffset, iSize);
			}
			break;
		case BOX_UDTA:
			{
				pBox = new UdtaBox(iOffset, iSize);
			}	
			break;
		case BOX_UUID:
			{
				UuidBox * pUuidBox = new UuidBox(iOffset, iSize);
				pBox = pUuidBox;
				Adopt(pBox);
				if(pBox->Load(this)!=0)
					RETURN_ERROR(-1);
				switch(pUuidBox->GetUuidType())
				{
				case UUID_MVML:
					m_pUuidMvml = pUuidBox;
					break;
				case UUID_ENCI:
					m_pUuidEnci = pUuidBox;
					break;
				case UUID_CPGD:
					m_pUuidCpgd = pUuidBox;
					break;
				}
				pBox = 0;
			}	
			break;
		case BOX_IVLD:
			{
				pBox = 0;
				RETURN_ERROR(0);
			}
		default:
			pBox = new Mp4Box((BoxType)iBoxType, iOffset, iSize, 0);
			break;
		}
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(this);
			if(ret!=0)
				RETURN_ERROR(-1);
		}

		iOffset+=iSize;

		if(bIsStreaming)
		{
			if(m_pMoovBox && m_pFtypBox)
				break;
		}
	}

	if(iSumSize != iOffset)
	{
		RETURN_ERROR(ISO_SYNTAX_ERROR);
	}

	return 0;
}

Mp4TrackStream * Mpeg3gpFile::GetTrackStream(HandlerType handler, bool bEncrypt )
{
	Mp4TrackFinder TrackFinder(handler);

	if(m_pMoovBox==0)
		RETURN_ERROR(0);
	m_pMoovBox->Navigate(&TrackFinder);

	TrakBox * pTrak = TrackFinder.GetTrack();
	if(pTrak==NULL)
		RETURN_ERROR(0);

	Mp4File * pFile = pTrak->GetSourceFile();
	Mp4TrackStream * pMp4TrackStream = 	new Mp4TrackStream(pTrak, pFile==NULL?this:pFile, bEncrypt);
	if (pMp4TrackStream == NULL)
	{
		RETURN_ERROR(0);
	}
	if (pMp4TrackStream->GetStreamSize() == 0)
	{
		delete pMp4TrackStream;
		RETURN_ERROR(0);
	}
	return pMp4TrackStream;
}

int Mpeg3gpFile::GetStreamInfo(StreamInfo* pStreamInfo, HandlerType Handler)
{
	Mp4BoxFinder BoxFinder(BOX_STCO);

	if(m_pMoovBox==0)
		RETURN_ERROR(-1);

	Mp4TrackFinder TrackFinder(Handler);
	m_pMoovBox->Navigate(&TrackFinder);

	TrakBox * pTrakBox = TrackFinder.GetTrack();
	if(pTrakBox==0)
		RETURN_ERROR(-1);

	StszBox* pStszBox = (StszBox *)BoxFinder.GetBox(BOX_STSZ,pTrakBox);
	if(pStszBox==0)
		RETURN_ERROR(-1);

	MdhdBox* pMdhdBox = (MdhdBox *)BoxFinder.GetBox(BOX_MDHD,pTrakBox);
	if(pMdhdBox==0)
		RETURN_ERROR(-1);

	memset(pStreamInfo, 0x0, sizeof(StreamInfo));
	pStreamInfo->m_fDuration 	= (float)pMdhdBox->m_duration/pMdhdBox->m_timescale;
	pStreamInfo->m_fFrameRate 	= (float)pStszBox->m_sample_count/pStreamInfo->m_fDuration;
	pStreamInfo->m_iNumSamples = pStszBox->m_sample_count;
	pStreamInfo->m_iStreamSize = pStszBox->GetTotalSampleSize();
	pStreamInfo->m_fBitRate		= (float)(pStreamInfo->m_iStreamSize*8./pStreamInfo->m_fDuration);
	pStreamInfo->m_iVideoWidth	=  0;
	pStreamInfo->m_iVideoHeight=  0;
	pStreamInfo->m_iTimeScale = pMdhdBox->m_timescale;
	pStreamInfo->m_iDuration = pMdhdBox->m_duration;
	// for audio, it is set to sampling rate.
	pStreamInfo->m_iFrequency =  pMdhdBox->m_timescale; 
	pStreamInfo->m_iMaxBitRate = 0;
	pStreamInfo->m_iMaxSampleSize = pStszBox->GetMaxSampleSize();

	Mp4Box* pVideoBox = 0;
	if ((pVideoBox = BoxFinder.GetBox(BOX_MP4V, pTrakBox)) != 0)
	{
		Mp4vBox* pMp4vBox = (Mp4vBox*)pVideoBox;
		pStreamInfo->m_iVideoWidth = pMp4vBox->m_width;
		pStreamInfo->m_iVideoHeight = pMp4vBox->m_height;
	}
	else if ((pVideoBox = BoxFinder.GetBox(BOX_DIVX, pTrakBox)) != 0)
	{
		DivxBox* pDivxBox = (DivxBox*)pVideoBox;
		pStreamInfo->m_iVideoWidth = pDivxBox->m_width;
		pStreamInfo->m_iVideoHeight = pDivxBox->m_height;
	}
	else if ((pVideoBox = BoxFinder.GetBox(BOX_S263, pTrakBox)) != 0)
	{
		S263Box* p263Box = (S263Box*)pVideoBox;
		pStreamInfo->m_iVideoWidth = p263Box->m_width;
		pStreamInfo->m_iVideoHeight = p263Box->m_height;
	}
	else if ((pVideoBox = BoxFinder.GetBox(BOX_AVC1, pTrakBox)) != 0)
	{
		AvcBox* pAvcBox = (AvcBox*)pVideoBox;
		pStreamInfo->m_iVideoWidth = pAvcBox->m_width;
		pStreamInfo->m_iVideoHeight = pAvcBox->m_height;
	}

	Mp4Box* pAudioBox = 0;
	if ((pAudioBox = BoxFinder.GetBox(BOX_MP4A, pTrakBox)) != 0)
	{
		Mp4aBox* pMp4aBox = (Mp4aBox*)pAudioBox;
		pStreamInfo->m_iNumChannels = pMp4aBox->m_channelcount;
		pStreamInfo->m_iBitSize = pMp4aBox->m_samplesize;
	}
	else if ((pAudioBox = BoxFinder.GetBox(BOX_SAMR, pTrakBox)) != 0)
	{
		SamrBox* pSamrBox = (SamrBox*)pAudioBox;
		pStreamInfo->m_iNumChannels = pSamrBox->m_channelcount;
		pStreamInfo->m_iBitSize = pSamrBox->m_samplesize;
		if (pSamrBox->m_pDamrBox)
		{
			switch (pSamrBox->m_pDamrBox->m_mode_set)
			{
			case 1:
				pStreamInfo->m_iMaxBitRate = 4750;
				break;
			case 2:
				pStreamInfo->m_iMaxBitRate = 5150;
				break;
			case 4:
				pStreamInfo->m_iMaxBitRate = 5900;
				break;
			case 8:
				pStreamInfo->m_iMaxBitRate = 6700;
				break;
			case 16:
				pStreamInfo->m_iMaxBitRate = 7400;
				break;
			case 32:
				pStreamInfo->m_iMaxBitRate = 7950;
				break;
			case 64:
				pStreamInfo->m_iMaxBitRate = 10200;
				break;
			case 128:
				pStreamInfo->m_iMaxBitRate = 12200;
				break;
			default:
				break;
			}
		}
	}
	else if ((pAudioBox = BoxFinder.GetBox(BOX_IMA4, pTrakBox)) != 0)
	{
		Ima4Box* pImaBox = (Ima4Box*)pAudioBox;
		pStreamInfo->m_iNumChannels = pImaBox->m_channelcount;
		pStreamInfo->m_iBitSize = pImaBox->m_samplesize;
	}
	else if ((pAudioBox = BoxFinder.GetBox(BOX_ALAW, pTrakBox)) != 0)
	{
		AlawBox* pAlawBox = (AlawBox*)pAudioBox;
		pStreamInfo->m_iNumChannels	= pAlawBox->m_channelcount;
		pStreamInfo->m_iBitSize = pAlawBox->m_samplesize;
	}
	else if ((pAudioBox = BoxFinder.GetBox(BOX_ULAW, pTrakBox)) != 0)
	{
		UlawBox* pUlawBox = (UlawBox*)pAudioBox;
		pStreamInfo->m_iNumChannels	= pUlawBox->m_channelcount;
		pStreamInfo->m_iBitSize = pUlawBox->m_samplesize;
	}
	else if ((pAudioBox = BoxFinder.GetBox(BOX_MP3_, pTrakBox)) != 0)
	{
		Mp3Box* pMp3Box = (Mp3Box*)pAudioBox;
		pStreamInfo->m_iNumChannels = pMp3Box->m_channelcount;
		pStreamInfo->m_iBitSize = pMp3Box->m_samplesize;
	}

	return 0;
}

void Mpeg3gpFile::DumpHeader(Formatter* pFormatter)
{
	pFormatter->Print("<mp4iso>\n");
}
void Mpeg3gpFile::DumpFooter(Formatter* pFormatter)
{
	pFormatter->Print("</mp4iso>\n");
}

int Mpeg3gpFile::GetSampleIdx(HandlerType handler, int msTime)
{
	Mp4TrackFinder TrackFinder(handler);

	if(m_pMoovBox==0)
		RETURN_ERROR(-1);

	m_pMoovBox->Navigate(&TrackFinder);

	TrakBox * pTrak = TrackFinder.GetTrack();

	if(pTrak==0)
		RETURN_ERROR(-1);

	Mp4TrackStream TrackStream(pTrak, this, 0);
	return TrackStream.GetSampleIdx(msTime);
}

TrakBox * Mpeg3gpFile::GetTrack(HandlerType Handler)
{
	if(m_pMoovBox==0)
		RETURN_ERROR(0);

	Mp4TrackFinder TrackFinder(Handler);
	m_pMoovBox->Navigate(&TrackFinder);

	TrakBox * pTrakBox = TrackFinder.GetTrack();
	if(pTrakBox==0)
		RETURN_ERROR(0);

	return pTrakBox;
}

// return true if video is DIVX or XVID
bool Mpeg3gpFile::IsMp4Video()
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	TrakBox * pTrak = GetTrack(HANDLER_VIDEO);
	if(pTrak==NULL)
		return false;	
	if(BoxFinder.GetBox(BOX_DIVX,pTrak))
	{
		return true;
	}
	if(BoxFinder.GetBox(BOX_XVID,pTrak))
	{
		return true;
	}	

	return 	false;
}

// return true if video is h.263.
bool Mpeg3gpFile::HasH263()
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	TrakBox * pTrak = GetTrack(HANDLER_VIDEO);
	if(pTrak==NULL)
		return false;

	return BoxFinder.GetBox(BOX_S263,pTrak)!=NULL;
}

bool Mpeg3gpFile::HasH264()
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	TrakBox * pTrak = GetTrack(HANDLER_VIDEO);
	if(pTrak==NULL)
		return false;

	return BoxFinder.GetBox(BOX_AVC1,pTrak)!=NULL;
}	

bool Mpeg3gpFile::IsAmrAudio()
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	TrakBox * pTrak = GetTrack(HANDLER_AUDIO);
	if(pTrak==NULL)
		return false;

	return 	BoxFinder.GetBox(BOX_SAMR,pTrak)!=NULL ? true : false;
}

bool Mpeg3gpFile::IsAlacAudio()
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	TrakBox * pTrak = GetTrack(HANDLER_AUDIO);
	if(pTrak==NULL)
		return false;

	return 	BoxFinder.GetBox(BOX_ALAC,pTrak)!=NULL ? true : false;
}

int Mpeg3gpFile::GetObjectTypeIndication(HandlerType Handler)
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	TrakBox * pTrak = GetTrack(Handler);
	if(pTrak==NULL)
		return -1;

	EsdsBox* pEsdsBox = (EsdsBox*)BoxFinder.GetBox(BOX_ESDS,pTrak);

	if(pEsdsBox==NULL)
		return -1;
	if(pEsdsBox->m_pES_Descr==NULL)
		return -1;
	if(pEsdsBox->m_pES_Descr==NULL)
		return -1;
	if(pEsdsBox->m_pES_Descr->m_pDecoderConfigDescr==NULL)
		return -1;

	return 0xff & pEsdsBox->m_pES_Descr->m_pDecoderConfigDescr->m_objectTypeIndication;
}

int Mpeg3gpFile::GetAudioObjectType(HandlerType Handler)
{
	//14496-3 1.6.2.1
	Mp4BoxFinder BoxFinder(BOX_STSD);
	TrakBox * pTrak = GetTrack(Handler);
	if(pTrak==NULL)
		return -1;

	EsdsBox* pEsdsBox = (EsdsBox*)BoxFinder.GetBox(BOX_ESDS,pTrak);

	if(pEsdsBox==NULL)
		return -1;
	if(pEsdsBox->m_pES_Descr==NULL)
		return -1;
	if(pEsdsBox->m_pES_Descr==NULL)
		return -1;
	if(pEsdsBox->m_pES_Descr->m_pDecoderConfigDescr==NULL)
		return -1;
	if(pEsdsBox->m_pES_Descr->m_pDecoderConfigDescr->m_pDecoderSpecificInfo==NULL)
		return -1;

	unsigned char* pAudioSpecificConfig = (unsigned char *)pEsdsBox->m_pES_Descr->m_pDecoderConfigDescr->m_pDecoderSpecificInfo->info;
	if(NULL == pAudioSpecificConfig)
	{
		return -1;
	}
	unsigned int nAudioObjectType = 0;
	nAudioObjectType = (*pAudioSpecificConfig)>>3;
	if(nAudioObjectType == 31)
	{
		unsigned int nExtAudioObjectType = 0;
		nExtAudioObjectType = *pAudioSpecificConfig & 0x7;
		nExtAudioObjectType = (nExtAudioObjectType <<3) + (*(pAudioSpecificConfig+ 1)) >> 5;
		nAudioObjectType = 32 + nExtAudioObjectType;
	}
	return nAudioObjectType;
}

bool Mpeg3gpFile::IsAlawAudio()
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	TrakBox * pTrak = GetTrack(HANDLER_AUDIO);
	if(pTrak==NULL)
		return false;

	return 	BoxFinder.GetBox(BOX_ALAW,pTrak)!=NULL ? true : false;
}

bool Mpeg3gpFile::IsUlawAudio()
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	TrakBox * pTrak = GetTrack(HANDLER_AUDIO);
	if(pTrak==NULL)
		return false;

	return 	BoxFinder.GetBox(BOX_ULAW,pTrak)!=NULL ? true : false;
}

bool Mpeg3gpFile::IsImaAudio()
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	TrakBox * pTrak = GetTrack(HANDLER_AUDIO);
	if(pTrak==NULL)
		return false;

	return 	BoxFinder.GetBox(BOX_IMA4,pTrak)!=NULL ? true : false;
}

bool Mpeg3gpFile::IsMp3Audio()
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	TrakBox * pTrak = GetTrack(HANDLER_AUDIO);
	if(pTrak==NULL)
		return false;

	return 	BoxFinder.GetBox(BOX_MP3_,pTrak)!=NULL ? true : false;
}

Mp4Box* Mpeg3gpFile::GetBox(BoxType aBoxType)
{
	unsigned long i;
	Mp4Box* pBox;
	Mp4BoxFinder BoxFinder(BOX_STCO);
	for(i=0;i<m_ulNumChildren;i++)
	{
		if(m_Children[i]->GetType()==MP4BOX)
		{
			pBox = BoxFinder.GetBox(aBoxType,(Mp4Box *)m_Children[i]);
			if(pBox!=NULL)
				return pBox;
		}
	}
	return 0;
}

#ifdef MP4_MUX
void	Mpeg3gpFile::DeleteTrack(HandlerType Handler)
{
	TrakBox * pTrak = GetTrack(Handler);
	if(pTrak==NULL)
		return;
	Mp4Box * pBox = GetBox(BOX_MOOV);
	pBox->Remove(pTrak);
}
#endif

#if 0
void	Mpeg3gpFile::AddTrack(TrakBox* pTrak)
{
	MoovBox * pMoov = (MoovBox *)GetBox(BOX_MOOV);
	if(pMoov==NULL)
		return;

	pMoov->Adopt(pTrak);
}

TrakBox * Mpeg3gpFile::ExtractTrack(HandlerType Handler)
{
	TrakBox * pTrakBox = GetTrack(Handler);
	if(pTrakBox)
	{
		if(m_pMoovBox)
			m_pMoovBox->Remove(pTrakBox,false);
		return pTrakBox;
	}
	return 0;
}

int Mpeg3gpFile::Extract(HandlerType handler, char * output, bool bWithHeader)
{
	int nWrite=0;
	Mp4TrackStream* pTrackStream = GetTrackStream(handler, 0);

	if(pTrackStream==NULL)
		RETURN_ERROR(-1);

	if(m_lStartTime > 0)
		m_lStartPosition = pTrackStream->GetPosition(m_lStartTime);
	else
		m_lStartPosition = 0;

	pTrackStream->SetPos(m_lStartPosition);

	if(m_lStopTime > 0)
	{
		m_lStopPosition = pTrackStream->GetPosition(m_lStopTime);
		nWrite = m_lStopPosition - m_lStartPosition;
	}

	if(pTrackStream->Write(output, bWithHeader,nWrite)<1)
	{
		delete pTrackStream;
		RETURN_ERROR(-1);
	}

	delete pTrackStream;

	return 0;
}

int Mpeg3gpFile::DumpStsz(HandlerType handler, char * output)
{
	Mp4TrackStream* pTrackStream = GetTrackStream(handler, 0);

	if(pTrackStream==NULL)
		RETURN_ERROR(0);

	if(pTrackStream->DumpStsz(output)!=0)
	{
		delete pTrackStream;
		RETURN_ERROR(-1);
	}

	delete pTrackStream;

	return 0;
}

int Mpeg3gpFile::Extract(HandlerType handler, FileCtrl * pFileCtrl, bool bWithHeader)
{
	Mp4TrackFinder TrackFinder(handler);

	if(m_pMoovBox==0)
		RETURN_ERROR(-1);

	m_pMoovBox->Navigate(&TrackFinder);

	TrakBox * pTrak = TrackFinder.GetTrack();

	if(pTrak==0)
		RETURN_ERROR(-1);

	Mp4TrackStream TrackStream(pTrak, this, 0);

	int nWrite=0;

	if(m_lStartTime > 0)
	{
		m_lStartPosition = TrackStream.GetPosition(m_lStartTime);
	}
	else
	{
		m_lStartPosition = 0;
	}
	TrackStream.SetPos(m_lStartPosition);

	if(m_lStopTime > 0)
	{
		m_lStopPosition = TrackStream.GetPosition(m_lStopTime);
		nWrite = m_lStopPosition - m_lStartPosition;
	}

	//DBG(("m_lStartPosition:%d,m_lStopPosition:%d,StreamSize:%d\n",
	//m_lStartPosition,m_lStopPosition,TrackStream.GetStreamSize()));

	if(TrackStream.Write(pFileCtrl, bWithHeader,nWrite)<1)
		RETURN_ERROR(-1);

	return 0;
}
#define TIMESCALE_90KHZ	90000
int Mpeg3gpFile::UpdateSystemInfo()
{
	StreamInfo audifo;	
	StreamInfo vidifo;	
	float fMaxDuration=0;
	int iTrackIdx=1;
	int iTimeScale = 0;
	int iDuration = 0;
	Mp4BoxFinder BoxFinder(BOX_STCO);
	TrakBox * pTrak = 0;
	TkhdBox * pVideoTkhd = 0;
	TkhdBox * pAudioTkhd = 0;
	HdlrBox  * pHdlr;

	MoovBox * pMoov = (MoovBox *)GetBox(BOX_MOOV);
	if(pMoov==NULL)
		RETURN_ERROR(-1);
	MvhdBox * pMvhd = (MvhdBox *)GetBox(BOX_MVHD);
	if(pMvhd==NULL)
		RETURN_ERROR(-1);

	if(GetStreamInfo(&audifo, HANDLER_AUDIO)==0)
	{
		pTrak = GetTrack(HANDLER_AUDIO);
		pAudioTkhd = (TkhdBox *)BoxFinder.GetBox(BOX_TKHD,pTrak);
		iTrackIdx++;
		pHdlr = (HdlrBox *)BoxFinder.GetBox(BOX_HDLR, pTrak);
		if(fMaxDuration < audifo.m_fDuration)
		{
#ifndef TIMESCALE_90KHZ
			iTimeScale 		= audifo.m_iTimeScale;
			iDuration 		= audifo.m_iDuration;
#else
			iTimeScale		= TIMESCALE_90KHZ;
			iDuration		= (int)(audifo.m_fDuration * TIMESCALE_90KHZ);
#endif
			fMaxDuration 	= audifo.m_fDuration;
		}		
		//KDDI-AU 1.1.6 pg. 30.
		pAudioTkhd->m_width 	= 0x0;
		pAudioTkhd->m_height = 0x0;
	}

	if(GetStreamInfo(&vidifo, HANDLER_VIDEO)==0)
	{
		pTrak = GetTrack(HANDLER_VIDEO);
		pVideoTkhd = (TkhdBox *)BoxFinder.GetBox(BOX_TKHD,pTrak);
		iTrackIdx++;
		pHdlr = (HdlrBox *)BoxFinder.GetBox(BOX_HDLR, pTrak);
		if(fMaxDuration < vidifo.m_fDuration)
		{
#ifndef TIMESCALE_90KHZ
			iTimeScale 		= vidifo.m_iTimeScale;
			iDuration 		= vidifo.m_iDuration;
#else
			iTimeScale		= TIMESCALE_90KHZ;
			iDuration		= (int)(vidifo.m_fDuration * TIMESCALE_90KHZ);
#endif
			fMaxDuration 	= vidifo.m_fDuration;
		}		
		//KDDI-AU 1.1.6 pg. 30.
		pVideoTkhd->m_width 	= 0x01400000;
		pVideoTkhd->m_height = 0x00f00000;
	}

#ifndef TIMESCALE_90KHZ
	pMvhd->m_timescale 	= iTimeScale;
	pMvhd->m_duration 	= iDuration;
#else
	pMvhd->m_timescale 	= TIMESCALE_90KHZ;
	pMvhd->m_duration 	= iDuration;
#endif

	if(pVideoTkhd)
		pVideoTkhd->m_duration = (int)(pMvhd->m_timescale * vidifo.m_fDuration);
	if(pAudioTkhd)
		pAudioTkhd->m_duration = (int)(pMvhd->m_timescale * audifo.m_fDuration);

	pMvhd->m_next_track_ID = iTrackIdx;

	return 0;
}

int Mpeg3gpFile::Write(char * filename)
{
	FileCtrl * pFileCtrl = CreateFileCtrl();
	if(pFileCtrl==0)
		return -1;
	pFileCtrl->Open(filename, 0, FILECTRL_OPEN_WRITE);
	int ret = Write(pFileCtrl);
	delete pFileCtrl;
	return ret;
}

//
// dump data structure and media data into the target file.
//
int Mpeg3gpFile::Write(FileCtrl *pFileCtrlWrite)
{
	Mp4Serializer Dumper;
	long lTotalSize = 0;
	switch(m_lFileFormat)
	{
	case KDDI_3GP_FILE_FORMAT:
		{
			m_pFtypBox->MakeKddi();
			if(m_pUuidCpgd)
				m_pUuidCpgd->SetLoaded(true);
		}
		break;
	case KDDI_AMC_FILE_FORMAT:
		{
			m_pFtypBox->MakeKddiAmc();
			if(m_pUuidCpgd)
				m_pUuidCpgd->SetLoaded(true);
		}
		break;
	default:
		break;
	}

	//
	// DrmBox is the DRM of DoCoMo.
	//
	DrmBox * pDrm = (DrmBox *)GetBox(BOX_DRM);
	if(pDrm)
	{
		if(m_lFileFormat==KDDI_3GP_FILE_FORMAT ||
			m_lFileFormat==KDDI_AMC_FILE_FORMAT)
		{
			pDrm->SetLoaded(false);
		}
		else
		{
			pDrm->SetLoaded(true);
			pDrm->m_bForward 	= m_bForward;
			pDrm->m_bRinger 	= m_bRinger;
		}
	}

	//
	// To support carrier specific file format like KDDI AU.
	//
	Dumper.SetFileFormat(m_lFileFormat);

	//
	// check all the time information for each track and synchronize
	// movie header inforamtion.
	//
	UpdateSystemInfo();

	Mp4TrackStream * pVideoTrack = GetTrackStream(HANDLER_VIDEO, 0);
	if(pVideoTrack)
		lTotalSize += pVideoTrack->GetStreamSize();
	//printf("1:lTotalSize=%d,0x%x\n",lTotalSize,lTotalSize);
	Mp4TrackStream * pAudioTrack = GetTrackStream(HANDLER_AUDIO, 0);
	if(pAudioTrack)
		lTotalSize += pAudioTrack->GetStreamSize();
	//printf("2:lTotalSize=%d,0x%x\n",lTotalSize,lTotalSize);

	// determine the size of box.
	Dumper.SetOutput(pFileCtrlWrite);
	Dumper.PrepareForNavigation();
	Navigate(&Dumper);

	FileCtrl * pFileCtrl = Dumper.m_pFileFormatter->GetFileCtrl();

	lTotalSize += 8;

	Dumper.m_pFileFormatter->Put(0,(long)lTotalSize);
	Dumper.m_pFileFormatter->Put(0,(long)BOX_MDAT);

	// interleave chunks
	int iVideo=1, iAudio=1;
	int iTotal=0;
	if(pVideoTrack && pAudioTrack)
	{
		while(iVideo>0 || iAudio>0)
		{
			iAudio = pAudioTrack->WriteChunk(pFileCtrl,pFileCtrl->Tell());
			if(iAudio>0)
			{
				iTotal += iAudio;
			}
			iVideo = pVideoTrack->WriteChunk(pFileCtrl,pFileCtrl->Tell());
			if(iVideo>0)
			{
				iTotal += iVideo;
			}
		}
	}
	else if(pVideoTrack)
	{
		while(iVideo>0)
		{
			iVideo = pVideoTrack->WriteChunk(pFileCtrl,pFileCtrl->Tell());
			if(iVideo>0)
				iTotal += iVideo;
		}
	}
	else if(pAudioTrack)
	{
		while(iAudio>0)
		{
			iAudio = pAudioTrack->WriteChunk(pFileCtrl,pFileCtrl->Tell());
			if(iAudio>0)
				iTotal += iAudio;
		}
	}

	//
	// DrmBox is the DRM of DoCoMo.
	//
	if(pDrm)
	{
		if(m_lFileFormat!=KDDI_3GP_FILE_FORMAT &&
			m_lFileFormat!=KDDI_AMC_FILE_FORMAT)
		{
			// for DoCoMo
			pDrm->NotifyFileSize(pFileCtrl->Tell());
		}
	}

	//printf("total %d is writtent to the file\n",iTotal);

	//
	// rewrite box structures with updated information.
	//
	// now dump the file.
	pFileCtrl->Seek(0);
	Dumper.PrepareForNavigation();
	Navigate(&Dumper);


	//
	// NOTE: do not delete pFileCtrl as it belongs to Dumper.
	//

	return 0;
}

int Mpeg3gpFile::SetSegment(int iFrom, int iTo)
{
	m_lStartTime = iFrom;
	m_lStopTime  = iTo;	
	return 0;
}

//
//
//
StblBox * Mpeg3gpFile::BuildMp4vStbl(float fFrameRate)
{
	int 	iVop, iPictureCount, iSyncPoint, iPos;
	int 	iFirstVopPos=0;
	bool 	bContinue;
	float fDuration = 0;

	iPictureCount = iSyncPoint = 0;
	bContinue = true;
	while(bContinue)
	{
		iVop = SearchNextVop(iPos);
		switch(iVop)
		{
		case I_VOP:
			++iSyncPoint;
		case P_VOP:
		case B_VOP:
		case S_VOP:
			++iPictureCount;
			if(iPictureCount==1)
			{
				iFirstVopPos = iPos;
			}
			break;
		default:
			bContinue = false;
			break;
		}
	}

	fDuration = (float)iPictureCount/fFrameRate;

	StblBox * pStbl = new StblBox(0,8,0);

	StszBox * pStsz = new StszBox(0,8+12+4*iPictureCount,pStbl);
	pStbl->m_pStszBox = pStsz;
	pStsz->m_version = 0;
	memset(pStsz->m_flags, 0x0, 3);
	pStsz->m_sample_size = 0;
	pStsz->m_sample_count = iPictureCount;
	if(iPictureCount < 1)
		RETURN_ERROR(0);
	pStsz->m_entry_size = new long [iPictureCount];

	float fDuration90khz = (float)(fDuration*90000.);
	int   delta = (int)(fDuration90khz/(float)iPictureCount);
	SttsBox * pStts = new SttsBox(0,8+16,pStbl);
	pStbl->m_pSttsBox = pStts;
	pStts->m_version = 0;
	memset(pStts->m_flags, 0x0, 3);

	int deltamod = (int)fDuration90khz%iPictureCount;

	if(deltamod!=0)
	{
		pStts->m_entry_count = 2;
		//		pStts->m_sample_count =  new long [2];
		//		pStts->m_sample_count[0] = iPictureCount-1;
		//		pStts->m_sample_count[1] = 1;
		//		pStts->m_sample_delta =  new long [2];
		//		pStts->m_sample_delta[0] = delta;
		//		pStts->m_sample_delta[1] = (int)(fDuration90khz-(float)delta*(iPictureCount-1));
		//		pStts->m_total_delta=pStts->m_sample_delta[0]*pStts->m_sample_count[0];
		//		pStts->m_total_delta+=pStts->m_sample_delta[1]*pStts->m_sample_count[1];
		//		pStts->m_sample_num = pStts->m_sample_count[0];
		//		pStts->m_sample_num += pStts->m_sample_count[1];

		pStts->m_entry = new tagSTTSBOX[2];
		pStts->m_entry[0].sample_count = iPictureCount - 1;
		pStts->m_entry[0].sample_delta = delta;
		pStts->m_entry[1].sample_count = 1;
		pStts->m_entry[1].sample_delta = (int)(fDuration90khz-(float)delta*(iPictureCount-1));
		pStts->m_total_delta = pStts->m_entry[0].sample_count*pStts->m_entry[0].sample_delta;
		pStts->m_total_delta += pStts->m_entry[1].sample_count*pStts->m_entry[1].sample_delta;
		pStts->m_sample_num = pStts->m_entry[0].sample_count;
		pStts->m_sample_num += pStts->m_entry[1].sample_count;
	}
	else
	{
		pStts->m_entry_count = 1;
		//		pStts->m_sample_count =  new long [1];
		//		pStts->m_sample_count[0] = iPictureCount;
		//		pStts->m_sample_delta =  new long [1];
		//		pStts->m_sample_delta[0] = delta;
		//		pStts->m_total_delta=pStts->m_sample_delta[0]*pStts->m_sample_count[0];
		//		pStts->m_sample_num = pStts->m_sample_count[0];

		pStts->m_entry = new tagSTTSBOX[1];
		pStts->m_entry[0].sample_count = iPictureCount;
		pStts->m_entry[0].sample_delta = delta;
		pStts->m_total_delta = pStts->m_entry[0].sample_count*pStts->m_entry[0].sample_delta;
		pStts->m_sample_num = pStts->m_entry[0].sample_count;
	}

	StssBox * pStss = new StssBox(0,8+8+4*iSyncPoint,pStbl);
	pStbl->m_pStssBox = pStss;
	pStss->m_version = 0;
	memset(pStss->m_flags, 0x0, 3);
	pStss->m_entry_count = iSyncPoint;
	if(iSyncPoint > 0)
	{
		pStss->m_sample_number = new long [iSyncPoint];
	}

	StscBox * pStsc = new StscBox(0,8,pStbl);
	pStbl->m_pStscBox = pStsc;
	pStsc->m_version = 0;
	memset(pStsc->m_flags, 0x0, 3);

	float fChunkTimeUnit = VIDEO_CHUNK_TIME;
	float fChunkTime = 0;
	int iChunkedSample = 0;
	float fChunkedTime = 0;
	int iChunk=0, iLastChunk=0;
	int iChunkIdx = 0;
	int iMaxEntryCount = (int)(fDuration/fChunkTimeUnit + 1);

	pStsc->m_entry_count = 0;
	//	pStsc->m_first_chunk = new long [iMaxEntryCount];
	//	pStsc->m_samples_per_chunk = new long [iMaxEntryCount];
	//	pStsc->m_sample_description_index = new long [iMaxEntryCount];
	pStsc->m_entry = new tagSTSCBOX[iMaxEntryCount];

	StcoBox * pStco = new StcoBox(0,8,pStbl);
	pStco->m_version = 0;
	pStbl->m_pStcoBox = pStco;
	memset(pStco->m_flags, 0x0, 3);
	pStco->m_chunk_offset = new long [iMaxEntryCount];

	while(iChunkedSample < iPictureCount)
	{
		fChunkedTime = (float)iChunkedSample / fFrameRate;
		iChunkIdx++;
		iChunk = (int)(((fChunkTime+fChunkTimeUnit)-fChunkedTime)*fFrameRate);
		if((iChunk+iChunkedSample) > iPictureCount)
			iChunk = iPictureCount - iChunkedSample;
		if(iChunk < 1)
			iChunk = 1;
		if(iChunk!=iLastChunk)
		{
			//			pStsc->m_first_chunk[pStsc->m_entry_count] = iChunkIdx;
			//			pStsc->m_samples_per_chunk[pStsc->m_entry_count] = iChunk;
			//			pStsc->m_sample_description_index[pStsc->m_entry_count] = 1;
			pStsc->m_entry[pStsc->m_entry_count].first_chunk = iChunkIdx;
			pStsc->m_entry[pStsc->m_entry_count].samples_per_chunk = iChunk;
			pStsc->m_entry[pStsc->m_entry_count].sample_description_index = 1;
			++(pStsc->m_entry_count);
		}

		pStco->m_chunk_offset[iChunkIdx-1] = iChunkedSample+1;

		iChunkedSample += iChunk;
		fChunkTime+=fChunkTimeUnit;
		iLastChunk = iChunk;
	}

	//printf("ChunkedSample:%d, PictureCount:%d\n",iChunkedSample,iPictureCount);

	pStco->m_entry_count = iChunkIdx;
	bContinue = true;

	int iPictureIdx=0;
	int iLastPos = m_lFileFormat==KDDI_AMC_FILE_FORMAT ? 0 : iFirstVopPos;
	int iStssIdx = 0;
	int iMaxFrameSize = 0;
	iChunkIdx = 0;
	SetPos(0);
	while(bContinue)
	{
		iVop = SearchNextVop(iPos);
		switch(iVop)
		{
		case I_VOP:
			pStss->m_sample_number[iStssIdx] = iPictureIdx+1;
			++iStssIdx;
		case P_VOP:
		case B_VOP:
		case S_VOP:
			if(iPictureIdx > 0)
			{
				pStsz->m_entry_size[iPictureIdx-1] = iPos-iLastPos;
				if(pStsz->m_entry_size[iPictureIdx-1] > iMaxFrameSize)
					iMaxFrameSize=pStsz->m_entry_size[iPictureIdx-1];
				if(iPictureIdx==(iPictureCount-1))
				{
					pStsz->m_entry_size[iPictureIdx] = GetFileSize()-iPos;
					if(pStsz->m_entry_size[iPictureIdx] > iMaxFrameSize)
						iMaxFrameSize=pStsz->m_entry_size[iPictureIdx];
				}
			}
			iPictureIdx++;
			if(iPictureIdx==pStco->m_chunk_offset[iChunkIdx])
			{
				if(m_lFileFormat==KDDI_AMC_FILE_FORMAT && iPictureIdx==1)
				{
					pStco->m_chunk_offset[iChunkIdx] = 0;
					iPos = 0;
				}
				else
					pStco->m_chunk_offset[iChunkIdx] = iPos;
				++iChunkIdx;
			}
			iLastPos = iPos;
			break;
		default:
			bContinue = false;
			break;
		}
	}

	StsdBox * pStsd = new StsdBox(0,8,pStbl);
	pStbl->m_pStsdBox = pStsd;
	pStsd->m_version = 0;
	memset(pStsd->m_flags, 0x0, 3);
	pStsd->m_entry_count = 1;

	Mp4vBox * pMp4v = new Mp4vBox(0,8,pStbl);

	//
	// This is just initialization, the actual video size will be extracted from
	// DSI later on.
	//
	if(m_lFileFormat==KDDI_AMC_FILE_FORMAT)
	{
		switch(m_cKddiAmcRecMode)
		{
		case AmcResolution96x80:
			pMp4v->m_width 	= 96;
			pMp4v->m_height 	= 80;
			break;
		case AmcResolution128x96:
			pMp4v->m_width 	= 128;
			pMp4v->m_height 	= 96;
			break;
		case AmcResolution176x144:
			pMp4v->m_width 	= 176;
			pMp4v->m_height 	= 144;
			break;
		default:
			pMp4v->m_width 	= 128;
			pMp4v->m_height 	= 96;
			break;
		}
	}
	else
	{
		pMp4v->m_width 	= 176;
		pMp4v->m_height 	= 144;
	}

	pStsd->Adopt(pMp4v);
	EsdsBox * pEsds 					= new EsdsBox(0,8,pMp4v);
	pMp4v->Adopt(pEsds);
	pMp4v->m_pEsdsBox 				= pEsds;
	ES_Descr * pES_Descr 			= new ES_Descr(0,2);
	pES_Descr->SetBoxContainer(pEsds);
	pEsds->Adopt(pES_Descr);
	pEsds->m_pES_Descr 				= pES_Descr;
	pES_Descr->m_streamPriority 	= 16;	// KDDI AU 1.1.6 pg. 15.
	pES_Descr->m_ES_ID 				= 0;

	DecoderConfigDescr * pDecoderConfigDescr 
		= new	DecoderConfigDescr(0,2,pES_Descr);

	pDecoderConfigDescr->m_objectTypeIndication 	= 0x20;
	pDecoderConfigDescr->m_streamType 				= 0x04;
	pDecoderConfigDescr->m_upStream 					= 0x0;
	pDecoderConfigDescr->m_reserved 					= 0x1;
	pDecoderConfigDescr->m_bufferSizeDB 			= iMaxFrameSize+128;
	pDecoderConfigDescr->m_maxBitrate 				= 
		(int)(pDecoderConfigDescr->m_bufferSizeDB*8*fFrameRate);
	pDecoderConfigDescr->m_avgBitrate 				= 
		(int)( ((float)(GetFileSize()*8.))/fDuration+.5);
	pES_Descr->Adopt(pDecoderConfigDescr);
	pES_Descr->m_pDecoderConfigDescr 				= 	pDecoderConfigDescr;

	SLConfigDescr * pSLConfigDescr = new SLConfigDescr(0,2,pES_Descr);
	pSLConfigDescr->m_predefined = 0x2;
	pES_Descr->Adopt(pSLConfigDescr); 
	pES_Descr->m_pSLConfigDescr = pSLConfigDescr; 

	DecoderSpecificInfo * pDecoderSpecificInfo 
		= new DecoderSpecificInfo(0,2,pDecoderConfigDescr);
	pDecoderSpecificInfo->m_iSize = iFirstVopPos; // ??
	pDecoderSpecificInfo->info = new char [iFirstVopPos];

	SetPos(0);
	GetBuffer(pDecoderSpecificInfo->info, iFirstVopPos);
	pDecoderConfigDescr->Adopt(pDecoderSpecificInfo); 
	pDecoderConfigDescr->m_pDecoderSpecificInfo = pDecoderSpecificInfo; 

	//
	// parse decoder specific info to get video size information.
	//
	BufferFile Buffer(pDecoderSpecificInfo->info, pDecoderSpecificInfo->m_iSize);
	Buffer.SetPos(0);
	while(Buffer.GetPos() < Buffer.GetFileSize())
	{
		int start_code = Buffer.Get32();
		if( (start_code & 0xffffff00) == 0x00000100)
		{
			start_code &= 0xff;
			Buffer.SetPos(Buffer.GetPos()-4);
			if(start_code == VISUAL_OBJECT_SEQUENCE_START_CODE)
			{
				Vos* pVos = new Vos(0, 0);
				pVos->Load(&Buffer);
				if(pVos->pVisualObject)
				{
					if(pVos->pVisualObject->pVideoObjectLayer)
					{
						pMp4v->m_width = pVos->pVisualObject->pVideoObjectLayer->video_object_layer_width;
						pMp4v->m_height	= pVos->pVisualObject->pVideoObjectLayer->video_object_layer_height;
					}
				}
				delete pVos;
				break;
			}
			else if(start_code <= VIDEO_OBJECT_START_CODE_END && start_code >= VIDEO_OBJECT_START_CODE_BEGIN )  
			{
				Buffer.SetPos(Buffer.GetPos()+3);
			}
			else if(start_code <= VIDEO_OBJECT_LAYER_START_CODE_END && start_code >= VIDEO_OBJECT_LAYER_START_CODE_BEGIN )
			{
				VideoObjectLayer * pVideoObjectLayer = new VideoObjectLayer(0,0);
				if( pVideoObjectLayer->Load(&Buffer) == 0)
				{
					pMp4v->m_width = pVideoObjectLayer->video_object_layer_width;
					pMp4v->m_height = pVideoObjectLayer->video_object_layer_height;
				}
				delete pVideoObjectLayer;
				break;				
			} 
			else if(start_code == VISUAL_OBJECT_START_CODE)
			{
				VisualObject * pVisualObject= new VisualObject(0, 0);
				pVisualObject->Load(&Buffer);
				if(pVisualObject->pVideoObjectLayer)
				{
					pMp4v->m_width = pVisualObject->pVideoObjectLayer->video_object_layer_width;
					pMp4v->m_height = pVisualObject->pVideoObjectLayer->video_object_layer_height;
				}
				delete pVisualObject;
				break;
			}
			Buffer.SetPos(Buffer.GetPos()+1);
		}
	}

	pStbl->Adopt(pStsd);
	pStbl->Adopt(pStts);
	pStbl->Adopt(pStss);
	pStbl->Adopt(pStsc);
	pStbl->Adopt(pStsz);
	pStbl->Adopt(pStco);

	return pStbl;
}


//
// NOTE:
// BuildAacStbl has not been tested.
//
StblBox * Mpeg3gpFile::BuildAacStbl(char * szfile, unsigned char * pDsi, int iDsiSize, int & iSamplingRate)
{
	// ISO/IEC 14496-3:1998 FCD Subpart 1: 3.2.2 */
	static const unsigned long sampling_rates_tab[16] =
	{
		96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
		16000, 12000, 11025, 8000, /* reserved */
	};

	if(szfile==0 || pDsi==0 || iDsiSize < 1)
		RETURN_ERROR(0);

	BufferFile Buffer((char *)pDsi,iDsiSize);
	Buffer.SetPos(0);

	unsigned char cObjType = Buffer.GetBits(5);
	unsigned char cSamplingIdx = Buffer.GetBits(4);

	if(cSamplingIdx==0xf)
		iSamplingRate = Buffer.GetBits(24);
	else
		iSamplingRate = sampling_rates_tab[cSamplingIdx];

	unsigned char cChannels = Buffer.GetBits(4);
	unsigned char cFrameLengthFlag = Buffer.GetBits(1);
	unsigned char cDependsOnCoreCoder = Buffer.GetBits(1);
	unsigned char cExtensionFlag = Buffer.GetBits(1);

	// ISO/IEC 14496-3:1999/Amd.1:2000 5.1.1 */
	// ObjectType: AAC LC: 0x2
	if(cObjType!=0x2 || cChannels==0)
	{
		RETURN_ERROR(0);
	}

	// # samples in Frame ?
	int iFrameSize = cFrameLengthFlag ? 960 : 1024;

	FileCtrl * pFileCtrl = CreateFileCtrl();
	if(pFileCtrl->Open(szfile, 0, FILECTRL_OPEN_READ)!=0)
		RETURN_ERROR(0);

	int 	iFileSize = pFileCtrl->Size();

	int 	i;
	int   iFrameCount 	= iFileSize>>1;
	float fFrameDuration = (float)(iFrameSize)/
		(float)(iSamplingRate);
	float fDuration 		= (float)(iFrameCount*iFrameSize)/
		(float)(iSamplingRate);

	if(iFrameCount < 1)
		RETURN_ERROR(0);

	StblBox * pStbl 			= new StblBox(0,8,0);
	StszBox * pStsz 			= new StszBox(0,8+12+4*iFrameCount,pStbl);
	pStbl->m_pStszBox 		= pStsz;
	pStsz->m_version 			= 0;
	memset(pStsz->m_flags, 0x0, 3);
	pStsz->m_sample_size 	= 0;
	pStsz->m_sample_count 	= iFrameCount;
	pStsz->m_entry_size 		= new long [iFrameCount];

	int iRet;
	int iTotalSize=0;
	unsigned short tmp;
	for(i=0;i<iFrameCount;i++)
	{
		iRet = pFileCtrl->Read((char *)&tmp, sizeof(short));
		if(iRet < 2)
			break;
		pStsz->m_entry_size[i] = tmp;
		iTotalSize += pStsz->m_entry_size[i];
	}

	delete pFileCtrl;
	pFileCtrl = 0;

	int iRawDataSize = GetFileSize();
	if(iTotalSize > iRawDataSize)
	{
		int iDiff = iTotalSize-iRawDataSize;
		if(iDiff <= pStsz->m_entry_size[iFrameCount-1])
		{
			pStsz->m_entry_size[iFrameCount-1] -= iDiff;
		}
		else
		{
		}
	}


	int   delta = (int)(fFrameDuration*(float)iSamplingRate + 0.5);

	SttsBox * pStts 			= new SttsBox(0,8+16,pStbl);
	pStts->m_version 			= 0;
	memset(pStts->m_flags, 0x0, 3);
	pStts->m_entry_count 	= 1;
	//	pStts->m_sample_count 	= new long [1];
	//	pStts->m_sample_count[0]= iFrameCount;
	//	pStts->m_sample_delta 	= new long [1];
	//	pStts->m_sample_delta[0]= delta;
	pStts->m_entry = new tagSTTSBOX[1];
	pStts->m_entry[0].sample_count = iFrameCount;
	pStts->m_entry[0].sample_delta = delta;
	pStts->m_total_delta		= pStts->m_entry[0].sample_delta*pStts->m_entry[0].sample_count;
	pStts->m_sample_num 		= pStts->m_entry[0].sample_count;

	StscBox * pStsc 			= new StscBox(0,8,pStbl);
	pStbl->m_pStscBox 		= pStsc;
	pStsc->m_version 			= 0;
	memset(pStsc->m_flags, 0x0, 3);

	float fChunkTimeUnit = AAC_CHUNK_TIME;
	float fChunkTime = 0;
	int 	iChunkedSample = 0;
	float fChunkedTime = 0;
	int 	iChunk=0, iLastChunk=0;
	int 	iChunkIdx = 0;
	int 	iMaxEntryCount = (int)(fDuration/fChunkTimeUnit + 1);

	pStsc->m_entry_count = 0;
	//	pStsc->m_first_chunk = new long [iMaxEntryCount];
	//	pStsc->m_samples_per_chunk = new long [iMaxEntryCount];
	//	pStsc->m_sample_description_index = new long [iMaxEntryCount];
	pStsc->m_entry = new tagSTSCBOX[iMaxEntryCount];

	StcoBox * pStco = new StcoBox(0,8,pStbl);
	pStco->m_version = 0;
	pStbl->m_pStcoBox = pStco;
	memset(pStco->m_flags, 0x0, 3);
	pStco->m_chunk_offset = new long [iMaxEntryCount];

	int iChunkOffset = 0;
	int iStszIdx = 0;
	while(iChunkedSample < iFrameCount)
	{
		fChunkedTime = (float)iChunkedSample * fFrameDuration;
		iChunk =
			(int)(((fChunkTime+fChunkTimeUnit)-fChunkedTime)/fFrameDuration+0.5);
		pStco->m_chunk_offset[iChunkIdx] = iChunkOffset;

		if((iChunk+iChunkedSample) > iFrameCount)
			iChunk = iFrameCount - iChunkedSample;
		if(iChunk < 1)
			iChunk = 1;
		if(iChunk!=iLastChunk)
		{
			//			pStsc->m_first_chunk[pStsc->m_entry_count] = iChunkIdx+1;
			//			pStsc->m_samples_per_chunk[pStsc->m_entry_count] = iChunk;
			//			pStsc->m_sample_description_index[pStsc->m_entry_count] = 1;
			pStsc->m_entry[pStsc->m_entry_count].first_chunk = iChunkIdx+1;
			pStsc->m_entry[pStsc->m_entry_count].samples_per_chunk = iChunk;
			pStsc->m_entry[pStsc->m_entry_count].sample_description_index = 1;
			++(pStsc->m_entry_count);
		}

		for(i=0;i<iChunk;i++,iStszIdx++)
		{
			iChunkOffset += pStsz->m_entry_size[iStszIdx];
		}

		iChunkIdx++;
		iChunkedSample += iChunk;
		fChunkTime		+= fChunkTimeUnit;
		iLastChunk 		 = iChunk;
	}

	pStco->m_entry_count = iChunkIdx;

	StsdBox * pStsd = new StsdBox(0,8,pStbl);
	pStsd->m_version = 0;
	memset(pStsd->m_flags, 0x0, 3);
	pStsd->m_entry_count = 1;

	Mp4aBox * pMp4a = new Mp4aBox(0,8,pStbl);

	memset(pMp4a->m_reserved1,0x0,6);
	pMp4a->m_data_reference_index = 1;
	memset(pMp4a->m_reserved2,0x0,8);
	pMp4a->m_channelcount 	= 2;
	pMp4a->m_samplesize 		= 16;
	pMp4a->m_pre_defined 	= 0;
	pMp4a->m_reserved3 		= 0;
	pMp4a->m_sampleratehi 	= iSamplingRate;
	pMp4a->m_sampleratelo 	= 0;
	pStsd->Adopt(pMp4a);

	EsdsBox * pEsds = new EsdsBox(0,8,pMp4a);
	pMp4a->Adopt(pEsds);
	pMp4a->m_pEsdsBox 	= pEsds;
	ES_Descr * pES_Descr = new ES_Descr(0,2);
	pES_Descr->SetBoxContainer(pEsds);
	pEsds->Adopt(pES_Descr);
	pEsds->m_pES_Descr 			 = pES_Descr;
	pES_Descr->m_streamPriority 	= 16;	// KDDI AU 1.1.6 pg. 15.
	pES_Descr->m_ES_ID 				= 0;

	DecoderConfigDescr * pDecoderConfigDescr 
		= new	DecoderConfigDescr(0,2,pES_Descr);

	pDecoderConfigDescr->m_objectTypeIndication 	= 0x40;
	pDecoderConfigDescr->m_streamType 				= 0x05;
	pDecoderConfigDescr->m_upStream 					= 0x0;
	pDecoderConfigDescr->m_reserved 					= 0x1;
	pDecoderConfigDescr->m_bufferSizeDB 			= 24000;
	pDecoderConfigDescr->m_maxBitrate 				= 48000;
	pDecoderConfigDescr->m_avgBitrate 				= 48000;
	pES_Descr->Adopt(pDecoderConfigDescr);
	pES_Descr->m_pDecoderConfigDescr = 	pDecoderConfigDescr;

	DecoderSpecificInfo * pDecoderSpecificInfo 
		= new DecoderSpecificInfo(0,2,pDecoderConfigDescr);
	pDecoderSpecificInfo->m_iSize = iDsiSize; // ??
	pDecoderSpecificInfo->info = new char [iDsiSize];
	memcpy(pDecoderSpecificInfo->info,pDsi,iDsiSize);
	pDecoderConfigDescr->m_pDecoderSpecificInfo = pDecoderSpecificInfo;
	pDecoderConfigDescr->Adopt(pDecoderSpecificInfo);

	SLConfigDescr * pSLConfigDescr = new SLConfigDescr(0,2,pES_Descr);
	pSLConfigDescr->m_predefined = 0x2;
	pES_Descr->Adopt(pSLConfigDescr); 
	pES_Descr->m_pSLConfigDescr = pSLConfigDescr; 

	pStbl->Adopt(pStsd);
	pStbl->Adopt(pStts);
	pStbl->Adopt(pStsc);
	pStbl->Adopt(pStsz);
	pStbl->Adopt(pStco);

	pStbl->m_pStsdBox = pStsd;
	pStbl->m_pSttsBox = pStts;
	pStbl->m_pStscBox = pStsc;
	pStbl->m_pStszBox	= pStsz;
	pStbl->m_pStcoBox = pStco;

	return pStbl;
}

StblBox * Mpeg3gpFile::BuildAmrStbl(int aMode)
{
	int iFrameSize 		= 1;
	int iMode;

	switch(aMode)
	{
	default:
		iFrameSize = 1;
		iMode = 15;
		break;
	case MR475:
		iFrameSize = 13;
		iMode = 0;
		break;
	case MR515:
		iFrameSize = 14;
		iMode = 1;
		break;
	case MR59:
		iFrameSize = 16;
		iMode = 2;
		break;
	case MR67:
		iFrameSize = 18;
		iMode = 3;
		break;
	case MR74:
		iFrameSize = 20;
		iMode = 4;
		break;
	case MR795:
		iFrameSize = 21;
		iMode = 5;
		break;
	case MR102:
		iFrameSize = 27;
		iMode = 6;
		break;
	case MR122:
		iFrameSize = 32;
		iMode = 7;
		break;
	}


	int   iFileSize 		= GetFileSize();
	int   iFrameCount 	= iFileSize/iFrameSize;
	float fFrameDuration = (float)0.02; // 20ms, 160 samples fixed.
	float fDuration 		= (float)iFrameCount * fFrameDuration;

	if(iFrameCount < 1)
		RETURN_ERROR(0);

	StblBox * pStbl = new StblBox(0,8,0);
	StszBox * pStsz = new StszBox(0,8+12+4*iFrameCount,pStbl);
	pStsz->m_version 			= 0;
	memset(pStsz->m_flags, 0x0, 3);
	pStsz->m_sample_size 	= iFrameSize;
	pStsz->m_sample_count 	= iFrameCount;
	pStsz->m_entry_size 		= 0;

	int   delta = (int)(8000.*fFrameDuration+0.1);

	SttsBox * pStts 			= new SttsBox(0,8+16,pStbl);
	pStts->m_version 			= 0;
	memset(pStts->m_flags, 0x0, 3);
	pStts->m_entry_count 	= 1;
	//	pStts->m_sample_count 	= new long [1];
	//	pStts->m_sample_count[0]= iFrameCount;
	//	pStts->m_sample_delta 	= new long [1];
	//	pStts->m_sample_delta[0]= delta;
	pStts->m_entry = new tagSTTSBOX[1];
	pStts->m_entry[0].sample_count = iFrameCount;
	pStts->m_entry[0].sample_delta = delta;
	pStts->m_total_delta		= pStts->m_entry[0].sample_delta*pStts->m_entry[0].sample_count;
	pStts->m_sample_num 		= pStts->m_entry[0].sample_count;

	StscBox * pStsc 	= new StscBox(0,8,pStbl);
	pStsc->m_version 	= 0;
	memset(pStsc->m_flags, 0x0, 3);

	float fChunkTimeUnit = (float)AMR_CHUNK_TIME;
	float fChunkTime = 0;
	int 	iChunkedSample = 0;
	float fChunkedTime = 0;
	int 	iChunk=0, iLastChunk=0;
	int 	iChunkIdx = 0;
	int 	iMaxEntryCount = (int)(fDuration/fChunkTimeUnit + 1);

	pStsc->m_entry_count = 0;
	//	pStsc->m_first_chunk = new long [iMaxEntryCount];
	//	pStsc->m_samples_per_chunk = new long [iMaxEntryCount];
	//	pStsc->m_sample_description_index = new long [iMaxEntryCount];
	pStsc->m_entry = new tagSTSCBOX[iMaxEntryCount];

	StcoBox * pStco = new StcoBox(0,8,pStbl);
	pStco->m_version = 0;
	memset(pStco->m_flags, 0x0, 3);
	pStco->m_chunk_offset = new long [iMaxEntryCount];

	while(iChunkedSample < iFrameCount && iChunkIdx<iMaxEntryCount)
	{
		fChunkedTime = (float)iChunkedSample * fFrameDuration;
		pStco->m_chunk_offset[iChunkIdx] = iChunkedSample*iFrameSize;
		iChunk =(int)(((fChunkTime+fChunkTimeUnit)-fChunkedTime)/fFrameDuration+0.5);
		if((iChunk+iChunkedSample) > iFrameCount)
			iChunk = iFrameCount - iChunkedSample;
		if(iChunk < 1)
			iChunk = 1;
		if(iChunk!=iLastChunk)
		{
			//			pStsc->m_first_chunk[pStsc->m_entry_count] 					= iChunkIdx+1;
			//			pStsc->m_samples_per_chunk[pStsc->m_entry_count] 			= iChunk;
			//			pStsc->m_sample_description_index[pStsc->m_entry_count] 	= 1;
			pStsc->m_entry[pStsc->m_entry_count].first_chunk = iChunkIdx+1;
			pStsc->m_entry[pStsc->m_entry_count].samples_per_chunk = iChunk;
			pStsc->m_entry[pStsc->m_entry_count].sample_description_index = 1;
			++(pStsc->m_entry_count);
		}

		iChunkIdx++;
		iChunkedSample += iChunk;
		fChunkTime+=fChunkTimeUnit;
		iLastChunk = iChunk;
	}

	//printf("ChunkedSample:%d\n",iChunkedSample);

	pStco->m_entry_count = iChunkIdx;

	StsdBox * pStsd = new StsdBox(0,8,pStbl);
	pStsd->m_version = 0;
	memset(pStsd->m_flags, 0x0, 3);
	pStsd->m_entry_count = 1;

	SamrBox * pSamr = new SamrBox(0,8,pStbl);
	memset(pSamr->m_reserved1, 0x0, 6);
	pSamr->m_data_reference_index = 1;
	memset(pSamr->m_reserved2, 0x0, sizeof(long)*2);
	pSamr->m_channelcount 	= 2;
	pSamr->m_samplesize 		= 16;
	pSamr->m_pre_defined 	= 0;
	pSamr->m_reserved3 		= 0;
	pSamr->m_sampleratehi 	= 8000;
	pSamr->m_sampleratelo 	= 0;
	pStsd->Adopt(pSamr);

	DamrBox * pDamr 					= new DamrBox(0,8,pSamr);
	pDamr->m_vendor 					= 'i'<<24|'v'<<16|'i'<<8|'i';
	pDamr->m_decoder_version 		= 0;
	pDamr->m_mode_set 				= 1 << (iMode&0xff);
	pDamr->m_mode_change_period 	= 0;
	pDamr->m_frames_per_sample 	= 1;
	pSamr->m_pDamrBox 				= pDamr;
	pSamr->Adopt(pDamr);

	pStbl->Adopt(pStsd);
	pStbl->Adopt(pStts);
	pStbl->Adopt(pStsc);
	pStbl->Adopt(pStsz);
	pStbl->Adopt(pStco);

	pStbl->m_pStsdBox = pStsd;
	pStbl->m_pSttsBox = pStts;
	pStbl->m_pStscBox = pStsc;
	pStbl->m_pStszBox = pStsz;
	pStbl->m_pStcoBox = pStco;

	return pStbl;
}

/** pahtname should be in .qcp format.
*/
StblBox * Mpeg3gpFile::BuildQcelpStbl(char * pathname, unsigned char octet0)
{
	RiffFile * pRiff = new RiffFile(pathname);
	if(pRiff->Load()<0)
		return 0;

	// prepare DSI information.
	int iBeginOffset 	= 8;
	int iEndOffset 	= pRiff->m_pRiffRiff->m_pRiffQcpFmt->m_iBeginOffset +
		pRiff->m_pRiffRiff->m_pRiffQcpFmt->GetSize()-1;
	int iDsiSize 			= iEndOffset-iBeginOffset+1;
	if(iDsiSize < 0)
		RETURN_ERROR(0);
	char * pDsi = new char [iDsiSize];

	FileCtrl * pFileCtrl = CreateFileCtrl();
	if(pFileCtrl->Open(pathname, 0, FILECTRL_OPEN_READ)!=0)
		RETURN_ERROR(0);
	pFileCtrl->Seek(8);
	pFileCtrl->Read(pDsi,iDsiSize);
	pFileCtrl->Close();
	delete pFileCtrl;

	//printf("**** iDsiSize=%d, octet0=%d\n",iDsiSize,octet0);

	//
	// QCELP RTP FRAMESIZE TABLE -- RFC2658
	//
	static const int qcelp_framesize_tab[] = { 1,4,8,17,35,8,1 };
	static const int qcelp_rate_tab[] = { 0,13200/8,13200/4,13200/2,13200,0,0 };

	//
	// for now, we just use fixed rate 
	// frame contains 160 samples, which corresponds 20ms.
	//
	int iFrameSize  = qcelp_framesize_tab[octet0];
	int iFrameCount = GetFileSize()/iFrameSize;
	int iRemainder  = GetFileSize()%iFrameSize;

	//printf("**** iFrameSize=%d, iFrameCount=%d, iRemainder=%d\n",iFrameSize,iFrameCount,iRemainder);

	if(iRemainder) // should be matched with octet0 information.
	{
		// chopt remainder out from muxing.
		//RETURN_ERROR(0);
	}

	StblBox * pStbl 			= new StblBox(0,8,0);

	StszBox * pStsz 			= new StszBox(0,8+12+4,pStbl);
	pStbl->m_pStszBox	= pStsz;
	pStsz->m_version 			= 0;
	memset(pStsz->m_flags, 0x0, 3);
	pStsz->m_sample_size 	= iFrameSize;
	pStsz->m_sample_count 	= iFrameCount;
	pStsz->m_entry_size 		= 0;

	SttsBox * pStts 			= new SttsBox(0,8+16,pStbl);
	pStts->m_version 			= 0;
	memset(pStts->m_flags, 0x0, 3);
	pStts->m_entry_count 	= 1;
	//	pStts->m_sample_count 	= new long [1];
	//	pStts->m_sample_count[0]= iFrameCount;
	//	pStts->m_sample_delta 	= new long [1];
	//	pStts->m_sample_delta[0]= 160;
	//	pStts->m_total_delta		= pStts->m_sample_delta[0]*pStts->m_sample_count[0];
	//	pStts->m_sample_num 		= pStts->m_sample_count[0];
	pStts->m_entry = new tagSTTSBOX[1];
	pStts->m_entry[0].sample_count = iFrameCount;
	pStts->m_entry[0].sample_delta = 160;
	pStts->m_total_delta		= pStts->m_entry[0].sample_delta*pStts->m_entry[0].sample_count;
	pStts->m_sample_num 		= pStts->m_entry[0].sample_count;

	StscBox * pStsc 	= new StscBox(0,8,pStbl);
	pStsc->m_version 	= 0;
	memset(pStsc->m_flags, 0x0, 3);

	float fDuration = (float)iFrameCount * (float)0.02;
	float fChunkTimeUnit = (float)QCELP_CHUNK_TIME;
	float fChunkTime = 0;
	int 	iChunkedSample = 0;
	float fChunkedTime = 0;
	int 	iChunk=0, iLastChunk=0;
	int 	iChunkIdx = 0;
	int 	iMaxEntryCount = (int)(fDuration/fChunkTimeUnit + 1);

	pStsc->m_entry_count 					= 0;
	//	pStsc->m_first_chunk 					= new long [iMaxEntryCount];
	//	pStsc->m_samples_per_chunk 			= new long [iMaxEntryCount];
	//	pStsc->m_sample_description_index 	= new long [iMaxEntryCount];
	pStsc->m_entry = new tagSTSCBOX[iMaxEntryCount];

	StcoBox * pStco = new StcoBox(0,8,pStbl);
	pStco->m_version = 0;
	memset(pStco->m_flags, 0x0, 3);
	pStco->m_chunk_offset = new long [iMaxEntryCount];

	//printf("fDuration:%f,iMaxEntryCount:%d\n",fDuration,iMaxEntryCount);

	while(iChunkedSample < iFrameCount && iChunkIdx<iMaxEntryCount)
	{
		//printf("iChunkedSample:%d\n",iChunkedSample);
		fChunkedTime = (float)iChunkedSample * (float)0.02;
		//printf("fChunkedTime:%f\n",fChunkedTime);
		pStco->m_chunk_offset[iChunkIdx] = iChunkedSample*iFrameSize;
		iChunk =
			(int)(((fChunkTime+fChunkTimeUnit)-fChunkedTime)/0.02+0.5);
		//printf("Idx:%d,iChunk:%d,offset:%d\n",iChunkIdx,iChunk, pStco->m_chunk_offset[iChunkIdx]);
		if((iChunk+iChunkedSample) > iFrameCount)
			iChunk = iFrameCount - iChunkedSample;
		if(iChunk < 1)
			iChunk = 1;
		if(iChunk!=iLastChunk)
		{
			//			pStsc->m_first_chunk[pStsc->m_entry_count] = iChunkIdx+1;
			//			pStsc->m_samples_per_chunk[pStsc->m_entry_count] = iChunk;
			//			pStsc->m_sample_description_index[pStsc->m_entry_count] = 1;
			pStsc->m_entry[pStsc->m_entry_count].first_chunk = iChunkIdx+1;
			pStsc->m_entry[pStsc->m_entry_count].samples_per_chunk = iChunk;
			pStsc->m_entry[pStsc->m_entry_count].sample_description_index = 1;
			++(pStsc->m_entry_count);
		}

		iChunkIdx++;
		iChunkedSample += iChunk;
		fChunkTime+=fChunkTimeUnit;
		iLastChunk = iChunk;
	}

	pStco->m_entry_count = iChunkIdx;

	StsdBox * pStsd = new StsdBox(0,8,pStbl);
	pStsd->m_version = 0;
	memset(pStsd->m_flags, 0x0, 3);
	pStsd->m_entry_count = 1;

	Mp4aBox * pMp4a = new Mp4aBox(0,8,pStbl);
	memset(pMp4a->m_reserved1,0x0,6);
	pMp4a->m_data_reference_index = 1;
	memset(pMp4a->m_reserved2,0x0,8);
	pMp4a->m_channelcount 	= 2;
	pMp4a->m_samplesize 		= 16;
	pMp4a->m_pre_defined 	= 0;
	pMp4a->m_reserved3 		= 0;
	pMp4a->m_sampleratehi 	= 8000;
	pMp4a->m_sampleratelo 	= 0;
	pStsd->Adopt(pMp4a);

	EsdsBox * pEsds = new EsdsBox(0,8,pMp4a);
	pMp4a->Adopt(pEsds);
	pMp4a->m_pEsdsBox 	= pEsds;
	ES_Descr * pES_Descr = new ES_Descr(0,2);
	pES_Descr->SetBoxContainer(pEsds);
	pEsds->Adopt(pES_Descr);
	pEsds->m_pES_Descr 			 = pES_Descr;
	pES_Descr->m_streamPriority 	= 16;	// KDDI AU 1.1.6 pg. 15.
	pES_Descr->m_ES_ID 				= 0;

	DecoderConfigDescr * pDecoderConfigDescr 
		= new	DecoderConfigDescr(0,2,pES_Descr);

	pDecoderConfigDescr->m_objectTypeIndication 	= (char )0xE1;
	pDecoderConfigDescr->m_streamType 				= 0x05;
	pDecoderConfigDescr->m_upStream 					= 0x0;
	pDecoderConfigDescr->m_reserved 					= 0x1;
	pDecoderConfigDescr->m_bufferSizeDB 			= 850;
	pDecoderConfigDescr->m_maxBitrate 				= qcelp_rate_tab[octet0];
	pDecoderConfigDescr->m_avgBitrate 				= qcelp_rate_tab[octet0];
	pES_Descr->Adopt(pDecoderConfigDescr);
	pES_Descr->m_pDecoderConfigDescr = 	pDecoderConfigDescr;

	DecoderSpecificInfo * pDecoderSpecificInfo 
		= new DecoderSpecificInfo(0,2,pDecoderConfigDescr);
	pDecoderSpecificInfo->m_iSize = iDsiSize; // ??
	pDecoderSpecificInfo->info = pDsi;
	pDecoderConfigDescr->m_pDecoderSpecificInfo = pDecoderSpecificInfo;
	pDecoderConfigDescr->Adopt(pDecoderSpecificInfo);

	SLConfigDescr * pSLConfigDescr = new SLConfigDescr(0,2,pES_Descr);
	pSLConfigDescr->m_predefined = 0x2;
	pES_Descr->Adopt(pSLConfigDescr); 
	pES_Descr->m_pSLConfigDescr = pSLConfigDescr; 


	//printf("** pStbl->m_pStszBox:%p, pStsz:%p\n",pStbl->m_pStszBox, pStsz);

	pStbl->Adopt(pStsd);
	pStbl->Adopt(pStts);
	pStbl->Adopt(pStsc);
	pStbl->Adopt(pStsz);
	pStbl->Adopt(pStco);

	pStbl->m_pStsdBox = pStsd;
	pStbl->m_pSttsBox = pStts;
	pStbl->m_pStscBox = pStsc;
	pStbl->m_pStszBox	= pStsz;
	pStbl->m_pStcoBox = pStco;

	return pStbl;
}

#include "Mp4Audio.h"

static const unsigned int sample_rate_tab[4] = { 22050, 24000, 16000, 0 };


/** pahtname should be in mpeg audio format.
*/
StblBox * Mpeg3gpFile::BuildMp3Stbl(int & iSamplingRate)
{
	int iPos;

	//	printf("** BuildMp3Stbl\n");

	StblBox * pStbl 			= new StblBox(0,8,0);

	StszBox * pStsz 			= new StszBox(0,8+12+4,pStbl);
	pStsz->m_version 			= 0;
	memset(pStsz->m_flags, 0x0, 3);
	pStsz->m_sample_count 	= 0;
	pStsz->m_entry_size 		= 0;
	SetPos(0);
	Mp4AudioFrameHeader Mp3Header(0,0);
	do {
		iPos = Mp3Header.SeekAndLoad(this);
		if(iPos < 0) 
			break;
		pStsz->m_sample_count++;
		if(pStsz->m_sample_count==2)
		{
			pStsz->m_sample_size = iPos;
			iSamplingRate = sample_rate_tab[Mp3Header.sampling_frequency];
		}
	} while(true);


	SttsBox * pStts 			= new SttsBox(0,8+16,pStbl);
	pStts->m_version 			= 0;
	memset(pStts->m_flags, 0x0, 3);
	pStts->m_entry_count 	= 1;
	//	pStts->m_sample_count 	= new long [1];
	//	pStts->m_sample_count[0]= pStsz->m_sample_count;
	//	pStts->m_sample_delta 	= new long [1];
	//	pStts->m_sample_delta[0]= 576;
	//	pStts->m_total_delta		= pStts->m_sample_delta[0]*pStts->m_sample_count[0];
	//	pStts->m_sample_num 		= pStts->m_sample_count[0];
	pStts->m_entry = new tagSTTSBOX[1];
	pStts->m_entry[0].sample_count = pStsz->m_sample_count;
	pStts->m_entry[0].sample_delta = 576;
	pStts->m_total_delta		= pStts->m_entry[0].sample_delta*pStts->m_entry[0].sample_count;
	pStts->m_sample_num 		= pStts->m_entry[0].sample_count;


	StscBox * pStsc 	= new StscBox(0,8,pStbl);
	pStsc->m_version 	= 0;
	memset(pStsc->m_flags, 0x0, 3);

	float fDuration = (float)pStsz->m_sample_count * (float)576/(float)iSamplingRate;
	float fChunkTimeUnit = (float)MP3_CHUNK_TIME;
	float fChunkTime = 0;
	int 	iChunkedSample = 0;
	float fChunkedTime = 0;
	int 	iChunk=0, iLastChunk=0;
	int 	iChunkIdx = 0;
	int 	iMaxEntryCount = (int)(fDuration/fChunkTimeUnit + 1);

	pStsc->m_entry_count 					= 0;
	//	pStsc->m_first_chunk 					= new long [iMaxEntryCount];
	//	pStsc->m_samples_per_chunk 			= new long [iMaxEntryCount];
	//	pStsc->m_sample_description_index 	= new long [iMaxEntryCount];
	pStsc->m_entry = new tagSTSCBOX[iMaxEntryCount];

	StcoBox * pStco = new StcoBox(0,8,pStbl);
	pStco->m_version = 0;
	memset(pStco->m_flags, 0x0, 3);
	pStco->m_chunk_offset = new long [iMaxEntryCount];

	while(iChunkedSample < pStsz->m_sample_count && iChunkIdx<iMaxEntryCount)
	{
		fChunkedTime = (float)iChunkedSample * (float)0.02;
		pStco->m_chunk_offset[iChunkIdx] = iChunkedSample*pStsz->m_sample_size;
		iChunk =	(int)(((fChunkTime+fChunkTimeUnit)-fChunkedTime)/0.02+0.5);
		if((iChunk+iChunkedSample) > pStsz->m_sample_count)
			iChunk = pStsz->m_sample_count - iChunkedSample;
		if(iChunk < 1)
			iChunk = 1;
		if(iChunk!=iLastChunk)
		{
			//			pStsc->m_first_chunk[pStsc->m_entry_count] = iChunkIdx+1;
			//			pStsc->m_samples_per_chunk[pStsc->m_entry_count] = iChunk;
			//			pStsc->m_sample_description_index[pStsc->m_entry_count] = 1;
			pStsc->m_entry[pStsc->m_entry_count].first_chunk = iChunkIdx+1;
			pStsc->m_entry[pStsc->m_entry_count].samples_per_chunk = iChunk;
			pStsc->m_entry[pStsc->m_entry_count].sample_description_index = 1;
			++(pStsc->m_entry_count);
		}

		iChunkIdx++;
		iChunkedSample += iChunk;
		fChunkTime+=fChunkTimeUnit;
		iLastChunk = iChunk;
	}

	pStco->m_entry_count = iChunkIdx;

	StsdBox * pStsd = new StsdBox(0,8,pStbl);
	pStsd->m_version = 0;
	memset(pStsd->m_flags, 0x0, 3);
	pStsd->m_entry_count = 1;

	Mp4aBox * pMp4a = new Mp4aBox(0,8,pStbl);
	memset(pMp4a->m_reserved1,0x0,6);
	pMp4a->m_data_reference_index = 1;
	memset(pMp4a->m_reserved2,0x0,8);
	pMp4a->m_channelcount 	= 2;
	pMp4a->m_samplesize 		= 16;
	pMp4a->m_pre_defined 	= 0;
	pMp4a->m_reserved3 		= 0;
	pMp4a->m_sampleratehi 	= iSamplingRate;
	pMp4a->m_sampleratelo 	= 0;
	pStsd->Adopt(pMp4a);

	EsdsBox * pEsds = new EsdsBox(0,8,pMp4a);
	pMp4a->Adopt(pEsds);
	pMp4a->m_pEsdsBox 	= pEsds;
	ES_Descr * pES_Descr = new ES_Descr(0,2);
	pES_Descr->SetBoxContainer(pEsds);
	pEsds->Adopt(pES_Descr);
	pEsds->m_pES_Descr 			 = pES_Descr;
	pES_Descr->m_streamPriority 	= 16;	// KDDI AU 1.1.6 pg. 15.
	pES_Descr->m_ES_ID 				= 0;

	DecoderConfigDescr * pDecoderConfigDescr 
		= new	DecoderConfigDescr(0,2,pES_Descr);

	pDecoderConfigDescr->m_objectTypeIndication 	= (char )0x69;
	pDecoderConfigDescr->m_streamType 				= 0x05;
	pDecoderConfigDescr->m_upStream 					= 0x0;
	pDecoderConfigDescr->m_reserved 					= 0x1;
	pDecoderConfigDescr->m_bufferSizeDB 			= 8000;
	pDecoderConfigDescr->m_maxBitrate 				= 0xfa00;
	pDecoderConfigDescr->m_avgBitrate 				= 0xfa00;
	pES_Descr->Adopt(pDecoderConfigDescr);
	pES_Descr->m_pDecoderConfigDescr = 	pDecoderConfigDescr;

	pDecoderConfigDescr->m_pDecoderSpecificInfo = 0;

	SLConfigDescr * pSLConfigDescr = new SLConfigDescr(0,2,pES_Descr);
	pSLConfigDescr->m_predefined = 0x2;
	pES_Descr->Adopt(pSLConfigDescr); 
	pES_Descr->m_pSLConfigDescr = pSLConfigDescr; 

	pStbl->Adopt(pStsd);
	pStbl->Adopt(pStts);
	pStbl->Adopt(pStsc);
	pStbl->Adopt(pStsz);
	pStbl->Adopt(pStco);

	pStbl->m_pStsdBox = pStsd;
	pStbl->m_pSttsBox = pStts;
	pStbl->m_pStscBox = pStsc;
	pStbl->m_pStszBox	= pStsz;
	pStbl->m_pStcoBox = pStco;

	return pStbl;
}

void Mpeg3gpFile::SetFileFormat(long FileFormat)
{
	m_lFileFormat = FileFormat;
}

void Mpeg3gpFile::SetKddiAmcRecMode(char cRecMode)
{
	m_cKddiAmcRecMode 			= cRecMode;
	if(m_pUuidMvml)
		m_pUuidMvml->m_rec_mode 	= m_cKddiAmcRecMode;
}

void Mpeg3gpFile::SetDrm(bool bForward, bool bRinger)
{
	m_bForward = bForward;
	m_bRinger  = bRinger;
}


void RiffFile::DumpHeader(Formatter* pFormatter)
{
	pFormatter->Print("<riff>\n");
}

void RiffFile::DumpFooter(Formatter* pFormatter)
{
	pFormatter->Print("</riff>\n");
}

/** write DATA chunk into file */
int RiffFile::WriteData(char *pathname)
{
	if(m_pRiffRiff==0)
		RETURN_ERROR(-1);	
	if(m_pRiffRiff->m_pRiffData==0)
		RETURN_ERROR(-1);	

	m_pRiffRiff->m_pRiffData->Write(this, pathname);

	return 0;
}

int RiffFile::Load()
{
	int iChunkId, iChunkSize;

	// read from the beginning ...
	SetPos(0);

	if(Mp4Riff::GetChunkIdNSize(this,&iChunkId,&iChunkSize)<0)
		RETURN_ERROR(-1);

	if(iChunkId==RIFF_RIFF)
	{
		if(m_pRiffRiff)
			delete m_pRiffRiff;
		m_pRiffRiff = new RiffRiff(0,iChunkSize+8);
		if(m_pRiffRiff)
		{
			if(m_pRiffRiff->Load(this)<0)
				RETURN_ERROR(-1);
			Adopt(m_pRiffRiff);
		}
		return 0;
	}

	RETURN_ERROR(-1);
}

int RiffFile::MakeWave(char *pcmfile, char *wavefile,int iSamplingRate,int iChannels)
{
	Mp4Serializer Dumper;

	RiffRiff * pRiffRiff 	= new RiffRiff(0,8+4);
	pRiffRiff->Format 		= RIFF_WAVE;
	pRiffRiff->SetLoaded(true);

	RiffFmt * pRiffFmt 		= new RiffFmt(0,8+16);
	pRiffFmt->AudioFormat 	= 1;
	pRiffFmt->NumChannels 	= iChannels;
	pRiffFmt->SampleRate  	= iSamplingRate;
	pRiffFmt->ByteRate 		= iSamplingRate*iChannels*2;
	pRiffFmt->BlockAlign 	= iChannels * 2;
	pRiffFmt->BitsPerSample = 16;
	pRiffFmt->SetLoaded(true);
	pRiffFmt->SetSize(24);
	pRiffRiff->m_pRiffFmt   = pRiffFmt;

	MovieFile	PcmAudioFile(pcmfile);
	RiffData * pRiffData 	= new RiffData(0,8);
	pRiffData->SetPcmFile(pcmfile);
	pRiffData->SetLoaded(true);
	pRiffData->SetSize(PcmAudioFile.GetFileSize()+8);

	pRiffRiff->SetSize(pRiffData->GetSize()+pRiffFmt->GetSize()+pRiffRiff->GetSize());
	pRiffRiff->m_pRiffData  = pRiffData;
	pRiffRiff->Adopt(pRiffFmt);
	pRiffRiff->Adopt(pRiffData);

	Adopt(pRiffRiff);

	return Write(wavefile);
}

int RiffFile::Write(char *pathname)
{
	Mp4Serializer Dumper;
	Dumper.SetOutput(pathname);
	Dumper.PrepareForNavigation();
	Navigate(&Dumper);

	return 0;
}

#endif
