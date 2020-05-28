#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Mp4Com.h"
#include "Mp4File.h"
#include "Mp4Ve.h"
#include "Mp4Box.h"
#include "Mp4Navigator.h"
#include "Mp4Riff.h"

int Mp4Riff::Load(Mp4File * pMp4File)
{
	ChunkSize = m_iSize - 8;
	return 0;
}

/** ChunkId and ChunkSize are already dumped in DumpHeader function.
*/
void Mp4Riff::Dump(Formatter* pFormatter)
{
}

int Mp4Riff::GetChunkIdNSize(Mp4File * pMp4File,int *pId, int * pSize)
{
	pMp4File->SetReadLittleEndian(false);
	*pId 		= pMp4File->Get32();
	pMp4File->SetReadLittleEndian(true);
	*pSize 	= pMp4File->Get32();
	return 0;
}

void Mp4Riff::SetSize(int iSize)
{
	m_iSize = iSize;
	ChunkSize = iSize - 8;
}

int RiffRiff::Load(Mp4File * pMp4File)
{
	int offset;
	int iChunkId = 0;
	int iChunkSize = 0;
	Mp4Riff * pMp4Riff=0;
	
	pMp4File->SetPos(m_iBeginOffset+8);
	
	ChunkSize 	= m_iSize - 8;
	pMp4File->SetReadLittleEndian(false);
	Format  		= pMp4File->Get32();
	offset 		= m_iBeginOffset+8+4;;
	
	while(offset+3 < m_iBeginOffset+m_iSize)
	{
		pMp4Riff = 0;
		pMp4File->SetPos(offset);
		if(GetChunkIdNSize(pMp4File,&iChunkId,&iChunkSize)<0)
			break;
		
		switch(iChunkId)
		{
		case RIFF_FMT:
			if(Format==RIFF_QLCM)
				pMp4Riff=m_pRiffQcpFmt=new RiffQcpFmt(offset,iChunkSize+8,this);
			else
				pMp4Riff=m_pRiffFmt=new RiffFmt(offset,iChunkSize+8,this);
			break;
		case RIFF_DATA:
			pMp4Riff=m_pRiffData=new RiffData(offset,iChunkSize+8,this);
			break;
		case RIFF_VRAT:
			pMp4Riff = new RiffVrat(offset,iChunkSize+8,this);
			break;
		case RIFF_LABL:
			pMp4Riff = new RiffLabl(offset,iChunkSize+8,this);
			break;
		case RIFF_OFFS:
			pMp4Riff = new RiffOffs(offset,iChunkSize+8,this);
			break;
		case RIFF_CNFG:
			pMp4Riff = new RiffCnfg(offset,iChunkSize+8,this);
			break;
		case RIFF_TEXT:
			pMp4Riff = new RiffText(offset,iChunkSize+8,this);
			break;
		default:
			pMp4Riff = new Mp4Riff(iChunkId,offset,iChunkSize+8,this);
			break;
		}
		if(NULL == pMp4Riff)
		{
			return -2;
		}
		if(pMp4Riff)
		{
			pMp4Riff->Load(pMp4File);
			Adopt(pMp4Riff);
		}
		offset += iChunkSize+8;
	}
	return 0;
}

void RiffRiff::Dump(Formatter* pFormatter)
{
	pFormatter->ConvertToBig(true);
	pFormatter->Print("<format>\n");
	if(pFormatter->UseDisplay())
		pFormatter->PrintInt((char *)&Format,4);
	else
		pFormatter->Put(0,Format);
	pFormatter->Print("\n</format>\n");
}

int RiffFmt::Load(Mp4File * pMp4File)
{
	ChunkSize 	= m_iSize - 8;
	pMp4File->SetPos(m_iBeginOffset+8);
	pMp4File->SetReadLittleEndian(true);
	AudioFormat 	= pMp4File->Get16();
	NumChannels 	= pMp4File->Get16();
	SampleRate 		= pMp4File->Get32();
	ByteRate			= pMp4File->Get32();
	BlockAlign		= pMp4File->Get16();
	BitsPerSample	= pMp4File->Get16();
	return 0;
}

