#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "Mp4Com.h"
#include "Mp4File.h"
#include "Mp4Ve.h"
#include "Mp4Box.h"
#include "Mp4Navigator.h"
#include "Mp4Riff.h"

#define SAFEDELETE(x)         if (x != NULL) {delete x; x = NULL;}

int Mp4Box::GetIntB(char * cBuf)
{
	unsigned char *us = reinterpret_cast<unsigned char *>(cBuf);
	return us[3]<<24|us[2]<<16|us[1]<<8|us[0];
}

int Mp4Box::GetIntL(char * cBuf)
{
	unsigned char *us = reinterpret_cast<unsigned char *>(cBuf);
	return us[0]<<24|us[1]<<16|us[2]<<8|us[3];
}

short Mp4Box::GetShortL(char * cBuf)
{
	unsigned char *us = reinterpret_cast<unsigned char *>(cBuf);
	return us[0]<<8|us[1];
}

short Mp4Box::GetShortB(char * cBuf)
{
	unsigned char *us = reinterpret_cast<unsigned char *>(cBuf);
	return us[1]<<8|us[0];
}

// Expandable classes's size.
long Mp4Box::GetSizeOfInstance(Mp4File * pMp4File, int * count)
{
	bool	bNextByte = true;
	char	cByte=0;
	long	lSizeOfInstance=0;
	if(count)
		*count = 0;
	while(bNextByte)
	{
		cByte = pMp4File->GetChar();
		if(count)
			(*count)++;
		bNextByte = ((cByte>>7)&0x1)==0x1;
		lSizeOfInstance = lSizeOfInstance<<7 | (cByte&0x7f);
	}

	return lSizeOfInstance;
}

int FtypBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_major_brand = pMp4File->Get32();
	m_minor_version = pMp4File->Get32();
	m_n_compatible_brands = (m_iSize-16)/4;
	if(m_compatible_brands)
		delete [] m_compatible_brands;
	if(m_n_compatible_brands > 0)
	{
		m_compatible_brands = new long[m_n_compatible_brands];
		if (m_compatible_brands == NULL)
			RETURN_ERROR(-2);
		for(int i=0;i<m_n_compatible_brands;i++)
			m_compatible_brands[i] = pMp4File->Get32();
	}
	return 0;
}

int FtypBox::MakeKddi()
{
	int i;

	// KDDI AU 1.1.0 pg. 15
	static const int kddi_compatible_brands [6] = 
	{
		'k'<<24|'d'<<16|'d'<<8|'i',
		'3'<<24|'g'<<16|'2'<<8|'a',
		'3'<<24|'g'<<16|'p'<<8|'5',
		'3'<<24|'g'<<16|'p'<<8|'4',
		'm'<<24|'p'<<16|'4'<<8|'1',
		'm'<<24|'p'<<16|'4'<<8|'2',
	};
	m_major_brand 				= 'k'<<24|'d'<<16|'d'<<8|'i';
	m_minor_version 			= 0;
	m_n_compatible_brands 	= 6;
	if(m_compatible_brands)
		delete [] m_compatible_brands;
	m_compatible_brands = new long[m_n_compatible_brands];
	if (m_compatible_brands == NULL)
	{
		RETURN_ERROR(-2);
	}
	for(i=0;i<m_n_compatible_brands;i++)
		m_compatible_brands[i] = kddi_compatible_brands[i];
	return 0;
}

int FtypBox::MakeKddiAmc()
{
	int i;

	// KDDI AU 1.1.6 pg. 26
	static const int kddi_compatible_brands [6] = 
	{
		'm'<<24|'p'<<16|'4'<<8|'1',
	};
	m_major_brand 				= 'i'<<24|'s'<<16|'o'<<8|'m';
	m_minor_version 			= 0;
	m_n_compatible_brands 	= 1;
	if(m_compatible_brands)
		delete [] m_compatible_brands;
	m_compatible_brands = new long[m_n_compatible_brands];
	if (m_compatible_brands == NULL)
	{
		RETURN_ERROR(-2);
	}
	for(i=0;i<m_n_compatible_brands;i++)
		m_compatible_brands[i] = kddi_compatible_brands[i];
	return 0;
}

void FtypBox::Dump(Formatter* pFormatter)
{
	if(pFormatter->UseDisplay())
	{
		pFormatter->Print("<major_brand>\n");
		pFormatter->PrintInt((char *)&m_major_brand,4);
		pFormatter->Print("</major_brand>\n");
	}
	else
		pFormatter->Put("<major_brand val=\"0x%x\"/>\n", m_major_brand);
	pFormatter->Put("<minor_version val=\"0x%x\"/>\n",m_minor_version);
	if(m_compatible_brands)
	{
		int i;
		for(i=0;i<m_n_compatible_brands;i++)
		{
			pFormatter->Print("<compatible_brands>");
			if(pFormatter->UseDisplay())
				pFormatter->PrintInt((char *)(&m_compatible_brands[i]),4);
			else
				pFormatter->Put(0,m_compatible_brands[i]);
			pFormatter->Print("</compatible_brands>\n");
		}
	}
}


int MoovBox::LoadMoovUdta(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size=0;
	UdtaBox *pBox = 0;
	pMp4File->SetPos(offset);

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size<1)
			return 0;

        if(iBoxType == BOX_UDTA)
        {
            pBox = m_pUdtaBox = new UdtaBox(offset, size, this);
            if (pBox == NULL)
                RETURN_ERROR(-2);
        }
        if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->LoadUdtaMeta(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
            if(iBoxType == BOX_UDTA)
                break;
		}
		offset += size;	
	}

	return 0;
}


int MoovBox::LoadMvhd(Mp4File * pMp4File)
{
    int iBoxType = 0;
    int offset = m_iBeginOffset+8;
    int size=0;
    MvhdBox *pBox = 0;
    pMp4File->SetPos(offset);

    while(offset+3 < m_iBeginOffset+m_iSize)
    {
        pMp4File->SetPos(offset);
        iBoxType = pMp4File->GetBoxHead(&size);
        if(size<1)
            return 0;

        if(iBoxType == BOX_MVHD)
        {
            pBox = m_pMvhdBox = new MvhdBox(offset, size, this);
            if (pBox == NULL)
                RETURN_ERROR(-2);
        }
        if(pBox)
        {
            Adopt(pBox);
            int ret = pBox->Load(pMp4File);
            if (ret < 0)
                RETURN_ERROR(ret);
            if(iBoxType == BOX_MVHD)
                break;
        }
        offset += size; 
    }

    return 0;
}



int MoovBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size=0;
	Mp4Box * pBox=0;
	pMp4File->SetPos(offset);

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size<1)
			return 0;
		switch(iBoxType)
		{
		case BOX_MVHD:
			{
				if(m_pMvhdBox)
					delete m_pMvhdBox;
				pBox = m_pMvhdBox = new MvhdBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_IODS: // InitialObjectDescriptor (MP4 extension)
			{
				//
				// defined in Part 14 but should not be used for KDDI.
				// -- format V1.1.0 Pg. 12.
				//
				pBox = new IodsBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_TRAK:
			{
				if(m_iNextTrak < MAX_TRACK_NUM)
				{
					pBox = m_pTrakBox[m_iNextTrak] = new TrakBox(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
					m_iNextTrak++;
				}
				else
					RETURN_ERROR(ISO_SYNTAX_ERROR);
			}	
			break;
		case BOX_UDTA:
			{
				pBox = m_pUdtaBox = new UdtaBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}	
			break;
		case BOX_MVEX:
			{
				if(m_pMvexBox)
					RETURN_ERROR(ISO_SYNTAX_ERROR);
				pBox = m_pMvexBox = new MvexBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}	
			break;
		case BOX_DRM:
			{
				if(m_pDrmBox)
					RETURN_ERROR(ISO_SYNTAX_ERROR);
				pBox = m_pDrmBox = new DrmBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}	
			break;
		case BOX_IVLD:
			{
				pBox = 0;
				RETURN_ERROR(-1);
			}
		default:
			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
			if (pBox == NULL)
				RETURN_ERROR(-2);
			break;
		}
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
		}
		offset += size;	
	}

	return 0;
}

typedef struct
{
	uint8_t*  pu8DataBegin;
	uint32_t  u32DataLength;
	uint8_t*  pu8CurrReadPtr;
	int32_t   s32BytesLeft;
}BitStream;

int32_t BitStreamAttach(BitStream* pBitStream, int8_t* ps8DataBuffer, uint32_t u32DataBufLength)
{
	if ((pBitStream == 0) || (ps8DataBuffer == 0) || (u32DataBufLength == 0))
		return -1;

	pBitStream->pu8DataBegin = (uint8_t*)ps8DataBuffer;
	pBitStream->u32DataLength = u32DataBufLength;
	pBitStream->pu8CurrReadPtr = (uint8_t*)ps8DataBuffer;
	pBitStream->s32BytesLeft = u32DataBufLength;

	return 0;
}

uint32_t GetBuffer(BitStream* pBitStream, uint8_t *buf, uint32_t u32Bytes)
{
	if (pBitStream->s32BytesLeft < (int32_t)u32Bytes)
		return 0;

	memcpy(buf, pBitStream->pu8CurrReadPtr, u32Bytes);
	pBitStream->pu8CurrReadPtr += u32Bytes;
	pBitStream->s32BytesLeft -= u32Bytes;

	return u32Bytes;
}

int MoovBox::GetCanData(char *buf, unsigned int *frameSize)
{
	static int first = 1;
	static BitStream stBs;
	static int nCanFrames = 0, frame_no = 0;
	int can_frame_size;
	unsigned int sync_word;

    if (m_pUdtaBox->m_pMetaBox->m_pIlstBox == NULL || m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pCanDataBox == NULL)
	{
		printf("ERROR: No Can Data!\n");
		return -1;
	}

	if (first == 1)
	{
		int8_t *pCanBuf;
		uint32_t size;

        pCanBuf = (int8_t *)m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pCanDataBox->m_content;
        size = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pCanDataBox->m_size;

		BitStreamAttach(&stBs, pCanBuf, size);
		GetBuffer(&stBs, (uint8_t *)&nCanFrames, 4);

		first = 0;
	}

	if (frame_no > nCanFrames)
	{
		printf("reach end of can buffer, total can frames:%d\n", nCanFrames);
		return -1;
	}

	GetBuffer(&stBs, (uint8_t *)&sync_word, 4);
	if (sync_word != 0xdeadbeef)
	{
		printf("can buffer lost sync\n");
		return -1;
	}

	GetBuffer(&stBs, (uint8_t *)&can_frame_size, 4);

	if (frameSize != NULL)
	{
		if (*frameSize < can_frame_size)
		{
			printf("can buffer not large enough, can_frame_size=%d", can_frame_size);
			return -1;
		}
		if (buf != NULL)
			GetBuffer(&stBs, (uint8_t *)buf, can_frame_size);

		*frameSize = can_frame_size;
	}

	frame_no += 1;

	return 0;
}

int MoovBox::GetUdtaData(MP4Demux_UserData* pUserData)
{
	memset(pUserData, 0, sizeof(MP4Demux_UserData));
	if (m_pUdtaBox )
	{
		if (m_pUdtaBox->m_pMetaBox && m_pUdtaBox->m_pMetaBox->m_pIlstBox)
		{
			if (m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pCpyBox)
			{
				pUserData->copyright.szContent = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pCpyBox->m_content;
				pUserData->copyright.nCodes    = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pCpyBox->m_codes;
				pUserData->copyright.nSize     = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pCpyBox->m_size;
			}

			if (m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pNamBox)
			{
				pUserData->title.szContent = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pNamBox->m_content;
				pUserData->title.nCodes    = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pNamBox->m_codes;
				pUserData->title.nSize     = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pNamBox->m_size;
			}

			if (m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pPrfBox)
			{
				pUserData->artist.szContent = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pPrfBox->m_content;
				pUserData->artist.nCodes    = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pPrfBox->m_codes;
				pUserData->artist.nSize     = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pPrfBox->m_size;
			}

			if (m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pPrdBox)
			{
				pUserData->author.szContent = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pPrdBox->m_content;
				pUserData->author.nCodes    = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pPrdBox->m_codes;
				pUserData->author.nSize     = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pPrdBox->m_size;
			}

			if (m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pInfBox)
			{
				pUserData->description.szContent = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pInfBox->m_content;
				pUserData->description.nCodes    = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pInfBox->m_codes;
				pUserData->description.nSize     = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pInfBox->m_size;
			}
			
			if (m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pAlbBox)
			{
				pUserData->album.szContent = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pAlbBox->m_content;
				pUserData->album.nCodes    = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pAlbBox->m_codes;
				pUserData->album.nSize     = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pAlbBox->m_size;
			}

            if (m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pCovrBox)
            {
                pUserData->artwork.szContent = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pCovrBox->m_content;
                pUserData->artwork.nCodes = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pCovrBox->m_codes;
                pUserData->artwork.nSize = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pCovrBox->m_size;
            }

            if (m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pDescBox)
            {
                pUserData->description.szContent = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pDescBox->m_content;
                pUserData->description.nCodes = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pDescBox->m_codes;
                pUserData->description.nSize = m_pUdtaBox->m_pMetaBox->m_pIlstBox->m_pDescBox->m_size;
            }

			return 0;
	

		}
		else {
			if (m_pUdtaBox->m_pCpyBox){
				pUserData->copyright.szContent = m_pUdtaBox->m_pCpyBox->m_content;
				pUserData->copyright.nCodes    = m_pUdtaBox->m_pCpyBox->m_codes;
				pUserData->copyright.nSize    = m_pUdtaBox->m_pCpyBox->m_size;
			}
			else if (m_pUdtaBox->m_pCprtBox){
				pUserData->copyright.szContent   = m_pUdtaBox->m_pCprtBox->m_notice;
				pUserData->copyright.nCodes      = m_pUdtaBox->m_pCprtBox->m_codes;
				pUserData->copyright.nSize    = m_pUdtaBox->m_pCprtBox->m_size;
			}

			if (m_pUdtaBox->m_pPrfBox){
				pUserData->artist.szContent = m_pUdtaBox->m_pPrfBox->m_content;
				pUserData->artist.nCodes      = m_pUdtaBox->m_pPrfBox->m_codes;
				pUserData->artist.nSize    = m_pUdtaBox->m_pPrfBox->m_size;
			}
			else if (m_pUdtaBox->m_pPerfBox){
				pUserData->artist.szContent = m_pUdtaBox->m_pPerfBox->m_notice;
				pUserData->artist.nCodes      = m_pUdtaBox->m_pPerfBox->m_codes;
				pUserData->artist.nSize    = m_pUdtaBox->m_pPerfBox->m_size;
			}

			if (m_pUdtaBox->m_pPrdBox){
				pUserData->author.szContent = m_pUdtaBox->m_pPrdBox->m_content;
				pUserData->author.nCodes      = m_pUdtaBox->m_pPrdBox->m_codes;
				pUserData->author.nSize    = m_pUdtaBox->m_pPrdBox->m_size;
			}
			else if (m_pUdtaBox->m_pAuthBox){
				pUserData->author.szContent = m_pUdtaBox->m_pAuthBox->m_notice;
				pUserData->author.nCodes      = m_pUdtaBox->m_pAuthBox->m_codes;
				pUserData->author.nSize    = m_pUdtaBox->m_pAuthBox->m_size;
			}

			if (m_pUdtaBox->m_pNamBox){
				pUserData->title.szContent = m_pUdtaBox->m_pNamBox->m_content;
				pUserData->title.nCodes      = m_pUdtaBox->m_pNamBox->m_codes;
				pUserData->title.nSize    = m_pUdtaBox->m_pNamBox->m_size;
			}
			else if (m_pUdtaBox->m_pTitlBox){
				pUserData->title.szContent = m_pUdtaBox->m_pTitlBox->m_notice;
				pUserData->title.nCodes      = m_pUdtaBox->m_pTitlBox->m_codes;
				pUserData->title.nSize    = m_pUdtaBox->m_pTitlBox->m_size;
			}

			if (m_pUdtaBox->m_pInfBox){
				pUserData->description.szContent = m_pUdtaBox->m_pInfBox->m_content;
				pUserData->description.nCodes      = m_pUdtaBox->m_pInfBox->m_codes;
				pUserData->description.nSize    = m_pUdtaBox->m_pInfBox->m_size;
			}
			else if (m_pUdtaBox->m_pDscpBox){
				pUserData->description.szContent = m_pUdtaBox->m_pDscpBox->m_notice;
				pUserData->description.nCodes      = m_pUdtaBox->m_pDscpBox->m_codes;
				pUserData->description.nSize    = m_pUdtaBox->m_pDscpBox->m_size;
			}

			return 0;
		}
	}

	return -1;
}


int MvhdBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
#ifndef TI_ARM		
	m_creation_time = m_version==1?(unsigned long)pMp4File->Get64():pMp4File->Get32();
	m_modification_time = m_version==1?(unsigned long)pMp4File->Get64():pMp4File->Get32();
#else
	m_creation_time = m_version==1?(unsigned long)pMp4File->Get64().low:pMp4File->Get32();
	m_modification_time = m_version==1?(unsigned long)pMp4File->Get64().low:pMp4File->Get32();
#endif	
	m_timescale = pMp4File->Get32();
	m_duration 	= pMp4File->Get32();
	m_rate 		= pMp4File->Get32();
	m_volume 	= pMp4File->Get16();
	m_reserved1 = pMp4File->Get16();
	for(i=0;i<2;i++)
		m_reserved2[i] = pMp4File->Get32();
	for(i=0;i<9;i++)
		m_matrix[i] = pMp4File->Get32();
	for(i=0;i<6;i++)
		m_pre_defined[i] = pMp4File->Get32();
	m_next_track_ID = pMp4File->Get32();

	return 0;
}

void MvhdBox::Dump(Formatter* pFormatter)
{
	int i;
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Print("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Print("</flags>\n");
	pFormatter->Put("<creation_time val=\"0x%x\" />\n", (long)m_creation_time);
	pFormatter->Put("<modification_time val=\"0x%x\" />\n",(long)m_modification_time);
	pFormatter->Put("<timescale val=\"%d\" />\n",m_timescale);
	pFormatter->Put("<duration val=\"%d\" />\n",m_duration);
	// 14496-12_F6 Page 23
	pFormatter->Put("<rate val=\"0x%x\" />\n",m_rate);
	pFormatter->Put("<volume val=\"0x%x\" />\n",m_volume);
	pFormatter->Put("<reserved1 val=\"%d\" />\n",m_reserved1);
	pFormatter->Print("<reserved2>\n");
	pFormatter->Put(0,m_reserved2[0]);
	pFormatter->Put(0,m_reserved2[1]);
	pFormatter->Print("</reserved2>\n");
	pFormatter->Put("<matrix>");
	for(i=0;i<9;i++)
	{
		pFormatter->Put("0x%x ",m_matrix[i]);
	}
	pFormatter->Put("</matrix>\n");
	pFormatter->Put("<pre_defined>");
	for(i=0;i<6;i++)
		pFormatter->Put("%d ",m_pre_defined[i]);
	pFormatter->Put("</pre_defined>\n");
	pFormatter->Put("<next_track_ID val=\"%d\" />\n",m_next_track_ID);
}

int TrakBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box * pBox=0;
	pMp4File->SetPos(offset);
	
	SetSourceFile(pMp4File);

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size<1)
			return 0;
		switch(iBoxType)
		{
		case BOX_TKHD:
			{
				if(m_pTkhdBox)
					RETURN_ERROR(ISO_SYNTAX_ERROR);
				pBox = m_pTkhdBox = new TkhdBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_TREF: 
			{
				pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_EDTS:
			{
				pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}	
			break;
		case BOX_MDIA:
			{
				if(m_pMdiaBox)
					RETURN_ERROR(ISO_SYNTAX_ERROR);
				pBox = m_pMdiaBox = new MdiaBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}	
			break;
		case BOX_UDTA:
			{
				if(m_pUdtaBox)
					RETURN_ERROR(ISO_SYNTAX_ERROR);
				pBox = m_pUdtaBox = new UdtaBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}	
			break;
		case BOX_IVLD:
			{
				pBox = 0;
				RETURN_ERROR(-1);
			}
		default:
			//pFormatter->Put("TRAK: Unknown Type\n");
			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
			if (pBox == NULL)
				RETURN_ERROR(-2);
			break;
		}
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
		}
		offset += size;	
	}

	return 0;
}

int TkhdBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
#ifndef TI_ARM		
	m_creation_time = m_version==1?(unsigned long)pMp4File->Get64():pMp4File->Get32();
	m_modification_time = m_version==1?(unsigned long)pMp4File->Get64():pMp4File->Get32();
#else
	m_creation_time = m_version==1?(unsigned long)pMp4File->Get64().low:pMp4File->Get32();
	m_modification_time = m_version==1?(unsigned long)pMp4File->Get64().low:pMp4File->Get32(); 
#endif	
	m_track_ID 	= pMp4File->Get32();
	m_reserved1 = pMp4File->Get32();
#ifndef TI_ARM	
	m_duration = m_version==1?(unsigned long)pMp4File->Get64():pMp4File->Get32();
#else
	m_duration = m_version==1?(unsigned long)pMp4File->Get64().low:pMp4File->Get32();
#endif	
	for(i=0;i<2;i++)
		m_reserved2[i] = pMp4File->Get32();
	m_pre_defined 	= pMp4File->Get32();
	m_volume 		= pMp4File->Get16();
	m_reserved3 	= pMp4File->Get16();
	for(i=0;i<9;i++)
		m_matrix[i] = pMp4File->Get32();
	m_width 	= pMp4File->Get32();
	m_height = pMp4File->Get32();

	return 0;
}

