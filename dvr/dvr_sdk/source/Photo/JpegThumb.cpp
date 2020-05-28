#include <gst/gst.h>
#include "jhead/jhead.h"
#include <jpeglib.h>
#include <setjmp.h>
#include "dprint.h"
#include "JpegThumb.h"

struct my_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */
 
  jmp_buf setjmp_buffer;    /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

extern gdouble ms_time(void);
static void  my_error_exit (j_common_ptr cinfo)
{
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    my_error_ptr myerr = (my_error_ptr) cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}

CJpegThumb::CJpegThumb()
{

}

CJpegThumb::~CJpegThumb()
{

}

unsigned char* CJpegThumb::ReadJpeg(const char* path, int& width, int& height)
{
	FILE *file = fopen(path, "rb");
	if ( file == NULL )	
    {
        DPrint(DPRINT_ERR, "path is not exited!!!\n");
		return NULL;
	}
 
	struct jpeg_decompress_struct info; //for our jpeg info
 
    struct my_error_mgr my_err;

    info.err = jpeg_std_error(&my_err.pub);
    my_err.pub.error_exit = my_error_exit;

    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(my_err.setjmp_buffer))
    {
         /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */
          DPrint(DPRINT_ERR, "Error occured\n");
         jpeg_destroy_decompress(&info);
         fclose(file);
         return NULL;
	 }
 
	jpeg_create_decompress( &info ); //fills info structure
	jpeg_stdio_src( &info, file );        //void
 
	int ret_Read_Head = jpeg_read_header( &info, 1 ); //int
	if(ret_Read_Head != JPEG_HEADER_OK)
    {
		 DPrint(DPRINT_ERR, "jpeg_read_header failed\n");
		fclose(file);
		jpeg_destroy_decompress(&info);
		return NULL;
	}
 
	bool bStart = jpeg_start_decompress( &info );
	if(!bStart)
    {
		 DPrint(DPRINT_ERR, "jpeg_start_decompress failed\n");
		fclose(file);
		jpeg_destroy_decompress(&info);
		return NULL;
	}
	int w = width = info.output_width;
	int h = height = info.output_height;
	int numChannels = info.num_components; // 3 = RGB, 4 = RGBA
	//DPrint(DPRINT_INFO, "ReadJpeg out_color_space =%d\n", info.out_color_space);
	unsigned long dataSize = w * h * numChannels;
 
	// read RGB(A) scanlines one at a time into jdata[]
	unsigned char *data = (unsigned char *)malloc( dataSize );
	if(!data)
        return NULL;
 
	unsigned char* rowptr;
	while ( info.output_scanline < h )
	{
		rowptr = data + info.output_scanline * w * numChannels;
		jpeg_read_scanlines( &info, &rowptr, 1 );
	}
 
	jpeg_finish_decompress( &info );    
 
	fclose(file);
	return data;
}

unsigned char* CJpegThumb::do_Stretch_Linear(int w_Dest, int h_Dest, int bit_depth, unsigned char *src, int w_Src, int h_Src)
{
	int sw = w_Src-1, sh = h_Src - 1, dw = w_Dest - 1, dh = h_Dest - 1;
	int B, N, x, y;
	int nPixelSize = bit_depth/8;
	unsigned char *pLinePrev,*pLineNext;
	unsigned char *pDest = new unsigned char[w_Dest*h_Dest*bit_depth/8];
	unsigned char *tmp;
	unsigned char *pA,*pB,*pC,*pD;
 
	for(int i=0;i<=dh;++i)
	{
		tmp =pDest + i*w_Dest*nPixelSize;
		y = i*sh/dh;
		N = dh - i*sh%dh;
		pLinePrev = src + (y++)*w_Src*nPixelSize;
		//pLinePrev =(unsigned char *)aSrc->m_bitBuf+((y++)*aSrc->m_width*nPixelSize);
		pLineNext = (N==dh) ? pLinePrev : src+y*w_Src*nPixelSize;
		//pLineNext = ( N == dh ) ? pLinePrev : (unsigned char *)aSrc->m_bitBuf+(y*aSrc->m_width*nPixelSize);
		for(int j=0;j<=dw;++j)
		{
			x = j*sw/dw*nPixelSize;
			B = dw-j*sw%dw;
			pA = pLinePrev+x;
			pB = pA+nPixelSize;
			pC = pLineNext + x;
			pD = pC + nPixelSize;
			if(B == dw)
			{
				pB=pA;
				pD=pC;
			}
 
			for(int k=0;k<nPixelSize;++k)
			{
				*tmp++ = ( unsigned char )( int )(
					( B * N * ( *pA++ - *pB - *pC + *pD ) + dw * N * *pB++
					+ dh * B * *pC++ + ( dw * dh - dh * B - dw * N ) * *pD++
					+ dw * dh / 2 ) / ( dw * dh ) );
			}
		}
	}
	return pDest;
}

int CJpegThumb::MakeThumb(const char* filename, unsigned char *pPreviewBuf, int nSize)
{
    if(filename == NULL || pPreviewBuf == NULL)
        return -1;

    int width = 0, height = 0;
    double start = ms_time();
    unsigned char* buff = ReadJpeg(filename, width, height);
    if(buff == NULL)
    {
        DPrint(DPRINT_ERR, "ReadJpeg Failed!!!\n");
        return -1;
    }

	int tb_w = DVR_THUMBNAIL_WIDTH, tb_h = DVR_THUMBNAIL_HEIGHT;
	unsigned char * img_buf = do_Stretch_Linear(tb_w, tb_h, 24, buff, width, height);
	free(buff);
    memcpy(pPreviewBuf, img_buf, nSize);
	free(img_buf);
    double end = ms_time();
    DPrint(DPRINT_INFO, "MakeThumb=========width %d height %d,takes %lf ms\n",width, height, end-start);
    return 0;
}
