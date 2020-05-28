#include <tchar.h>
#include <stdio.h>
#include "MP4Demux.h"

#include "MovieFile.h"
#include "Mp4Com.h"
#include "Mp4File.h"
#include "Mp4Navigator.h"
#include "Mp4Box.h"

class MP4DemuxBrokerFileCtrl : public FileCtrl
{
public:
	
	MP4DemuxBrokerFileCtrl()
	{	
		m_bClosedFile = FALSE;
	}
	
	virtual ~MP4DemuxBrokerFileCtrl()
	{	
		Close();
	}	

	int Open(char* pFilename)
	{
		strcpy(m_FileName, pFilename);
		m_hFile=(void*)fopen(m_FileName, "rb");
		if(m_hFile==NULL)
			return -1;
		m_bClosedFile = TRUE;
		return 0;
	}

	/** open file */
	virtual int	Open(char * pathname,  int filesize, int mode)
	{
		return 0;
	}
	/** close file */
	virtual int Close()
	{
		if(m_bClosedFile&&m_hFile)
		{
			fclose((FILE*)m_hFile);
			m_hFile = NULL;
		}
		return 0;
	}
	/** read data into the buf */
	virtual int Read(char * buf, int nbytes)
	{
		if (nbytes <= 0 || m_hFile == NULL)
			return -1;
#if 1
		if(nbytes<m_uiAlignSize)
			return fread(buf, 1, nbytes, (FILE*)m_hFile);
		else
		{
			int iPos = Tell();
			if(iPos&(m_uiAlignSize-1))
			{
				int iRead = 0;
				iPos = m_uiAlignSize-(iPos&(m_uiAlignSize-1)); //512 bytes align
				iRead = fread(buf, 1, iPos, (FILE*)m_hFile);				
				iRead += fread(buf+iPos, 1, nbytes-iPos, (FILE*)m_hFile);
				return iRead;
			}
			else
				return fread(buf, 1, nbytes, (FILE*)m_hFile);
		}
#else
		return fread(buf, 1, nbytes, (FILE*)m_hFile);
#endif
	}
	/** write data into the file */
	virtual int Write(char * buf, int nbytes)
	{
		return 0;
	}
	/** set file reading/writing position */
	virtual int Seek(int pos)
	{
	    if(m_hFile == NULL)
            return -1;
		fseek((FILE*)m_hFile, pos, SEEK_SET);
		return 0;
	}
	/** return file reading/writing position */
	virtual int Tell()
	{
	    if(m_hFile == NULL)
            return -1;
		int iPos = ftell((FILE*)m_hFile);
		return iPos;
	}
	/** return file size */
	virtual int Size()
	{
	    if(m_hFile == NULL)
            return -1;
        struct stat stat_results;
	    if(fstat(fileno((FILE*)m_hFile),&stat_results) < 0)
        {
            printf("%s %d (%s)\n",__FILE__,__LINE__,strerror(errno));
            return 0;
        }

		return stat_results.st_size;
	}
	virtual void SetOptions(const MP4Demux_OpenOptions *pOptions)
	{
		if(pOptions)
			m_hFile = (HANDLE)pOptions->pvDataContext;
	}
	virtual void SetPosition(int pos)
	{
	}
private:
	HANDLE m_hFile;	 
	int m_bClosedFile;
	char m_FileName[MAX_PATH];	
	static const unsigned int m_uiAlignSize;
};

const unsigned int MP4DemuxBrokerFileCtrl::m_uiAlignSize = 512;

class MP4DemuxExternalFileCtrl : public MP4DemuxBrokerFileCtrl
{
public:
	/** open file */
	virtual int	Open(char * pathname,  int filesize, int mode)
	{
		return 0;
	}
	/** close file */
	virtual int Close()
	{
		return 0;
	}
	/** read data into the buf */
	virtual int Read(char * buf, int nbytes)
	{
		if (nbytes <= 0)
			return -1;
		return m_pfnExternRead(m_pvFile, buf, nbytes);
	}
	/** write data into the file */
	virtual int Write(char * buf, int nbytes)
	{
		return 0;
	}
	/** set file reading/writing position */
	virtual int Seek(int pos)
	{
		return m_pfnExternSeek(m_pvFile, pos, SEEK_SET);
	}
	/** return file reading/writing position */
	virtual int Tell()
	{
		return m_pfnExternTell(m_pvFile);
	}
	/** return file size */
	virtual int Size()
	{
		return m_pfnExternSize(m_pvFile);
	}
	virtual void SetOptions(const MP4Demux_OpenOptions *pOptions)
	{
		if (pOptions)
		{
			m_pvFile = (void*)pOptions->pvDataContext;
			m_pfnExternRead = pOptions->pfnExternRead;
			m_pfnExternSeek = pOptions->pfnExternSeek;
			m_pfnExternTell = pOptions->pfnExternTell;
			m_pfnExternSize = pOptions->pfnExternSize;
		}
	}
	virtual void SetPosition(int pos)
	{
	}
private:
	void* m_pvFile;
	PFN_MP4DEMUX_EXTERN_READ m_pfnExternRead;
	PFN_MP4DEMUX_EXTERN_SEEK m_pfnExternSeek;
	PFN_MP4DEMUX_EXTERN_TELL m_pfnExternTell;
	PFN_MP4DEMUX_EXTERN_SIZE m_pfnExternSize;
};

class Mp4DemuxStreamingCtrl : public MP4DemuxBrokerFileCtrl
{
public:
	Mp4DemuxStreamingCtrl()
	{
		m_pfnGetData = NULL;
		m_pContextCb = NULL;
		m_dwTotalSize = 0;
		m_dwOffset = 0;
		m_pBuffer = NULL;
		m_bEOS = FALSE;
	}

	virtual ~Mp4DemuxStreamingCtrl()
	{
		Close();
		m_pfnGetData = NULL;
		m_pContextCb = NULL;
		m_dwTotalSize = 0;
		m_dwOffset = 0;
		m_pBuffer = NULL;
	}

	/** open file */
	virtual int	Open(char * pathname,  int filesize, int mode)
	{
		m_dwOffset = 0;
		m_dwTotalSize = 0;
		m_dwPos = 0;
		m_bEOS = FALSE;

		if(m_pBuffer)
		{
			delete [] m_pBuffer;
			m_pBuffer = NULL;
		}
		m_pBuffer = new char[m_dwBufferSize];
		return 0;
	}
	/** close file */
	virtual int Close()
	{
		m_dwOffset = 0;
		m_dwTotalSize = 0;
		m_dwPos = 0;
		m_bEOS = FALSE;

		if(m_pBuffer)
		{
			delete [] m_pBuffer;
			m_pBuffer = NULL;
		}
		return 0;
	}
	/** read data into the buf */
	virtual int Read(char * buf, int nbytes)
	{			
		unsigned int dwToRead1, dwToRead2;	
		unsigned int dwTotalRead = 0;

		if (nbytes <= 0)
			return -1;

		if(m_bEOS)
		{
			nbytes = min(m_dwTotalSize-m_dwPos, (unsigned int)nbytes);
		}

		//internal buffer
		dwToRead1 = (m_dwTotalSize>=m_dwPos)? (m_dwTotalSize-m_dwPos) : 0;
		dwToRead1 = (dwToRead1>=(unsigned int)nbytes)? nbytes : dwToRead1;
		dwToRead2 = nbytes - dwToRead1;

		if(dwToRead1)
		{
			memcpy(buf, m_pBuffer+(m_dwPos%m_dwBufferSize), dwToRead1);
			dwTotalRead += dwToRead1;
			m_dwPos += dwToRead1;
			buf += dwToRead1;
		}

		if(dwToRead2)
		{
			while(dwTotalRead<(unsigned int)nbytes)
			{
				int hr;
				unsigned int dwThisRead;
				unsigned int dwOffset = m_dwTotalSize%m_dwBufferSize;
				dwThisRead = m_dwBufferSize - dwOffset;
				hr = LoadBuffer(dwThisRead);
				dwThisRead = min(dwToRead2, dwThisRead);
				memcpy(buf, m_pBuffer+(m_dwPos%m_dwBufferSize), dwThisRead);
				dwTotalRead += dwThisRead;
				m_dwPos += dwThisRead;
				buf += dwThisRead;
				dwToRead2 -= dwThisRead;
				if(hr==E_FAIL)
					break;
			};
		}
		return dwTotalRead > 0 ? (int)dwTotalRead : -1;
	}
	/** write data into the file */
	virtual int Write(char * buf, int nbytes)
	{
		return 0;
	}