void TkhdBox::Dump(Formatter* pFormatter)
{
	int i;
	pFormatter->Put("<version val=\"%d\"/>\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<creation_time val=\"0x%x\"/>\n",(long)m_creation_time);
	pFormatter->Put("<modification_time val=\"0x%x\"/>\n",(long)m_modification_time);
	if(!pFormatter->UseDisplay())
	{
		m_track_ID = pFormatter->GetNextTrackId();
	}
	pFormatter->Put("<track_ID val=\"%d\"/>\n",m_track_ID);
	pFormatter->Put("<reserved1 val=\"%d\"/>\n",m_reserved1);
	pFormatter->Put("<duration val=\"%d\"/>\n",(long)m_duration);
	pFormatter->Put("<reserved2>\n");
	pFormatter->Put(0,m_reserved2[0]);
	pFormatter->Put(0,m_reserved2[1]);
	pFormatter->Put("</reserved2>\n");
	pFormatter->Put("<pre_defined val=\"%d\"/>\n",m_pre_defined);
	pFormatter->Put("<volume val=\"%d\"/>\n",m_volume);
	pFormatter->Put("<reserved3 val=\"%d\"/>\n",m_reserved3);
	pFormatter->Put("<matrix>");
	for(i=0;i<9;i++)
	{
		pFormatter->Put("0x%x ",m_matrix[i]);
	}
	pFormatter->Put("</matrix>\n");
	pFormatter->Put("<width val=\"0x%08x\"/>\n",m_width);
	pFormatter->Put("<height val=\"0x%08x\"/>\n",m_height);
}

int MdiaBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box* pBox = 0;
	pMp4File->SetPos(offset);

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size<1)
			return 0;
		//Mp4Box::DumpChar(iBoxType);
		//pFormatter->Put("MDIA:offset:%d, size:%d\n",offset,size);
		switch(iBoxType)
		{
		case BOX_MDHD:
			{
				if(m_pMdhdBox)
					delete m_pMdhdBox;
				pBox = m_pMdhdBox = new MdhdBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_HDLR: 
			{
				if(m_pHdlrBox)
					delete m_pHdlrBox;
				pBox = m_pHdlrBox = new HdlrBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_MINF:
			{
				if(m_pMinfBox)
					delete m_pMinfBox;
				pBox = m_pMinfBox = new MinfBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}	
			break;
		case BOX_IVLD:
			{
				pBox = 0;
				RETURN_ERROR(-1);
			}
		default:
			//pFormatter->Put("MDIA: Unknown Type\n");
			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
			if (pBox == NULL)
				RETURN_ERROR(-2);
			break;
		}
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
		}
		offset += size;	
	}

	return 0;
}

// check delta and duration
void MdiaBox::Dump(Formatter* pFormatter)
{
	Mp4BoxFinder BoxFinder(BOX_MDIA);
	MdhdBox* pMdhdBox = (MdhdBox *)BoxFinder.GetBox(BOX_MDHD,this);
	SttsBox* pSttsBox = (SttsBox *)BoxFinder.GetBox(BOX_STTS,this);

	if(pMdhdBox && pSttsBox)
	{
		if(pMdhdBox->m_duration != pSttsBox->m_total_delta)
		{
			pFormatter->Print("<error>Mdhd's duration %d!= Stts's Total Delta %d</error>",
				pMdhdBox->m_duration,pSttsBox->m_total_delta);
		}
	}
}

int MdhdBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
#ifndef TI_ARM		
	m_creation_time = m_version==1?(unsigned long)pMp4File->Get64():pMp4File->Get32();
	m_modification_time = m_version==1?(unsigned long)pMp4File->Get64():pMp4File->Get32();
	m_timescale = m_version==1?(unsigned long)pMp4File->Get64():pMp4File->Get32();
	m_duration = m_version==1?(unsigned long)pMp4File->Get64():pMp4File->Get32();
#else
	m_creation_time = m_version==1?(unsigned long)pMp4File->Get64().low:pMp4File->Get32();
	m_modification_time = m_version==1?(unsigned long)pMp4File->Get64().low:pMp4File->Get32();
	m_timescale = m_version==1?(unsigned long)pMp4File->Get64().low:pMp4File->Get32();
	m_duration = m_version==1?(unsigned long)pMp4File->Get64().low:pMp4File->Get32();
#endif	
	if(pMp4File->GetBuffer(m_language, 2)!=2)
		RETURN_ERROR(-1);
	m_pad = m_language[0]>>7;
	m_language[0] &= 0x7f; // take off pad bit.
	m_pre_defined = pMp4File->Get16();

	return 0;
}

void MdhdBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<creation_time val=\"0x%x\"/>\n",(long)m_creation_time);
	pFormatter->Put("<modification_time val=\"0x%x\"/>\n", (long)m_modification_time);
	pFormatter->Put("<timescale val=\"%d\"/>\n", (long)m_timescale);
	pFormatter->Put("<duration val=\"%d\"/>\n", (long)m_duration);
	pFormatter->Print("<pad val=\"%d\"/>\n",m_pad);
	if(!pFormatter->UseDisplay() && 
		(pFormatter->m_FileFormat==KDDI_3GP_FILE_FORMAT ||
		pFormatter->m_FileFormat==KDDI_AMC_FILE_FORMAT)
		)
	{
		memset(m_language, 0x0, 2); //KDDI-AU 1.1.6 pg. 30.
	}
	pFormatter->Put("<language>\n");
	pFormatter->Put(0,m_pad?(char)(m_language[0]|0x80):m_language[0]);
	pFormatter->Put(0,m_language[1]);
	pFormatter->Put("</language>\n");
	pFormatter->Put("<pre_defined val=\"%d\"/>\n",m_pre_defined);
}

int HdlrBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_pre_defined 	= pMp4File->Get32();
	m_handler_type = pMp4File->Get32();
	if(pMp4File->GetBuffer(m_reserved, 12)!=12)
		RETURN_ERROR(-1);

	m_name[0] = 0;
	if(m_iSize > 32)
	{
		char ch=0;
		int  iStringLen = 0;
		while((ch=pMp4File->GetChar())!=0 && iStringLen < 2047)	
		{
			m_name[iStringLen] = ch;	
			iStringLen++;
		}
		m_name[iStringLen] = 0;	
	}

	return 0;
}

void HdlrBox::Dump(Formatter* pFormatter)
{
	int i;
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<pre_defined val=\"0x%x\"/>\n",m_pre_defined);
	pFormatter->Put("<handler_type>");
	if(pFormatter->UseDisplay())
		pFormatter->PrintInt((char *)&m_handler_type,4);
	else	
		pFormatter->Put(0,m_handler_type);
	pFormatter->Put("</handler_type>\n");
	pFormatter->Put("<reserved>");
	for(i=0;i<12;i++)
		pFormatter->Put("%d,",m_reserved[i]);
	pFormatter->Put("</reserved>\n");
	if(!pFormatter->UseDisplay() 
		&& (pFormatter->m_FileFormat==KDDI_3GP_FILE_FORMAT ||
		pFormatter->m_FileFormat==KDDI_AMC_FILE_FORMAT ||
		pFormatter->m_FileFormat==DOCOMO_3GP_FILE_FORMAT ||
		pFormatter->m_FileFormat==MP4_FILE_FORMAT )
		)
		m_name[0]=0; //KDDI-AU 1.1.6 pg. 31.
	pFormatter->Put("<name>\n");
	for(i=0;i<2048;i++)
	{
		if(m_name[i]==0)
			break;
		pFormatter->Put(0,m_name[i]);
	}
	pFormatter->Put("</name>\n");
}

int MinfBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box* pBox = 0;

	pMp4File->SetPos(offset);

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size<1)
			return 0;
		//Mp4Box::DumpChar(iBoxType);
		//pFormatter->Put("MINF:offset:%d, size:%d\n",offset,size);
		switch(iBoxType)
		{
		case BOX_VMHD:
			{
				if(m_pVmhdBox)
					delete m_pVmhdBox;
				pBox = m_pVmhdBox = new VmhdBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_SMHD: 
			{
				if(m_pSmhdBox)
					delete m_pSmhdBox;
				pBox = m_pSmhdBox = new SmhdBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_NMHD:
			{
				if(m_pNmhdBox)
					delete m_pNmhdBox;
				pBox = m_pNmhdBox = new NmhdBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}	
			break;
		case BOX_DINF:
			{
				if(m_pDinfBox)
					delete m_pDinfBox;
				pBox = m_pDinfBox = new DinfBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}	
			break;
		case BOX_STBL:
			{
				if(m_pStblBox)
					delete m_pStblBox;
				pBox = m_pStblBox = new StblBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}	
			break;
		case BOX_IVLD:
			{
				pBox = 0;
				RETURN_ERROR(-1);
			}
			break;
		case BOX_HDLR:
			{
				if (m_pHdlrBox)
					delete m_pHdlrBox;
				pBox = m_pHdlrBox = new HdlrBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		default:
			//pFormatter->Put("MINF: Unknown Type\n");
			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
			if (pBox == NULL)
				RETURN_ERROR(-2);
			break;
		}
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
		}
		offset += size;	
	}
	return 0;
}


int VmhdBox::Load(Mp4File * pMp4File)
{
	char buffer[32];
	int i=0;
	int nTimeBytes = 4;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	nTimeBytes = m_version==1 ? 8 : 4;
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	if(pMp4File->GetBuffer(buffer, 2)!=2)
		RETURN_ERROR(-1);
	m_graphicsmode = GetShortL(buffer);
	if(pMp4File->GetBuffer(buffer, 6)!=6)
		RETURN_ERROR(-1);
	for(i=0;i<3;i++)
		m_opcolor[i] = GetShortL(buffer+i*2);
	return 0;
}

void VmhdBox::Dump(Formatter* pFormatter)
{
	int i;
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<graphicsmode val=\"%d\"/>\n",m_graphicsmode);
	pFormatter->Put("<opcolor>");
	for(i=0;i<3;i++)
		pFormatter->Put("%d,",m_opcolor[i]);
	pFormatter->Put("</opcolor>\n");
}

int SmhdBox::Load(Mp4File * pMp4File)
{
	char buffer[32];
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	if(pMp4File->GetBuffer(buffer, 2)!=2)
		RETURN_ERROR(-1);
	m_balance = GetShortL(buffer);
	if(pMp4File->GetBuffer(buffer, 2)!=2)
		RETURN_ERROR(-1);
	m_reserved = GetShortL(buffer);
	return 0;
}

void SmhdBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\"/>\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<balance val=\"%d\"/>\n",m_balance);
	pFormatter->Put("<reserved val=\"%d\"/>\n",m_reserved);
}

int NmhdBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	return 0;
}

void NmhdBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
}

int DinfBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box * pBox = 0;

	pMp4File->SetPos(offset);

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size<1)
			return 0;
		//Mp4Box::DumpChar(iBoxType);
		//pFormatter->Put("DINF:offset:%d, size:%d\n",offset,size);
		switch(iBoxType)
		{	
		case BOX_DREF:
			{
				if(m_pDrefBox)
					delete m_pDrefBox;
				pBox = m_pDrefBox = new DrefBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_IVLD:
			{
				pBox = 0;
				RETURN_ERROR(-1);
			}
		default:
			//pFormatter->Put("DINF: Unknown Type\n");

			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
			if (pBox == NULL)
				RETURN_ERROR(-2);
			break;
		}
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
		}
		offset += size;	
	}
	return 0;
}

int DrefBox::Load(Mp4File * pMp4File)
{
	Mp4Box* pBox=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_entry_count = pMp4File->Get32();
	if( m_entry_count > 0)
	{
		m_data_entry = new UrlBox * [m_entry_count];
		if (m_data_entry == NULL)
			RETURN_ERROR(-2);
		int i,offset,size=0,iBoxType;
		offset = m_iBeginOffset + 16;
		int iUrl;
		for(i=0,iUrl=0;i<m_entry_count;i++)
		{
			iBoxType = pMp4File->GetBoxHead(&size);
			if(size<1)
				return 0;
			pMp4File->SetPos(offset);
			if(iBoxType!=BOX_URL)
			{
				//Mp4Box::DumpChar(iBoxType);
				//pFormatter->Put("\nDREF:offset:%d, size:%d\n",offset,size);
				//pFormatter->Put("!! SYNTAX ERROR BOX_URL\n");
				pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
				//break;
			}
			else
			{
				pBox = m_data_entry[iUrl] = new UrlBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
				iUrl++;
			}
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
			offset += size;
		}
	}
	return 0;
}

void DrefBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<entry_count val=\"%d\"/>\n",m_entry_count);
}

int UrlBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	return 0;
}

void UrlBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
}

int StblBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box* pBox=0;

	pMp4File->SetPos(offset);

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size<1)
			return 0;
		switch(iBoxType)
		{
		case BOX_STSD:
			{
				if(m_pStsdBox)
					delete m_pStsdBox;
				pBox = m_pStsdBox = new StsdBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_STTS:
			{
				if(m_pSttsBox)
					delete m_pSttsBox;
				pBox = m_pSttsBox = new SttsBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_STSC:
			{
				if(m_pStscBox)
					delete m_pStscBox;
				pBox = m_pStscBox = new StscBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_STSZ:
			{
				if(m_pStszBox)
					delete m_pStszBox;
				pBox = m_pStszBox = new StszBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_STCO:
			{
				if(m_pStcoBox)
					delete m_pStcoBox;
				pBox = m_pStcoBox = new StcoBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_CO64:
			{
				if(m_pStcoBox)
					delete m_pStcoBox;
				pBox = m_pStcoBox = new StcoBox(offset, size, this);
				m_pStcoBox->SetCo64(true);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_STSS:
			{
				if(m_pStssBox)
					delete m_pStssBox;
				pBox = m_pStssBox = new StssBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_CTTS:
			{
				if(m_pCttsBox)
					delete m_pCttsBox;
				pBox = m_pCttsBox = new CttsBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_IVLD:
			{
				pBox = 0;
				RETURN_ERROR(-1);
			}
		default:
			//pFormatter->Put("STBL: Unknown Type\n");
			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
			if (pBox == NULL)
				RETURN_ERROR(-2);
			break;
		}
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
		}
		offset += size;	
	}
	return 0;
}

int StsdBox::Load(Mp4File * pMp4File)
{
	Mp4Box* pBox=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_entry_count = pMp4File->Get32();
	if( m_entry_count > 0)
	{
		m_SampleEntry = new Mp4Box * [m_entry_count];
		if (m_SampleEntry == NULL)
			RETURN_ERROR(-2);
		int i,offset,size=0,iBoxType;
		offset = m_iBeginOffset + 16;
		for(i=0;i<m_entry_count;i++)
		{
			pMp4File->SetPos(offset);
			iBoxType = pMp4File->GetBoxHead(&size);
			if(size<1)
				return 0;
			switch(iBoxType)
			{
			case BOX_MP4V:
				{
					pBox = m_SampleEntry[i] = new Mp4vBox(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_XVID:
				{
					pBox = m_SampleEntry[i] = new XvidBox(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_DX50:
			case BOX_DIVX:
				{
					pBox = m_SampleEntry[i] = new DivxBox(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_MP4A:
				{
					pBox = m_SampleEntry[i] = new Mp4aBox(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_AVC1:
				{
					pBox = m_SampleEntry[i] = new AvcBox(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_S263:
			case BOX_H263:
			case BOX_D263:
				{
					pBox = m_SampleEntry[i] = new S263Box(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_SAMR:
				{
					pBox = m_SampleEntry[i] = new SamrBox(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_TX3G:
				{
					pBox = m_SampleEntry[i] = new Tx3gBox(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_ULAW:
				{
					pBox = m_SampleEntry[i] = new UlawBox(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_ALAW:
				{
					pBox = m_SampleEntry[i] = new AlawBox(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_ALAC:
				{
					pBox = m_SampleEntry[i] = new AlacBox(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_IMA4:
				{
					pBox = m_SampleEntry[i] = new Ima4Box(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_MP3_:
				{
					pBox = m_SampleEntry[i] = new Mp3Box(offset, size, this);
					if (pBox == NULL)
						RETURN_ERROR(-2);
				}
				break;
			case BOX_IVLD:
				{
					pBox = 0;
					RETURN_ERROR(-1);
				}
				break;
			default:
				pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
				m_SampleEntry[i] = pBox;
				break;
			}
			if(pBox)
			{
				Adopt(pBox);
				int ret = pBox->Load(pMp4File);
				if (ret < 0)
					RETURN_ERROR(ret);
			}
			offset += size;
		}
	}
	return 0;
}

void StsdBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<entry_count val=\"%d\"/>\n",m_entry_count);
}

Mp4vBox::Mp4vBox(int offset, int size, Mp4Box * aContainer) 
: Mp4Box(BOX_MP4V,offset,size,aContainer)
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
	m_pEsdsBox = 0;
}

int Mp4vBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	if(pMp4File->GetBuffer(m_reserved1, 6)!=6)
		RETURN_ERROR(-1);
	m_data_reference_index = pMp4File->Get16();
	m_pre_defined1 	= pMp4File->Get16();
	m_reserved2 		= pMp4File->Get16();
	for(i=0;i<3;i++)
		m_reserved3[i] = pMp4File->Get32();
	m_width 				= pMp4File->Get16();
	m_height 			= pMp4File->Get16();
	m_hor_resolution 	= pMp4File->Get32();
	m_vert_resolution = pMp4File->Get32();
	m_reserved4 		= pMp4File->Get32();
	m_pre_defined2 	= pMp4File->Get16();
	if(pMp4File->GetBuffer(m_compressor_name, 32)!=32)
		RETURN_ERROR(-1);
	m_depth 				= pMp4File->Get16();
	m_pre_defined3 	= pMp4File->Get16();
	if(m_pEsdsBox)
		delete m_pEsdsBox;
	int size;
	int offset = m_iBeginOffset+78+8;
	while (offset + 8 - m_iBeginOffset < m_iSize)
	{
		int box_type = pMp4File->GetBoxHead(&size);
		if(box_type==BOX_ESDS)
		{
			if(size<1)
				return 0;
			m_pEsdsBox = new EsdsBox(offset, size, this);
			if (m_pEsdsBox == NULL)
				RETURN_ERROR(-2);
			if(size>0)
			{
				int ret = m_pEsdsBox->Load(pMp4File);
				if (ret < 0)
				{
					SAFEDELETE(m_pEsdsBox);
					RETURN_ERROR(ret);
				}
			}
			Adopt(m_pEsdsBox);
		}
		else
		{
			if(size<8)
				RETURN_ERROR(-1);
			pMp4File->SetPos(offset + size);
		}
		offset += size;
	}
	

	return 0;
}

void Mp4vBox::Dump(Formatter* pFormatter)
{
	int i;
	//pFormatter->Put("Mp4v\n");
	pFormatter->Print("<reserved>");
	for(i=0;i<6;i++)
		pFormatter->Put("%d ",m_reserved1[i]);
	pFormatter->Print("</reserved>\n");
	pFormatter->Put("<data_reference_index val=\"%d\"/>\n",m_data_reference_index);
	pFormatter->Put("<pre_defined val=\"%d\"/>\n",m_pre_defined1);
	pFormatter->Put("<reserved val=\"%d\"/>\n",m_reserved2);
	pFormatter->Print("<reserved>\n");
	pFormatter->Put("0x%x",m_reserved3[0]);
	pFormatter->Put("0x%x",m_reserved3[1]);
	pFormatter->Put("0x%x",m_reserved3[2]);
	pFormatter->Print("</reserved>\n");
	pFormatter->Put("<width val=\"%d\"/>\n",m_width);
	pFormatter->Put("<height val=\"%d\"/>\n",m_height);
	pFormatter->Put("<hor_resolution val=\"0x%x\"/>\n",m_hor_resolution);
	pFormatter->Put("<vert_resolution val=\"0x%x\"/>\n",m_vert_resolution);
	pFormatter->Put("<reserved val=\"%d\"/>\n",m_reserved4);
	pFormatter->Put("<pre_defined val=\"%d\"/>\n",m_pre_defined2);
	m_compressor_name[32] = 0;
	pFormatter->Print("<compressor_name val=\"%s\">\n",m_compressor_name);
	for(i=0;i<32;i++)
		pFormatter->Put(0,m_compressor_name[i]);
	pFormatter->Print("</compressor_name>\n");
	pFormatter->Put("<depth val=\"%d\"/>\n",m_depth);
	pFormatter->Put("<pre_defined val=\"%d\"/>\n",m_pre_defined3);
}

int Mp4vBox::GetObjectTypeIndication()
{
	Mp4BoxFinder BoxFinder(BOX_STCO);
	EsdsBox* pEsdsBox = (EsdsBox*)BoxFinder.GetBox(BOX_ESDS,this);

	if(pEsdsBox==NULL)
		return -1;
	if(pEsdsBox->m_pES_Descr==NULL)
		return -1;
	if(pEsdsBox->m_pES_Descr->m_pDecoderConfigDescr==NULL)
		return -1;

	return 0xff & pEsdsBox->m_pES_Descr->m_pDecoderConfigDescr->m_objectTypeIndication;
}


DivxBox::DivxBox(int offset, int size, Mp4Box * aContainer) 
: Mp4Box(BOX_DIVX,offset,size,aContainer)
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
	m_pGlblBox = 0;
}

int DivxBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	if(pMp4File->GetBuffer(m_reserved1, 6)!=6)
		RETURN_ERROR(-1);
	m_data_reference_index = pMp4File->Get16();
	m_pre_defined1 	= pMp4File->Get16();
	m_reserved2 		= pMp4File->Get16();
	for(i=0;i<3;i++)
		m_reserved3[i] = pMp4File->Get32();
	m_width 				= pMp4File->Get16();
	m_height 			= pMp4File->Get16();
	m_hor_resolution 	= pMp4File->Get32();
	m_vert_resolution = pMp4File->Get32();
	m_reserved4 		= pMp4File->Get32();
	m_pre_defined2 	= pMp4File->Get16();
	if(pMp4File->GetBuffer(m_compressor_name, 32)!=32)
		RETURN_ERROR(-1);
	m_depth 				= pMp4File->Get16();
	m_pre_defined3 	= pMp4File->Get16();
	if (m_pGlblBox)
		delete m_pGlblBox;
	int size;
	int offset = m_iBeginOffset+86;
	while (offset-m_iBeginOffset < m_iSize)
	{
		int box_type = pMp4File->GetBoxHead(&size);
		if (box_type == BOX_GLBL)
		{
			if (size < 1)
				return 0;
			m_pGlblBox = new GlblBox(offset, size, this);
			if (m_pGlblBox == NULL)
				RETURN_ERROR(-2);
			if (size > 0)
			{
				int ret = m_pGlblBox->Load(pMp4File);
				if (ret < 0)
				{
					SAFEDELETE(m_pGlblBox);
					RETURN_ERROR(ret);
				}
			}
			Adopt(m_pGlblBox);
		}
		else
		{
			if(size<8)
				RETURN_ERROR(-1);
			pMp4File->SetPos(offset + size);
		}
		offset += size;
	}
	return 0;
}

XvidBox::XvidBox(int offset, int size, Mp4Box * aContainer) 
: Mp4Box(BOX_XVID,offset,size,aContainer)
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
}

int XvidBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	if(pMp4File->GetBuffer(m_reserved1, 6)!=6)
		RETURN_ERROR(-1);
	m_data_reference_index = pMp4File->Get16();
	m_pre_defined1 	= pMp4File->Get16();
	m_reserved2 		= pMp4File->Get16();
	for(i=0;i<3;i++)
		m_reserved3[i] = pMp4File->Get32();
	m_width 				= pMp4File->Get16();
	m_height 			= pMp4File->Get16();
	m_hor_resolution 	= pMp4File->Get32();
	m_vert_resolution = pMp4File->Get32();
	m_reserved4 		= pMp4File->Get32();
	m_pre_defined2 	= pMp4File->Get16();
	if(pMp4File->GetBuffer(m_compressor_name, 32)!=32)
		RETURN_ERROR(-1);
	m_depth 				= pMp4File->Get16();
	m_pre_defined3 	= pMp4File->Get16();
	return 0;
}

AvcBox::AvcBox(int offset, int size, Mp4Box * aContainer) 
: Mp4Box(BOX_AVC1,offset,size,aContainer)
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

	numOfSequenceParameterSets = 0;
	numOfPictureParameterSets = 0;
	lengthSizeMinusOne = 0;
	sps = NULL;
	pps = NULL;
}

AvcBox::~AvcBox()
{
	int i;
	if(sps)
	{
		for (i=0; i< numOfSequenceParameterSets;  i++)
		{
			if(sps[i].sequenceParameterSetLength)
				delete [] sps[i].sequenceParameterSetNALUnit;
		}
		delete [] sps;
	}
	if(pps)
	{
		for (i=0; i< numOfSequenceParameterSets;  i++)
		{
			if(pps[i].pictureParameterSetLength)
				delete [] pps[i].pictureParameterSetNALUnit;
		}
		delete pps;
	}
}
int AvcBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	if(pMp4File->GetBuffer(m_reserved1, 6)!=6)
		RETURN_ERROR(-1);
	m_data_reference_index = pMp4File->Get16();
	m_pre_defined1 	= pMp4File->Get16();
	m_reserved2 		= pMp4File->Get16();
	for(i=0;i<3;i++)
		m_reserved3[i] = pMp4File->Get32();
	m_width 				= pMp4File->Get16();
	m_height 			= pMp4File->Get16();
	m_hor_resolution 	= pMp4File->Get32();
	m_vert_resolution = pMp4File->Get32();
	m_reserved4 		= pMp4File->Get32();
	m_pre_defined2 	= pMp4File->Get16();
	if(pMp4File->GetBuffer(m_compressor_name, 32)!=32)
		RETURN_ERROR(-1);
	m_depth 				= pMp4File->Get16();
	m_pre_defined3 	= pMp4File->Get16();
	int size = 0;
	BoxType boxtype;
	do 
	{
		if ((boxtype = pMp4File->GetBoxHead(&size)) != BOX_AVCC && size > 8)
		{
			char * p = new char[size - 8];
			if (p == NULL)
			{
				RETURN_ERROR(-2);
			}
			if(pMp4File->GetBuffer(p , size - 8)!=size - 8)
			{
				delete []p;
				RETURN_ERROR(-1);
			}
			delete []p;
		}
		else
		{
			break;
		}
	} while(1);

	if(  boxtype == BOX_AVCC  && size > 8 )
	{
		configurationVersion = pMp4File->GetChar();
		AVCProfileIndication = pMp4File->GetChar();
		profile_compatibility = pMp4File->GetChar();
		AVCLevelIndication = pMp4File->GetChar();
		lengthSizeMinusOne = pMp4File->GetChar() & 0x03;
		numOfSequenceParameterSets = pMp4File->GetChar() & 0x1f;

		if(numOfSequenceParameterSets)
		{
			sps = new AVC_SPS_NAL[numOfSequenceParameterSets];
			if (sps == NULL)
			{
				goto error;
			}
		}
		for (i=0; i< numOfSequenceParameterSets;  i++)
		{
			sps[i].sequenceParameterSetLength = pMp4File->Get16();
			sps[i].sequenceParameterSetNALUnit = new char[sps[i].sequenceParameterSetLength];
			if (sps[i].sequenceParameterSetNALUnit == NULL)
			{
				RETURN_ERROR(-2);
			}
			pMp4File->GetBuffer(sps[i].sequenceParameterSetNALUnit, sps[i].sequenceParameterSetLength);
		}	

		numOfPictureParameterSets = pMp4File->GetChar();
		if(numOfPictureParameterSets)
		{
			pps = new AVC_PPS_NAL[numOfPictureParameterSets];
			if (pps == NULL)
			{
				RETURN_ERROR(-2);
			}
		}
		for (i=0; i< numOfPictureParameterSets;  i++)
		{
			pps[i].pictureParameterSetLength = pMp4File->Get16();
			pps[i].pictureParameterSetNALUnit = new char[pps[i].pictureParameterSetLength];
			if (pps[i].pictureParameterSetNALUnit == NULL)
			{
				RETURN_ERROR(-2);
			}
			pMp4File->GetBuffer(pps[i].pictureParameterSetNALUnit, pps[i].pictureParameterSetLength);
		}
	}

	return 0;
error:
	if(sps)
	{
		for (i=0; i< numOfSequenceParameterSets;  i++)
		{
			if(sps[i].sequenceParameterSetNALUnit)
				delete[] sps[i].sequenceParameterSetNALUnit;
		}
		delete[] sps;
	}
	if(pps)
	{
		for (i=0; i< numOfPictureParameterSets;  i++)
		{
			if(pps[i].pictureParameterSetNALUnit)
				delete[] pps[i].pictureParameterSetNALUnit;
		}
		delete[] pps;
	}

	RETURN_ERROR(-2);
}

int EsdsBox::Load(Mp4File * pMp4File)
{
	int offset=0, size=0, count=0;

	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	//pFormatter->Put("Begin:0x%x\n",m_iBeginOffset);

	char Tag;
	offset = m_iBeginOffset+12;	
	while(offset < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		Tag = pMp4File->GetChar();
		size = GetSizeOfInstance(pMp4File,&count);
		offset += 1 + count;
		//pFormatter->Put("ESDS:offset:0x%x,Tag:%d, size:%d,m_iSize:%d\n",offset,Tag,size,m_iSize);
		switch(Tag)
		{
		case kES_DescrTag:
			if(m_pES_Descr)
				delete m_pES_Descr;
			m_pES_Descr = new ES_Descr(offset, size);
			if (m_pES_Descr == NULL)
			{
				RETURN_ERROR(-2);
			}
			m_pES_Descr->SetBoxContainer(this);
			m_pES_Descr->Load(pMp4File);
			Adopt(m_pES_Descr);
			break;
		default:
			//pFormatter->Put("ESDS:Unknown Tag found \n");
			break;
		}

		offset += size;
		//pFormatter->Put("ESDS:offset(N):0x%x,Tag:%d, size:%d,m_iSize:%d\n",offset,Tag,size,m_iSize);
	}

	return 0;
}

void EsdsBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
}

int GlblBox::Load(Mp4File * pMp4File)
{
	if (m_pGlblInfo != NULL)
		delete [] m_pGlblInfo;
	m_pGlblInfo = NULL; m_lInfoLen = 0;
	m_pGlblInfo = new char [m_iSize];
	if (m_pGlblInfo == NULL)
		RETURN_ERROR(-1);
	m_lInfoLen = m_iSize;
	if (pMp4File->GetBuffer(m_pGlblInfo, m_iSize) != m_iSize)
		RETURN_ERROR(-2);
	return 0;
}

int ES_Descr::Load(Mp4File * pMp4File)
{
	int offset, size, count;
	char ch, Tag;
	int ret;

	pMp4File->SetPos(m_iBeginOffset);
	m_ES_ID = pMp4File->Get16();
	ch = pMp4File->GetChar();	
	m_streamDependenceFlag = (ch>>7)&0x1;
	m_URL_Flag = (ch>>6)&0x1;
	m_OCRstreamFlag = (ch>>5)&0x1;
	m_streamPriority = ch&0x1f;

	//
	// TODO: following codes is a legacy codes to support PacketVideo's mp4 variation.
	//       need to confirm with PacketVideo.
	//
	if (pMp4File->m_Author==AuthorPacketVideo || pMp4File->m_Author==AuthorPhillips)
	{
		m_streamDependenceFlag	=  0;
		m_URL_Flag				= 0;
		m_OCRstreamFlag			= 0;
	}

	if(m_streamDependenceFlag)
		m_dependsOn_ES_ID = pMp4File->Get16();
	if(m_URLstring)
		delete [] m_URLstring;
	if(m_URL_Flag)
	{
		m_URLlength = (unsigned char)pMp4File->GetChar();
		m_URLstring = new char[m_URLlength];
		if (m_URLstring == NULL)
			RETURN_ERROR(-2);
		pMp4File->GetBuffer(m_URLstring, m_URLlength);
	}
	else
	{
		m_URLlength = 0;
	}
	if(m_OCRstreamFlag)
	{
		m_OCR_ES_ID = pMp4File->Get16();
	}

	offset = pMp4File->GetPos();
	//pFormatter->Put("ES_Descr = 0x%x(%d)\n",offset,offset);	
	Tag = pMp4File->GetChar();
	if(Tag!=kDecoderConfigDescrTag)
		return 0;	
	count = 0;
	size = Mp4Box::GetSizeOfInstance(pMp4File,&count);
	offset += 1 + count;

	if(m_pDecoderConfigDescr)
		delete m_pDecoderConfigDescr;
	m_pDecoderConfigDescr = new DecoderConfigDescr(offset,size,this);
	if (m_pDecoderConfigDescr == NULL)
		RETURN_ERROR(-2);
	ret = m_pDecoderConfigDescr->Load(pMp4File);
	if (ret < 0)
	{
		SAFEDELETE(m_pDecoderConfigDescr);
		RETURN_ERROR(ret);
	}
	Adopt(m_pDecoderConfigDescr);

	offset += size;

	if(m_pSLConfigDescr)
		RETURN_ERROR(ISO_SYNTAX_ERROR);

	pMp4File->SetPos(offset);
	Tag = pMp4File->GetChar();
	if(Tag!=kSLConfigDescrTag)
		RETURN_ERROR(ISO_SYNTAX_ERROR);
	count = 0;
	size = Mp4Box::GetSizeOfInstance(pMp4File,&count);
	offset += 1 + count;

	m_pSLConfigDescr = new SLConfigDescr(offset,size,this);
	if (m_pSLConfigDescr == NULL)
		RETURN_ERROR(-2);
	ret = m_pSLConfigDescr->Load(pMp4File);
	if (ret < 0)
	{
		SAFEDELETE(m_pSLConfigDescr);
		RETURN_ERROR(ret);
	}
	Adopt(m_pSLConfigDescr);

	return 0;
}

void ES_Descr::Dump(Formatter* pFormatter)
{
	char ch=0;
	pFormatter->Put("<ES_ID>%d</ES_ID>\n",m_ES_ID);

	pFormatter->Print("<streamDependenceFlag>%d</streamDependenceFlag>\n",m_streamDependenceFlag);
	ch |= m_streamDependenceFlag&0x1; ch <<= 1;
	pFormatter->Print("<URL_Flag>%d</URL_Flag>\n",m_URL_Flag);
	ch |= m_URL_Flag&0x1; ch <<= 1;
	pFormatter->Print("<OCRstreamFlag>%d</OCRstreamFlag>\n",m_OCRstreamFlag);
	ch |= m_OCRstreamFlag&0x1; ch <<= 5;
	pFormatter->Print("<streamPriority>%d</streamPriority>\n",m_streamPriority);
	ch |= m_streamPriority&0x1f;
	pFormatter->Print("<char>\n");
	pFormatter->Put(0,ch);
	pFormatter->Print("</char>\n");
	if(m_streamDependenceFlag)
		pFormatter->Put("<dependsOn_ES_ID>%d</dependsOn_ES_ID>\n",(short)m_dependsOn_ES_ID);
	if(m_URL_Flag)
	{
		pFormatter->Put("<URLlength>%d</URLlength>\n",(char)m_URLlength);
		int i;
		for(i=0;i<m_URLlength;i++)
			pFormatter->Put(0,(char)m_URLstring[i]);
	}
	if(m_OCRstreamFlag)
	{
		pFormatter->Put("<OCRstreamFlag>%d</OCRstreamFlag>\n",(short)m_OCR_ES_ID);
	}
}

int DecoderConfigDescr::Load(Mp4File * pMp4File)
{
	char Tag, ch;
	int offset, size, count;
	int ret;
	pMp4File->SetPos(m_iBeginOffset);
	m_objectTypeIndication = pMp4File->GetChar();
	ch = pMp4File->GetChar();
	m_streamType = (ch>>2)&0x3f;
	m_upStream = (ch>>1)&0x1;
	m_reserved = ch&0x1;
	m_bufferSizeDB = pMp4File->Get24();
	m_maxBitrate = pMp4File->Get32();
	m_avgBitrate = pMp4File->Get32();
	offset = m_iBeginOffset + 13;
	Tag = pMp4File->GetChar();
	if(Tag!=kDecSpecificInfoTag)
	{
		//pFormatter->Put("Can't locate kDecSpecificInfoTag = 0x05,\n");
		return 0;
	}
	size = Mp4Box::GetSizeOfInstance(pMp4File,&count);
	offset += 1 + count;
	if(m_pDecoderSpecificInfo)
		delete m_pDecoderSpecificInfo;
	m_pDecoderSpecificInfo = new DecoderSpecificInfo(offset, size, this);
	if (m_pDecoderSpecificInfo == NULL)
		RETURN_ERROR(-2);
	ret = m_pDecoderSpecificInfo->Load(pMp4File);
	if (ret < 0)
	{
		SAFEDELETE(m_pDecoderSpecificInfo);
		RETURN_ERROR(ret);
	}
	Adopt(m_pDecoderSpecificInfo);

	return 0;
}

void DecoderConfigDescr::Dump(Formatter* pFormatter)
{
	char ch=0;
	pFormatter->Put("<objectTypeIndication val=\"0x%x\"/>\n", (char)m_objectTypeIndication);
	ch = m_streamType&0x3f; ch <<= 1;
	pFormatter->Print("<streamType val=\"0x%x\"/>\n", m_streamType);
	ch |= m_upStream&0x1; ch <<= 1;
	pFormatter->Print("<upStream val=\"0x%x\"/>\n", m_upStream);
	ch |= m_reserved&0x1; 
	pFormatter->Print("<reserved val=\"0x%x\"/>\n", m_reserved);
	pFormatter->Print("<char>\n");
	pFormatter->Put(0,ch);
	pFormatter->Print("</char>\n");
	if(pFormatter->UseDisplay())
		pFormatter->Print("<buffersizeDB val=\"%d\"/>\n", m_bufferSizeDB);
	else
		pFormatter->Put24(0,m_bufferSizeDB);
	pFormatter->Put("<maxBitrate val=\"0x%x\"/>\n", m_maxBitrate);
	pFormatter->Put("<avgBitrate val=\"0x%x\"/>\n", m_avgBitrate);
}

DecoderSpecificInfo::~DecoderSpecificInfo()
{
	if(info)
		delete [] info;
	info = 0;
}

int DecoderSpecificInfo::Load(Mp4File * pMp4File)
{
	int ret;

	if(info)
		delete [] info;
	info = new char [m_iSize];
	if (info == NULL)
	{
		RETURN_ERROR(-2);
	}
	pMp4File->SetPos(m_iBeginOffset);
	if(pMp4File->GetBuffer(info, m_iSize)!=m_iSize)
		RETURN_ERROR(-1);

	DecoderConfigDescr * pDecConfig = 
		(DecoderConfigDescr *)GetContainer(kDecoderConfigDescrTag);

	if(pDecConfig)
	{
		if(pDecConfig->m_objectTypeIndication==(char)0xe1) // QCELP
		{
			if(m_iSize < 12)
				return 0;
			BufferFile Buffer(info,m_iSize);
			Buffer.SetPos(4); 
			m_pQcpFmt = new RiffQcpFmt(4,m_iSize-4);
			if (m_pQcpFmt == NULL)
				RETURN_ERROR(-2);
			ret = m_pQcpFmt->Load(&Buffer);
			if (ret < 0)
			{
				SAFEDELETE(m_pQcpFmt);
				RETURN_ERROR(ret);
			}
			m_pQcpFmt->MakeSerializable(false);
			Adopt(m_pQcpFmt);
		}
		else if(pDecConfig->m_objectTypeIndication==(char)0x20) // mpeg4 visual
		{
			if(m_iSize < 12)
				return 0;
			m_pVos = new Vos(pMp4File->GetPos(), 0);
			if (m_pVos == NULL)
				RETURN_ERROR(-2);
			BufferFile Buffer(info,m_iSize);
			Buffer.SetPos(0);
			ret = m_pVos->Load(&Buffer);
			//Gavin : do not return error if we can't get VOS information.
			//we shall use DSI as decoder open parameters
			if (ret == 0)
			{
				m_pVos->MakeSerializable(false);
				Adopt(m_pVos);
			}
			else
			{
				delete m_pVos;
				m_pVos = NULL;
			}
		}
	}

	return 0;
}


void DecoderSpecificInfo::Dump(Formatter* pFormatter)
{
	int i=0;
	pFormatter->Print("<data len=\"%d\">",m_iSize);
	for(i=0;i<m_iSize;i++)
		pFormatter->Put("0x%02x ",(char)(info[i]&0xff));
	pFormatter->Put("</data>\n");
}

int SLConfigDescr::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset);
	m_predefined = pMp4File->GetChar();
	if(m_predefined==0)
	{
		m_useAccessUnitStartFlag = (char)pMp4File->GetBits(1);
		m_useAccessUnitEndFlag = (char)pMp4File->GetBits(1);
		m_useRandomAccessPointFlag = (char)pMp4File->GetBits(1);
		m_hasRandomAccessUnitsOnlyFlag = (char)pMp4File->GetBits(1);
		m_usePaddingFlag = (char)pMp4File->GetBits(1);
		m_useTimeStampsFlag = (char)pMp4File->GetBits(1);
		m_useIdleFlag = (char)pMp4File->GetBits(1);
		m_durationFlag = (char)pMp4File->GetBits(1);
		m_timeStampResolution = pMp4File->Get32();
		m_OCRResolution = pMp4File->Get32();
		m_timeStampLength = pMp4File->GetChar();
		m_OCRLength = pMp4File->GetChar();
		m_AU_Length = pMp4File->GetChar();
		m_instantBitrateLength = pMp4File->GetChar();
		m_degradationPriorityLength = pMp4File->GetBits(4);
		m_AU_seqNumLength = pMp4File->GetBits(5);
		m_apcketSeqNumLegnth = pMp4File->GetBits(5);
		m_reserved = pMp4File->GetBits(2);

		if(m_durationFlag)
		{
			m_timeScale = pMp4File->Get32();
			m_accessUnitDuration = pMp4File->Get16();
			m_compositionUnitDuration = pMp4File->Get16();
		}
		if(m_useTimeStampsFlag==0)
		{
#ifndef TI_ARM		
			m_startDecodingTimeStamp = 0;
			m_startCompositionTimeStamp = 0;
#else
			int64s zero={0};
			m_startDecodingTimeStamp = zero;
			m_startCompositionTimeStamp = zero;        
#endif        		
		}
	}
	else
	{
		//what should be the values of predefined=1 and predefined=2? Need to check the spec
		m_useAccessUnitStartFlag = 0;
		m_useAccessUnitEndFlag = 0;
		m_useRandomAccessPointFlag = 0;
		m_hasRandomAccessUnitsOnlyFlag = 0;
		m_usePaddingFlag = 0;
		m_useTimeStampsFlag = 0;
		m_useIdleFlag = 0;
		m_durationFlag = 0;
		m_timeStampResolution = 0;
		m_OCRResolution = 0;
		m_timeStampLength = 0;
		m_OCRLength = 0;
		m_AU_Length = 0;
		m_instantBitrateLength = 0;
		m_degradationPriorityLength = 0;
		m_AU_seqNumLength = 0;
		m_apcketSeqNumLegnth = 0;
		m_reserved = 0;
	}
	

	return 0;
}

void SLConfigDescr::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<PREDEFINED>%d</PREDEFINED>\n",m_predefined);
	if(m_predefined!=0)
		return;
	char chFlag = 0;
	pFormatter->Print("<USEACCESSUNITSTARTFLAG>%d</USEACCESSUNITSTARTFLAG>\n", m_useAccessUnitStartFlag);
	chFlag |= m_useAccessUnitStartFlag;
	pFormatter->Print("<USE_ACCESSUNITENDFLAG>%d</USE_ACCESSUNITENDFLAG>\n",m_useAccessUnitEndFlag);
	chFlag <<= 1;
	chFlag |= m_useAccessUnitEndFlag;
	pFormatter->Print("<USERANDOMACCESSPOINTFLAG>%d</USERANDOMACCESSPOINTFLAG>\n", m_useRandomAccessPointFlag);
	chFlag <<= 1;
	chFlag |= m_useRandomAccessPointFlag;
	pFormatter->Print("<hasRandomAccessUnitsOnlyFlag>%d</hasRandomAccessUnitsOnlyFlag>\n", m_hasRandomAccessUnitsOnlyFlag);
	chFlag <<= 1;
	chFlag |= m_hasRandomAccessUnitsOnlyFlag;
	pFormatter->Print("<usePaddingFlag>%d</usePaddingFlag>\n",m_usePaddingFlag);
	chFlag <<= 1;
	chFlag |= m_usePaddingFlag;
	pFormatter->Print("<useTimeStampsFlag>%d</useTimeStampsFlag>\n",m_useTimeStampsFlag);
	chFlag <<= 1;
	chFlag |= m_useTimeStampsFlag;
	pFormatter->Print("<useIdleFlag>%d</useIdleFlag>\n",m_useIdleFlag);
	chFlag <<= 1;
	chFlag |= m_useIdleFlag;
	pFormatter->Print("<durationFlag>%d</durationFlag>\n",m_durationFlag);
	chFlag <<= 1;
	chFlag |= m_durationFlag;
	if(!pFormatter->UseDisplay())
		pFormatter->Put(0, chFlag);

	pFormatter->Put("<timeStampResolution>%d</timeStampResolution>\n",m_timeStampResolution);
	pFormatter->Put("<OCRResolution>%d</OCRResolution>\n",m_OCRResolution);
	pFormatter->Put("<timeStampLength>%d</timeStampLength>\n",m_timeStampLength);
	pFormatter->Put("<OCRLength>%d</OCRLength>\n",m_OCRLength);
	pFormatter->Put("<AU_Length>%d</AU_Length>\n",m_AU_Length);
	pFormatter->Put("<instantBitrateLength>%d</instantBitrateLength>\n",m_instantBitrateLength);

	long lVal=0;
	pFormatter->Print("<degradationPriorityLength>%d</degradationPriorityLength>\n",m_degradationPriorityLength);
	lVal |= m_degradationPriorityLength;
	pFormatter->Print("<AU_seqNumLength>%d</AU_seqNumLength>\n",m_AU_seqNumLength);
	lVal <<= 4;
	lVal |= m_AU_seqNumLength;
	pFormatter->Print("<apcketSeqNumLegnth>%d</apcketSeqNumLegnth>\n",m_apcketSeqNumLegnth);
	lVal <<= 5;
	lVal |= m_apcketSeqNumLegnth;
	pFormatter->Print("<reserved>%d</reserved>\n",m_reserved);
	lVal <<= 5;
	lVal |= m_apcketSeqNumLegnth;
	if(!pFormatter->UseDisplay())
		pFormatter->Put(0,lVal);

	pFormatter->Put("<timeScale>%d</timeScale>\n",m_timeScale);
	pFormatter->Put("<accessUnitDuration>%d</accessUnitDuration>\n",m_accessUnitDuration);
	pFormatter->Put("<compositionUnitDuration>%d</compositionUnitDuration>\n",m_compositionUnitDuration);
	pFormatter->Put("<startDecodingTimeStamp>%d</startDecodingTimeStamp>\n",m_startDecodingTimeStamp);
	pFormatter->Put("<startCompositionTimeStamp>%d</startCompositionTimeStamp>\n", m_startCompositionTimeStamp);
}

int ObjectDescriptor::Load(Mp4File * pMp4File)
{

	int offset=0, size, count;
	unsigned char Tag;
	char buf[2];

	pMp4File->SetPos(m_iBeginOffset);

	if(pMp4File->GetBuffer(buf, 2)!=2)
		RETURN_ERROR(-1);

	m_ObjectDescriptorID = buf[0] << 8 | buf[1];
	m_ObjectDescriptorID >>= 6;
	m_URL_Flag = (buf[1] >> 5) & 0x1;
	m_reserved = (buf[1]) & 0x1f;

	if(m_reserved!=0x1f)
		RETURN_ERROR(ISO_SYNTAX_ERROR)

		Descr * pDescr = 0;

	if(m_URL_Flag)
	{
		m_URLlength = (unsigned char)(pMp4File->GetChar());
		m_URLstring = new char [m_URLlength+1];
		if (m_URLstring == NULL)
			RETURN_ERROR(-2);
		if(pMp4File->GetBuffer(m_URLstring, m_URLlength)!=m_URLlength)
			RETURN_ERROR(-1);
		m_URLstring[m_URLlength] = 0;

		offset = pMp4File->GetPos();
		while(offset+3 < m_iBeginOffset+m_iSize)
		{
			Tag = pMp4File->GetChar();
			count = 0;
			size = Mp4Box::GetSizeOfInstance(pMp4File,&count);
			offset += 1 + count;
			if((int)Tag<ExtDescrTagStartRange || (int)Tag>ExtDescrTagEndRange)
			{
				break;
			}
			/*
			TODO: handle ExtensionDescriptor
			pDescr = new ExtensionDescriptor(offset,size,this);
			pDescr->Load(pMp4File);
			Adopt(pDescr);
			*/

			offset += size;
		}
	}
	else
	{
		offset = pMp4File->GetPos();
		while(offset+3 < m_iBeginOffset+m_iSize)
		{
			Tag = pMp4File->GetChar();
			count = 0;
			size = Mp4Box::GetSizeOfInstance(pMp4File,&count);
			offset += 1 + count;
			switch(Tag)
			{
			case kES_DescrTag:
				pDescr = new ES_Descr(offset,size,this);
				if (pDescr == NULL)
					RETURN_ERROR(-2);
				break;
			case kIPMP_DescrTag:
				pDescr = new IPMP_Descriptor(offset,size,this);
				if (pDescr == NULL)
					RETURN_ERROR(-2);
				break;
			default:
				if(Tag>=ExtDescrTagStartRange && Tag <=ExtDescrTagEndRange)
				{
				}
				else if(Tag>=OCIDescrTagStartRange && Tag <=OCIDescrTagEndRange)
				{
				}
				break;
			};

			if(pDescr)
			{
				int ret = pDescr->Load(pMp4File);
				if (ret < 0)
				{
					SAFEDELETE(pDescr);
					RETURN_ERROR(ret);
				}
				Adopt(pDescr);
			}

			offset += size;
		}
	}

	return 0;
}


void ObjectDescriptor::Dump(Formatter* pFormatter)
{
	//
	// TODO : Put() is not an accurate function().
	//
	pFormatter->Put("<ObjectDescriptorID val=\"0x%x\"/>\n",m_ObjectDescriptorID);
	pFormatter->Put("<URL_Flag val=\"0x%x\"/>\n",m_URL_Flag);
	pFormatter->Put("<reserved val=\"0x%x\"/>\n",m_reserved);
	if (m_URL_Flag) 
	{
		pFormatter->Put("<URLlength val=\"%d\"/>\n",(char)m_URLlength);
		pFormatter->Put("<URLstring val=\"%s\"/>\n",m_URLstring);
	}
}

int InitialObjectDescriptor::Load(Mp4File * pMp4File)
{

	int offset, size, count=0;
	unsigned char Tag;
	char buf[2];

	pMp4File->SetPos(m_iBeginOffset);

	if(pMp4File->GetBuffer(buf, 2)!=2)
		RETURN_ERROR(-1);

	m_ObjectDescriptorID = buf[0] << 8 | buf[1];
	m_ObjectDescriptorID >>= 6;
	m_URL_Flag = (buf[1] >> 5) & 0x1;
	m_includeInlineProfileLevelFlag = (buf[1] >> 4) & 0x1;
	m_reserved = (buf[1]) & 0xf;

	if(m_reserved!=0xf)
		RETURN_ERROR(ISO_SYNTAX_ERROR)

		Descr * pDescr = 0;

	if(m_URL_Flag)
	{
		m_URLlength = (unsigned char)pMp4File->GetChar();
		m_URLstring = new char [m_URLlength+1];
		if (m_URLstring == NULL)
			RETURN_ERROR(-2);
		if(pMp4File->GetBuffer(m_URLstring, m_URLlength)!=m_URLlength)
			RETURN_ERROR(-1);
		m_URLstring[m_URLlength] = 0;

		offset = pMp4File->GetPos();
		while(offset+3 < m_iBeginOffset+m_iSize)
		{
			Tag = pMp4File->GetChar();
			count = 0;
			size = Mp4Box::GetSizeOfInstance(pMp4File,&count);
			offset += 1 + count;
			if((int)Tag<ExtDescrTagStartRange || (int)Tag>ExtDescrTagEndRange)
			{
				break;
			}
			/*
			TODO: handle ExtensionDescriptor
			pDescr = new ExtensionDescriptor(offset,size,this);
			pDescr->Load(pMp4File);
			Adopt(pDescr);
			*/

			offset += size;
		}
	}
	else
	{
		m_ODProfileLevelIndication = pMp4File->GetChar();
		m_sceneProfileLevelIndication = pMp4File->GetChar();
		m_audioProfileLevelIndication = pMp4File->GetChar();
		m_visualProfileLevelIndication = pMp4File->GetChar();
		m_graphicsProfileLevelIndication = pMp4File->GetChar();

		offset = pMp4File->GetPos();
		while(offset+3 < m_iBeginOffset+m_iSize)
		{
			Tag = pMp4File->GetChar();
			count = 0;
			size = Mp4Box::GetSizeOfInstance(pMp4File,&count);
			offset += 1 + count;

			switch(Tag)
			{
			case kES_DescrTag:
				pDescr = new ES_Descr(offset,size,this);
				if (pDescr == NULL)
					RETURN_ERROR(-2);
				break;
			case kIPMP_DescrTag:
				pDescr = new IPMP_Descriptor(offset,size,this);
				if (pDescr == NULL)
					RETURN_ERROR(-2);
				break;
			default:
				if(Tag>=ExtDescrTagStartRange && Tag <=ExtDescrTagEndRange)
				{
				}
				else if(Tag>=OCIDescrTagStartRange && Tag <=OCIDescrTagEndRange)
				{
				}
				break;
			};

			if(pDescr)
			{
				int ret = pDescr->Load(pMp4File);
				if (ret < 0)
				{
					SAFEDELETE(pDescr);
					RETURN_ERROR(ret);
				}
				Adopt(pDescr);
			}

			offset += size;
		}
	}

	return 0;
}


void InitialObjectDescriptor::Dump(Formatter* pFormatter)
{
	//
	// TODO : Put() is not an accurate function().
	//
	pFormatter->Put("<ObjectDescriptorID val=\"0x%x\"/>\n",m_ObjectDescriptorID);
	pFormatter->Put("<URL_Flag val=\"0x%x\"/>\n",m_URL_Flag);
	pFormatter->Put("<includeInlineProfileLevelFlag val=\"0x%x\"/>\n",m_includeInlineProfileLevelFlag);
	pFormatter->Put("<reserved val=\"0x%x\"/>\n",m_reserved);
	if (m_URL_Flag) 
	{
		pFormatter->Put("<URLlength val=\"%d\"/>\n",(char)m_URLlength);
		pFormatter->Put("<URLstring val=\"%s\"/>\n",m_URLstring);
	}
	else
	{
		pFormatter->Put("<ODProfileLevelIndication val=\"0x%x\"/>\n",m_ODProfileLevelIndication);
		pFormatter->Put("<sceneProfileLevelIndication val=\"0x%x\"/>\n",m_sceneProfileLevelIndication);
		pFormatter->Put("<audioProfileLevelIndication val=\"0x%x\"/>\n",m_audioProfileLevelIndication);
		pFormatter->Put("<visualProfileLevelIndication val=\"0x%x\"/>\n",m_visualProfileLevelIndication);
		pFormatter->Put("<graphicsProfileLevelIndication val=\"0x%x\"/>\n",m_graphicsProfileLevelIndication);
	}
}

int IPMP_Descriptor::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset);

	m_IPMP_DescriptorID = pMp4File->GetChar();
	m_IPMPS_Type = pMp4File->Get16();
	if (m_IPMPS_Type == 0) {
		m_URLString = new char [m_iSize-3];
		if (m_URLString == NULL)
			RETURN_ERROR(-2);
		if(pMp4File->GetBuffer(m_URLString,m_iSize-3)!=(m_iSize-3))
			RETURN_ERROR(ISO_SYNTAX_ERROR);	
	} else {
		m_IPMP_data = new char [m_iSize-3];
		if (m_IPMP_data == NULL)
			RETURN_ERROR(-2);
		if(pMp4File->GetBuffer(m_IPMP_data,m_iSize-3)!=(m_iSize-3))
			RETURN_ERROR(ISO_SYNTAX_ERROR);	
	}

	return 0;
}

void IPMP_Descriptor::Dump(Formatter* pFormatter)
{
	int i;
	pFormatter->Put("<IPMP_DescriptorID val=\"0x%x\"/>\n",m_IPMP_DescriptorID);
	pFormatter->Put("<IPMPS_Type val=\"0x%x\"/>\n",m_IPMPS_Type);
	if (m_IPMPS_Type == 0) 
	{
		pFormatter->Print("<URLString>");
		for(i=0;i<m_iSize-3;i++)
		{
			pFormatter->Put("0x%x ",m_URLString[i]);
		}
		pFormatter->Print("</URLString>\n");
	} 
	else 
	{
		pFormatter->Print("<IPMP_data>");
		for(i=0;i<m_iSize-3;i++)
		{
			pFormatter->Put("0x%x ",m_IPMP_data[i]);
		}
		pFormatter->Print("</IPMP_data>\n");
	}
}

/*
int ContentClassificationDescriptor::Load(Mp4File * pMp4File)
{
int iSize = m_iSize-6;
pMp4File->SetPos(m_iBeginOffset);

m_classificationEntity = pMp4File->Get32();
m_classificationTable = pMp4File->Get16();
m_contentClassificationData = new char[iSize];
pMp4File->GetBuffer(m_contentClassificationData,iSize);

return 0;
}

void ContentClassificationDescriptor::Dump(Formatter* pFormatter)
{
int i;
pFormatter->Put("<classificationEntity val=\"0x%x\"/>\n",m_classificationEntity);
pFormatter->Put("<classificationTable val=\"0x%x\"/>\n",m_classificationTable);
pFormatter->Print("<m_contentClassificationData>");
for(i=0;i<m_iSize-6;i++)
pFormatter->Put("0x%x ",m_contentClassificationData[i]);
pFormatter->Print("</m_contentClassificationData>\n");
}

int KeyWordDescriptor::Load(Mp4File * pMp4File)
{
int i;
pMp4File->SetPos(m_iBeginOffset);

if(pMp4File->GetBuffer((char *)&m_languageCode,3)!=3)
RETURN_ERROR(ISO_SYNTAX_ERROR);

m_isUTF8_string = pMp4File->GetChar() & 0x1;
m_keyWordCount = pMp4File->GetChar();
m_keyWordLength = new char [m_keyWordCount];
for(i=0;i<m_keyWordCount;i++)
{
m_keyWordLength[i] = pMp4File->GetChar();
if(m_isUTF8_string)
{
m_keyWord8[i] = new char [m_keyWordLength[i]];
if(pMp4File->GetBuffer(m_keyWord8[i],m_keyWordLength[i])!=m_keyWordLength[i])
RETURN_ERROR(ISO_SYNTAX_ERROR);		
}
else
{
m_keyWord16[i] = new short [m_keyWordLength[i]];
if(pMp4File->GetBuffer((char *)m_keyWord16[i],m_keyWordLength[i]*2)!=m_keyWordLength[i]*2)
RETURN_ERROR(ISO_SYNTAX_ERROR);		
}
}

return 0;
}

void KeyWordDescriptor::Dump(Formatter* pFormatter)
{
int i,j;
pFormatter->Put("<languageCode val=\"0x%x\"/>\n",m_languageCode);
pFormatter->Put("<isUTF8_string val=\"0x%x\"/>\n",m_isUTF8_string);
for(i=0;i<m_keyWordCount;i++)
{
pFormatter->Put("<m_keyWordLength val=\"%d\"/>\n",m_keyWordLength[i] );
if(m_isUTF8_string)
{
pFormatter->Print("<keyWord8>");
for(j=0;j<m_keyWordLength[i];j++)
pFormatter->Put("0x%x ",m_keyWord8[i][j]);
pFormatter->Print("</keyWord8>\n");
}
else
{
pFormatter->Print("<keyWord16>");
for(j=0;j<m_keyWordLength[i];j++)
pFormatter->Put("0x%x ",m_keyWord16[i][j]);
pFormatter->Print("</keyWord16>\n");
}
}
}

int RatingDescriptor::Load(Mp4File * pMp4File)
{
pMp4File->SetPos(m_iBeginOffset);

m_ratingEntity = pMp4File->Get32();
m_ratingCriteria = pMp4File->Get16();
m_ratingInfo = new char [m_iSize-6];
if(pMp4File->GetBuffer(m_ratingInfo,m_iSize-6)!=(m_iSize-6))
RETURN_ERROR(ISO_SYNTAX_ERROR);		

return 0;
}

void RatingDescriptor::Dump(Formatter* pFormatter)
{
int i;
pFormatter->Put("<ratingEntity val=\"0x%x\"/>\n",m_ratingEntity);
pFormatter->Put("<ratingCriteria val=\"0x%x\"/>\n",m_ratingCriteria);
pFormatter->Print("<m_ratingInfo>");
for(i=0;i<m_iSize-6;i++)
{
pFormatter->Put("0x%x ",m_ratingInfo[i]);
}
pFormatter->Print("</m_ratingInfo>\n");
}

int ShortTextualDescriptor::Load(Mp4File * pMp4File)
{
pMp4File->SetPos(m_iBeginOffset);

if(pMp4File->GetBuffer((char *)&m_languageCode,3)!=3)
RETURN_ERROR(ISO_SYNTAX_ERROR);
m_isUTF8_string = (pMp4File->GetChar()>>7)&0x1;
m_nameLength = pMp4File->GetChar();

if(m_isUTF8_string)
{
m_eventName8 = new char [m_nameLength];
if(pMp4File->GetBuffer(m_eventName8,m_nameLength)!=m_nameLength)
RETURN_ERROR(ISO_SYNTAX_ERROR);
m_textLength = pMp4File->GetChar();
m_eventText8 = new char [m_textLength];
if(pMp4File->GetBuffer(m_eventText8,m_textLength)!=m_textLength)
RETURN_ERROR(ISO_SYNTAX_ERROR);
}
else
{
m_eventName16 = new short [m_nameLength];
if(pMp4File->GetBuffer((char *)m_eventName16,2*m_nameLength)!=(2*m_nameLength))
RETURN_ERROR(ISO_SYNTAX_ERROR);
m_textLength = pMp4File->GetChar();
m_eventText16 = new short [m_textLength];
if(pMp4File->GetBuffer((char *)&m_eventText16,2*m_textLength)!=(2*m_textLength))
RETURN_ERROR(ISO_SYNTAX_ERROR);
}

return 0;
}

void ShortTextualDescriptor::Dump(Formatter* pFormatter)
{
int i;
pFormatter->Put("<languageCode val=\"0x%x\"/>\n",m_languageCode);
pFormatter->Put("<isUTF8_string val=\"0x%x\"/>\n",m_isUTF8_string);
pFormatter->Put("<nameLength val=\"0x%x\"/>\n",m_nameLength);

if(m_isUTF8_string)
{
pFormatter->Print("<eventName>");
for(i=0;i<m_nameLength;i++)
pFormatter->Put("0x%x ",m_eventName8[i]);
pFormatter->Print("</eventName>\n");
pFormatter->Put("<textLength val=\"0x%x\"/>\n",m_textLength);
pFormatter->Print("<eventText>");
for(i=0;i<m_textLength;i++)
pFormatter->Put("0x%x ",m_eventText8[i]);
pFormatter->Print("</eventText>\n");
}
else
{
pFormatter->Print("<eventName>");
for(i=0;i<m_nameLength;i++)
pFormatter->Put("0x%x ",m_eventName16[i]);
pFormatter->Print("</eventName>\n");
pFormatter->Put("<textLength val=\"0x%x\"/>\n",m_textLength);
pFormatter->Print("<eventText>");
for(i=0;i<m_textLength;i++)
pFormatter->Put("0x%x ",m_eventText16[i]);
pFormatter->Print("</eventText>\n");
}
}
*/
AlawBox::AlawBox(int offset, int size, Mp4Box * aContainer)
: Mp4Box(BOX_ALAW, offset,size,aContainer)
{
	memset(m_reserved1, 0x0, 6);
	m_data_reference_index = 1;
	m_reserved2[0] = m_reserved2[1] = 0;
	m_channelcount = 2;
	m_samplesize = 16;
	m_pre_defined = 0;
	m_reserved3 = 0;
	m_sampleratehi = 16000;
	m_sampleratelo = 0;
}

int AlawBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	if(pMp4File->GetBuffer(m_reserved1, 6)!=6)
		RETURN_ERROR(-1);
	m_data_reference_index = pMp4File->Get16();

	for(i=0;i<2;i++)
		m_reserved2[i] = pMp4File->Get32();
	m_channelcount		= pMp4File->Get16();
	m_samplesize		= pMp4File->Get16();
	m_pre_defined 		= pMp4File->Get16();
	m_reserved3			= pMp4File->Get16();
	m_sampleratehi 	= pMp4File->Get16();
	m_sampleratelo 	= pMp4File->Get16();
	return 0;
}

UlawBox::UlawBox(int offset, int size, Mp4Box * aContainer)
: Mp4Box(BOX_ULAW, offset,size,aContainer)
{
	memset(m_reserved1, 0x0, 6);
	m_data_reference_index = 1;
	m_reserved2[0] = m_reserved2[1] = 0;
	m_channelcount = 2;
	m_samplesize = 16;
	m_pre_defined = 0;
	m_reserved3 = 0;
	m_sampleratehi = 16000;
	m_sampleratelo = 0;
}

int UlawBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	if(pMp4File->GetBuffer(m_reserved1, 6)!=6)
		RETURN_ERROR(-1);
	m_data_reference_index = pMp4File->Get16();

	for(i=0;i<2;i++)
		m_reserved2[i] = pMp4File->Get32();
	m_channelcount		= pMp4File->Get16();
	m_samplesize		= pMp4File->Get16();
	m_pre_defined 		= pMp4File->Get16();
	m_reserved3			= pMp4File->Get16();
	m_sampleratehi 	= pMp4File->Get16();
	m_sampleratelo 	= pMp4File->Get16();
	return 0;
}

Ima4Box::Ima4Box(int offset, int size, Mp4Box * aContainer)
: Mp4Box(BOX_IMA4, offset,size,aContainer)
{
	memset(m_reserved1, 0x0, 6);
	m_data_reference_index = 1;
	m_reserved2[0] = m_reserved2[1] = 0;
	m_channelcount = 2;
	m_samplesize = 16;
	m_pre_defined = 0;
	m_reserved3 = 0;
	m_sampleratehi = 16000;
	m_sampleratelo = 0;
}

int Ima4Box::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	if(pMp4File->GetBuffer(m_reserved1, 6)!=6)
		RETURN_ERROR(-1);
	m_data_reference_index = pMp4File->Get16();

	for(i=0;i<2;i++)
		m_reserved2[i] = pMp4File->Get32();
	m_channelcount		= pMp4File->Get16();
	m_samplesize		= pMp4File->Get16();
	m_pre_defined 		= pMp4File->Get16();
	m_reserved3			= pMp4File->Get16();
	m_sampleratehi 	= pMp4File->Get16();
	m_sampleratelo 	= pMp4File->Get16();
	return 0;
}

Mp3Box::Mp3Box(int offset, int size, Mp4Box * aContainer)
: Mp4Box(BOX_MP3_, offset,size,aContainer)
{
	memset(m_reserved1, 0x0, 6);
	m_data_reference_index = 1;
	m_reserved2[0] = m_reserved2[1] = 0;
	m_channelcount = 2;
	m_samplesize = 16;
	m_pre_defined = 0;
	m_reserved3 = 0;
	m_sampleratehi = 16000;
	m_sampleratelo = 0;
}

int Mp3Box::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	if(pMp4File->GetBuffer(m_reserved1, 6)!=6)
		RETURN_ERROR(-1);
	m_data_reference_index = pMp4File->Get16();

	for(i=0;i<2;i++)
		m_reserved2[i] = pMp4File->Get32();
	m_channelcount		= pMp4File->Get16();
	m_samplesize		= pMp4File->Get16();
	m_pre_defined 		= pMp4File->Get16();
	m_reserved3			= pMp4File->Get16();
	m_sampleratehi 	= pMp4File->Get16();
	m_sampleratelo 	= pMp4File->Get16();
	return 0;
}

Mp4aBox::Mp4aBox(int offset, int size, Mp4Box * aContainer) 
: Mp4Box(BOX_MP4A,offset,size,aContainer)
{
	memset(m_reserved1, 0x0, 6);
	m_data_reference_index = 1;
	m_reserved2[0] = m_reserved2[1] = 0;
	m_channelcount = 2;
	m_samplesize = 16;
	m_pre_defined = 0;
	m_reserved3 = 0;
	m_sampleratehi = 16000;
	m_sampleratelo = 0;
	m_pEsdsBox = 0;
}

int Mp4aBox::GetObjectTypeIndication()
{
	if(m_pEsdsBox==NULL)
		return -1;
	if(m_pEsdsBox->m_pES_Descr==NULL)
		return -1;
	if(m_pEsdsBox->m_pES_Descr==NULL)
		return -1;
	if(m_pEsdsBox->m_pES_Descr->m_pDecoderConfigDescr==NULL)
		return -1;

	return 0xff & m_pEsdsBox->m_pES_Descr->m_pDecoderConfigDescr->m_objectTypeIndication;
}

int Mp4aBox::Load(Mp4File * pMp4File)
{
	int i=0;
	int version, revision, vendor;
	int reserved;
	int size;
	int offset = m_iBeginOffset+36;	

	pMp4File->SetPos(m_iBeginOffset+8);
	if(pMp4File->GetBuffer(m_reserved1, 6)!=6)
		RETURN_ERROR(-1);
	m_data_reference_index = pMp4File->Get16();

	//
	// TODO: following codes is a legacy codes to support PacketVideo's mp4 variation.
	//       need to confirm with PacketVideo.
	//
	if(pMp4File->m_Author==AuthorPacketVideo)
	{
		m_data_reference_index++;
	}


	//reserved 64 bits
	version				= pMp4File->Get16();
	revision			= pMp4File->Get16();
	vendor				= pMp4File->Get32();
	
	if(version==2)
	{
		//always3, A 16-bit integer field that must be set to 3.
		reserved = pMp4File->Get16();

		//always16, A 16-bit integer field that must be set to 16 (0x0010).
		reserved = pMp4File->Get16();

		//alwaysMinus2, A 16-bit integer field that must be set to -2 (0xFFFE).
		reserved = pMp4File->Get16();

		//always0, A 16-bit integer field that must be set to 0.
		reserved = pMp4File->Get16();

		//always65536, A 32-bit integer field that must be set to 65536.
		reserved = pMp4File->Get32();

		//sizeOfStructOnly, A 32-bit integer field providing the offset to sound sample description structure�s extensions.
		reserved = pMp4File->Get32();

		//audioSampleRate, A 64-bit floating point number representing the number of audio frames per second, for example: 44,100.0.
		m_sampleratehi = pMp4File->Get32();
		m_sampleratelo = pMp4File->Get32();

		//numAudioChannels, A 32-bit integer field set to the number of audio channels; any channel assignment will be expressed in an extension.
		m_channelcount = pMp4File->Get32();

		//always7F000000, A 32-bit integer field that must be set to 0x7F000000.
		reserved = pMp4File->Get32();

		//constBitsPerChannel, A 32-bit integer field which is set only if constant and only for uncompressed audio. For all other cases set to 0.
		reserved = pMp4File->Get32();

		//formatSpecificFlags, A 32-bit integer field which carries LPCM flag values defined in �LPCM flag values?below. 
		reserved = pMp4File->Get32();

		//constBytesPerAudioPacket, A 32-bit unsigned integer set to the number of bytes per packet only if this value is constant. For other cases set to 0.
		reserved = pMp4File->Get32();

		//constLPCMFramesPerAudioPacket, A 32-bit unsigned integer set to the number of PCM frames per packet only if this value is constant. For other cases set to 0.
		reserved = pMp4File->Get32();	

		offset = m_iBeginOffset+8+8+8+48;
	}
	else
	{	
		m_channelcount	= pMp4File->Get16();
		m_samplesize	= pMp4File->Get16();
		m_pre_defined 	= pMp4File->Get16();
		m_reserved3		= pMp4File->Get16();
		m_sampleratehi 	= pMp4File->Get16();
		m_sampleratelo 	= pMp4File->Get16();

		offset = m_iBeginOffset+8+8+8+12;
	}

	//fixed .mov [see quicktime fileformat]
	if(version==1)
	{
		char extSoundDesc[48];
		if(pMp4File->GetBuffer(extSoundDesc, 48)!=48)
			RETURN_ERROR(-1);
		offset += 48;
	}		

	int iBoxType = 0;
	iBoxType = pMp4File->GetBoxHead(&size);
	if(iBoxType==BOX_ESDS)
	{
		//pFormatter->Put("Mp4a:0x%x,off:0x%x\n",m_iBeginOffset,offset);
		m_pEsdsBox = new EsdsBox(offset, size, this);
		if (m_pEsdsBox == NULL)
		{
			RETURN_ERROR(-2);
		}
		if(size > 0)
			m_pEsdsBox->Load(pMp4File);
		Adopt(m_pEsdsBox);
	}
	else if(version==2&&iBoxType==BOX_WAVE)
	{
		Mp4Box* pBox=0;
		pBox = new WaveBox(offset, size, this);
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
		}
	}

	return 0;
}

void Mp4aBox::Dump(Formatter* pFormatter)
{
	int i;
	//pFormatter->Put("Mp4a:\n");
	pFormatter->Put("<reserved>");
	for(i=0;i<6;i++)
		pFormatter->Put("%d ",m_reserved1[i]);
	pFormatter->Put("</reserved>\n");
	pFormatter->Put("<data_reference_index val=\"%d\"/>\n",m_data_reference_index);
	pFormatter->Print("<reserved>\n");
	pFormatter->Put(0,m_reserved2[0]);
	pFormatter->Put(0,m_reserved2[1]);
	pFormatter->Print("</reserved>\n");
	pFormatter->Put("<channelcount val=\"%d\"/>\n",m_channelcount);
	pFormatter->Put("<samplesize val=\"%d\"/>\n",m_samplesize);
	pFormatter->Put("<pre_defined val=\"%d\"/>\n",m_pre_defined);
	pFormatter->Put("<reserved val=\"%d\"/>\n",m_reserved3);
	pFormatter->Put("<sampleratehi val=\"%d\"/>\n",m_sampleratehi);
	pFormatter->Put("<sampleratelo val=\"%d\"/>\n",m_sampleratelo);
}

AlacBox::AlacBox(int offset, int size, Mp4Box * aContainer)
: Mp4Box(BOX_ALAC, offset,size,aContainer)
{
	memset(m_extra_data, 0x0, 36);
}

int AlacBox::Load(Mp4File * pMp4File)
{
	int size=0, tmp = 0, offset = 0;
	pMp4File->SetPos(m_iBeginOffset);
	size = pMp4File->Get32(); //atom size
	pMp4File->Get32(); // "alac"
	offset = 8;
	while (size > 0)
	{
		tmp = pMp4File->Get32();
		offset += 4;
		if (tmp == 36)
		{
			pMp4File->SetPos(m_iBeginOffset + offset - 4);
			if(pMp4File->GetBuffer(m_extra_data, 36)!=36)
				RETURN_ERROR(-1);
			break;
		}
		size -= 4;
	}

	return 0;
}

int S263Box::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	if(pMp4File->GetBuffer(m_reserved1, 6)!=6)
		RETURN_ERROR(-1);
	m_data_reference_index = pMp4File->Get16();
	m_pre_defined1 = pMp4File->Get16();
	m_reserved2 = pMp4File->Get16();
	for(i=0;i<3;i++)
		m_reserved3[i] = pMp4File->Get32();
	m_width	= pMp4File->Get16();
	m_height = pMp4File->Get16();
	m_hor_resolution = pMp4File->Get32();
	m_vert_resolution = pMp4File->Get32();
	m_reserved4 = pMp4File->Get32();
	m_pre_defined2 = pMp4File->Get16();
	if(pMp4File->GetBuffer(m_compressor_name, 32)!=32)
		RETURN_ERROR(-1);
	m_depth = pMp4File->Get16();
	m_pre_defined3 = pMp4File->Get16();

	int size;
	int offset = m_iBeginOffset+78+8;
	if(pMp4File->GetBoxHead(&size)==BOX_D263) // optional.
	{
		m_pD263Box = new D263Box(offset, size, this);
		if (m_pD263Box == NULL)
			RETURN_ERROR(-2);
		if(size > 0)
		{
			int ret = m_pD263Box->Load(pMp4File);
			if(ret < 0)
			{
				SAFEDELETE(m_pD263Box);
				RETURN_ERROR(ret);
			}
			
		}
		Adopt(m_pD263Box);
	}
	//	else	//mov h263 box
	//		RETURN_ERROR(-1);

	return 0;
}

void S263Box::Dump(Formatter* pFormatter)
{
	int i;
	//pFormatter->Put("Samr\n");
	pFormatter->Put("<reserved>");
	for(i=0;i<6;i++)
		pFormatter->Put("%d ",m_reserved1[i]);
	pFormatter->Put("</reserved>\n");
	pFormatter->Put("<data_reference_index val=\"%d\"/>\n",m_data_reference_index);
	pFormatter->Put("<pre_defined val=\"%d\"/>\n",m_pre_defined1);
	pFormatter->Put("<reserved val=\"%d\"/>\n",m_reserved2);
	pFormatter->Put("<reserved>");
	for(i=0;i<3;i++)
		pFormatter->Put("%d ",m_reserved3[i]);
	pFormatter->Put("</reserved>\n");
	pFormatter->Put("<width val=\"%d\"/>\n",m_width);
	pFormatter->Put("<height val=\"%d\"/>\n",m_height);
	pFormatter->Put("<hor_resolution val=\"%d\"/>\n",m_hor_resolution);
	pFormatter->Put("<vert_resolution val=\"%d\"/>\n",m_vert_resolution);
	pFormatter->Put("<reserved val=\"%d\"/>\n",m_reserved4);
	pFormatter->Put("<pre_defined val=\"%d\"/>\n",m_pre_defined2);
	m_compressor_name[32] = 0;
	for(i=0;i<32;i++)
		pFormatter->Put(0,m_compressor_name[i]);
	pFormatter->Put("<depth val=\"%d\"/>\n",m_depth);
	pFormatter->Put("<pre_defined val=\"%d\"/>\n",m_pre_defined3);
}

int D263Box::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_vendor			= pMp4File->Get32();
	m_decoder_version	= pMp4File->GetChar();
	m_h263_level 		= pMp4File->GetChar();
	m_h263_profile		= pMp4File->GetChar();

	int size;
	int offset = m_iBeginOffset+7+8;
	if(pMp4File->GetBoxHead(&size)==BOX_BITR) // optional.
	{
		if(size > 0)
		{
			m_pBitrBox = new BitrBox(offset, size, this);
			if (m_pBitrBox == NULL)
				RETURN_ERROR(-2);
		}
		int ret = m_pBitrBox->Load(pMp4File);
		if (ret < 0)
		{
			SAFEDELETE(m_pBitrBox);
			RETURN_ERROR(ret);
		}
		Adopt(m_pBitrBox);
	}

	return 0;
}

void D263Box::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<vendor val=\"%d\"/>\n",m_vendor);
	pFormatter->Put("<decoder_version val=\"%d\"/>\n",m_decoder_version);
	pFormatter->Put("<h263_level val=\"%d\"/>\n",m_h263_level);
	pFormatter->Put("<h263_profile val=\"%d\"/>\n",m_h263_profile);
}

int BitrBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_avg_bitrate 		= pMp4File->Get32();
	m_max_bitrate		= pMp4File->Get32();

	return 0;
}

void BitrBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<avg_bitrate val=\"%d\"/>\n",m_avg_bitrate);
	pFormatter->Put("<max_bitrate val=\"%d\"/>\n",m_max_bitrate);
}


int SamrBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	if(pMp4File->GetBuffer(m_reserved1, 6)!=6)
		RETURN_ERROR(-1);
	m_data_reference_index = pMp4File->Get16();
	for(i=0;i<2;i++)
		m_reserved2[i] = pMp4File->Get32();
	m_channelcount		= pMp4File->Get16();
	m_samplesize		= pMp4File->Get16();
	m_pre_defined 		= pMp4File->Get16();
	m_reserved3			= pMp4File->Get16();
	m_sampleratehi 	= pMp4File->Get16();
	m_sampleratelo 	= pMp4File->Get16();

	int size;
	int offset = m_iBeginOffset+36;
	if(pMp4File->GetBoxHead(&size)==BOX_DAMR)
	{
		//pFormatter->Put("SAMR:0x%x,off:0x%x\n",m_iBeginOffset,offset);
		m_pDamrBox = new DamrBox(offset, size, this);
		if (m_pDamrBox == NULL)
			RETURN_ERROR(-2);
		if(size > 0)
		{
			int ret = m_pDamrBox->Load(pMp4File);
			if(ret < 0)
			{
				SAFEDELETE(m_pDamrBox);
				RETURN_ERROR(ret);
			}
		}
		Adopt(m_pDamrBox);
	}

	return 0;
}

