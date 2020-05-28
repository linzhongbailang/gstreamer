#ifndef MP4DEMUX_H_
#define MP4DEMUX_H_

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	double	dFrameRate;			/// video rendering frame rate.
	unsigned int	dwWidth;			/// frame width.
	unsigned int	dwHeight;			/// frame height.
	unsigned int	dwTotalFrames;		/// # video frames.
	unsigned int	dwSamplesPerSec;	/// audio sampling freq.
	unsigned int	dwChannels;			/// audio # channels.
	unsigned int	dwBitsPerSample;	/// audio quant. length.
	unsigned int	dwVideoType;		/// MP4DEMUX_XXX.
	unsigned int	dwAudioType;		/// MP4DEMUX_XXX.
	unsigned int	dwAudioStreamSize;	/// raw stream size in bytes.
	unsigned int	dwVideoStreamSize;	/// raw stream size in bytes.
	unsigned int	dwAudioTimeScale;	/// audio timescale.
	unsigned int	dwVideoTimeScale;	/// video timescale.
	unsigned int	dwAudioDuration;	/// playback duration time in dwAudioTimeScale.
	unsigned int	dwVideoDuration;	/// playback duration time in dwVideoTimeScale.
	char *	pcbAudioDsiBuffer;		/// audio DSI buffer
	unsigned int	dwAudioDsiBufferSize;
	char *	pcbVideoDsiBuffer;		/// video DSI buffer
	unsigned int	dwVideoDsiBufferSize;
	unsigned int	dwMaxVideoBitRate;		/// BPS,will be used to determine internal buffering size.
	unsigned int	dwMaxAudioBitRate;		/// BPS,will be used to determine internal buffering size.
	unsigned int	dwMaxVideoSampleSize;		
	unsigned int	dwMaxAudioSampleSize;		
	unsigned int	dwAVCCLengthSize;
	int    bAVdataInterleaved;
} MP4Demux_StreamInfo;

typedef struct{
	char* szContent;  
	int   nSize;
	int   nCodes;  //character codes. include three character codes.
	               //0:utf-8  1:utf-16 2:Macintosh-encoded
}UserData_Item;
	
typedef struct
{
	UserData_Item copyright;  
	UserData_Item artist;
	UserData_Item author;      
	UserData_Item title;
	UserData_Item description;
	UserData_Item album;
    UserData_Item artwork;
	//...

}MP4Demux_UserData;


enum
{
	MP4DEMUX_UNKNOWN,
	MP4DEMUX_AMR,
	MP4DEMUX_AMR_IF2,
	MP4DEMUX_MP3,
	MP4DEMUX_AAC,
	MP4DEMUX_G723,
	MP4DEMUX_G726_64,
	MP4DEMUX_G711_A,
	MP4DEMUX_G711_U,
	MP4DEMUX_IMA,
	MP4DEMUX_CELP,
	MP4DEMUX_HVXC,
	MP4DEMUX_BSAC,
	MP4DEMUX_ALAC,
	MP4DEMUX_MP4VIDEO,
	MP4DEMUX_H263,
	MP4DEMUX_H264,
	MP4DEMUX_MPEG1VIDEO,
	MP4DEMUX_MPEG2VIDEO
};

typedef struct
{
	unsigned int	dwSampleIdx;/// sample index of stream.
	unsigned int	dwByte;		/// stream's byte position.
	unsigned int	dwTime1khz;	/// stream's time position in 1khz.
}MP4Demux_Positions;

enum
{
	MP4DEMUX_POS_SAMPLE,
	MP4DEMUX_POS_BYTE,
	MP4DEMUX_POS_1KHZ,
};

typedef
int
(*PFN_MP4DEMUX_GET_DATA)(
    IN void* pvContext,
    OUT char *pbOutBuffer,
    OUT unsigned int *pcbNumberOfBytesRead
    );
typedef
int
(*PFN_MP4DEMUX_EXTERN_READ)(
	 IN void* pvFile,
	 OUT char* pcBuff,
	 OUT int iBuffSize
	 );
typedef
int
(*PFN_MP4DEMUX_EXTERN_SEEK)(
    IN void* pvFile,
    IN int iOffset,
    IN int iSeekType
    );
typedef
int
(*PFN_MP4DEMUX_EXTERN_TELL)(
    IN void* pvFile
	);
typedef
int
(*PFN_MP4DEMUX_EXTERN_SIZE)(
    IN void* pvFile
	);

