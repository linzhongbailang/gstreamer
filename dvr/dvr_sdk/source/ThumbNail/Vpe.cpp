
#ifdef VPE_HW_ACCEL

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "Vpe.h"

Vpe::Vpe()
{
	m_fpVpe = 0;

	memset(&m_InputData, 0, sizeof(VpeInterfaceData_t));
	memset(&m_OutputData, 0, sizeof(VpeInterfaceData_t));
}

Vpe::~Vpe()
{
	if (m_fpVpe > 0)
	{
		closeVpe();
	}

	m_fpVpe = 0;
}

int Vpe::openVpe()
{
	m_fpVpe = open("/dev/video0", O_RDWR);
	if (m_fpVpe<0)
	{
		Vpe_error("open VPE error\n");
	}
	return(m_fpVpe);
}

int Vpe::closeVpe()
{
	int retVal = 0;

	if (m_fpVpe>0)
	{
		retVal = close(m_fpVpe);
		if (retVal<0)
		{
			Vpe_error("close VPE error\n");
		}
		m_fpVpe = 0;
	}
	return(retVal);
}

int Vpe::setVpeProperty(int fd, int setId, void *pSetParam)
{
	int retVal = 0;
	struct v4l2_format		*pVpeFmt = NULL;
	struct	v4l2_control *pVpeCtrl = NULL;
	struct v4l2_buffer		*pVpeBuffer = NULL;
	struct v4l2_requestbuffers	*pVpeReqbuf = NULL;
	int *pVpeSwtich = NULL;

	if (!pSetParam)
	{
		Vpe_error("pSetParam=NULL\n");
		retVal = -2;
		return(retVal);
	}

	switch (setId)
	{
	case  VPE_VIDIOC_S_FMT:
	{
		pVpeFmt = (struct v4l2_format *)pSetParam;
		retVal = ioctl(fd, VIDIOC_S_FMT, pVpeFmt);
		if (retVal < 0)
		{
			Vpe_error("ioctol for VIDIOC_S_FMT error\n");
			retVal = -3;
		}
	}
	break;
	case VPE_VIDIOC_S_CTRL:
	{
		pVpeCtrl = (struct	v4l2_control *)pSetParam;
		retVal = ioctl(fd, VIDIOC_S_CTRL, pVpeCtrl);
		if (retVal < 0)
		{
			Vpe_error("ioctol for VIDIOC_S_CTRL error\n");
			retVal = -4;
		}
	}
	break;
	case VPE_VIDIOC_REQBUFS:
	{
		pVpeReqbuf = (struct v4l2_requestbuffers	*)pSetParam;
		retVal = ioctl(fd, VIDIOC_REQBUFS, pVpeReqbuf);
		if (retVal < 0)
		{
			Vpe_error("ioctol for VIDIOC_REQBUFS error\n");
			retVal = -5;
		}
	}
	break;
	case VPE_VIDIOC_QUERYBUF:
	{
		pVpeBuffer = (struct v4l2_buffer	*)pSetParam;
		retVal = ioctl(fd, VIDIOC_QUERYBUF, pVpeBuffer);
		if (retVal < 0)
		{
			Vpe_error("ioctol for VIDIOC_QUERYBUF error\n");
			retVal = -6;
		}
	}
	break;
	case VPE_VIDIOC_QBUF:
	{
		pVpeBuffer = (struct v4l2_buffer	*)pSetParam;
		retVal = ioctl(fd, VIDIOC_QBUF, pVpeBuffer);
		if (retVal < 0)
		{
			Vpe_error("ioctol for _VIDIOC_QBUF error\n");
			retVal = -7;
		}
	}
	break;
	case VPE_VIDIOC_DQBUF:
	{
		pVpeBuffer = (struct v4l2_buffer	*)pSetParam;
		retVal = ioctl(fd, VIDIOC_DQBUF, pVpeBuffer);
		if (retVal < 0)
		{
			Vpe_error("ioctol for VIDIOC_DQBUF error\n");
			retVal = -8;
		}
	}
	break;
	case VPE_VIDIOC_STREAMON:
	{
		pVpeSwtich = (int *)pSetParam;
		retVal = ioctl(fd, VIDIOC_STREAMON, pVpeSwtich);
		if (retVal < 0)
		{
			Vpe_error("ioctol for VIDIOC_STREAMON error\n");
			retVal = -9;
		}
	}
	break;
	case VPE_VIDIOC_STREAMOFF:
	{
		pVpeSwtich = (int *)pSetParam;
		retVal = ioctl(fd, VIDIOC_STREAMOFF, pVpeSwtich);
		if (retVal < 0)
		{
			Vpe_error("ioctol for VIDIOC_STREAMOFF error\n");
			retVal = -10;
		}
	}
	break;
	default:
	{
		Vpe_error("invalid param:setId\n");
		retVal = -11;
		break;
	}

	}

	return(retVal);
}
// todo :getVpeProperty();
int Vpe::getVideoFrameFormatInfo(char *format, int width, int height, picFormatInfo_t  *p2PictureInfo)
{
	picFormatInfo_t  *pPictureInfo = p2PictureInfo;
	int	*size = &(pPictureInfo->size);
	int	*fourcc = &(pPictureInfo->fourCC);
	int	*coplanar = &(pPictureInfo->coplanar);
	strcpy(pPictureInfo->formatName, format);

	pPictureInfo->width = width;
	pPictureInfo->height = height;

	*size = -1;
	*fourcc = -1;
	if (strcmp(format, "rgb24") == 0)
	{
		*fourcc = V4L2_PIX_FMT_RGB24;
		*size = height * width * 3;
		*coplanar = 0;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SRGB;

	}
	else if (strcmp(format, "bgr24") == 0)
	{
		*fourcc = V4L2_PIX_FMT_BGR24;
		*size = height * width * 3;
		*coplanar = 0;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SRGB;

	}
	else if (strcmp(format, "argb32") == 0)
	{
		*fourcc = V4L2_PIX_FMT_RGB32;
		*size = height * width * 4;
		*coplanar = 0;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SRGB;

	}
	else if (strcmp(format, "abgr32") == 0)
	{
		*fourcc = V4L2_PIX_FMT_BGR32;
		*size = height * width * 4;
		*coplanar = 0;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SRGB;

	}
	else if (strcmp(format, "yuv444") == 0)
	{
		*fourcc = V4L2_PIX_FMT_YUV444;
		*size = height * width * 3;
		*coplanar = 0;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SMPTE170M;

	}
	else if (strcmp(format, "yvyu") == 0)
	{
		*fourcc = V4L2_PIX_FMT_YVYU;
		*size = height * width * 2;
		*coplanar = 0;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SMPTE170M;

	}
	else if (strcmp(format, "yuyv") == 0)
	{
		*fourcc = V4L2_PIX_FMT_YUYV;
		*size = height * width * 2;
		*coplanar = 0;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SMPTE170M;

	}
	else if (strcmp(format, "uyvy") == 0)
	{
		*fourcc = V4L2_PIX_FMT_UYVY;
		*size = height * width * 2;
		*coplanar = 0;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SMPTE170M;

	}
	else if (strcmp(format, "vyuy") == 0)
	{
		*fourcc = V4L2_PIX_FMT_VYUY;
		*size = height * width * 2;
		*coplanar = 0;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SMPTE170M;

	}
	else if (strcmp(format, "nv16") == 0)
	{
		*fourcc = V4L2_PIX_FMT_NV16;
		*size = height * width * 2;
		*coplanar = 0;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SMPTE170M;

	}
	else if (strcmp(format, "nv61") == 0)
	{
		*fourcc = V4L2_PIX_FMT_NV61;
		*size = height * width * 2;
		*coplanar = 0;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SMPTE170M;

	}
	else if (strcmp(format, "nv12") == 0)
	{
		*fourcc = V4L2_PIX_FMT_NV12;
		*size = height * width * 1.5;
		*coplanar = 1;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SMPTE170M;

	}
	else if (strcmp(format, "nv21") == 0)
	{
		*fourcc = V4L2_PIX_FMT_NV21;
		*size = height * width * 1.5;
		*coplanar = 1;
		pPictureInfo->colorspace = V4L2_COLORSPACE_SMPTE170M;

	}
	else
	{
		return -1;

	}

	return 0;
}
//not consider the interlaced video.
//	retVal = allocVpeBuffer(bufferType, pPicFormatInfo, &inBufferInfo, bufferCount);
int Vpe::allocVpeBuffer(int bufferType, picFormatInfo_t *pictureInfo, bufferInfo_t *getBuffer, int  bufferCount)
{
	struct v4l2_format		fmt;
	struct v4l2_requestbuffers	reqbuf;
	struct v4l2_buffer		buffer;
	struct v4l2_plane		buf_planes[2];

	int				i = 0;
	int				ret = -1;
	int type = bufferType;
	int width = pictureInfo->width;
	int height = pictureInfo->height;
	int fourcc = pictureInfo->fourCC;
	int coplanar = pictureInfo->coplanar;

	int interlace = 0;
	void  **base = getBuffer->base_y;
	void **base_uv = getBuffer->base_uv;
	int *sizeimage_y = &(getBuffer->yBufferSize);
	int *sizeimage_uv = &(getBuffer->uvBufferSize);
	getBuffer->bufferCount = bufferCount;
	int allocBufferCount = bufferCount;

	int fd = m_fpVpe;

	bzero(&fmt, sizeof(fmt));
	fmt.type = type;
	fmt.fmt.pix_mp.width = width;
	fmt.fmt.pix_mp.height = height;
	fmt.fmt.pix_mp.pixelformat = fourcc;
	fmt.fmt.pix_mp.colorspace = pictureInfo->colorspace;

	if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE && interlace)
		fmt.fmt.pix_mp.field = V4L2_FIELD_ALTERNATE;
	else
		fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;

	ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
	if (ret < 0)
	{
		Vpe_error("Cant set color format\n");
		return(ret);
	}
	else
	{
		*sizeimage_y = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
		*sizeimage_uv = fmt.fmt.pix_mp.plane_fmt[1].sizeimage;
	}

	bzero(&reqbuf, sizeof(reqbuf));
	reqbuf.count = allocBufferCount;
	reqbuf.type = type;
	reqbuf.memory = V4L2_MEMORY_MMAP;


	ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0)
	{
		Vpe_error("Cant request buffers\n");
		return(ret);
	}
	else
	{
		allocBufferCount = reqbuf.count;
	}

	for (i = 0; i < allocBufferCount; i++)
	{
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = type;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;
		buffer.m.planes = buf_planes;
		buffer.length = coplanar ? 2 : 1;

		ret = ioctl(fd, VIDIOC_QUERYBUF, &buffer);
		if (ret < 0)
		{
			Vpe_error("Cant query buffers\n");
			return(ret);
		}

		base[i] = mmap(NULL, buffer.m.planes[0].length, PROT_READ | PROT_WRITE,MAP_SHARED, fd, buffer.m.planes[0].m.mem_offset);

		if (MAP_FAILED == base[i])
		{
			Vpe_error("Cant mmap buffers Y");
			while (i >=0)
			{
				/**	Unmap all previous buffers	*/
				munmap(base[i], *sizeimage_y);
				base[i] = NULL;
				i--;
			}
			return -1;
		}

		if (!coplanar)
			continue;

		base_uv[i] = mmap(NULL, buffer.m.planes[1].length, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, buffer.m.planes[1].m.mem_offset);

		if (MAP_FAILED == base_uv[i])
		{
			Vpe_error("Cant mmap buffers UV");
			while (i >=0)
			{
				/**	Unmap all previous buffers	*/
				munmap(base_uv[i], *sizeimage_uv);
				base[i] = NULL;
				i--;				
			}
			return -1;
		}
	}

	return 1;
}