	/** set file reading/writing position */
	virtual int Seek(int pos)
	{
		if (pos >= m_dwPos && pos < m_dwTotalSize)
		{
			m_dwPos = pos;
			m_bEOS = FALSE;
			return 0;
		}
		else
		{
			m_dwPos = m_dwTotalSize = pos;
			m_bEOS = FALSE;
			return m_pfnSeek(m_pContextCb, pos, SEEK_SET);
		}
	}


	/** return file reading/writing position */
	virtual int Tell()
	{		
		return m_dwPos;
	}
	/** return file size */
	virtual int Size()
	{		
		return INT_MAX;
	}
	virtual void SetOptions(const MP4Demux_OpenOptions *pOptions)
	{
		if(pOptions)
		{
			m_pfnGetData = pOptions->pfnDataCallback;
			m_pfnSeek = pOptions->pfnExternSeek;
			m_pContextCb = pOptions->pvDataContext;
			m_dwBufferSize = pOptions->dwBufferSize;
			if(m_dwBufferSize<=0)
				m_dwBufferSize = 256*1024;
		}
	}
	virtual void SetPosition(int pos)
	{
		m_dwTotalSize = m_dwPos = pos;
		m_bEOS = FALSE;
	}
	virtual int IsStreaming(){
		return 1;
	}

protected:
	int LoadBuffer(unsigned int &dwRead)
	{
		int hr;
		unsigned int dwOffset;
		char *pbOutBuffer = (char*)m_pBuffer;
		dwOffset = m_dwTotalSize%m_dwBufferSize;
		pbOutBuffer = (char*)m_pBuffer+dwOffset;		
		hr = m_pfnGetData(m_pContextCb, pbOutBuffer, &dwRead);
		m_dwTotalSize += dwRead;
		if(hr==E_FAIL)
			m_bEOS = TRUE;
		return hr;
	}

private:
	void *m_pContextCb;
	PFN_MP4DEMUX_GET_DATA m_pfnGetData;
	PFN_MP4DEMUX_EXTERN_SEEK m_pfnSeek;
	unsigned int m_dwOffset;
	unsigned int m_dwTotalSize;
	unsigned int m_dwPos;
	unsigned int m_dwBufferSize;
	char *m_pBuffer;
	int m_bEOS;
};

