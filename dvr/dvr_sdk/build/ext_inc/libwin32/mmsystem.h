//=============================================================================
//	THIS SOURCE CODE IS PROPRIETARY INFORMATION BELONGING TO INTERVIDEO, INC.
// 	ANY USE INCLUDING BUT NOT LIMITED TO COPYING OF CODE, CONCEPTS, AND/OR
//	ALGORITHMS IS PROHIBITED EXCEPT WITH EXPRESS WRITTEN PERMISSION BY THE 
//	COMPANY.
//
// 	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// 	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// 	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// 	PURPOSE.
//
// 	Copyright (c) 2004 - 2005  InterVideo Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------

#ifndef _MMSYSTEM_H
#define _MMSYSTEM_H

#include "windef.h"

#include <unistd.h>
#include <fcntl.h>

/* mmsystem.h -- windows header for multimedia APIs */

#define MMSYSERR_NOERROR 0
typedef UINT MMRESULT;  /* error return code, 0 means no error */
                        /* call as if(err=xxxx(...)) Error(err); else */

/**************************/
/* Waveform audio support */
/**************************/

typedef int  HWAVEOUT;  /* handle to audio waveout device */

/* wave data block header */
typedef struct wavehdr_tag {
    LPSTR       lpData;                 /* pointer to locked data buffer */
    DWORD       dwBufferLength;         /* length of data buffer */
    DWORD       dwBytesRecorded;        /* used for input only */
    DWORD       dwUser;                 /* for client's use */
    DWORD       dwFlags;                /* assorted flags (see defines) */
    DWORD       dwLoops;                /* loop control counter */
    struct wavehdr_tag *lpNext;         /* reserved for driver */
    DWORD       reserved;               /* reserved for driver */
} WAVEHDR, *PWAVEHDR, *LPWAVEHDR;

/* flags for dwFlags field of WAVEHDR */
#define WHDR_DONE       0x00000001  /* done bit */
#define WHDR_PREPARED   0x00000002  /* set if this header has been prepared */

/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_

/*
 *  extended waveform format structure used for all non-PCM formats. this
 *  structure is common to all non-PCM formats.
 */
typedef struct tWAVEFORMATEX
{
    WORD        wFormatTag;         /* format type */
    WORD        nChannels;          /* number of channels (i.e. mono, stereo...)
 */
    DWORD       nSamplesPerSec;     /* sample rate */
    DWORD       nAvgBytesPerSec;    /* for buffer estimation */
    WORD        nBlockAlign;        /* block size of data */
    WORD        wBitsPerSample;     /* number of bits per sample of mono data */
    WORD        cbSize;             /* the count in bytes of the size of */
                                    /* extra information (after cbSize) */
} WAVEFORMATEX, *PWAVEFORMATEX, NEAR *NPWAVEFORMATEX, FAR *LPWAVEFORMATEX;
typedef const WAVEFORMATEX FAR *LPCWAVEFORMATEX;
#endif /* _WAVEFORMATEX_ */


/*****************/
/* Timer support */
/*****************/

static inline MMRESULT timeBeginPeriod(UINT uPeriod) {return MMSYSERR_NOERROR;}
static inline MMRESULT timeEndPeriod  (UINT uPeriod) {return MMSYSERR_NOERROR;}

DWORD    timeGetTime(void);

/*******************************/
/* Multimedia File I/O support */
/*******************************/

typedef int    HMMIO;
typedef void * LPMMIOINFO;
typedef char * HPSTR;

/* read/write mode numbers (bit field MMIO_RWMODE) */
#define MMIO_READ       0x00000000      /* open file for reading only */
#define MMIO_WRITE      0x00000001      /* open file for writing only */
#define MMIO_READWRITE  0x00000002      /* open file for reading and writing */

static inline HMMIO mmioOpen(LPSTR szFilename, LPMMIOINFO lpmmioinfo,
							 DWORD dwOpenFlags) {
  switch(dwOpenFlags) {
    case MMIO_READ: {
	  int res=open(szFilename,O_RDONLY,0);
	  if (res>0)
		return res;
	  else if (res==0)
		close(res);
	  break;
	}
  }
  return 0;
}
 
static inline LONG mmioRead(HMMIO hmmio, HPSTR pch, LONG cch) {
  return hmmio ? read(hmmio,pch,cch) : 0;
}
 
static inline MMRESULT mmioClose(HMMIO hmmio, UINT wFlags) {
  if (hmmio)
	close(hmmio);
  return 0;
}

static inline LONG mmioSeek(HMMIO hmmio, LONG lOffset, int iOrigin) {
  return lseek(hmmio,lOffset,iOrigin);
}

//#define USE_DUMMY_WAVEOUT 1
#ifdef USE_DUMMY_WAVEOUT
static inline MMRESULT waveOutDummy() { return 0; }
#define waveOutGetNumDevs()           waveOutDummy()
#define waveOutOpen(a,b,c,d,e,f)      waveOutDummy()
#define waveOutClose(x)               waveOutDummy()
#define waveOutReset(x)               waveOutDummy()
#define waveOutRestart(x)             waveOutDummy()
#define waveOutWrite(a,b,c)           waveOutDummy()
#define waveOutPause(x)               waveOutDummy()
#define waveOutGetVolume(a,b)         waveOutDummy()
#define waveOutSetVolume(a,b)         waveOutDummy()
#define waveOutPrepareHeader(a,b,c)   waveOutDummy()
#define waveOutUnprepareHeader(a,b,c) waveOutDummy()
#endif

#endif /* _MMSYSTEM_H */
