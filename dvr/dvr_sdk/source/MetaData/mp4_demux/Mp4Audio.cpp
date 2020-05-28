#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Mp4Com.h"
#include "Mp4File.h"
#include "Mp4Ve.h"
#include "Mp4Box.h"
#include "Mp4Navigator.h"
#include "Mp4Audio.h"

int 	Mp4AudioFrameHeader::Load(Mp4File * pMp4File)
	{
	MakeSerializable(false);

	syncword 		= pMp4File->GetBits(12);
	id 				= pMp4File->GetBits(1);
	layer 			= pMp4File->GetBits(2);
	protection_bit = pMp4File->GetBits(1);
	bitrate_index 	= pMp4File->GetBits(4);
	sampling_frequency = pMp4File->GetBits(2);
	padding_bit 	= pMp4File->GetBits(1);
	private_bit 	= pMp4File->GetBits(1);
	mode 				= pMp4File->GetBits(2);
	mode_extension = pMp4File->GetBits(2);
	copyright 		= pMp4File->GetBits(1);
	original_home 	= pMp4File->GetBits(1);
	emphasis 		= pMp4File->GetBits(2);

	return 0;
	}

void 	Mp4AudioFrameHeader::Dump(Formatter* pFormatter)
	{
	if(!pFormatter->UseDisplay())
		return;	// for now, we don't support serialization of element stream.

	pFormatter->Print("<id val=\"0x%x\" />",id);
	pFormatter->Print("<layer val=\"0x%x\" />",layer);
	pFormatter->Print("<protection_bit val=\"0x%x\" />",protection_bit);
	pFormatter->Print("<bitrate_index val=\"0x%x\" />",bitrate_index);
	pFormatter->Print("<sampling_frequency val=\"0x%x\" />",sampling_frequency);
	pFormatter->Print("<padding_bit val=\"0x%x\" />",padding_bit);
	pFormatter->Print("<private_bit val=\"0x%x\" />",private_bit);
	pFormatter->Print("<mode val=\"0x%x\" />",mode);
	pFormatter->Print("<mode_extension val=\"0x%x\" />",mode_extension);
	pFormatter->Print("<copyright val=\"0x%x\" />",copyright);
	pFormatter->Print("<original_home val=\"0x%x\" />",original_home);
	pFormatter->Print("<emphasis val=\"0x%x\" />",emphasis);
	}

int Mp4AudioFrameHeader::SeekAndLoad(Mp4File * pMp4File)
	{
	int CurPos = pMp4File->GetPos();
	int iFileSize = pMp4File->GetFileSize();
	char ch = 0;

	while(CurPos < iFileSize)
		{
		ch = pMp4File->GetChar(); CurPos++;
		if((ch&0xff)==0xff)
			{
			ch = pMp4File->GetChar(); CurPos++;
			if((ch&0xf0)==0xf0)
				{
				pMp4File->SetPos(CurPos-2);
				Load(pMp4File);
				return CurPos-2;
				}
			}
		}

	return -1;
	}