class MP4DemuxBroker
{
public:
	MP4DemuxBroker() 
		: m_p3gpFile(NULL),
		m_pVideoStream(NULL),
		m_pAudioStream(NULL),
		m_pVideoDsiExtractor(NULL),
		m_pAudioDsiExtractor(NULL),
		m_pStssBox(NULL),
		m_pFile(NULL),
		m_pMp4FileAudio(NULL),
		m_pMp4FileVideo(NULL),
		m_pFileCtrlAudio(NULL),
		m_pFileCtrlVideo(NULL),
		m_bAVdataInterleaved(TRUE)
	{
		iTotal = 0;
		m_iSpeedType = 0;
		m_iFramesCount = 0;
		m_iNextIFrame = 0;
		m_iCommand = 0;
		m_lSearchTime = 0;
		m_dwNextITS = 0;
		m_bCanSeek = TRUE;
	}
	~MP4DemuxBroker()
	{
		Close();
	}
	int Open(const MP4Demux_OpenOptions *pOptions)
	{
		bool bIsStreaming = false;

		if(pOptions->pfnDataCallback)
		{
			m_pFile = new Mp4DemuxStreamingCtrl();
			bIsStreaming = true;
		}
		else if (pOptions->pfnExternRead != NULL)
		{
			if (pOptions->pvDataContext == NULL ||
				pOptions->pfnExternSeek == NULL ||
				pOptions->pfnExternTell == NULL ||
				pOptions->pfnExternSize == NULL)
				RETURN_ERROR(-2);
			m_pFile = new MP4DemuxExternalFileCtrl;
			bIsStreaming = false;
		}
		else
		{
			m_pFile = new MP4DemuxBrokerFileCtrl();
			bIsStreaming = false;
		}

		m_bIsStreaming = bIsStreaming;

		if(m_pFile==NULL)
			RETURN_ERROR(-2);

		//m_pFile->SetOptions(pOptions);
		//m_pFile->Open(NULL, 0, 0);
        m_pFile->Open(pOptions->pFilename);
		
		//unsigned char	password[8];
		bool	bEncrypt = false;
		//const unsigned char iac_tag[8] = {0x00, 0x00, 0x00, 0x08, 0x49, 0x33, 0x47, 0x50 };
		//m_File.Read((char*)password, 8);
		//if(memcmp(password, iac_tag, 8) == 0)
		//{
		//	int filesize = 0;
		//	filesize = m_File.Size();
		//	m_File.Seek(filesize-8);
		//	m_File.Read((char*)password, 8);
		//	m_File.Seek( 8 );
		//	bEncrypt = true;
		//}
		//else
		//	m_File.Seek( 0 ) ;
		////end sunstars

		ZeroMemory(&m_StrmIfo,sizeof(m_StrmIfo));
		m_bReadVos = false;
		m_p3gpFile = new Mpeg3gpFile(m_pFile);
		if (m_p3gpFile == NULL)
		{
			Close();
			RETURN_ERROR(-2);
		}

        if(pOptions->loadall)
        {
            if(m_p3gpFile->LoadAll(bIsStreaming) != 0)
            {
                Close();
                delete m_p3gpFile;
                m_p3gpFile = NULL;
                return E_FAIL;
            }
        }
        else
        {
            if(m_p3gpFile->Load(bIsStreaming) != 0)
            {
                Close();
                delete m_p3gpFile;
                m_p3gpFile = NULL;
                return E_FAIL;
            }
		    return S_OK;
        }

		int iObjType=0;
		int nAudioObjType = 0;
		m_StrmIfo.dwAudioType=MP4DEMUX_UNKNOWN;
		memset(&m_AudioStreamInfo, 0 , sizeof(m_AudioStreamInfo));
		memset(&m_VideoStreamInfo, 0 , sizeof(m_VideoStreamInfo));
		m_pAudioStream = m_p3gpFile->GetTrackStream( HANDLER_AUDIO, bEncrypt );
		if(m_pAudioStream)
		{
			if(!bIsStreaming)
				m_pAudioStream->SetPos(0);
			m_p3gpFile->GetStreamInfo(&m_AudioStreamInfo,HANDLER_AUDIO);

			if(m_p3gpFile->IsAmrAudio())
			{
				m_StrmIfo.dwAudioType = MP4DEMUX_AMR;
			}
			else if(m_p3gpFile->IsUlawAudio())
			{
				m_StrmIfo.dwAudioType = MP4DEMUX_G711_U;
			}
			else if(m_p3gpFile->IsAlawAudio())
			{
				m_StrmIfo.dwAudioType = MP4DEMUX_G711_A;
			}
			else if(m_p3gpFile->IsImaAudio())
			{
				m_StrmIfo.dwAudioType = MP4DEMUX_IMA;
			}
			else if(m_p3gpFile->IsMp3Audio())
			{
				m_StrmIfo.dwAudioType = MP4DEMUX_MP3;
			}
			else if(m_p3gpFile->IsAlacAudio())
			{
				m_StrmIfo.dwAudioType = MP4DEMUX_ALAC;
			}
			else 
			{
				iObjType = m_p3gpFile->GetObjectTypeIndication(HANDLER_AUDIO);
				nAudioObjType = m_p3gpFile->GetAudioObjectType(HANDLER_AUDIO);
				switch(iObjType)
				{
				case 0x69:  // Audio ISO/IEC 13818-3
				case 0x6b:  // Audio ISO/IEC 11172-3
					m_StrmIfo.dwAudioType = MP4DEMUX_MP3;
					break;
				case 0x40:  // Audio ISO/IEC 14496-3
					if(nAudioObjType == 22)
					{
						m_StrmIfo.dwAudioType = MP4DEMUX_BSAC;
						break;
					}
					if(nAudioObjType == 8 || nAudioObjType == 24)
					{
						m_StrmIfo.dwAudioType = MP4DEMUX_CELP;
						break;
					}
					if(nAudioObjType == 9 || nAudioObjType == 25)
					{
						m_StrmIfo.dwAudioType = MP4DEMUX_HVXC;
						break;
					}
					//ELSE fall through
				case 0x67:  // Audio ISO/IEC 13818-7 LowComplexity Profile
					m_StrmIfo.dwAudioType = MP4DEMUX_AAC;
					break;
				case 0xC1: // Private:
					m_StrmIfo.dwAudioType = MP4DEMUX_G723;
					break;
				case 0x60: // PacketVideo's GSM-AMR -- Visual 13818-2
					m_StrmIfo.dwAudioType = MP4DEMUX_AMR_IF2;
					break;
				case 0xF0:	// Private:
					m_StrmIfo.dwAudioType = MP4DEMUX_G726_64;
					break;
				default:
					m_StrmIfo.dwAudioType = MP4DEMUX_UNKNOWN;
					break;
				}
			}

			if(m_StrmIfo.dwAudioType==MP4DEMUX_UNKNOWN)
			{
				delete m_pAudioStream;
				m_pAudioStream = NULL;
			}
		}



		m_StrmIfo.dwVideoType = MP4DEMUX_UNKNOWN;

		//check if audio and video is interleaved 
		//if it is not interleaved, create another file for video reading
		if(!m_bIsStreaming)
		{	
			TrakBox* pTrak = m_p3gpFile->GetTrack(HANDLER_VIDEO);

			if(m_pAudioStream&&pTrak)
			{
				Mp4BoxFinder BoxFinder(BOX_STCO);
				StcoBox *pStcoBox = (StcoBox *)BoxFinder.GetBox(BOX_STCO,pTrak);
				if(pStcoBox)
				{
					long lFirstPosA = m_pAudioStream->GetSamplePos(0);
					long lFirstPosV = pStcoBox->m_chunk_offset[0];					
					if(abs(lFirstPosA-lFirstPosV)>512*1024)
						m_bAVdataInterleaved = FALSE;
				}
			}
			
			if(!m_bAVdataInterleaved && pOptions->pFilename!=NULL)
			{
				m_pFileCtrlVideo = new MP4DemuxBrokerFileCtrl();
				if(m_pFileCtrlVideo)
				{
					if(m_pFileCtrlVideo->Open(pOptions->pFilename)>=0)
					{
						m_pMp4FileVideo = new Mp4File(m_pFileCtrlVideo);

						if(m_pMp4FileVideo)
							pTrak->SetSourceFile(m_pMp4FileVideo);
					}
				}
			}
		}
		m_StrmIfo.bAVdataInterleaved = m_bAVdataInterleaved;
		

		m_pVideoStream = m_p3gpFile->GetTrackStream( HANDLER_VIDEO, bEncrypt );
		if(m_pVideoStream)
		{
			if(m_p3gpFile->IsMp4Video() ) // 'mp4v' or 'mp4s' element.
			{
				m_StrmIfo.dwVideoType = MP4DEMUX_MP4VIDEO;
			}
			else if( m_p3gpFile->HasH263() ) // 'h263' or 's263'
			{
				m_StrmIfo.dwVideoType = MP4DEMUX_H263;
			}
			else if( m_p3gpFile->HasH264() ) // 'avc1'
			{
				m_StrmIfo.dwVideoType = MP4DEMUX_H264;
			}
			else
			{
				iObjType = m_p3gpFile->GetObjectTypeIndication(HANDLER_VIDEO);
				switch(iObjType)
				{
				case 0x20: // Visual ISO/IEC 14496-2 
				case 0xC2: 
					m_StrmIfo.dwVideoType = MP4DEMUX_MP4VIDEO;
					break;
				case 0x60: // Visual ISO/IEC 13818-2 Simple Profile   
				case 0x61: // Visual ISO/IEC 13818-2 Main Profile   
				case 0x62: // Visual ISO/IEC 13818-2 SNR Profile   
				case 0x63: // Visual ISO/IEC 13818-2 Spatial Profile   
				case 0x64: // Visual ISO/IEC 13818-2 High Profile 
				case 0x65: // Visual ISO/IEC 13818-2 422 Profile  
					m_StrmIfo.dwVideoType = MP4DEMUX_MPEG2VIDEO;
					break;
				case 0x6A: // Visual ISO/IEC 11172-2   
					m_StrmIfo.dwVideoType = MP4DEMUX_MPEG1VIDEO;
					break;
				case 0x6C: // Visual ISO/IEC 10918-1   
					break;				
				}
			}
			if(m_StrmIfo.dwVideoType == MP4DEMUX_UNKNOWN)
			{
				delete m_pVideoStream;
				m_pVideoStream = 0;
			}				
		}

		if(m_pVideoStream)
		{
			Mp4BoxFinder BoxFinder(BOX_STSS);
			TrakBox* pTrak = m_p3gpFile->GetTrack(HANDLER_VIDEO);
			StblBox* pStblBox = (StblBox *)BoxFinder.GetBox(BOX_STBL,(Mp4Box *)pTrak);
			m_pStssBox = (StssBox *)BoxFinder.GetBox(BOX_STSS,pStblBox);

			if(!bIsStreaming)
				m_pVideoStream->SetPos(0);
			m_pVideoDsiExtractor = m_pVideoStream->GetDsi();
			if(m_pVideoDsiExtractor)
			{
				if(m_StrmIfo.dwVideoType == MP4DEMUX_H264)
				{
					AvcBox* pAvcBox = (AvcBox*)BoxFinder.GetBox(BOX_AVC1, pStblBox);
					if(pAvcBox)
					{
						// Lin: sync code for SPS and PPS should be "0x00 00 00 01"
						// although 00 00 01 works. Also, every SPS/PPS needs a start code
						int i = 0;
						int size = 0;
						char pSyncCode[] = {0x00, 0x00, 0x00, 0x01};
						int iSyncCodeSize = sizeof(pSyncCode);
						m_pVideoDsiExtractor->m_iSize = 0;
						for (i=0; i< pAvcBox->numOfSequenceParameterSets;  i++)
						{
							m_pVideoDsiExtractor->m_iSize += iSyncCodeSize + pAvcBox->sps[i].sequenceParameterSetLength;
						}
						for (i=0; i< pAvcBox->numOfPictureParameterSets;  i++)
						{
							m_pVideoDsiExtractor->m_iSize += iSyncCodeSize + pAvcBox->pps[i].pictureParameterSetLength;
						}
						if (m_pVideoDsiExtractor->m_iSize > 0)
						{
							m_pVideoDsiExtractor->m_info = new char[m_pVideoDsiExtractor->m_iSize];
							if (m_pVideoDsiExtractor->m_info == NULL)
							{
								Close();
								RETURN_ERROR(-2);
							}
							for (i=0; i< pAvcBox->numOfSequenceParameterSets;  i++)
							{
								memcpy(m_pVideoDsiExtractor->m_info+size, pSyncCode, iSyncCodeSize);
								size += iSyncCodeSize;
								memcpy( m_pVideoDsiExtractor->m_info + size, 
									pAvcBox->sps[i].sequenceParameterSetNALUnit,
									pAvcBox->sps[i].sequenceParameterSetLength);
								size += pAvcBox->sps[i].sequenceParameterSetLength;
							}
							for (i=0; i< pAvcBox->numOfPictureParameterSets;  i++)
							{
								memcpy(m_pVideoDsiExtractor->m_info+size, pSyncCode, iSyncCodeSize);
								size += iSyncCodeSize;
								memcpy( m_pVideoDsiExtractor->m_info + size, 
									pAvcBox->pps[i].pictureParameterSetNALUnit,
									pAvcBox->pps[i].pictureParameterSetLength);
								size += pAvcBox->pps[i].pictureParameterSetLength;
							}
						}
						//gavin : get buffer length size information from AVCC box
						if(pAvcBox->lengthSizeMinusOne > 0)
						{
							m_StrmIfo.dwAVCCLengthSize = pAvcBox->lengthSizeMinusOne + 1;
						}
						else
						{
							m_StrmIfo.dwAVCCLengthSize = 0;
						}
					}
				}
				m_bReadVos = m_pVideoDsiExtractor->m_iSize > 0;
				m_StrmIfo.pcbVideoDsiBuffer		= m_pVideoDsiExtractor->m_info;
				m_StrmIfo.dwVideoDsiBufferSize	= m_pVideoDsiExtractor->m_iSize;
			}
			m_p3gpFile->GetStreamInfo(&m_VideoStreamInfo,HANDLER_VIDEO);
			//m_pVideoStream->SetSamplePosition(1); // read from the first sample
			m_pVideoStream->m_iCurSample = 1;
		}

		if(m_pAudioStream)
		{
			if (m_StrmIfo.dwAudioType == MP4DEMUX_ALAC)
			{
				Mp4BoxFinder BoxFinder(BOX_STCO);
				TrakBox* pTrak = m_p3gpFile->GetTrack(HANDLER_AUDIO);
				AlacBox* pAlacBox = (AlacBox*)BoxFinder.GetBox(BOX_ALAC, pTrak);
				if(pAlacBox)
				{
					m_pAudioDsiExtractor = m_pAudioStream->GetDsi();
					m_pAudioDsiExtractor->m_iSize = 36;
					m_pAudioDsiExtractor->m_info = new char[36];
					memcpy(m_pAudioDsiExtractor->m_info, pAlacBox->m_extra_data, 36);
					m_StrmIfo.pcbAudioDsiBuffer		= m_pAudioDsiExtractor->m_info;
					m_StrmIfo.dwAudioDsiBufferSize	= m_pAudioDsiExtractor->m_iSize;
				}
			}
			else
			{
				m_pAudioDsiExtractor = m_pAudioStream->GetDsi();
				if(m_pAudioDsiExtractor)
				{
					m_StrmIfo.pcbAudioDsiBuffer		= m_pAudioDsiExtractor->m_info;
					m_StrmIfo.dwAudioDsiBufferSize	= m_pAudioDsiExtractor->m_iSize;
				}
				m_p3gpFile->GetStreamInfo(&m_AudioStreamInfo,HANDLER_AUDIO);
				//m_pAudioStream->SetSamplePosition(1); // read from the first sample
				m_pAudioStream->m_iCurSample = 1;
			}
		}
		if(m_pVideoStream)
		{
			m_StrmIfo.dFrameRate		= m_VideoStreamInfo.m_fFrameRate;
			m_StrmIfo.dwWidth			= m_VideoStreamInfo.m_iVideoWidth;
			m_StrmIfo.dwHeight			= m_VideoStreamInfo.m_iVideoHeight;
			m_StrmIfo.dwTotalFrames		= m_VideoStreamInfo.m_iNumSamples;
			m_StrmIfo.dwVideoTimeScale	= m_VideoStreamInfo.m_iTimeScale;
			m_StrmIfo.dwVideoStreamSize	= m_VideoStreamInfo.m_iStreamSize;
			m_StrmIfo.dwVideoDuration	= m_VideoStreamInfo.m_iDuration;
			m_StrmIfo.dwMaxVideoBitRate	= m_VideoStreamInfo.m_iMaxBitRate;
			m_StrmIfo.dwMaxVideoSampleSize	= m_VideoStreamInfo.m_iMaxSampleSize + m_pVideoDsiExtractor->m_iSize;
		}

		if(m_pAudioStream)
		{
			m_StrmIfo.dwSamplesPerSec	= m_AudioStreamInfo.m_iFrequency;
			m_StrmIfo.dwChannels		= m_AudioStreamInfo.m_iNumChannels;
			m_StrmIfo.dwBitsPerSample	= m_AudioStreamInfo.m_iBitSize;
			m_StrmIfo.dwAudioTimeScale	= m_AudioStreamInfo.m_iTimeScale;
			m_StrmIfo.dwAudioStreamSize	= m_AudioStreamInfo.m_iStreamSize;
			m_StrmIfo.dwAudioDuration	= m_AudioStreamInfo.m_iDuration;
			m_StrmIfo.dwMaxAudioBitRate	= m_AudioStreamInfo.m_iMaxBitRate;
			m_StrmIfo.dwMaxAudioSampleSize	= m_AudioStreamInfo.m_iMaxSampleSize + m_pAudioDsiExtractor->m_iSize;
		}


		//
		// ???: 3GP AMR only allows mono(?).
		//
		// QuickTime authored 3gp file's channelcount is set to 2
		// but actual audio stream is MONO.
		//
		if(m_StrmIfo.dwAudioType==MP4DEMUX_AMR_IF2 || m_StrmIfo.dwAudioType==MP4DEMUX_AMR)
			m_StrmIfo.dwChannels = 1;

#ifdef IAC_ENCRYPT		//sunstars IAC 3gp
		if(bEncrypt)
		{
			if(m_pAudioStream)
				m_pAudioStream->EnableEncrypt(bEncrypt, (char*)password, m_p3gpFile->m_mdat_pos);
			if(m_pVideoStream)
				m_pVideoStream->EnableEncrypt(bEncrypt, (char*)password, m_p3gpFile->m_mdat_pos);
		}
#endif		//end sunstars
		return S_OK;
	}