void SamrBox::Dump(Formatter* pFormatter)
{
	int i;
	//pFormatter->Put("Samr\n");
	pFormatter->Put("<reserved>");
	for(i=0;i<6;i++)
		pFormatter->Put("%d ",m_reserved1[i]);
	pFormatter->Put("</reserved>\n");
	pFormatter->Put("<data_reference_index val=\"%d\"/>\n",m_data_reference_index);
	pFormatter->Print("<reserved2>0x%x,0x%x\n",m_reserved2[0],m_reserved2[1]);
	pFormatter->Put(0,m_reserved2[0]);
	pFormatter->Put(0,m_reserved2[1]);
	pFormatter->Print("</reserved2>\n");
	pFormatter->Put("<channelcount val=\"%d\"/>\n",m_channelcount);
	pFormatter->Put("<samplesize val=\"%d\"/>\n",m_samplesize);
	pFormatter->Put("<pre_defined val=\"%d\"/>\n",m_pre_defined);
	pFormatter->Put("<reserved val=\"%d\"/>\n",m_reserved3);
	pFormatter->Put("<sampleratehi val=\"%d\"/>\n",m_sampleratehi);
	pFormatter->Put("<sampleratelo val=\"%d\"/>\n",m_sampleratelo);
}

int DamrBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_vendor = pMp4File->Get32();
	m_decoder_version = pMp4File->GetChar();
	m_mode_set = pMp4File->Get16();
	m_mode_change_period = pMp4File->GetChar();
	m_frames_per_sample = pMp4File->GetChar();

	return 0;
}

void DamrBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<vendor val=\"%d\"/>\n",m_vendor);
	pFormatter->Put("<decoder_version val=\"%d\"/>\n",m_decoder_version);
	pFormatter->Put("<mode_set val=\"%d\"/>\n",m_mode_set);
	pFormatter->Put("<mode_change_period val=\"%d\"/>\n",m_mode_change_period);
	pFormatter->Put("<frames_per_sample val=\"%d\"/>\n",m_frames_per_sample);
}

int FtabBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_entry_count = pMp4File->Get16();
	m_FontRecord.font_id = pMp4File->Get16();
	m_FontRecord.font_name_length = pMp4File->GetChar();
	if(m_FontRecord.font_name_length > 0)
	{
		m_FontRecord.font = new char [m_FontRecord.font_name_length+1];
		if (m_FontRecord.font == NULL)
			RETURN_ERROR(-2);
		if(pMp4File->GetBuffer(m_FontRecord.font, m_FontRecord.font_name_length)!=m_FontRecord.font_name_length)
			RETURN_ERROR(-1);
		m_FontRecord.font[m_FontRecord.font_name_length] = 0;
	}

	return 0;
}

void FtabBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<entry_count val=\"%d\"/>\n",m_entry_count);
	pFormatter->Put("<FontRecord>\n");
	pFormatter->Put("<font_id val=\"%d\"/>\n",m_FontRecord.font_id);
	pFormatter->Put("<font_name_length val=\"%d\"/>\n",m_FontRecord.font_name_length);
	if(m_FontRecord.font_name_length>0)
	{
		pFormatter->Put("<font val=\"%s\"/>\n",m_FontRecord.font);
	}
	pFormatter->Put("</FontRecord>\n");
}

int Tx3gBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);

	if(pMp4File->GetBuffer(m_reserved, 6)!=6)
		RETURN_ERROR(-1);
	m_data_reference_index = pMp4File->Get16();
	m_displayflags = pMp4File->Get32();
	m_horizontal_justification = pMp4File->GetChar();
	m_vertical_justification = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_background_color_rgba, 4)!=4)
		RETURN_ERROR(-1);
	m_BoxRecord.top = pMp4File->Get16();
	m_BoxRecord.left = pMp4File->Get16();
	m_BoxRecord.bottom = pMp4File->Get16();
	m_BoxRecord.right = pMp4File->Get16();
	m_StyleRecord.startchar = pMp4File->Get16();
	m_StyleRecord.endchar = pMp4File->Get16();
	m_StyleRecord.font_id = pMp4File->Get16();
	m_StyleRecord.face_style_flags = pMp4File->GetChar();
	m_StyleRecord.font_size = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_StyleRecord.text_color_rgba, 4)!=4)
		RETURN_ERROR(-1);

	int size=0;
	int offset = pMp4File->GetPos();
	if(pMp4File->GetBoxHead(&size)==BOX_FTAB)
	{
		m_pFtabBox = new FtabBox(offset, size, this);
		if (m_pFtabBox == NULL)
			RETURN_ERROR(-2);
		if(size > 0)
		{
			int ret = m_pFtabBox->Load(pMp4File);
			if (ret < 0)
			{
				SAFEDELETE(m_pFtabBox);
				RETURN_ERROR(ret);
			}
		}
		Adopt(m_pFtabBox);
	}
	else
	{
		//		DBG(("***!!! Can't locate FTAB box!\n"));
		RETURN_ERROR(-1);
	}

	return 0;
}

