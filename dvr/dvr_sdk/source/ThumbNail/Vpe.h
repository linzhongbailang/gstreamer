#ifndef _VPE_H_
#define _VPE_H_

#ifdef VPE_HW_ACCEL
#include <linux/videodev2.h>
#endif

#define VPE_SET_PRO_ID(X)				(1<<(X))
#define VPE_VIDIOC_S_CTRL				 (VPE_SET_PRO_ID(1))
#define VPE_VIDIOC_REQBUFS		(VPE_SET_PRO_ID(2))
#define VPE_VIDIOC_QUERYBUF		(VPE_SET_PRO_ID(3))
#define VPE_VIDIOC_QBUF				(VPE_SET_PRO_ID(4))
#define VPE_VIDIOC_DQBUF				(VPE_SET_PRO_ID(5))
#define VPE_VIDIOC_STREAMON	(VPE_SET_PRO_ID(6))
#define VPE_VIDIOC_STREAMOFF	(VPE_SET_PRO_ID(7))
#define VPE_VIDIOC_S_FMT				(VPE_SET_PRO_ID(8))

#define V4L2_CID_TRANS_NUM_BUFS         (V4L2_CID_USER_TI_VPE_BASE + 0)

#define _VPE_DEBUG_
#ifdef _VPE_DEBUG_
#define  Vpe_info(...) \
do \
{ \
	printf("INFO:%s,%s,%d: ",__FILE__,__FUNCTION__,__LINE__); \
	printf(__VA_ARGS__); \
}while(0)
#else
#define  Vpe_info(...)
#endif

#define  Vpe_error(...) \
do \
{ \
	printf("ERROR:%s,%s,%d,errno=%d,strerror:%s ",__FILE__,__FUNCTION__,__LINE__,errno,strerror(errno)); \
	printf(__VA_ARGS__); \
}while(0)

typedef unsigned int		DVR_U32;	

typedef struct _tagPicFormatInfo
{
	int width;
	int height;
	int fourCC;
	int coplanar;
	enum v4l2_colorspace colorspace;
	int size;
	char formatName[8];
}picFormatInfo_t;

typedef struct _tagBufferInfo
{
	void *base_y[1];
	void  *base_uv[1];
	int yBufferSize;
	int uvBufferSize;
	int bufferCount;
}bufferInfo_t;

typedef struct _tagInterfaceData
{
	char formatName[8];
	unsigned char *pDataBuffer;
	int dataSize;
	int width;
	int height;
}VpeInterfaceData_t;

class Vpe
{
public:
	Vpe();
	~Vpe();

	int imgProcess(void);

	VpeInterfaceData_t m_InputData;
	VpeInterfaceData_t m_OutputData;

private:
	int openVpe();
	int closeVpe();
	int getVpeHandle();
	int setVpeProperty(int vpeHandle, int setId, void *pSetParam);
	int getVideoFrameFormatInfo(char *format, int width, int height, picFormatInfo_t  *p2PictureInfo);
	int allocVpeBuffer(int bufferType, picFormatInfo_t *pictureInfo, bufferInfo_t *getBuffer, int  bufferCount);
	int releaseVpeBuffer(void **buffer, int bufferCount, int bufferSize);
	int queueData(int	type, int	index, int	field, int	size_y, int size_uv);
	int dequeueData(int	type, struct v4l2_buffer *buf, struct v4l2_plane *buf_planes);
	int startVpe(int bufferType);
	int stopVpe(int bufferType);
	int queueAllBuffers(int bufferType, int bufferCount);

	int m_fpVpe;
};
#endif