	int Close()
	{
		if(m_pVideoStream)
			delete m_pVideoStream;
		m_pVideoStream = 0;
		if(m_pAudioStream)
			delete m_pAudioStream;
		m_pAudioStream = 0;
		if(m_p3gpFile)
			delete m_p3gpFile;
		m_p3gpFile = 0;
		if(m_pVideoDsiExtractor)
			delete m_pVideoDsiExtractor;
		m_pVideoDsiExtractor = 0;
		if(m_pAudioDsiExtractor)
			delete m_pAudioDsiExtractor;
		m_pAudioDsiExtractor = 0;		

		if(m_pMp4FileAudio)
		{
			delete m_pMp4FileAudio;
			m_pMp4FileAudio = NULL;
		}

		if(m_pMp4FileVideo)
		{
			delete m_pMp4FileVideo;
			m_pMp4FileVideo = NULL;
		}

		if(m_pFile)
		{
			delete m_pFile;
			m_pFile = NULL;
		}

		if(m_pFileCtrlAudio)
		{
			delete m_pFileCtrlVideo;
			m_pFileCtrlVideo = NULL;
		}

		if(m_pFileCtrlVideo)
		{
			delete m_pFileCtrlVideo;
			m_pFileCtrlVideo = NULL;
		}

		return S_OK;
	}