void Tx3gBox::Dump(Formatter* pFormatter)
{
	int i;

	pFormatter->Put("<reserved>");
	for(i=0;i<6;i++)
		pFormatter->Put("%d ",m_reserved[i]);
	pFormatter->Put("</reserved>\n");
	pFormatter->Put("<data_reference_index val=\"%d\"/>\n",m_data_reference_index);
	pFormatter->Put("<displayflags val=\"%d\"/>\n",m_displayflags);
	pFormatter->Put("<horizontal_justification val=\"%d\"/>\n",m_horizontal_justification);
	pFormatter->Put("<vertical_justification val=\"%d\"/>\n",m_vertical_justification);
	pFormatter->Put("<background_color_rgba>");
	for(i=0;i<4;i++)
		pFormatter->Put("%d ",m_background_color_rgba[i]);
	pFormatter->Put("</background_color_rgba>\n");
	pFormatter->Put("<BoxRecord top=\"%d\"/>\n",
		m_BoxRecord.top);
	pFormatter->Put("<BoxRecord left=\"%d\"/>\n",
		m_BoxRecord.left);
	pFormatter->Put("<BoxRecord bottom=\"%d\"/>\n",
		m_BoxRecord.bottom);
	pFormatter->Put("<BoxRecord right=\"%d\"/>\n",
		m_BoxRecord.right);
	pFormatter->Put("<StyleRecord>\n");
	pFormatter->Put("<startchar val=\"%d\"/>\n",m_StyleRecord.startchar);
	pFormatter->Put("<endchar val=\"%d\"/>\n",m_StyleRecord.endchar);
	pFormatter->Put("<font_id val=\"%d\"/>\n",m_StyleRecord.font_id);
	pFormatter->Put("<face_style_flags val=\"%d\"/>\n",m_StyleRecord.face_style_flags);
	pFormatter->Put("<font_size val=\"%d\"/>\n",m_StyleRecord.font_size);
	pFormatter->Put("<text_color_rgba>");
	for(i=0;i<4;i++)
		pFormatter->Put("%d ",m_StyleRecord.text_color_rgba[i]);
	pFormatter->Put("</text_color_rgba>\n");
	pFormatter->Put("</StyleRecord>\n");
}

int SttsBox::Load(Mp4File * pMp4File)
{
	int i=0;
	int j=0;
	int nTimeBytes = 4;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	nTimeBytes = m_version==1 ? 8 : 4;
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_entry_count = pMp4File->Get32();
	if(m_entry_count < 1)
		return 0;

	//	if(m_sample_count)
	//		delete [] m_sample_count;
	//	m_sample_count = new long [m_entry_count];
	//	if(m_sample_delta)
	//		delete [] m_sample_delta;
	//	m_sample_delta = new long [m_entry_count];
	if(m_entry)
		delete m_entry;
	m_entry = new tagSTTSBOX[m_entry_count];
	if (m_entry == NULL)
		RETURN_ERROR(-2);
	//	if(m_sample_starttime)
	//		delete [] m_sample_starttime;
	//	m_sample_starttime = new long [m_entry_count];
	if(m_sample_startidx)
		delete [] m_sample_startidx;
	m_sample_startidx = new long [m_entry_count];
	if (m_sample_startidx == NULL)
		RETURN_ERROR(-2);

	//sunstars read optimize
	if(pMp4File->GetBuffer((char*)m_entry, m_entry_count*sizeof(tagSTTSBOX))!=m_entry_count*sizeof(tagSTTSBOX))
		RETURN_ERROR(-1);

	m_sample_num  = 0;
	m_total_delta = 0;
	for(i=0;i<m_entry_count;i++)
	{
		//		m_entry[i].sample_count 	=  pMp4File->Get32();
		//		m_entry[i].sample_delta 	=  pMp4File->Get32();
		m_entry[i].sample_count 	=  GetIntL((char*)&m_entry[i].sample_count);
		m_entry[i].sample_delta 	=  GetIntL((char*)&m_entry[i].sample_delta);
		//		m_sample_num  			+= m_sample_count[i];
		//		m_total_delta 			+= m_sample_count[i]*m_sample_delta[i];
		m_sample_num 			+= m_entry[i].sample_count;
		m_total_delta 			+= m_entry[i].sample_count*m_entry[i].sample_delta;

		//		m_sample_starttime[i]=  m_total_delta;
		m_sample_startidx[i] =  m_sample_num;
	}
	if(m_sample_starttime)
		delete [] m_sample_starttime;

    //linux more than one hour
    //printf("m_sample_num = %d m_entry_count %d m_sample_num %d\n",m_sample_num,m_entry_count,m_sample_num);
    if(/*m_sample_num > 10 * m_entry_count ||*/ m_sample_num > 90000)
    {
        printf("m_sample_num ================%d\n",m_sample_num);
		RETURN_ERROR(-2);
    }

	if( m_entry_count == 1 )
	{
		m_sample_delta = m_entry[0].sample_delta;
	}
	else
	{
		m_sample_starttime = new long [m_sample_num+1];
		if (m_sample_starttime == NULL)
			RETURN_ERROR(-2);
		m_sample_starttime[0] = 0;
		for(i=1; i<=m_sample_num;i++)
		{
			m_sample_starttime[i] = m_sample_starttime[i-1] + m_entry[j].sample_delta;
			if(i>=m_sample_startidx[j])
				j++;
		}
	}

	return 0;
}

// iTime's scale should be in Mdhd's time scale.
// return Idx [1..]
int SttsBox::GetSampleIdx(int iTime)
{
	int Idx;
	if(m_sample_delta)
	{
		Idx = iTime/m_sample_delta;
		if(Idx<1)
			return 1;
		if(Idx>m_sample_num-1)
			return m_sample_num-1;
	}
	else
	{
		if (m_entry == NULL)
		{
			return -1;
		}
		if(iTime < m_entry[1].sample_delta)
			return 1;
		if(iTime >= m_total_delta)
			return m_sample_num;

		int BeginIdx = 0;
		int EndIdx = m_sample_num-1;
		int Idx = m_sample_num/2;
		while(Idx>1)
		{
			if( m_sample_starttime[Idx] > iTime )
			{
				if(m_sample_starttime[Idx-1] < iTime)
					return Idx;
				EndIdx = Idx;
				Idx = BeginIdx + ((Idx-BeginIdx)>>1);
				if (((Idx-BeginIdx)>>1) == 0)
				{
					return Idx;
				}
			}
			else if(m_sample_starttime[Idx] < iTime)
			{
				if( m_sample_starttime[Idx+1] >= iTime)
					return Idx+1;
				BeginIdx = Idx;
				Idx = Idx + ((EndIdx-Idx)>>1);
				if (((EndIdx-Idx)>>1) == 0)
				{
					return Idx;
				}
			}
			else
				return Idx;
		}
	}
	return Idx;
}

long SttsBox::GetCTS(long lIdx)
{
	if(lIdx < 1) 
		return 0;
	if(lIdx > m_sample_num)
		return m_total_delta;
	if(m_sample_delta)
		return m_sample_delta*lIdx;
	else
		return m_sample_starttime[lIdx-1];
	//	long lastindex = 0, startindex = 1;
	//	int i;
	//	for(i=0;i<m_entry_count;i++)
	//		{
	//		lastindex += m_entry[i].sample_count;
	//		if(lIdx <= lastindex)
	//			{
	//			return m_sample_starttime[i] + (lIdx-startindex)*m_entry[i].sample_delta;
	//			}
	//		startindex = lastindex+1;
	//		}
	//	return 0;
}

void SttsBox::Dump(Formatter* pFormatter)
{
	int i;
	pFormatter->Put("<version val=\"%d\"/>\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<entry_count val=\"%d\"/>\n",m_entry_count);
	for(i=0;i<m_entry_count;i++)
	{
		pFormatter->Print("<index val=\"%d\">\n",i+1);
		pFormatter->Put("<sample_count val=\"%d\"/>\n",m_entry[i].sample_count);
		pFormatter->Put("<sample_delta val=\"%d\"/>\n",m_entry[i].sample_delta);
		pFormatter->Print("</index>\n");
		if(m_entry[i].sample_count < 1)
		{
			pFormatter->Print("<error>STTS: Check!:sample_count[%d]=%d</error>\n",i,m_entry[i].sample_count);
		}
		if(m_entry[i].sample_delta < 1)
		{
			pFormatter->Print("<error>STTS: Check!:sample_delta[%d]=%d</error>\n",i,m_entry[i].sample_delta);
		}
	}
	pFormatter->Print("<total delta=\"%d\" sample=\"%d\"/>\n",m_total_delta,m_sample_num);
}

int CttsBox::Load(Mp4File * pMp4File)
{
	int i=0;
	int nTimeBytes = 4;
	m_sample_num = 0;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	nTimeBytes = m_version==1 ? 8 : 4;
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_entry_count = pMp4File->Get32();
	if(m_entry_count < 1)
		return 0;

	if(m_sample_count)
		delete [] m_sample_count;
	m_sample_count = new long [m_entry_count];
	if (m_sample_count == NULL)
		RETURN_ERROR(-2);
	if(m_sample_compo_offset)
		delete [] m_sample_compo_offset;
	m_sample_compo_offset = new long [m_entry_count];
	if (m_sample_compo_offset == NULL)
		RETURN_ERROR(-2);

	for(i=0;i<m_entry_count;i++)
	{
		m_sample_count[i] = pMp4File->Get32();
		m_sample_compo_offset[i] = pMp4File->Get32();
		m_sample_num += m_sample_count[i];
	}

	//remap to composition offset for each sample
	m_sample_offset = new long [m_sample_num];
	if (m_sample_offset == NULL)
		RETURN_ERROR(-2);
	memset(m_sample_offset, 0, sizeof(long)*m_sample_num);
	int entry_processing = 0;
	int counting = 0;
	for(i = 0; i < m_sample_num; i++)
	{
		m_sample_offset[i] = m_sample_compo_offset[entry_processing];
		counting++;
		if(counting >= m_sample_count[entry_processing])
		{
			counting = 0;
			entry_processing++;
			if(entry_processing >= m_entry_count)
			{
				break;
			}
		}
	}

	if(m_sample_compo_offset)
	{
		delete [] m_sample_compo_offset;
		m_sample_compo_offset = NULL;
	}

	return 0;
}
void CttsBox::Dump(Formatter* pFormatter)
{
//	int i;
//	pFormatter->Put("<version val=\"%d\" />\n",m_version);
//	pFormatter->Put("<flags>\n");
//	pFormatter->Put(0,m_flags[0]);
//	pFormatter->Put(0,m_flags[1]);
//	pFormatter->Put(0,m_flags[2]);
//	pFormatter->Put("</flags>\n");
//	pFormatter->Put("<entry_count val=\"%d\"/>\n",m_entry_count);
//	for(i=0;i<m_entry_count;i++)
//	{
//		pFormatter->Print("<index val=\"%d\">\n",i+1);
//		pFormatter->Put("<sample_count val=\"%d\"/>\n",m_sample_count[i]);
//		pFormatter->Put("<sample_offset val=\"%d\"/>\n",m_sample_compo_offset[i]);
//		pFormatter->Put("</index>\n");
//		if(m_sample_count[i] < 1)
//		{
//			pFormatter->Print("<error>CTTS: Check!:sample_count[%d]=%d</error>\n",i,m_sample_count[i]);
//		}
//		if(m_sample_compo_offset[i] < 1)
//		{
//			pFormatter->Print("<error>CTTS: Check!:sample_offset[%d]=%d</error>\n",i,m_sample_compo_offset[i]);
//		}
//	}
}
long CttsBox::GetCTSoffset(long lIdx)
{
	if(lIdx < 1) 
		return 0;
	if(lIdx > m_sample_num)
		return 0;

	return m_sample_offset[lIdx-1];
	
}

int StscBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_entry_count = pMp4File->Get32();
	if(m_entry_count < 1)
		return 0;
	//	if(m_first_chunk)
	//		delete [] m_first_chunk;
	//	m_first_chunk = new long [m_entry_count];	
	//	if(m_samples_per_chunk)
	//		delete [] m_samples_per_chunk;
	//	m_samples_per_chunk = new long [m_entry_count];	
	//	if(m_sample_description_index)
	//		delete [] m_sample_description_index;
	//	m_sample_description_index = new long [m_entry_count];	
	//	for(i=0;i<m_entry_count;i++)
	//		{
	//		m_first_chunk[i] = pMp4File->Get32();
	//		m_samples_per_chunk[i] = pMp4File->Get32();
	//		m_sample_description_index[i] = pMp4File->Get32();
	//		}
	if(m_entry)
		delete [] m_entry;
	m_entry = new tagSTSCBOX[m_entry_count];
	if (m_entry == NULL)
		RETURN_ERROR(-2);
	if(pMp4File->GetBuffer((char*)m_entry, m_entry_count*sizeof(tagSTSCBOX))!=m_entry_count*sizeof(tagSTSCBOX))
		RETURN_ERROR(-1);
	for(i=0;i<m_entry_count;i++)
	{
		m_entry[i].first_chunk = GetIntL((char*)&m_entry[i].first_chunk);
		m_entry[i].samples_per_chunk = GetIntL((char*)&m_entry[i].samples_per_chunk);
		m_entry[i].sample_description_index = GetIntL((char*)&m_entry[i].sample_description_index);
	}

#define WORKAROUND_MP4_AUTHORING_ERROR
#if defined(WORKAROUND_MP4_AUTHORING_ERROR)
	//	if(m_first_chunk[0]==0)
	//		{
	//		for(i=0;i<m_entry_count;i++)
	//			{
	//			m_first_chunk[i]++;
	//			if(m_sample_description_index[i]==0)
	//				m_sample_description_index[i]=1;
	//			}
	//		}
	if(m_entry[0].first_chunk == 0)
	{
		for(i=0;i<m_entry_count;i++)
		{
			m_entry[i].first_chunk++;
			if(m_entry[i].sample_description_index==0)
				m_entry[i].sample_description_index=1;
		}
	}
#endif

	return 0;
}
void StscBox::Dump(Formatter* pFormatter)
{
	int i, total_samples=0;
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<entry_count val=\"%d\"/>\n",m_entry_count);
	for(i=0;i<m_entry_count;i++)
	{
		pFormatter->Print("<index val=\"%d\">\n",i+1);
		//		pFormatter->Put("<first_chunk val=\"%d\"/>\n",m_first_chunk[i]);
		//		pFormatter->Put("<samples_per_chunk val=\"%d\"/>\n",m_samples_per_chunk[i]);
		//		pFormatter->Put("<sample_description_index val=\"%d\"/>\n",m_sample_description_index[i]);
		pFormatter->Put("<first_chunk val=\"%d\"/>\n",m_entry[i].first_chunk);
		pFormatter->Put("<samples_per_chunk val=\"%d\"/>\n",m_entry[i].samples_per_chunk);
		pFormatter->Put("<sample_description_index val=\"%d\"/>\n",m_entry[i].sample_description_index);	

		pFormatter->Put("</index>\n");
		if(m_entry[i].first_chunk < 1)
		{
			pFormatter->Print("<error>STSC:m_first_chunk[%d]=%d</error>\n",i,m_entry[i].first_chunk);
		}
		if(m_entry[i].samples_per_chunk < 1)
		{
			pFormatter->Print("<error>STSC:m_samples_per_chunk[%d]=%d</error>\n",i,m_entry[i].samples_per_chunk);
		}
		if(m_entry[i].sample_description_index < 1)
		{
			pFormatter->Print("<error>STSC:m_sample_description_index[%d]=%d</error>\n",i,m_entry[i].sample_description_index);
		}
		total_samples += m_entry[i].samples_per_chunk;
	}
	pFormatter->Print("<total sample=\"%d\"/>\n",total_samples);
}

int StszBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_sample_size = pMp4File->Get32();
	m_sample_count = pMp4File->Get32();
	if(m_sample_count < 1)
		return 0;
	if(m_entry_size)
		delete [] m_entry_size;
	if(m_entry_offset)
		delete [] m_entry_offset;
	if(m_sample_size==0)
	{
		m_entry_size = new long [m_sample_count];	
		if (m_entry_size == NULL)
			RETURN_ERROR(-2);
		m_entry_offset = new long [m_sample_count];	
		if (m_entry_offset == NULL)
			RETURN_ERROR(-2);
		m_lTotalSampleSize = 0;
		//		for(i=0;i<m_sample_count;i++)
		//		{
		//			m_entry_size[i] = pMp4File->Get32();
		//			m_lTotalSampleSize += m_entry_size[i];
		//		}
		//sunstars read optimize
		if(pMp4File->GetBuffer((char*)m_entry_size, m_sample_count*sizeof(long))!= m_sample_count*sizeof(long))
			RETURN_ERROR(-1);
		for(i=0;i<m_sample_count;i++)
		{
			m_entry_size[i] = GetIntL((char*)&m_entry_size[i]);
			m_entry_offset[i] = m_lTotalSampleSize;
			m_lTotalSampleSize += m_entry_size[i];
			if(m_lMaxSampleSize < m_entry_size[i])
			{
				m_lMaxSampleSize = m_entry_size[i];
			}
		}
	}
	else
	{
		m_lTotalSampleSize = m_sample_size * m_sample_count;
		m_lMaxSampleSize = m_sample_size;
	}
	return 0;
}

long StszBox::GetSampleIdx(long lSampleLocation)
{//sunstars search optimize
	if(m_sample_size!=0)
	{
		return (lSampleLocation/m_sample_size) + 1;
	}
	if(lSampleLocation < m_entry_offset[1])
		return 1;
	if(lSampleLocation > m_entry_offset[m_sample_count-1])
		return m_sample_count;

	int BeginIdx = 0;
	int EndIdx = m_sample_count;
	int Idx = m_sample_count/2;
	while(Idx>1)
	{
		if( m_entry_offset[Idx] > lSampleLocation )
		{
			if(m_entry_offset[Idx] <= lSampleLocation+m_entry_size[Idx-1])
				return Idx;
			EndIdx = Idx;
			Idx = BeginIdx + ((Idx-BeginIdx)>>1);
		}
		else if(m_entry_offset[Idx] < lSampleLocation)
		{
			if( (m_entry_offset[Idx]+m_entry_size[Idx]) >= lSampleLocation)
				return Idx+1;
			BeginIdx = Idx;
			Idx = Idx + ((EndIdx-Idx)>>1);
		}
		else
			return Idx+1;
	}

	//	for(i=0;i<m_sample_count && SumSize < lSampleLocation;i++)
	//	{
	//		SumSize += m_entry_size[i];
	//	}
	return Idx;
}

void StszBox::Dump(Formatter* pFormatter)
{
	int i; 
	unsigned long total_sample_size=0;
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Print("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Print("</flags>\n");
	pFormatter->Put("<sample_size val=\"%d\"/>\n",m_sample_size);
	pFormatter->Put("<sample_count val=\"%d\"/>\n",m_sample_count);
	total_sample_size=m_sample_size*m_sample_count;
	if(m_sample_size==0)
	{
		if(m_sample_size==0)
		{
			for(i=0;i<m_sample_count;i++)
			{
				pFormatter->Print("<entry_size index=\"%d\">",i+1);
				pFormatter->Put("%d",m_entry_size[i]);
				pFormatter->Put("</entry_size>\n");
				if(m_entry_size[i] < 1 || m_entry_size[i] > 1024*1024)
				{
					pFormatter->Print("<error>STSZ: Check!:entry_size[%d]=%d</error>\n",i,m_entry_size[i]);
				}
				else if(m_entry_size[i] > 15*1024)
				{
					pFormatter->Print("<warning>size=%d is so big to cause decoding error on P2102V</warning>\n",m_entry_size[i]);
				}
				total_sample_size	+= m_entry_size[i];
			}
		}
	}
	pFormatter->Print("<total sample=\"%d\"/>\n",(long)total_sample_size);
}

long StszBox::SetSampleSize( long SampleSize ,long MaxSampleSize)
{
	if(m_sample_size)
	{
		//only update when m_sample_size is set
		m_sample_size = SampleSize;
		m_lTotalSampleSize = m_sample_size * m_sample_count;
		if(MaxSampleSize == 0)
		{
			m_lMaxSampleSize = SampleSize;
		}
		else
		{
			m_lMaxSampleSize = MaxSampleSize;
		}
	}
	return 0;
}

int StcoBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_entry_count = pMp4File->Get32();
	if(m_entry_count < 1)
		return 0;
	if(m_chunk_offset)
		delete m_chunk_offset;
	m_chunk_offset = new unsigned long long[m_entry_count];
	if (m_chunk_offset == NULL)
		RETURN_ERROR(-2);
	if(m_isCo64 == false)
	{
		for(i=0;i<m_entry_count;i++)
		{
			m_chunk_offset[i] = pMp4File->Get32();
		}
	}
	else
	{
		for(i=0;i<m_entry_count;i++)
		{
			m_chunk_offset[i] = pMp4File->Get64();
		}
	}

	return 0;
}

void StcoBox::Dump(Formatter* pFormatter)
{
#if 0
	int i;
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<entry_count val=\"%d\"/>\n",m_entry_count);
	for(i=0;i<m_entry_count;i++)
	{
		pFormatter->Print("<chunk_offset index=\"%d\">",i+1);
		pFormatter->Put("%d",m_chunk_offset[i]);
		pFormatter->Put("</chunk_offset>\n");
		if(m_chunk_offset[i] < 1)
		{
			pFormatter->Print("<error>STCO: Check!:chunk_offset[%d]=%d</error>\n",i,m_chunk_offset[i]);
		}
		if(i>1)
		{
			if(m_chunk_offset[i] < m_chunk_offset[i-1])
			{
				pFormatter->Print("<error>chunk_offset[%d]=%d,",i-1,m_chunk_offset[i-1]);
				pFormatter->Print("chunk_offset[%d]=%d</error>\n",i,m_chunk_offset[i]);
			}
		}
	}
#endif
}

void StcoBox::SetCo64( bool isCo64 )
{
	m_isCo64 = isCo64;
}