int  Vpe::releaseVpeBuffer(void **buffer, int bufferCount, int bufferSize )
{
	int retVal = 0;
	while (bufferCount > 0)
	{
		bufferCount--;
		retVal=munmap(buffer[bufferCount], bufferSize);
		if (retVal<0)
		{
			Vpe_error("munmap error\n");
			return(retVal);
		}
	}
}

int Vpe::queueAllBuffers(int bufferType, int bufferCount)
{
	int retVal=0;

	int fd = m_fpVpe;
	int type = bufferType;
	int numbuf = bufferCount;
	struct v4l2_buffer	buffer;
	struct v4l2_plane	buf_planes[2];
	int i = 0;
	int ret = -1;
	int lastqueued = -1;

	for (i = 0; i < numbuf; i++)
	{
		memset(&buffer, 0, sizeof(buffer));
		buffer.type = type;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;
		buffer.m.planes = buf_planes;
		buffer.length = 2;

		ret = ioctl(fd, VIDIOC_QBUF, &buffer);
		if (-1 == ret)
		{
			Vpe_error("qbufer error\n");
			retVal=-1;
			break;
		}
		else
		{
			lastqueued = i;
			retVal=lastqueued;
		}
	}
	return retVal;
}

int Vpe::queueData(int type, int index, int field, int size_y, int size_uv)
{
	int fd = m_fpVpe;
	struct v4l2_buffer	buffer;
	struct v4l2_plane	buf_planes[2];
	int	ret = -1;

	buf_planes[0].length = buf_planes[0].bytesused = size_y;
	buf_planes[1].length = buf_planes[1].bytesused = size_uv;
	buf_planes[0].data_offset = buf_planes[1].data_offset = 0;

	memset(&buffer, 0, sizeof(buffer));
	buffer.type = type;
	buffer.memory = V4L2_MEMORY_MMAP;
	buffer.index = index;
	buffer.m.planes = buf_planes;
	buffer.field = field;
	buffer.length = 2;

	ret = ioctl(fd, VIDIOC_QBUF, &buffer);
	if (-1 == ret)
	{
		Vpe_error("Failed to queue\n");
	}
	return ret;
}