	int	ReadAudio(
		OUT char *pbOutBuffer,
		OUT unsigned int *pcbNumberOfBytesRead
		)
	{
		unsigned int dwRead = *pcbNumberOfBytesRead;
		int iRead;
		if(m_pAudioStream==NULL)
			return E_FAIL;
		iRead = m_pAudioStream->Read(reinterpret_cast<char *>(pbOutBuffer), dwRead);
		if(iRead < 1)
			return E_FAIL;
		*pcbNumberOfBytesRead = iRead;
		return S_OK;
	}

	int	ReadVideo(
		OUT char *pbOutBuffer,
		OUT unsigned int *pcbNumberOfBytesRead
		)
	{
		unsigned int dwRead = *pcbNumberOfBytesRead;
		int iRead;
		int iNextIFrame = 0;
		if(m_pVideoStream==NULL)
			return E_FAIL;
		if(m_bReadVos)
		{
			m_bReadVos = false;

			if(m_pVideoDsiExtractor->m_iSize > 12 && (int)dwRead>m_pVideoDsiExtractor->m_iSize)
			{
				memcpy((char *)pbOutBuffer, m_pVideoDsiExtractor->m_info, m_pVideoDsiExtractor->m_iSize);
				*pcbNumberOfBytesRead = m_pVideoDsiExtractor->m_iSize;
				return S_OK;
			}
		}
		if(m_iCommand != 0)
		{
			m_iCommand = 0;
			if(m_iSpeedType < 0)
				m_pVideoStream->SetCurrentSampleIdx(m_pVideoStream->GetCurrentSampleIdx() - 15);
			if( m_pVideoStream->SeekToI(m_iSpeedType>0 ? SEEKI_FORWARD : SEEKI_BACKWARD) == 1)
				return E_FAIL;
		}

		iRead = m_pVideoStream->Read(reinterpret_cast<char *>(pbOutBuffer), dwRead);
		if(iRead < 1)
			return E_FAIL;

		*pcbNumberOfBytesRead = iRead;
		return S_OK;
	}

	int GetStreamInfo(OUT MP4Demux_StreamInfo *pStreamInfo)
	{
		*pStreamInfo = m_StrmIfo;
		return S_OK;
	}

	int GetPositions(
		OUT MP4Demux_Positions *pAudioPosition,
		OUT MP4Demux_Positions *pVideoPosition
		)
	{
		if(pVideoPosition && m_pVideoStream)
		{
			long lSearchTime = m_pVideoStream->GetSampleCts(m_pVideoStream->GetCurrentSampleIdx());
			if (lSearchTime < 0)
			{
				pVideoPosition->dwTime1khz = 0;
				pVideoPosition->dwByte = 0;
				pVideoPosition->dwSampleIdx = 0;
				return S_FALSE;
			}
			pVideoPosition->dwTime1khz = (long)((__int64)lSearchTime * 1000./m_VideoStreamInfo.m_iTimeScale) + 1;
			pVideoPosition->dwSampleIdx = m_pVideoStream->GetCurrentSampleIdx();
			pVideoPosition->dwByte = m_pVideoStream->GetSamplePos(m_pVideoStream->GetCurrentSampleIdx());				
		}
		if(pAudioPosition && m_pAudioStream)
		{
			long lSearchTime = m_pAudioStream->GetSampleCts(m_pAudioStream->GetCurrentSampleIdx());
			if (lSearchTime < 0)
			{
				pAudioPosition->dwTime1khz = 0;
				pAudioPosition->dwByte = 0;
				pAudioPosition->dwSampleIdx = 0;
				return S_FALSE;
			}
			pAudioPosition->dwTime1khz = (long)((__int64)lSearchTime * 1000./m_AudioStreamInfo.m_iTimeScale);
			pAudioPosition->dwSampleIdx = m_pAudioStream->GetCurrentSampleIdx();
			pAudioPosition->dwByte = m_pAudioStream->GetSamplePos(m_pAudioStream->GetCurrentSampleIdx());
		}

		return S_OK;
	}