void RiffFmt::Dump(Formatter* pFormatter)
{
	pFormatter->ConvertToBig(false);
	pFormatter->Put("<audioformat val=\"%d\"/>\n",AudioFormat);
	pFormatter->Put("<numchannels val=\"%d\"/>\n",NumChannels);
	pFormatter->Put("<samplerate val=\"%d\"/>\n",SampleRate);
	pFormatter->Put("<byterate val=\"%d\"/>\n",ByteRate);
	pFormatter->Put("<blockalign val=\"%d\"/>\n",BlockAlign);
	pFormatter->Put("<bitspersample val=\"%d\"/>\n",BitsPerSample);
}

int RiffQcpFmt::Load(Mp4File * pMp4File)
{
	int i;
	ChunkSize 	= m_iSize - 8;
	pMp4File->SetPos(m_iBeginOffset+8);
	pMp4File->SetReadLittleEndian(true);
	Major				= pMp4File->GetChar();
	Minor 			= pMp4File->GetChar();
	pMp4File->GetBuffer(CodecGuid,16); CodecGuid[16] = 0;
	CodecVersion 	= pMp4File->Get16();
	pMp4File->GetBuffer(CodecName,80); CodecName[80] 	= 0;
	AverageBps 		= pMp4File->Get16();
	PacketSize 		= pMp4File->Get16();
	BlockSize 		= pMp4File->Get16();
	SamplingRate 	= pMp4File->Get16();
	SamplingSize 	= pMp4File->Get16();
	NumRates 		= pMp4File->Get32();
	for(i=0;i<8;i++)
	{
		RateSize[i] = pMp4File->GetChar();
		RateOctet[i] = pMp4File->GetChar();
	}
	/** reserved 5*UINT32 */
	for(i=0;i<5;i++)
		pMp4File->Get32();
	return 0;
}

void RiffQcpFmt::Dump(Formatter* pFormatter)
{
	int i;
	ChunkSize 	= m_iSize - 8;
	pFormatter->ConvertToBig(false);
	pFormatter->Put("<major val=\"%d\"/>\n",Major);
	pFormatter->Put("<minor val=\"%d\"/>\n",Minor);
	pFormatter->Print("<codecguid>\n");
	for(i=0;i<16;i++)
		pFormatter->Put(0,CodecGuid[i]);
	pFormatter->Print("</codecguid>\n");
	pFormatter->Put("<codecversion val=\"%d\"/>\n",CodecVersion);
	pFormatter->Print("<codecname>\n");
	for(i=0;i<16;i++)
		pFormatter->Put(0,CodecName[i]);
	pFormatter->Print("</codecname>\n");
	pFormatter->Put("<averagebps val=\"%d\"/>\n",AverageBps);
	pFormatter->Put("<packetsize val=\"%d\"/>\n",PacketSize);
	pFormatter->Put("<blocksize val=\"%d\"/>\n",BlockSize);
	pFormatter->Put("<samplingrate val=\"%d\"/>\n",SamplingRate);
	pFormatter->Put("<samplingsize val=\"%d\"/>\n",SamplingSize);
	pFormatter->Put("<numrates val=\"%d\"/>\n",NumRates);
	pFormatter->Print("<rate_map_table>\n");
	for(i=0;i<8;i++)
	{
		pFormatter->Put("<ratesize val=\"%d\"/>\n",RateSize[i]);
		pFormatter->Put("<rateoctet val=\"%d\"/>\n",RateOctet[i]);
	}
	pFormatter->Print("</rate_map_table>\n");
}

int RiffVrat::Load(Mp4File * pMp4File)
{
	ChunkSize 	= m_iSize - 8;
	pMp4File->SetPos(m_iBeginOffset+8);
	VarRateFlag		= pMp4File->Get32();
	SizeInPackets	= pMp4File->Get32();
	return 0;
}

void RiffVrat::Dump(Formatter* pFormatter)
{
	pFormatter->ConvertToBig(false);
	pFormatter->Put("<varrateflag val=\"%d\"/>\n",VarRateFlag);
	pFormatter->Put("<sizeinpackets val=\"%d\"/>\n",SizeInPackets);
}