struct MP4Demux_OpenOptions
{
	PFN_MP4DEMUX_GET_DATA pfnDataCallback;		/// Callback function provided by the client.  The decoder should call 
												/// this function when it needs more input data to perform decode.
	unsigned int dwBufferSize; //streaming buffer size

	//////////////////////////////////////////////////////////////////////////
	// the following four callback functions are used for external customized FILE IO
	PFN_MP4DEMUX_EXTERN_READ pfnExternRead;
	PFN_MP4DEMUX_EXTERN_SEEK pfnExternSeek;
	PFN_MP4DEMUX_EXTERN_TELL pfnExternTell;
	PFN_MP4DEMUX_EXTERN_SIZE pfnExternSize;
	//////////////////////////////////////////////////////////////////////////

    void * pvDataContext;						/// Context passed as a first argument to the data callback function.
	char* pFilename;
	bool  loadall;

	bool  onlyMoov;
};

//
// Creates a new video decoder object with a reference count
// of 1 and stores a pointer to it in ppDecoder.
//
// This object needs to be freed by MP4Demux_Release().
//
int MP4Demux_Create( OUT void **ppDecoder );
//
// Decrements reference count of the decoder object. 
// If the reference count goes to 0, frees up all
// resources used by video decoder.
//
// pDecoder - Decoder object created with MP4Demux_Create()
//
int MP4Demux_Release(IN void *pDecoder);

//
// Initializes the decoder with the OpenOptions.
// This is required after Creation of the decoder 
// object, before any decoding occurs.
// 
int MP4Demux_Open(
	IN void *pDecoder,
	IN const MP4Demux_OpenOptions *pOptions,
	IN const unsigned int dwSize
	);
//
// Deallocates any resources held by the decoder
// as part of decoding and displaying.  This frees
// up the runtime resources used by the decoder.
// 
int MP4Demux_GetIFramesInfo(
	IN void *pDecoder,
	IN int iFramesInfo[],
	IN const unsigned int dwSize
	);

int MP4Demux_Close(IN void *pDecoder);


int MP4Demux_ReadVideo(
	IN void * pDecoder,
    OUT char *pbOutBuffer,
    IN OUT unsigned int *pcbNumberOfBytesRead
	);

int MP4Demux_ReadVideoSample(
	IN void * pDecoder,
    OUT char *pbOutBuffer,
    OUT unsigned int *pcbNumberOfBytesRead,
	OUT unsigned int * plCts,
	OUT int * pbIsKeyFrame
	);

int MP4Demux_ReadAudio(
	IN void * pDecoder,
    OUT char *pbOutBuffer,
    IN OUT unsigned int *pcbNumberOfBytesRead
	);

int MP4Demux_ReadAudioSample(
	IN void * pDecoder,
    OUT char *pbOutBuffer,
    OUT unsigned int *pcbNumberOfBytesRead,
	OUT unsigned int * plCts
	);


int MP4Demux_ReadCan(
	IN void * pDecoder,
	OUT char *pbOutBuffer,
	OUT unsigned int *pcbNumberOfBytesRead
	);

int MP4Demux_GetNextSampleType(
	IN void * pDecoder,
    OUT unsigned int * pdwNextSampleType  // 0:none, 1:video, 2:audio
    );

int
MP4Demux_GetNextSampleSize(
IN void * pDecoder,
	IN unsigned int dwSampleType, //0: audio, 1: video
    OUT unsigned int * pdwNextSampleSize 
    );

int MP4Demux_GetStreamInfo(
	IN void * pDecoder,
    OUT MP4Demux_StreamInfo *pStreamInfo
	);

///
/// For media seeking.
///

int
MP4Demux_GetDuration(
	IN void * pDecoder,
    OUT int *pVideoDuration
	);

int MP4Demux_GetPositions(
	IN void * pDecoder,
    OUT MP4Demux_Positions *pAudioPosition,
    OUT MP4Demux_Positions *pVideoPosition
	);

int MP4Demux_SetPositions(
	IN void * pDecoder,
	IN unsigned int POS,
    IN OUT MP4Demux_Positions *pAudioPosition,
    IN OUT MP4Demux_Positions *pVideoPosition
	);

int MP4Demux_GetUserData(
    IN void*              pDemuxer,
    IN OUT MP4Demux_UserData* pUserData
    );

#ifdef __cplusplus
}
#endif

#endif