	int SetPositions(
		IN unsigned int POS,
		IN OUT MP4Demux_Positions *pAudioPosition,
		IN OUT MP4Demux_Positions *pVideoPosition
		)
	{
		int iSyncSampleIdx=0;
		long lSearchTime;
		bool bAudioOK = true;
		bool bVideoOK = true;
		m_bReadVos = true;
		if(pVideoPosition && m_pVideoStream)
		{
			switch(POS)
			{
			case MP4DEMUX_POS_SAMPLE:
				m_pVideoStream->SetSamplePosition(pVideoPosition->dwSampleIdx);
				break;
			case MP4DEMUX_POS_BYTE:
				m_pVideoStream->SetPos(pVideoPosition->dwByte);
				break;
			case MP4DEMUX_POS_1KHZ:
				{
					int NextIFrame = 0;
					lSearchTime = pVideoPosition->dwTime1khz;
					iSyncSampleIdx = m_pVideoStream->GetSampleIdx(lSearchTime);
					if(iSyncSampleIdx < 1)
					{
						bAudioOK = false;
						break;
					}
					int iCurSample = m_pVideoStream->GetCurrentSampleIdx();
					m_pVideoStream->SetCurrentSampleIdx(iSyncSampleIdx);
					if(iSyncSampleIdx > 5)
					{
						iSyncSampleIdx = m_pVideoStream->SeekToI(SEEKI_BACKWARD, m_bIsStreaming);
						if(iSyncSampleIdx < 1)
						{
							//fail to parse I frame in system layer, we just set the sample current searching
							iSyncSampleIdx = m_pVideoStream->GetSampleIdx(lSearchTime);
							m_pVideoStream->SetSamplePosition(iSyncSampleIdx);
							//m_pVideoStream->SetSamplePosition(iCurSample);
							//bAudioOK = false;
							//break;
						}
					}
					if(iSyncSampleIdx <= 5)
						m_pVideoStream->SetSamplePosition(1);

					lSearchTime = m_pVideoStream->GetSampleCts(iSyncSampleIdx);
					pVideoPosition->dwTime1khz = (long)((__int64)lSearchTime * 1000./m_VideoStreamInfo.m_iTimeScale) + 1;
					m_dwNextITS = 0;
					break;
				}
			default:
				return E_FAIL;
			}
		}
		else
		{
			bVideoOK = false;
		}

		if(pAudioPosition && m_pAudioStream)
		{
			switch(POS)
			{
			case MP4DEMUX_POS_SAMPLE:
				m_pAudioStream->SetSamplePosition(pAudioPosition->dwSampleIdx);
				break;
			case MP4DEMUX_POS_BYTE:
				if(pAudioPosition->dwByte >= 0)
				{
					unsigned int foundIdx = 1;
					if(pAudioPosition->dwByte <  m_pAudioStream->GetSamplePos(1))
					{
						foundIdx = 1;
					}
					else if (pAudioPosition->dwByte >  m_pAudioStream->GetSamplePos(m_pAudioStream-> m_pStszBox->m_sample_count ))
					{
						foundIdx = m_pAudioStream-> m_pStszBox->m_sample_count ;
					}
					else
					{
						for(unsigned int i  = 1; i <=  m_pAudioStream-> m_pStszBox->m_sample_count; i++)
						{
							if(pAudioPosition->dwByte <= m_pAudioStream->GetSamplePos(i))
							{
								foundIdx = i;
								break;
							}
						}
					}
					pAudioPosition->dwByte = m_pAudioStream->GetSamplePos(foundIdx);
					m_pAudioStream->SetSamplePosition(foundIdx);
					lSearchTime = m_pAudioStream->GetSampleCts(foundIdx);
					pAudioPosition->dwTime1khz = (long)((__int64)lSearchTime * 1000./m_AudioStreamInfo.m_iTimeScale);
				}
				break;
			case MP4DEMUX_POS_1KHZ:
				{
					if (pVideoPosition && pVideoPosition->dwTime1khz)
						lSearchTime = pVideoPosition->dwTime1khz; // for A/V sync after reset video demux position
					else 
						lSearchTime = pAudioPosition->dwTime1khz;

					iSyncSampleIdx = m_pAudioStream->GetSampleIdx(lSearchTime);
					if(iSyncSampleIdx < 1)
					{
						bVideoOK = false;
						break;
					}
					m_pAudioStream->SetSamplePosition(iSyncSampleIdx);
					lSearchTime = m_pAudioStream->GetSampleCts(iSyncSampleIdx);
					pAudioPosition->dwTime1khz = (long)((__int64)lSearchTime * 1000./m_AudioStreamInfo.m_iTimeScale);
					break;
				}
			default:
				return E_FAIL;
			}
		}
		else
		{
			bAudioOK = false;
		}

		m_iCommand = 0;

		if(m_bIsStreaming)
		{
			if(bVideoOK || bAudioOK)
			{
				unsigned int dwPos = 0;
				MP4Demux_Positions VidPos, AudPos;
				memset(&VidPos, 0, sizeof(VidPos));
				memset(&AudPos, 0, sizeof(AudPos));

				GetPositions(&AudPos, &VidPos);

				if(bVideoOK&&bAudioOK)
					dwPos = VidPos.dwByte<AudPos.dwByte?VidPos.dwByte:AudPos.dwByte;
				else if(bVideoOK)
					dwPos = VidPos.dwByte;
				else
					dwPos = AudPos.dwByte;

				m_pFile->SetPosition(dwPos);
			}
		}
		return bVideoOK || bAudioOK ? S_OK: E_FAIL;
	}

	int	ReadAudioSample(
		OUT char *pbOutBuffer,
		OUT unsigned int *pcbNumberOfBytesRead,
		OUT unsigned int * plCts
		)
	{
		unsigned int dwRead = *pcbNumberOfBytesRead;
		int iRead;
		if(m_pAudioStream==NULL)
			return E_FAIL;
		iRead = m_pAudioStream->ReadSample(reinterpret_cast<char *>(pbOutBuffer), dwRead, plCts);
		if(iRead < 1)
			return E_FAIL;
		*pcbNumberOfBytesRead = iRead;
		return S_OK;
	}

	int	ReadVideoSample(
		OUT char *pbOutBuffer,
		OUT unsigned int *pcbNumberOfBytesRead,
		OUT unsigned int * plCts,
		OUT int * pbIsKeyFrame
		)
	{
		int iRead;
		unsigned int dwRead = *pcbNumberOfBytesRead;
		if(m_pVideoStream==NULL)
			return E_FAIL;
		*pcbNumberOfBytesRead = 0;
		if(m_bReadVos || m_pVideoStream->GetCurrentSampleIdx() == 1)
		{
			m_bReadVos = false;
			if(m_pVideoDsiExtractor->m_iSize > 12 && (int)dwRead>m_pVideoDsiExtractor->m_iSize)
			{
				memcpy((char *)pbOutBuffer, m_pVideoDsiExtractor->m_info, m_pVideoDsiExtractor->m_iSize);
				*pcbNumberOfBytesRead = m_pVideoDsiExtractor->m_iSize;
			}
		}
		if(m_iCommand != 0)
		{
			m_iCommand = 0;
			int iCurSample = m_pVideoStream->GetCurrentSampleIdx();
			if(m_iSpeedType < 0)
				m_pVideoStream->SetCurrentSampleIdx(m_pVideoStream->GetCurrentSampleIdx() - 15);
			if( m_pVideoStream->SeekToI(m_iSpeedType>0 ? SEEKI_FORWARD : SEEKI_BACKWARD) < 1)
			{
				m_pVideoStream->SetSamplePosition(iCurSample);
				return E_FAIL;
			}
		}

		iRead = m_pVideoStream->ReadSample(reinterpret_cast<char *>(pbOutBuffer+*pcbNumberOfBytesRead), 
			dwRead-*pcbNumberOfBytesRead, plCts);
		if(iRead < 5)
			return E_FAIL;

		// Lin: fix H.264 bug, insert start code for each SEI and NAL unit here
		// actually we just replace, not insert, for performance reason.
		// start code for NAL is 0x 00 00 01, not 0x 00 00 00 01
		//static int iTotal = 0;
		unsigned int len;
		if(m_StrmIfo.dwVideoType == MP4DEMUX_H264 &&  0!= m_StrmIfo.dwAVCCLengthSize)
		{
			// Here iTotal is negative value or zero.
			unsigned char *pBuf = (unsigned char*)pbOutBuffer+*pcbNumberOfBytesRead - iTotal ;
			iTotal = iRead + iTotal;
			while(iTotal>0)
			{	
				//if m_StrmIfo.dwAVCCLengthSize = 0, we only add NALU sync header,
				//other case, we first find each unit then add NALU sync header
				int nBufLenSize = m_StrmIfo.dwAVCCLengthSize;
				len = 0;
				for(int i = 0; i < nBufLenSize; i++)// little endian
				{
					if(i != 0)
					{
						len <<= 8;
					}
					len |= pBuf[i];
				}

				if(iTotal < len)
				{
					break;
				}
				
				char pStartCode[] = { 0x00, 0x00, 0x01}; 
				//gavin:add in the start code ,revise all the data.
				memmove(pBuf + sizeof(pStartCode), pBuf + nBufLenSize, iTotal - nBufLenSize);
				memcpy(pBuf, pStartCode, sizeof(pStartCode));
				//next buffer has been moved sizeof(pStartCode)-nBufLenSize
				//origin next buffer pointer was pBuf + len + nBufLensize 
				// so new next buffer pointer will be pBuf + len +nBufLenSize + sizeof(pStartCode)-nBufLenSize = pBuf + len + sizeof(pStartCode);
				pBuf += (len + sizeof(pStartCode));			
				iTotal -= (nBufLenSize + len);
				*pcbNumberOfBytesRead += sizeof(pStartCode) - nBufLenSize;
			} 
			// Lin: add a start code at the end of the frame, our h.264 decoder desires it
			// Vincent: Only if iTotal equals zero , start code need to be added.
			//			That iTotal is not zero means the frame size is larger than MAX_VIDEO_READ_BUFFER_SIZE.
			//			So start code will be added next time.
			//if (iTotal == 0)
			//{
			//	char pStartCode[] = {0x00, 0x00, 0x01}; 
			//	memcpy(pBuf, pStartCode, sizeof(pStartCode));
			//	*pcbNumberOfBytesRead += sizeof(pStartCode);
			//}
		}
		*pcbNumberOfBytesRead += iRead - iTotal;
		iTotal = 0;
		if (pbIsKeyFrame != NULL)
		{
			*pbIsKeyFrame = FALSE;
			unsigned char* buf = (unsigned char*)pbOutBuffer;
			if (m_pVideoStream->m_VideoType == BOX_MP4V ||
				m_pVideoStream->m_VideoType == BOX_DIVX ||
				m_pVideoStream->m_VideoType == BOX_S263 ||
				m_pVideoStream->m_VideoType == BOX_XVID)
			{
				bool bVOP = buf[0]==0 && buf[1]==0 && buf[2]==1;
				unsigned long FrameType = (buf[4]>>6)&0x3;			
				if (bVOP)
				{
					if ((buf[3]==0xB6 && FrameType==0) ||  // I_VOP
						buf[3]==0xB3 ||  // GOP, follow perhaps needs to check inside
						buf[3]==0xB0 ||  //visual_object_sequence_start_code
						buf[3]==0xB5 ||  //visual_object_start_code
						buf[3]<=0x1f ||  //video_object_start_code
						buf[3]>=0x20 && buf[3]<=0x2f)  //video_object_layer_start_code
					{
						*pbIsKeyFrame = TRUE;
					}
				}
				else if (buf[0]==0 && buf[1]==0 && (buf[2]&0xfc)==0x80 && (buf[4]&0x2)==0)  //h.263
				{
					*pbIsKeyFrame = TRUE;
				}
			}
			else if (m_pVideoStream->m_VideoType == BOX_AVC1)
			{
				const unsigned char nal_ref_idc_mask = 0x60; 
				const unsigned char nal_unit_type_mask = 0x1f;
				unsigned char cNalFirstByte = 0xff;
				unsigned char nal_type = 0;
				// bypass start code
				for (long i = 0; i < *pcbNumberOfBytesRead-5; i++)
				{
					if (buf[i] == 0 && buf[i+1] == 0 &&
						(buf[i+2] == 1 || (buf[i+2] == 0 && buf[i+3] == 1)))
					{
						if (buf[i+2] == 1)
							cNalFirstByte = buf[i+3];
						else
							cNalFirstByte = buf[i+4];
						nal_type = cNalFirstByte&nal_unit_type_mask;
						if (nal_type == 5)
						{
							*pbIsKeyFrame = TRUE;
							break;
						}
						else if (nal_type < 5)
						{
							break;
						}
					}
				}
			}
			else if (m_pStssBox != NULL)
			{
				for (long i = 0; i < m_pStssBox->m_entry_count; i++)
				{
					if (m_pStssBox->m_sample_number[i] == m_pVideoStream->m_iCurSample)
					{
						*pbIsKeyFrame = TRUE;
						break;
					}
					else if (m_pStssBox->m_sample_number[i] > m_pVideoStream->m_iCurSample)
						break;
				}
			}
		}

		return S_OK;
	}