int RiffLabl::Load(Mp4File * pMp4File)
{
	int i;
	ChunkSize 	= m_iSize - 8;
	pMp4File->SetPos(m_iBeginOffset+8);
	pMp4File->SetReadLittleEndian(true);
	for(i=0;i<48;i++)
		Label[i] = pMp4File->GetChar();
	Label[48]=0;
	return 0;
}

void RiffLabl::Dump(Formatter* pFormatter)
{
	int i;
	pFormatter->ConvertToBig(false);
	pFormatter->Print("<label>\n");
	for(i=0;i<48;i++)
		pFormatter->Put(0,Label[i]);
	pFormatter->Print("</label>\n");
}

int RiffOffs::Load(Mp4File * pMp4File)
{
	int i;
	ChunkSize 	= m_iSize - 8;
	pMp4File->SetPos(m_iBeginOffset+8);
	pMp4File->SetReadLittleEndian(true);
	StepSize		= pMp4File->Get32();
	NumOffsets	= pMp4File->Get32();
	if(Offset)
		delete [] Offset;
	Offset = new long [NumOffsets];
	if(NULL == Offset)
	{
		return -2;
	}
	for(i=0;i<NumOffsets;i++)
		Offset[i] = pMp4File->Get32();
	return 0;
}

void RiffOffs::Dump(Formatter* pFormatter)
{
	int i;
	pFormatter->ConvertToBig(false);
	pFormatter->Put("<stepsize val=\"%d\"/>\n",StepSize);
	pFormatter->Put("<numoffsets val=\"%d\"/>\n",NumOffsets);
	pFormatter->Print("<offset>\n");
	for(i=0;i<NumOffsets;i++)
		pFormatter->Put(0,Offset[i]);
	pFormatter->Print("</offset>\n");
}

int RiffCnfg::Load(Mp4File * pMp4File)
{
	ChunkSize 	= m_iSize - 8;
	pMp4File->SetPos(m_iBeginOffset+8);
	pMp4File->SetReadLittleEndian(true);
	Config = pMp4File->Get16();
	return 0;
}

void RiffCnfg::Dump(Formatter* pFormatter)
{
	pFormatter->ConvertToBig(false);
	pFormatter->Put("<config val=\"0x%x\"/>\n",Config);
}

int RiffText::Load(Mp4File * pMp4File)
{
	int i;
	pMp4File->SetPos(m_iBeginOffset+8);
	for(i=0;i<4096;i++)
	{
		Text[i] = pMp4File->GetChar();
		if(Text[i]) break;
	}
	return 0;
}

void RiffText::Dump(Formatter* pFormatter)
{
	int i;
	pFormatter->ConvertToBig(false);
	pFormatter->Print("<text>\n");
	for(i=0;i<4096 && Text[i];i++)
	{
		pFormatter->Put(0,Text[i]);
	}
	pFormatter->Print("</text>\n");
}

void RiffData::Dump(Formatter* pFormatter)
{
	char buf[8192];
	int iRead;
	if(m_bNotLoaded || m_pcmfile[0]==0 || pFormatter->UseDisplay())
		return;
	
	MovieFile * pMovie = new MovieFile(m_pcmfile);
	if(NULL == pMovie)
	{
		return;
	}
	do	{
		iRead = pMovie->GetBuffer(buf, 8192);
		if(iRead < 1) break;
		pFormatter->PutStream(buf, iRead);
	} while(true);
	
	delete pMovie;
}

int RiffData::Write(Mp4File * pMp4File, char *pathname)
{
	char buf[8192];
	pMp4File->SetPos(m_iBeginOffset+8);
	int iRead;
	int iSize = ChunkSize;
	
	FileCtrl * pFileCtrl = CreateFileCtrl();
	pFileCtrl->Open(pathname, 0, FILECTRL_OPEN_WRITE);
	while(true)
	{
		if(iSize > 8192) 
			iRead = 8192;
		else
			iRead = iSize;
		iRead = pMp4File->GetBuffer(buf,iRead);
		if(iRead < 1)
			break;
		pFileCtrl->Write(buf, iRead);
		iSize -= iRead;
	}
	pFileCtrl->Close();
	delete pFileCtrl;
	
	return 0;
}