int Vpe::dequeueData(int type, struct v4l2_buffer *buf, struct v4l2_plane *buf_planes)
{
	int fd = m_fpVpe;
	int	ret = -1;

	memset(buf, 0, sizeof(*buf));
	buf->type = type;
	buf->memory = V4L2_MEMORY_MMAP;
	buf->m.planes = buf_planes;
	buf->length = 2;
	ret = ioctl(fd, VIDIOC_DQBUF, buf);
	if (ret<0)
	{
		Vpe_error("Failed to dequeue data\n");
	}
	return ret;
}

int Vpe::startVpe(int bufferType)
{
	int fd = m_fpVpe;
	int type = bufferType;
	int	ret = -1;
	ret = ioctl(fd, VIDIOC_STREAMON, &type);
	if (-1 == ret)
	{
		Vpe_error("Cant Stream on\n");
	}
	return(ret);
}

int Vpe::stopVpe(int bufferType)
{
	int fd = m_fpVpe;
	int type = bufferType;
	int	ret = -1;
	ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
	if (-1 == ret)
	{
		Vpe_error("Cant Stream stop\n");
	}
	return(ret);
}

// int Vpe::scalePicture(void *inputVideo, void *outputVideo,int picCount)
int Vpe::imgProcess(void)
{
	int retVal = 0;

	int fd = openVpe();
	if(fd<=0)
	{
		retVal=-2;
		Vpe_error("vpe open error\n");
		return(retVal);
	}

	VpeInterfaceData_t *pInputData = &m_InputData;
	VpeInterfaceData_t *pOutputData = &m_OutputData;
	if((pInputData->pDataBuffer==NULL)||(pOutputData->pDataBuffer==NULL))
	{
		Vpe_error("invalid paramete\n");
		retVal=-3;
		return(retVal);
	}

	if ((pOutputData->width%16)&&(pOutputData->height%16))
	{
		Vpe_error("Vpe:the output video width and height should be 4 aligned\n");
		retVal = -3;
		return(retVal);
	}

	picFormatInfo_t inPicFormatInfo;
	int width = pInputData->width;
	int height = pInputData->height;
	char format[8];
	memset(format, 0, 8);
	strcpy(format, pInputData->formatName);
	retVal = getVideoFrameFormatInfo(format, width, height, &inPicFormatInfo);
	if (retVal < 0)
	{
		Vpe_error("vpe not support the src format:%s\n", format);
		return (retVal);
	}

	picFormatInfo_t outPicFormatInfo;
	width = pOutputData->width;
	height = pOutputData->height;
	memset(format, 0, 8);
	strcpy(format, pOutputData->formatName);

	retVal = getVideoFrameFormatInfo(format, width, height, &outPicFormatInfo);
	if (retVal < 0)
	{
		Vpe_error("vpe not support the dst format:%s\n", format);
		return (retVal);
	}

	// set the Vpe property.
	struct	v4l2_control ctrl;
	memset(&ctrl, 0, sizeof(ctrl));
	ctrl.id = V4L2_CID_TRANS_NUM_BUFS;
	//ctrl.value=picCount
	ctrl.value = 1;
	retVal = setVpeProperty(fd, VPE_VIDIOC_S_CTRL, (void *)&ctrl);
	if (retVal < 0)
	{
		Vpe_error("set property error\n");
		return (retVal);
	}

	//alloc and map the buffer.
	int bufferType = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	picFormatInfo_t *pPicFormatInfo = &inPicFormatInfo;
	bufferInfo_t inBufferInfo;
	//	int bufferCount = picCount;
	int bufferCount = 1;

	retVal = allocVpeBuffer(bufferType, pPicFormatInfo, &inBufferInfo, bufferCount);
	if (retVal < 0)
	{
		Vpe_error("allocVpeBuffer for in error\n");
		return (retVal);
	}


	bufferType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	pPicFormatInfo = &outPicFormatInfo;
	bufferInfo_t outBufferInfo;
	retVal = allocVpeBuffer(bufferType, pPicFormatInfo, &outBufferInfo, bufferCount);
	if (retVal < 0)
	{
		Vpe_error("allocVpeBuffer for out error\n");
		return (retVal);
	}

	//queue all output buffer.
	bufferType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	bufferCount = 1;
	retVal = queueAllBuffers(bufferType, bufferCount);
	if (retVal < 0)
	{
		Vpe_error("queue allBuffer for output queue error\n");
		return (retVal);
	}

	//read data and queue in buffer.
	// todo if the picCount > 1.make a circle.
	memcpy(inBufferInfo.base_y[0], pInputData->pDataBuffer, inBufferInfo.yBufferSize);
	if (inPicFormatInfo.coplanar)
	{
		memcpy(inBufferInfo.base_uv[0], pInputData->pDataBuffer + inBufferInfo.yBufferSize, inBufferInfo.uvBufferSize);
	}
	// the inputBuffer is belong to the dvr,i have no right to release it.
#if 0	
	if(pInputData->pDataBuffer)
	{
		free(pInputData->pDataBuffer);
		pInputData->pDataBuffer=NULL;
	}
#endif	

	bufferType = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	int index = 0;
	int size_y = inBufferInfo.yBufferSize;
	int size_uv = inBufferInfo.uvBufferSize;
	int field = V4L2_FIELD_TOP;
	retVal = queueData(bufferType, index, field, size_y, size_uv);
	if (retVal < 0)
	{
		Vpe_error(" queue data for input error\n");
		return (retVal);
	}

	//start VPE on.
	retVal = startVpe(V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	if (retVal < 0)
	{
		Vpe_error(" startVpe error\n");
		return (retVal);
	}

	retVal = startVpe(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	if (retVal < 0)
	{
		Vpe_error(" startVpe error\n");
		return (retVal);
	}


	//deque  buffer.
	struct v4l2_buffer buf;
	struct v4l2_plane buf_planes[2];
	retVal = dequeueData(V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, &buf, buf_planes);
	if (retVal < 0)
	{
		Vpe_error(" deque error\n");
		return (retVal);
	}	
	retVal = dequeueData(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, &buf, buf_planes);
	if (retVal < 0)
	{
		Vpe_error(" deque error\n");
		return (retVal);
	}	

	//read the buffer.
	memcpy(pOutputData->pDataBuffer, outBufferInfo.base_y[0], outBufferInfo.yBufferSize);
	if (outPicFormatInfo.coplanar)
	{
		memcpy(pOutputData->pDataBuffer + outBufferInfo.yBufferSize, outBufferInfo.base_uv[0], outBufferInfo.uvBufferSize);
	}

	// stop Vpe.
	//start VPE on.
	retVal = stopVpe(V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	if (retVal < 0)
	{
		Vpe_error(" stopVpe error\n");
		return (retVal);
	}	
	retVal = stopVpe(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	if (retVal < 0)
	{
		Vpe_error(" stopVpe error\n");
		return (retVal);
	}

	// release the resource.
	void **buffer = inBufferInfo.base_y;
	int bufferSize = inBufferInfo.yBufferSize;
	bufferCount = 1;
	//void Vpe::releaseVpeBuffer(void **buffer, int bufferCount, int bufferSize )
	releaseVpeBuffer(buffer, bufferCount,bufferSize );

	if (inPicFormatInfo.coplanar)
	{
		buffer = inBufferInfo.base_uv;
		bufferSize = inBufferInfo.uvBufferSize;
		retVal = releaseVpeBuffer(buffer, bufferCount, bufferSize);
		if (retVal < 0)
		{
			Vpe_error(" releaseVpeBuffer error\n");
			return (retVal);
		}
	}

	buffer = outBufferInfo.base_y;
	bufferSize = outBufferInfo.yBufferSize;

	bufferCount = 1;
	//munmap(buffer[0], bufferSize);
	retVal = releaseVpeBuffer(buffer, bufferCount, bufferSize);
	if (retVal < 0)
	{
		Vpe_error(" releaseVpeBuffer error\n");
		return (retVal);
	}
	if (outPicFormatInfo.coplanar)
	{
		buffer = outBufferInfo.base_uv;
		bufferSize = outBufferInfo.uvBufferSize;

		retVal = releaseVpeBuffer(buffer, bufferCount, bufferSize);
		if (retVal < 0)
		{
			Vpe_error(" releaseVpeBuffer error\n");
			return (retVal);
		}

	}

	closeVpe();
	return(retVal);
}

#endif