	int GetNextSampleType(OUT unsigned int * pdwNextSampleType)
	{
		if (pdwNextSampleType == NULL)
			return E_FAIL;
		*pdwNextSampleType = 0;

		// if there is only video or audio
		if(!m_pVideoStream || !m_pAudioStream)
		{
			if(m_pVideoStream)
				*pdwNextSampleType = 1;
			else if(m_pAudioStream)
				*pdwNextSampleType = 2;
			return S_OK;
		}
		
		if(!m_bIsStreaming)
		{
			int iCurrPos = m_p3gpFile->GetPos();
			Mp4Chunk * pVideoChunk = m_pVideoStream->m_pChunk+m_pVideoStream->m_iCurChunk-1;
			if (iCurrPos >= pVideoChunk->m_BeginOffset && iCurrPos < pVideoChunk->m_EndOffset)
			{
				// file pointer is in current video chunk
				*pdwNextSampleType = 1;
			}
			else if (m_pVideoStream->m_iCurChunk < m_pVideoStream->m_iNumChunk)
			{
				pVideoChunk++;
				if (iCurrPos >= pVideoChunk->m_BeginOffset && iCurrPos < pVideoChunk->m_EndOffset)
					*pdwNextSampleType = 1;
			}
			Mp4Chunk * pAudioChunk = m_pAudioStream->m_pChunk+m_pAudioStream->m_iCurChunk-1;
			if (iCurrPos >= pAudioChunk->m_BeginOffset && iCurrPos < pAudioChunk->m_EndOffset)
			{
				// file pointer is in current audio chunk
				*pdwNextSampleType = 2;
			}
			else if (m_pAudioStream->m_iCurChunk < m_pAudioStream->m_iNumChunk)
			{
				pAudioChunk++;
				if (iCurrPos >= pAudioChunk->m_BeginOffset && iCurrPos < pAudioChunk->m_EndOffset)
					*pdwNextSampleType = 2;
			}
		}
		else
		{
			int iCurrPos = m_pFile->Tell();
			long lSamplePosV, lSamplePosA;
			int iSampleIdx;

			lSamplePosV = lSamplePosA = m_pFile->Size();
			
			//video
			iSampleIdx = m_pVideoStream->GetCurrentSampleIdx();
			lSamplePosV = m_pVideoStream->GetSamplePos(iSampleIdx);
			
			//audio
			iSampleIdx = m_pAudioStream->GetCurrentSampleIdx();
			lSamplePosA = m_pAudioStream->GetSamplePos(iSampleIdx);
			
			//choose the smaller one 
			if(lSamplePosV<lSamplePosA)
				*pdwNextSampleType = 1;
			else
				*pdwNextSampleType = 2;
		}
		
		return S_OK;
	}

	int GetUserData( IN OUT MP4Demux_UserData* pUserData )
	{
		if (m_p3gpFile->m_pMoovBox == NULL)
			return E_FAIL;
		int iRet = m_p3gpFile->m_pMoovBox->GetUdtaData(pUserData);
		return iRet==0?S_OK:E_FAIL;
	}

	int GetDuration( IN OUT int * duration )
	{
		if (m_p3gpFile->m_pMoovBox == NULL)
			return E_FAIL;

        long Precision = 0;
		long i_duration = m_p3gpFile->GetDuration(&Precision);
        if(i_duration <= 0)
            return E_FAIL;
		*duration = (int)(1000 * i_duration / Precision);
	}

	int ReadCan(
		OUT char *pbOutBuffer,
		OUT unsigned int *pcbNumberOfBytesRead
		)
	{
		if (m_p3gpFile->m_pMoovBox == NULL)
			return E_FAIL;

		int iRet = m_p3gpFile->m_pMoovBox->GetCanData(pbOutBuffer, pcbNumberOfBytesRead);
		return iRet == 0 ? S_OK : E_FAIL;
	}