int StssBox::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_entry_count = pMp4File->Get32();
	if(m_entry_count < 1)
		return 0;
	if(m_sample_number)
		delete m_sample_number;
	m_sample_number = new long [m_entry_count];
	if (m_sample_number == NULL)
		RETURN_ERROR(-2);
	//	for(i=0;i<m_entry_count;i++)
	//		{
	//		m_sample_number[i] = pMp4File->Get32();
	//		}
	//sunstars read optimize
	if(pMp4File->GetBuffer((char*)m_sample_number, m_entry_count*sizeof(long))!=m_entry_count*sizeof(long))
		RETURN_ERROR(-1);
	for(i=0;i<m_entry_count;i++)
		m_sample_number[i] = GetIntL((char*)&m_sample_number[i]);
	return 0;
}

void StssBox::Dump(Formatter* pFormatter)
{
	long i;
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<entry_count val=\"%d\"/>\n",m_entry_count);
	for(i=0;i<m_entry_count;i++)
	{
		pFormatter->Print("<sample_number index=\"%d\">",i+1);
		pFormatter->Put("%d", m_sample_number[i]);
		pFormatter->Put("</sample_number>\n");
		if(m_sample_number[i] < 1)
		{
			pFormatter->Print("<error>STSS: Check!:sample_number[%d]=%d</error>\n",i,m_sample_number[i]);
		}
		if(i>1)
		{
			if(m_sample_number[i] <= m_sample_number[i-1])
			{
				pFormatter->Print("<error>sample_number[%d]=%d,",i-1,m_sample_number[i-1]);
				pFormatter->Print("sample_number[%d]=%d</error>\n",i,m_sample_number[i]);
			}
		}
	}
}

long TrakBox::GetStreamSize()
{
	StszBox * pStszBox;
	Mp4BoxFinder BoxFinder(BOX_STSZ);
	Navigate(&BoxFinder);
	pStszBox = (StszBox *)BoxFinder.GetBox();

	if(pStszBox == 0)
		return 0;

	return pStszBox->GetTotalSampleSize();
}

int  CprtBox::Load(Mp4File * pMp4File)
{
	int nChar=0;
	unsigned char Ch;
	int iPos=0;

	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_language 	= pMp4File->Get16();
	iPos = pMp4File->GetPos();

	nChar=0;
	do {
		Ch = (unsigned char)pMp4File->GetChar();
		nChar++;
	} while(Ch!=0);

	if(nChar < 2)
		return 0;

	m_notice = new char [nChar];
	if (m_notice == NULL)
		RETURN_ERROR(-2);

	pMp4File->SetPos(iPos);
	nChar = 0;
	do {
		Ch = (unsigned char)pMp4File->GetChar();
		m_notice[nChar] = Ch;
		nChar++;
	} while(Ch!=0);

	return 0;
}

void CprtBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<LANGUAGE>0x%x</LANGUAGE>\n",m_language);
	//pFormatter->Put("<NOTICE>%s</NOTICE>\n",m_notice?m_notice:"nil");
}

int  UdtaBox::LoadUdtaMeta(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	MetaBox *pBox = 0;

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size==0) //by spec, For historical reasons, the data list is optionally terminated by a 32-bit integer set to 0.
			return 0;
        switch(iBoxType)
        {
            case BOX_META:
            {
                pBox = m_pMetaBox = new MetaBox(offset, size, this);
            }
            break;
            case BOX_IVLD:
            {
                pBox = 0;
                RETURN_ERROR(-1);
            }
            break;
            default:
                break;
        }
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->LoadMetaIlst(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
            if(iBoxType == BOX_META)
                break;
		}
		offset += size;
	}
	return 0;
}

int  MetaBox::LoadMetaIlst(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8+4;
	int size = 0;
	IlstBox *pBox = 0;

	pMp4File->SetPos(offset);
	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size<1)
			return 0;
		if(iBoxType==BOX_ILST)
		{
			if(m_pIlstBox)
				RETURN_ERROR(ISO_SYNTAX_ERROR);
			pBox = m_pIlstBox = new IlstBox(offset, size, this);
		}

		if(pBox)
		{
			Adopt(pBox);
			pBox->LoadIlstCvr(pMp4File);
            if(iBoxType == BOX_ILST)
                break;
		}
		offset += size;
	}
	return 0;
}

int  UdtaBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box * pBox = 0;

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size==0) //by spec, For historical reasons, the data list is optionally terminated by a 32-bit integer set to 0.
			return 0;
		if(iBoxType==BOX_CPRT)
		{
			if(m_pCprtBox)
				RETURN_ERROR(ISO_SYNTAX_ERROR);
			pBox = m_pCprtBox = new UdtaSubBox(BOX_CPRT, offset, size, this);
		}
		else if(iBoxType==BOX_PERF)
		{
			pBox = m_pPerfBox = new UdtaSubBox(BOX_PERF, offset, size, this);
		}
		else if(iBoxType==BOX_AUTH)
		{
			pBox = m_pAuthBox = new UdtaSubBox(BOX_AUTH, offset, size, this);
		}
		else if(iBoxType==BOX_TITL)
		{
			pBox = m_pTitlBox = new UdtaSubBox(BOX_TITL, offset, size, this);
		}
		else if(iBoxType==BOX_DSCP)
		{
			pBox = m_pDscpBox = new UdtaSubBox(BOX_DSCP, offset, size, this);
		}

		//for quicktime User data atoms,added by Gavin

		else if (iBoxType==BOX_META)
		{
			pBox = m_pMetaBox = new MetaBox(offset, size, this);
		}
		else if(iBoxType==BOX_CPY)
		{
			pBox = m_pCpyBox = new UdtaChildBox(BOX_CPY, offset, size, this);
		}
		else if(iBoxType==BOX_PRF)
		{
			pBox = m_pPrfBox = new UdtaChildBox(BOX_PRF, offset, size, this);
		}
		else if(iBoxType==BOX_PRD)
		{
			pBox = m_pPrdBox = new UdtaChildBox(BOX_PRD, offset, size, this);
		}
		else if(iBoxType==BOX_NAM)
		{
			pBox = m_pNamBox = new UdtaChildBox(BOX_NAM, offset, size, this);
		}
		else if(iBoxType==BOX_INF)
		{
			pBox = m_pInfBox = new UdtaChildBox(BOX_INF, offset, size, this);
		}
	 
		else if(iBoxType==BOX_IVLD)
		{
			pBox = 0;
			RETURN_ERROR(-1);
		}
		else
		{
			char fourcc[4];

			pMp4File->SetPos(m_iBeginOffset+8);
			if(pMp4File->GetBuffer(fourcc, 4)==4)
			{
				if(memcmp(fourcc,"pvmm",4)==0)
				{
					pMp4File->m_Author = AuthorPacketVideo;
				}
			}
			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
			if (pBox == NULL)
				RETURN_ERROR(-2);
		}

		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);

		}
		offset += size;	
	}

	return 0;
}

int  MetaBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8+4;
	int size = 0;
	Mp4Box * pBox = 0;

	pMp4File->SetPos(offset);
	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size<1)
			return 0;
		if(iBoxType==BOX_ILST)
		{
			if(m_pIlstBox)
				RETURN_ERROR(ISO_SYNTAX_ERROR);
			pBox = m_pIlstBox = new IlstBox(offset, size, this);
		}
		else
		{
			char fourcc[4];

			pMp4File->SetPos(m_iBeginOffset+8);
			if(pMp4File->GetBuffer(fourcc, 4)==4)
			{
				if(memcmp(fourcc,"pvmm",4)==0)
				{
					pMp4File->m_Author = AuthorPacketVideo;
				}
			}
			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
		}

 
		if(pBox)
		{
			Adopt(pBox);
			pBox->Load(pMp4File);
		}
		offset += size;	
	}

	return 0;
}

int  UdtaSubBox::Load(Mp4File * pMp4File)
{
	int nChar=0;
	unsigned char Ch;
	int iPos=0;

	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_language 	= pMp4File->Get16();
	iPos = pMp4File->GetPos();

	nChar=0;
	do {
		Ch = (unsigned char)pMp4File->GetChar();
		nChar++;
	} while(Ch!=0);

	if(nChar < 2)
		return 0;

	m_notice = new char [nChar];

	pMp4File->SetPos(iPos);
	nChar = 0;
	do {
		Ch = (unsigned char)pMp4File->GetChar();
		m_notice[nChar] = Ch;
		nChar++;
	} while(Ch!=0);
	
	m_size = nChar-1;

	if ((m_language<0x800)) //Macintosh text encoding.
	{
		m_codes = 2;
	}

	else if (nChar>1)   //Unicode text encoding
	{
		if (m_notice[0]==0xfe&&m_notice[1]==0xff)
			m_codes = 1;
		else
			m_codes = 0;
	}

	return 0;
}
 
void UdtaSubBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<LANGUAGE>0x%x</LANGUAGE>\n",m_language);
	//pFormatter->Put("<NOTICE>%s</NOTICE>\n",m_notice?m_notice:"nil");
}

int  UdtaChildBox::Load(Mp4File * pMp4File)
{
	int nChar=0;
	unsigned char Ch;
	int iPos=0;

	pMp4File->SetPos(m_iBeginOffset+8);
	m_size = pMp4File->Get16();
	m_language 	= pMp4File->Get16();
	iPos = pMp4File->GetPos();

	nChar=0;
	do {
		Ch = (unsigned char)pMp4File->GetChar();
		nChar++;
	} while(Ch!=0);

	if(nChar < 2)
		return 0;

	m_content = new char [nChar];

	pMp4File->SetPos(iPos);
	nChar = 0;
	do {
		Ch = (unsigned char)pMp4File->GetChar();
		m_content[nChar] = Ch;
		nChar++;
	} while(Ch!=0);

	if ((m_language<0x800)) //Macintosh text encoding.
	{
		m_codes = 2;
	}

	else if (nChar>1)   //Unicode text encoding
	{
		if (m_content[0]==0xfe&&m_content[1]==0xff)
			m_codes = 1;
		else
			m_codes = 0;
	}

	return 0;
}

int  IlstChildBox::Load(Mp4File * pMp4File)  //need ducoment for detail
{
	int nChar=0;
	unsigned char Ch;
	int iPos=0;

	if (m_boxtype == BOX_COVR) //can data
	{
		int type;
		int len;

		//pMp4File->SetPos(m_iBeginOffset + 8 + 2);
		len = pMp4File->Get32();
		pMp4File->Get32();	//skip "data"
		type = pMp4File->Get32();
		iPos = pMp4File->GetPos();

		if ((type == 0x0000000d || type == 0x0000000e) && len > 16)
		{
			m_content = new char[len - 16];
			pMp4File->SetPos(iPos + 4);
#if 0
            for (nChar = 0; nChar < len - 16; nChar++)
            {
                Ch = (unsigned char)pMp4File->GetChar();
                m_content[nChar] = Ch;
            }
#endif
            pMp4File->GetBuffer(m_content, len - 16);
		}

		m_size = len - 16;

		return 0;
	}
    else if (m_boxtype == BOX__CVR) //preview image
    {
        int type;
        int len;

        //pMp4File->SetPos(m_iBeginOffset + 8 + 2);
        len = pMp4File->Get32();
        pMp4File->Get32();	//skip "data"
        type = pMp4File->Get32();
        iPos = pMp4File->GetPos();

        if ((type == 0x0000000d || type == 0x0000000e) && len > 16)
        {
            m_content = new char[len - 16];
            pMp4File->SetPos(iPos + 4);

#if 0
            for (nChar = 0; nChar < len - 16; nChar++)
            {
                Ch = (unsigned char)pMp4File->GetChar();
                m_content[nChar] = Ch;
            }
#endif
            pMp4File->GetBuffer(m_content, len - 16);
        }

        m_size = len - 16;
        return 0;
    }
    else if (m_boxtype == BOX__DES)
    {
        int type;
        int len;

        //pMp4File->SetPos(m_iBeginOffset + 8 + 2);
        len = pMp4File->Get32();
        pMp4File->Get32();	//skip "data"
        type = pMp4File->Get32();
        iPos = pMp4File->GetPos();
        if (type == 0x00000001 && len > 16)
        {
            m_content = new char[len - 16 + 1];
            pMp4File->SetPos(iPos + 4);

            for (nChar = 0; nChar < len - 16; nChar++)
            {
                Ch = (unsigned char)pMp4File->GetChar();
                m_content[nChar] = Ch;
            }
            m_content[nChar] = 0;

            m_size = len - 16 + 1;
        }
        return 0;
    }

	pMp4File->SetPos(m_iBeginOffset+8+2);
	m_size = pMp4File->Get16();
	pMp4File->Get32();	//skip "data"
	pMp4File->Get16();
	m_language 	= pMp4File->Get16();
	pMp4File->Get32();
	iPos = pMp4File->GetPos();

	nChar=0;
	do {
		Ch = (unsigned char)pMp4File->GetChar();
		nChar++;
	} while(Ch!=0);

	if(nChar < 2)
		return 0;

	m_content = new char [nChar];

	pMp4File->SetPos(iPos);
	nChar = 0;
	do {
		Ch = (unsigned char)pMp4File->GetChar();
		m_content[nChar] = Ch;
		nChar++;
	} while(Ch!=0);

	m_size = nChar;
	
	m_codes = 0;
	

	return 0;
}

int  IlstBox::LoadIlstCvr(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box * pBox = 0;

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size<1)
			return 0;

        if (iBoxType == BOX__CVR)
        {
            pBox = m_pCovrBox = new IlstChildBox(BOX__CVR, offset, size, this);
        }
		else if(iBoxType==BOX_IVLD)
		{
			pBox = 0;
			RETURN_ERROR(-1);
		}

		if(pBox)
		{
			Adopt(pBox);
			pBox->Load(pMp4File);
            if (iBoxType == BOX__CVR)
                break;
		}
		offset += size;	
	}

	return 0;
}

int  IlstBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box * pBox = 0;

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size<1)
			return 0;

		//ARTIST
		if(iBoxType==BOX_ART)
		{
			pBox = m_pPrfBox = new IlstChildBox(BOX_ART, offset, size, this);
		}
		else if(iBoxType==BOX_WRT)
		{
			pBox = m_pPrfBox = new IlstChildBox(BOX_WRT, offset, size, this);
		} 
		else if(iBoxType==BOX_GRP)
		{
			pBox = m_pPrfBox = new IlstChildBox(BOX_GRP, offset, size, this);
		}

		//author
		else if(iBoxType==BOX_PRD)
		{
			pBox = m_pPrdBox = new IlstChildBox(BOX_PRD, offset, size, this);
		}	

		//TITLE
		else if(iBoxType==BOX_NAM)
		{
			pBox = m_pNamBox = new IlstChildBox(BOX_NAM, offset, size, this);
		}

		else if(iBoxType==BOX_PRD)
		{
			pBox = m_pPrfBox = new IlstChildBox(BOX_PRD, offset, size, this);
		}
		 
		//album
		else if(iBoxType==BOX_ALB)
		{
			pBox = m_pAlbBox = new IlstChildBox(BOX_ALB, offset, size, this);
		}

		//comment
		else if(iBoxType==BOX_TOO)
		{
			pBox = m_pInfBox = new IlstChildBox(BOX_TOO, offset, size, this);
		}

		else if (iBoxType == BOX_COVR)
		{
            pBox = m_pCanDataBox = new IlstChildBox(BOX_COVR, offset, size, this);
		}

        else if (iBoxType == BOX__CVR)
        {
            pBox = m_pCovrBox = new IlstChildBox(BOX__CVR, offset, size, this);
        }
	
        else if (iBoxType == BOX__DES)
        {
            pBox = m_pDescBox = new IlstChildBox(BOX__DES, offset, size, this);
        }

		else if(iBoxType==BOX_IVLD)
		{
			pBox = 0;
			RETURN_ERROR(-1);
		}

		else
		{
			char fourcc[4];

			pMp4File->SetPos(m_iBeginOffset+8);
			if(pMp4File->GetBuffer(fourcc, 4)==4)
			{
				if(memcmp(fourcc,"pvmm",4)==0)
				{
					pMp4File->m_Author = AuthorPacketVideo;
				}
			}
			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
		}

		if(pBox)
		{
			Adopt(pBox);
			pBox->Load(pMp4File);
		}
		offset += size;	
	}

	return 0;
}

int  TrexBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_track_ID 	= pMp4File->Get32();
	m_default_sample_description_index = pMp4File->Get32();
	m_default_sample_duration = pMp4File->Get32();
	m_default_sample_size = pMp4File->Get32();
	m_default_sample_flags = pMp4File->Get32();

	return 0;
}

void TrexBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<TRACK_ID>%d</TRACK_ID>\n",m_track_ID);
	pFormatter->Put("<DEFAULT_SAMPLE_DESCRIPTION_INDEX>%d</DEFAULT_SAMPLE_DESCRIPTION_INDEX>\n",m_default_sample_description_index);
	pFormatter->Put("<DEFAULT_SAMPLE_DURATION>%d</DEFAULT_SAMPLE_DURATION>\n",m_default_sample_duration);
	pFormatter->Put("<DEFAULT_SAMPLE_SIZE>%d</DEFAULT_SAMPLE_SIZE>\n",m_default_sample_size);
	pFormatter->Put("<DEFAULT_SAMPLE_FLAGS>%d</DEFAULT_SAMPLE_FLAGS>\n",m_default_sample_flags);
}

int  MehdBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_fragment_duration 	= pMp4File->Get32();
	return 0;
}

void MehdBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<fragment_duration val=\"%d\"/>\n",m_fragment_duration);
}

int  MvexBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box* pBox=0;
	pMp4File->SetPos(offset);

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size < 1)
			return 0;
		switch(iBoxType)
		{
		case BOX_TREX: 
			{
				if(m_pTrexBox)
					RETURN_ERROR(ISO_SYNTAX_ERROR);
				pBox = m_pTrexBox = new TrexBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_MEHD: 
			{
				if(m_pMehdBox)
					RETURN_ERROR(ISO_SYNTAX_ERROR);
				pBox = m_pMehdBox = new MehdBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_IVLD:
			{
				pBox = 0;
				RETURN_ERROR(-1);
			}
		default:
			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
			if (pBox == NULL)
				RETURN_ERROR(-2);
			break;
		}
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
		}
		offset += size;	
	}

	return 0;
}

int  TfhdBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_track_ID 	= pMp4File->Get32();
	m_base_data_offset 	= pMp4File->Get64();
	m_sample_description_index = pMp4File->Get32();
	m_default_sample_duration = pMp4File->Get32();
	m_default_sample_size = pMp4File->Get32();
	m_default_sample_flags = pMp4File->Get32();
	return 0;
}

void TfhdBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<TRACK_ID>%d</TRACK_ID>\n",m_track_ID);
	pFormatter->Put("<BASE_DATA_OFFSET>%d</BASE_DATA_OFFSET>\n",m_base_data_offset);
	pFormatter->Put("<SAMPLE_DESCRIPTION_INDEX>%d</SAMPLE_DESCRIPTION_INDEX>\n",m_sample_description_index);
	pFormatter->Put("<DEFAULT_SAMPLE_DURATION>%d</DEFAULT_SAMPLE_DURATION>\n",m_default_sample_duration);
	pFormatter->Put("<DEFAULT_SAMPLE_SIZE>%d</DEFAULT_SAMPLE_SIZE>\n",m_default_sample_size);
	pFormatter->Put("<DEFAULT_SAMPLE_FLAGS>0x%x</DEFAULT_SAMPLE_FLAGS>\n",m_default_sample_flags);
}

int  TrunBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_sample_count = pMp4File->Get32();
	m_data_offset = pMp4File->Get32();
	m_first_sample_flags = pMp4File->Get32();
	m_sample_duration = pMp4File->Get32();
	m_sample_size = pMp4File->Get32();
	m_sample_flags = pMp4File->Get32();
	m_sample_composition_time_offset = pMp4File->Get32();
	m_sample_degradation_priority = pMp4File->Get32();

	return 0;
}

void TrunBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<SAMPLE_COUNT>%d</SAMPLE_COUNT>\n",m_sample_count);
	pFormatter->Put("<DATA_OFFSET>%d</DATA_OFFSET>\n",m_data_offset);
	pFormatter->Put("<FIRST_SAMPLE_FLAGS>%d</FIRST_SAMPLE_FLAGS>\n",m_first_sample_flags);
	pFormatter->Put("<SAMPLE_DURATION>%d</SAMPLE_DURATION>\n",m_sample_duration);
	pFormatter->Put("<SAMPLE_SIZE>%d</SAMPLE_SIZE>\n",m_sample_size);
	pFormatter->Put("<SAMPLE_FLAGS>0x%x</SAMPLE_FLAGS>\n",m_sample_flags);
	pFormatter->Put("<SAMPLE_COMPOSITION_TIME_OFFSET>%d</SAMPLE_COMPOSITION_TIME_OFFSET>\n",m_sample_composition_time_offset);
	pFormatter->Put("<SAMPLE_DEGRADATION_PRIORITY>%d</SAMPLE_DEGRADATION_PRIORITY>\n",m_sample_degradation_priority);
}

int  TrafBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box* pBox=0;
	pMp4File->SetPos(offset);

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size < 1)
			return 0;
		switch(iBoxType)
		{
		case BOX_TFHD: 
			{
				if(m_pTfhdBox)
					RETURN_ERROR(ISO_SYNTAX_ERROR);
				pBox = m_pTfhdBox = new TfhdBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_TRUN: 
			{
				pBox = m_pTrunBox = new TrunBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_IVLD:
			{
				pBox = 0;
				RETURN_ERROR(-1);
			}
		default:
			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
			if (pBox == NULL)
				RETURN_ERROR(-2);
			break;
		}
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
		}
		offset += size;	
	}

	return 0;
}

int  MfhdBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_sequence_number = pMp4File->Get32();
	return 0;
}

void MfhdBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<sequence_number val=\"%d\"/>\n",m_sequence_number);
}

int  MoofBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box* pBox=0;
	pMp4File->SetPos(offset);

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size < 1)
			return 0;
		switch(iBoxType)
		{
		case BOX_MFHD: 
			{
				if(m_pMfhdBox)
					RETURN_ERROR(ISO_SYNTAX_ERROR);
				pBox = m_pMfhdBox = new MfhdBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_TRAF: 
			{
				pBox = m_pTrafBox = new TrafBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		case BOX_IVLD:
			{
				pBox = 0;
				RETURN_ERROR(-1);
			}
		default:
			break;
		}
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
		}
		offset += size;	
	}

	return 0;
}

DrmBox::DrmBox(int offset, int size, Mp4Box * aContainer) 
: Mp4Box(BOX_DRM,offset,size,aContainer),
m_data(0), 
m_data_len(0), 
m_bRinger(false), 
m_bForward(false),
m_iFileSize(0)
{
}

void DrmBox::NotifyFileSize(int iFileSize)
{
	m_iFileSize = iFileSize;
}

int  DrmBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_data_len = m_iSize - 8;
	if(m_data_len < 1)
		return 0;
	m_data = new char [m_data_len];
	if (m_data == NULL)
		RETURN_ERROR(-2);
	if(pMp4File->GetBuffer(m_data, m_data_len)!=m_data_len)
		RETURN_ERROR(-1);
	return 0;
}

#define ODD_FILESIZE_RINGER 0x0D
#define EVEN_FILESIZE_RINGER 0x09
void DrmBox::Dump(Formatter* pFormatter)
{
	//printf("m_bForward:%d, m_bRinger:%d, m_iFileSize:%d\n",m_bForward,m_bRinger,m_iFileSize);
	long i;

	if(m_bNotLoaded)
		return;
	if(m_iFileSize > 0 && !pFormatter->UseDisplay())
	{
		char DrmString[]	= {0x0,0x0,0x0,0xa,0x64,0x63,0x6d,0x64,0x0,0x0};
		if(m_bForward)
		{
			DrmString[9] |= 1;
		}

		if(m_bRinger)
		{
			// set it to defualt ringer value.
			if(m_iFileSize%2==0)
				DrmString[9] |= EVEN_FILESIZE_RINGER;
			else
				DrmString[9] |= ODD_FILESIZE_RINGER;
		}

		if(m_data_len!=10)
		{
			if(m_data)
				delete [] m_data;
			m_data_len = 10;
			m_data = new char [m_data_len];
			if (m_data == NULL)
			{
				return;
			}
		}

		memcpy(m_data, DrmString, 10);
	}

	pFormatter->Print("<data len=\"%d\">\n",m_data_len);
	for(i=0;i<m_data_len;i++)
	{
		pFormatter->Put("0x%x,",(char)(m_data[i]&0xff));
		//printf("0x%x,",(char)(m_data[i]&0xff));
	}
	pFormatter->Print("</data>\n");
	//printf("\n");
}

int StssBox::GetSafeSyncSampleIdx(int iSampleIdx)
{
	int i=0;

	if(m_entry_count==0)
		RETURN_ERROR(0);

	for(i=0;i<m_entry_count;i++)
	{
		if(m_sample_number[i] > iSampleIdx)
			break;
	}

	if(i<1)
		RETURN_ERROR(-1);

	return m_sample_number[i-1];
}

int StssBox::GetClosestSyncSampleIdx(int iSampleIdx)
{
	int i=0;
	int iMinIdx = -1, iMaxIdx = -1;

	if(m_entry_count==0)
		return 0;

	//pFormatter->Put("StssBox:m_entry_count=%d\n",m_entry_count);
	for(i=0;i<m_entry_count;i++)
	{
		//pFormatter->Put("--i:%d sample_number:%d %d, %d\n",i,m_sample_number[i],iSampleIdx,m_entry_count);
		if(m_sample_number[i] > iSampleIdx)
		{
			iMaxIdx = m_sample_number[i];
			break;
		}
		iMinIdx = m_sample_number[i];
	}
	if(i==m_entry_count)
		RETURN_ERROR(-1);
	if(iMinIdx < 1)
		return iMaxIdx;
	if(iMaxIdx < 1)
		return iMinIdx;
	return 	(iSampleIdx-iMinIdx)/(double)(iMaxIdx-iMinIdx)<0.5?iMaxIdx:iMinIdx;
}

static const unsigned char KDDI_cpgd[16] =
{
	'c','p','g','d', 0xA8, 0x8C, 0x11, 0xD4, 0x81, 0x97, 0x00, 0x90, 0x27, 0x08, 0x77, 0x03,
};
static const unsigned char KDDI_mvml[16] =
{
	'm','v','m','l', 0xA8, 0x8C, 0x11, 0xD4, 0x81, 0x97, 0x00, 0x90, 0x27, 0x08, 0x77, 0x03,
};
static const unsigned char KDDI_enci[16] =
{
	'e','n','c','i', 0xA8, 0x8C, 0x11, 0xD4, 0x81, 0x97, 0x00, 0x90, 0x27, 0x08, 0x77, 0x03,
};
static const unsigned char KDDI_prop[16] =
{
	'p','r','o','p', 0xA8, 0x8C, 0x11, 0xD4, 0x81, 0x97, 0x00, 0x90, 0x27, 0x08, 0x77, 0x03,
};

void UuidBox::SetUuidType(int iType)
{
	m_UuidType = iType;
	m_version  = 0;
	m_flags	  = 0;
	switch(iType)
	{
	case UUID_CPGD:
		memcpy(m_usertype,KDDI_cpgd,16);
		break;
	case UUID_MVML:
		memcpy(m_usertype,KDDI_mvml,16);
		break;
	case UUID_ENCI:
		memcpy(m_usertype,KDDI_enci,16);
		break;
	case UUID_PROP:
		memcpy(m_usertype,KDDI_prop,16);
		break;
	default:
		break;
	}
}

int  UuidBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	if(pMp4File->GetBuffer(reinterpret_cast<char *>(m_usertype), 16)!=16)
		RETURN_ERROR(-1);
	m_version 	= pMp4File->GetChar();
	m_flags 		= pMp4File->Get24();

	if(memcmp(m_usertype, KDDI_cpgd,16)==0)
	{
		m_copy_guard_attribute = pMp4File->Get32();
		m_limit_date 	= pMp4File->Get32();
		m_limit_period = pMp4File->Get32();
		m_limit_count 	= pMp4File->Get32();
		m_UuidType		= UUID_CPGD;
		return 0;
	}
	if(memcmp(m_usertype, KDDI_mvml,16)==0)
	{
		m_permission 		= pMp4File->Get24();
		m_rec_mode			= pMp4File->GetChar();
		m_rec_date			= pMp4File->Get32();
		m_UuidType		= UUID_MVML;
		return 0;
	}
	if(memcmp(m_usertype, KDDI_enci,16)==0)
	{
		if(pMp4File->GetBuffer(m_device, 8)!=8)
			RETURN_ERROR(-1);
		m_device[8] = 0;
		if(pMp4File->GetBuffer(m_model, 8)!=8)
			RETURN_ERROR(-1);
		m_model[8] = 0;
		if(pMp4File->GetBuffer(m_encoder, 8)!=8)
			RETURN_ERROR(-1);
		m_encoder[8] = 0;
		if(pMp4File->GetBuffer(m_multiplexer, 8)!=8)
			RETURN_ERROR(-1);
		m_multiplexer[8] = 0;
		m_UuidType		= UUID_ENCI;
		return 0;
	}

	m_bNotLoaded = true;

	return 0;
}

void UuidBox::Dump(Formatter* pFormatter)
{
	int i;

	if(m_bNotLoaded)
		return;

	pFormatter->Put("<usertype>\n");
	for(i=0;i<4;i++)
		pFormatter->Put("%c",m_usertype[i]);
	for(i=4;i<16;i++)
		pFormatter->Put(" %02X",m_usertype[i]);
	pFormatter->Put("</usertype>\n");
	pFormatter->Put("<version val=\"%d\" />\n",m_version);

	if(pFormatter->UseDisplay())
		pFormatter->Print("<flags val=\"0x%x\" />\n",m_flags);
	else
		pFormatter->Put24(0,m_flags);

	if(m_UuidType==UUID_CPGD)
	{
		pFormatter->Put("<copy_guard_attribute val=\"%d\" />\n",m_copy_guard_attribute);
		pFormatter->Put("<limit_date val=\"%d\" />\n",m_limit_date);
		pFormatter->Put("<limit_period val=\"%d\" />\n",m_limit_period);
		pFormatter->Put("<limit_count val=\"%d\" />\n",m_limit_count);
		return;
	}
	if(m_UuidType==UUID_MVML)
	{
		if(pFormatter->UseDisplay())
			pFormatter->Print("<permission val=\"0x%x\" />\n",m_permission);
		else
			pFormatter->Put24(0,m_permission);
		pFormatter->Put("<rec_mode val=\"%d\" />\n",m_rec_mode);
		pFormatter->Put("<rec_date val=\"0x%x\" />\n",m_rec_date);
		return;
	}
	if(m_UuidType==UUID_ENCI)
	{
		if(pFormatter->UseDisplay())
		{
			pFormatter->Put("<device val=\"%s\" />\n",m_device);
			pFormatter->Put("<model val=\"%s\" />\n",m_model);
			pFormatter->Put("<encoder val=\"%s\" />\n",m_encoder);
			pFormatter->Put("<multiplexer val=\"%s\" />\n",m_multiplexer);
		}
		else
		{
			pFormatter->PutStream(m_device,8);
			pFormatter->PutStream(m_model,8);
			pFormatter->PutStream(m_encoder,8);
			pFormatter->PutStream(m_multiplexer,8);
		}
		return;
	}
}

int  EdtsBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box* pBox=0;
	pMp4File->SetPos(offset);

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size < 1)
			return 0;
		switch(iBoxType)
		{
		case BOX_ELST: 
			{
				pBox = new ElstBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;
		default:
			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
			if (pBox == NULL)
				RETURN_ERROR(-2);
			break;
		}
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
		}
		offset += size;	
	}

	return 0;
}

int WaveBox::Load(Mp4File * pMp4File)
{
	int iBoxType = 0;
	int offset = m_iBeginOffset+8;
	int size = 0;
	Mp4Box* pBox=0;
	pMp4File->SetPos(offset);

	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		iBoxType = pMp4File->GetBoxHead(&size);
		if(size < 1)
			return 0;

		switch(iBoxType)
		{
		case BOX_ESDS: 
			{
				pBox = new EsdsBox(offset, size, this);
				if (pBox == NULL)
					RETURN_ERROR(-2);
			}
			break;			
		default:
			pBox = new Mp4Box((BoxType)iBoxType, offset, size, this);
			if (pBox == NULL)
				RETURN_ERROR(-2);
			break;
		}
		if(pBox)
		{
			Adopt(pBox);
			int ret = pBox->Load(pMp4File);
			if (ret < 0)
				RETURN_ERROR(ret);
		}
		offset += size;	
	}

	return 0;
}

int ElstBox::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_entry_count = pMp4File->Get32();
	if(m_version==1)
	{
#ifndef TI_ARM		
		m_segment_duration = (int)pMp4File->Get64();
		m_media_time = (int)pMp4File->Get64();
#else
		m_segment_duration = (int)pMp4File->Get64().low;
		m_media_time = (int)pMp4File->Get64().low;
#endif				
	}
	else
	{
		m_segment_duration = pMp4File->Get32();
		m_media_time = pMp4File->Get32();
	}

	m_media_rate_integer = pMp4File->Get16();
	m_media_rate_fraction = pMp4File->Get16();

	return 0;
}

void ElstBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<entry_count val=\"%d\"/>\n",m_entry_count);
	pFormatter->Put("<segment_duration val=\"%d\"/>\n",m_segment_duration);
	pFormatter->Put("<media_time val=\"%d\"/>\n",m_media_time);
	pFormatter->Put("<media_rate_integer val=\"%d\"/>\n",m_media_rate_integer);
	pFormatter->Put("<media_rate_fraction val=\"%d\"/>\n",m_media_rate_fraction);
}


int IodsBox::Load(Mp4File * pMp4File)
{
	int offset=0, size=0, count=0;

	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);

	char Tag;
	Descr * pDescr=0;
	offset = m_iBeginOffset+12;	
	while(offset < m_iBeginOffset+m_iSize)
	{
		pMp4File->SetPos(offset);
		Tag = pMp4File->GetChar();
		size = GetSizeOfInstance(pMp4File,&count);
		offset += 1 + count;
		switch(Tag)
		{
		case kObjectDescrTag:
			pDescr = new ObjectDescriptor(offset, size);
			if (pDescr == NULL)
				RETURN_ERROR(-2);
			break;
		case kInitialObjectDescrTag:
			pDescr = new InitialObjectDescriptor(offset, size);
			if (pDescr == NULL)
				RETURN_ERROR(-2);
			break;
		default:
			pDescr = NULL;
			break;
		}
		if(pDescr)
		{
			pDescr->SetBoxContainer(this);
			int ret = pDescr->Load(pMp4File);
			if (ret < 0)
			{
				SAFEDELETE(pDescr);
				RETURN_ERROR(ret);
			}
			Adopt(pDescr);
		}

		offset += size;
	}

	return 0;
}

void IodsBox::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
}

#ifdef MP4_MUX
StszBoxMem::StszBoxMem(int offset, int size, Mp4Box* aContainer) 
: Mp4Box(BOX_STSZ,offset,size,aContainer)
{
	m_lTotalSampleSize = 0;

}


void StszBoxMem::Dump(Formatter* pFormatter)
{
	unsigned long total_sample_size=0;
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Print("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Print("</flags>\n");
	pFormatter->Put("<sample_size val=\"%d\"/>\n",m_sample_size);
	pFormatter->Put("<sample_count val=\"%d\"/>\n",m_sample_count);
	total_sample_size=m_sample_size*m_sample_count;
	ResetIterater();
	if(m_sample_size==0)
	{
		int indx = 0;
		long lSampleSize = 0;
		for(int i=0; i<m_sample_count; i++)
			//		while (IterateElement(&lSampleSize))
		{
			IterateElement(&lSampleSize);

			pFormatter->Print("<entry_size index=\"%d\">", ++indx);
			pFormatter->Put("%d", lSampleSize);
			pFormatter->Put("</entry_size>\n");
			if(lSampleSize < 1 || lSampleSize > 1024*1024)
				pFormatter->Print("<error>STSZ: Check!:entry_size[%d]=%d</error>\n", indx, lSampleSize);
			else if(lSampleSize > 15*1024)
				pFormatter->Print("<warning>size=%d is so big to cause decoding error on P2102V</warning>\n", lSampleSize);
			total_sample_size += lSampleSize;
		}
	}
	pFormatter->Print("<total sample=\"%d\"/>\n",(long)total_sample_size);
}

int StszBoxMem::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_sample_size = pMp4File->Get32();
	m_sample_count = pMp4File->Get32();
	if(m_sample_count < 1)
		return 0;
	ReleaseBuf();
	if(m_sample_size==0)
	{
		m_lTotalSampleSize = 0;
		for(i=0;i<m_sample_count;i++)
		{
			long lEntrySize = pMp4File->Get32();
			AddElement(lEntrySize);
			m_lTotalSampleSize += lEntrySize;
		}
	}
	else
	{
		m_lTotalSampleSize = m_sample_size * m_sample_count;
	}
	return 0;
}

StssBoxMem::StssBoxMem(int offset, int size, Mp4Box* aContainer) 
: Mp4Box(BOX_STSS,offset,size,aContainer)
{

}

int StssBoxMem::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_entry_count = pMp4File->Get32();
	if(m_entry_count < 1)
		return 0;
	ReleaseBuf();
	for(i=0;i<m_entry_count;i++)
		AddElement(pMp4File->Get32());
	return 0;
}

void StssBoxMem::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<entry_count val=\"%d\"/>\n",m_entry_count);
	ResetIterater();
	int indx = 0;
	long lSampleNum = 0;
	for(int i=0; i<m_entry_count; i++)
		//	while (IterateElement(&lSampleNum))
	{
		IterateElement(&lSampleNum);

		pFormatter->Print("<sample_number index=\"%d\">", ++indx);
		pFormatter->Put("%d", lSampleNum);
		pFormatter->Put("</sample_number>\n");
		if(lSampleNum < 1)
			pFormatter->Print("<error>STSS: Check!:sample_number[%d]=%d</error>\n", indx, lSampleNum);
		//if(indx > 1 && pTmpNode->pBuf[i] <= pTmpNode->pBuf[i-1])
		//{
		//	pFormatter->Print("<error>sample_number[%d]=%d,", indx-1 , pTmpNode->pBuf[i-1]);
		//	pFormatter->Print("sample_number[%d]=%d</error>\n", indx, pTmpNode->pBuf[i]);
		//}
	}
}

int StcoBoxMem::Load(Mp4File * pMp4File)
{
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_entry_count = pMp4File->Get32();
	if(m_entry_count < 1)
		return 0;
	for(int i=0;i<m_entry_count;i++)
		AddElement(pMp4File->Get32());
	return 0;
}

void StcoBoxMem::Dump(Formatter* pFormatter)
{
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<entry_count val=\"%d\"/>\n",m_entry_count);
	ResetIterater();
	int indx = 0;
	long lChunkOffset = 0;
	for(int i=0; i<m_entry_count; i++)
		//	while (IterateElement(&lChunkOffset))
	{
		IterateElement(&lChunkOffset);

		pFormatter->Print("<chunk_offset index=\"%d\">", ++indx);
		pFormatter->Put("%d", lChunkOffset);
		pFormatter->Put("</chunk_offset>\n");
		if(lChunkOffset < 1)
			pFormatter->Print("<error>STCO: Check!:chunk_offset[%d]=%d</error>\n", indx, lChunkOffset);
	}
}

int StscBoxMem::Load(Mp4File * pMp4File)
{
	int i=0;
	pMp4File->SetPos(m_iBeginOffset+8);
	m_version = pMp4File->GetChar();
	if(pMp4File->GetBuffer(m_flags, 3)!=3)
		RETURN_ERROR(-1);
	m_entry_count = pMp4File->Get32();
	if(m_entry_count < 1)
		return 0;
	m_FirstChunk.ReleaseBuf();
	m_SampsPerChk.ReleaseBuf();
	m_SampDscIndx.ReleaseBuf();
	for(i = 0; i < m_entry_count; i++)
	{
		m_FirstChunk.AddElement(pMp4File->Get32());
		m_SampsPerChk.AddElement(pMp4File->Get32());
		m_SampDscIndx.AddElement(pMp4File->Get32());
	}

	//
	// To fix shazia-64.mp4 issue.
	//
#define WORKAROUND_MP4_AUTHORING_ERROR
#if defined(WORKAROUND_MP4_AUTHORING_ERROR)
	long* plFirstChk = m_FirstChunk.GetPtrAt(0);
	long* plSampDscIndx = NULL;
	if(*plFirstChk == 0)
	{
		for( i = 0; i < m_entry_count; i++)
		{
			plFirstChk = m_FirstChunk.GetPtrAt(i);
			if (plFirstChk)
				(*plFirstChk)++;
			plSampDscIndx = m_SampDscIndx.GetPtrAt(i);
			if(plSampDscIndx && *plSampDscIndx == 0)
				*plSampDscIndx = 1;
		}
	}
#endif

	return 0;
}
void StscBoxMem::Dump(Formatter* pFormatter)
{
	int total_samples=0;
	pFormatter->Put("<version val=\"%d\" />\n",m_version);
	pFormatter->Put("<flags>\n");
	pFormatter->Put(0,m_flags[0]);
	pFormatter->Put(0,m_flags[1]);
	pFormatter->Put(0,m_flags[2]);
	pFormatter->Put("</flags>\n");
	pFormatter->Put("<entry_count val=\"%d\"/>\n",m_entry_count);

	int indx = 0;
	long lFirstChunk = 0;
	long lSampsPerChk = 0;
	long lSampDscIndx = 0;
	m_FirstChunk.ResetIterater();
	m_SampsPerChk.ResetIterater();
	m_SampDscIndx.ResetIterater();

	for(int i=0; i<m_entry_count; i++)
		//	while (m_FirstChunk.IterateElement(&lFirstChunk) && m_SampsPerChk.IterateElement(&lSampsPerChk) &&
		//		   m_SampDscIndx.IterateElement(&lSampDscIndx))
	{
		m_FirstChunk.IterateElement(&lFirstChunk);
		m_SampsPerChk.IterateElement(&lSampsPerChk);
		m_SampDscIndx.IterateElement(&lSampDscIndx);

		pFormatter->Print("<index val=\"%d\">\n", ++indx);
		pFormatter->Put("<first_chunk val=\"%d\"/>\n", lFirstChunk);
		pFormatter->Put("<samples_per_chunk val=\"%d\"/>\n", lSampsPerChk);
		pFormatter->Put("<sample_description_index val=\"%d\"/>\n", lSampDscIndx);
		pFormatter->Put("</index>\n");
		if(lFirstChunk < 1)
		{
			pFormatter->Print("<error>STSC:m_first_chunk[%d]=%d</error>\n", indx, lFirstChunk);
		}
		if(lSampsPerChk < 1)
		{
			pFormatter->Print("<error>STSC:m_samples_per_chunk[%d]=%d</error>\n",indx, lSampsPerChk);
		}
		if(lSampDscIndx < 1)
		{
			pFormatter->Print("<error>STSC:m_sample_description_index[%d]=%d</error>\n",indx,lSampDscIndx);
		}
		total_samples += lSampsPerChk;
	}
	pFormatter->Print("<total sample=\"%d\"/>\n",total_samples);
}
#endif