	int GetNextSampleSize(IN unsigned int dwSampleType, OUT unsigned int * pdwNextSampleSize)
	{
		if (pdwNextSampleSize == NULL)
			return E_FAIL;
		*pdwNextSampleSize = 0;

		if(dwSampleType==0)
		{
			if(!m_pAudioStream)
				return S_OK;

			int iSampleIdx = m_pAudioStream->GetCurrentSampleIdx();
			*pdwNextSampleSize = m_pAudioStream->GetSampleSize(iSampleIdx);
			
		}
		else if(dwSampleType==1)
		{
			if(!m_pVideoStream)
				return S_OK;

			if(m_bReadVos || m_pVideoStream->GetCurrentSampleIdx() == 1)
			{				
				if(m_pVideoDsiExtractor->m_iSize > 12)
				{
					*pdwNextSampleSize += m_pVideoDsiExtractor->m_iSize;
				}
			}

			int iSampleIdx = m_pVideoStream->GetCurrentSampleIdx();
			*pdwNextSampleSize += m_pVideoStream->GetSampleSize(iSampleIdx);
		}		
		return S_OK;
	}

protected:
	Mpeg3gpFile * m_p3gpFile;
	MP4DemuxBrokerFileCtrl* m_pFile;
	Mp4TrackStream* m_pAudioStream;
	Mp4DsiExtractor* m_pVideoDsiExtractor;
	Mp4DsiExtractor* m_pAudioDsiExtractor;
	int	m_MaxAudioRead;
	int	m_MaxVideoRead;
	MP4Demux_StreamInfo m_StrmIfo;
	StreamInfo	m_AudioStreamInfo;
	bool		m_bReadVos;
	int			iTotal;
	unsigned int       m_dwNextITS;
	int        m_bCanSeek;
	int 		m_bIsStreaming;
	//add by john
public:
	Mp4TrackStream* m_pVideoStream;

	StssBox* m_pStssBox;
	StszBox* m_pStszBox;
	int m_iFramesCount;
	int m_iSpeedType;
	int m_iNextIFrame;
	int m_iCommand;
	long m_lSearchTime;
	StreamInfo	m_VideoStreamInfo;
	int  m_bAVdataInterleaved;

protected:
	MP4DemuxBrokerFileCtrl* m_pFileCtrlAudio;
	MP4DemuxBrokerFileCtrl* m_pFileCtrlVideo;
	Mp4File *m_pMp4FileAudio;
	Mp4File *m_pMp4FileVideo;
};

int
MP4Demux_Create(
				OUT void **ppDecoder
				)
{
	if(!ppDecoder)
		return E_FAIL;
	*ppDecoder = new MP4DemuxBroker;
	if (*ppDecoder == NULL)
	{
		RETURN_ERROR(-2);
	}
	return S_OK;
}

int
MP4Demux_Release(
					IN void *pDecoder
				 )
{
	MP4DemuxBroker *p = reinterpret_cast<MP4DemuxBroker *>(pDecoder);
	delete p;
	return S_OK;
}


int
MP4Demux_Open(
				IN void *pDecoder,
			  IN const MP4Demux_OpenOptions *pOptions,
			  IN const unsigned int dwSize
			  )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->Open(pOptions);
}

int
MP4Demux_GetIFramesInfo(
						IN void *pDecoder,
						IN int iFramesInfo[],
						IN const unsigned int dwSize )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	memcpy(iFramesInfo,pBroker->m_pStssBox->m_sample_number,
		pBroker->m_pStssBox->m_entry_count * sizeof(long));
	return S_OK;
}
int
MP4Demux_Close(
				IN void *pDecoder
			   )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->Close();
}

int
MP4Demux_ReadVideo(
					IN void * pDecoder,
				   OUT char *pbOutBuffer,
				   OUT unsigned int *pcbNumberOfBytesRead
				   )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->ReadVideo(pbOutBuffer, pcbNumberOfBytesRead);
}

int
MP4Demux_SetPara(
				IN void * pDecoder,
				 IN int mode,
				 IN int para
				 )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	if(mode==0)	
	{
		pBroker->m_iSpeedType = para;
		if(para)	
			pBroker->m_iCommand = 1;	
	}
	else
		pBroker->m_iCommand = 1;

	return S_OK;
}

int
MP4Demux_GetPara(
				IN void * pDecoder,
				 IN int mode,
				 IN int* para
				 )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	if(pBroker->m_pVideoStream)
		pBroker->m_iFramesCount = pBroker->m_pVideoStream->GetCurrentSampleIdx();
	*para = pBroker->m_iFramesCount;
	return S_OK;
}

int
MP4Demux_ReadAudio(
					IN void * pDecoder,
				   OUT char *pbOutBuffer,
				   OUT unsigned int *pcbNumberOfBytesRead
				   )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->ReadAudio(pbOutBuffer, pcbNumberOfBytesRead);
}

int
MP4Demux_ReadVideoSample(
						IN void * pDecoder,
						 OUT char *pbOutBuffer,
						 OUT unsigned int *pcbNumberOfBytesRead,
						 OUT unsigned int * plCts,
						 OUT int * pbIsKeyFrame
						 )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->ReadVideoSample(pbOutBuffer, pcbNumberOfBytesRead, plCts, pbIsKeyFrame);
}

int MP4Demux_ReadCan(
	IN void * pDecoder,
	OUT char *pbOutBuffer,
	OUT unsigned int *pcbNumberOfBytesRead
	)
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->ReadCan(pbOutBuffer, pcbNumberOfBytesRead);
}

int
MP4Demux_ReadAudioSample(
						IN void * pDecoder,
						 OUT char *pbOutBuffer,
						 OUT unsigned int *pcbNumberOfBytesRead,
						 OUT unsigned int * plCts
						 )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->ReadAudioSample(pbOutBuffer, pcbNumberOfBytesRead, plCts);
}

int
MP4Demux_GetNextSampleType(
							IN void * pDecoder,
						   OUT unsigned int * pdwNextSampleType  // 0:none, 1:video, 2:audio
						   )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->GetNextSampleType(pdwNextSampleType);
}

int
MP4Demux_GetStreamInfo(
						IN void * pDecoder,
					   OUT MP4Demux_StreamInfo *pStreamInfo
					   )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->GetStreamInfo(pStreamInfo);
}

int
MP4Demux_GetDuration(
						IN void * pDecoder,
					    OUT int *pVideoDuration
					  )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->GetDuration(pVideoDuration);
}

int
MP4Demux_GetPositions(
						IN void * pDecoder,
					  OUT MP4Demux_Positions *pAudioPosition,
					  OUT MP4Demux_Positions *pVideoPosition
					  )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->GetPositions(pAudioPosition, pVideoPosition);
}

int
MP4Demux_SetPositions(
					  IN void * pDecoder,
					  IN unsigned int POS,
					  IN OUT MP4Demux_Positions *pAudioPosition,
					  IN OUT MP4Demux_Positions *pVideoPosition
					  )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->SetPositions(POS, pAudioPosition, pVideoPosition);
}

int
MP4Demux_GetUserData(
	IN void*              pDemuxer,
    IN OUT MP4Demux_UserData* pUserData 
	)
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDemuxer;
	return pBroker->GetUserData(pUserData);
}


int
MP4Demux_GetNextSampleSize(
	IN void * pDecoder,
	IN unsigned int dwSampleType, //0: audio, 1: video
    OUT unsigned int * pdwNextSampleSize 
    )
{
	MP4DemuxBroker * pBroker = (MP4DemuxBroker *)pDecoder;
	return pBroker->GetNextSampleSize(dwSampleType, pdwNextSampleSize);
}
