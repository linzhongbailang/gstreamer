#include "dprint.h"
#include "SwScale.h"

double ms_time(void);

SwScale::SwScale()
{
    memset(&m_InputData, 0, sizeof(SwScaleInterfaceData_t));
    memset(&m_OutputData, 0, sizeof(SwScaleInterfaceData_t));
}

SwScale::~SwScale()
{

}

int SwScale::imgProcess()
{
    SwScaleInterfaceData_t *pInputData = &m_InputData;
    SwScaleInterfaceData_t *pOutputData = &m_OutputData;
    if ((pInputData->pDataBuffer == NULL) || 
		(pOutputData->pDataBuffer == NULL) ||
		(pInputData->width == 0) ||
		(pInputData->height == 0) ||
		(pInputData->dataSize == 0))
    {
        DPrint(DPRINT_ERR, "invalid parameter\n");
        return -1;
    }

    if ((pOutputData->width % 16) && (pOutputData->height % 16))
    {
        DPrint(DPRINT_ERR, "the output video width and height should be 4 aligned\n");
        return -1;
    }

    struct SwsContext *img_convert_ctx = sws_alloc_context();
    if (img_convert_ctx == NULL)
    {
        DPrint(DPRINT_ERR, "sws_alloc_context failed\n");
        return -1;
    }

    //Set Value
    av_opt_set_int(img_convert_ctx, "sws_flags", SWS_BICUBIC, 0);
    av_opt_set_int(img_convert_ctx, "srcw", pInputData->width, 0);
    av_opt_set_int(img_convert_ctx, "srch", pInputData->height, 0);
    av_opt_set_int(img_convert_ctx, "src_format", pInputData->pixfmt, 0);
    //'0' for MPEG (Y:0-235);'1' for JPEG (Y:0-255)
    av_opt_set_int(img_convert_ctx, "src_range", 1, 0);
    av_opt_set_int(img_convert_ctx, "dstw", pOutputData->width, 0);
    av_opt_set_int(img_convert_ctx, "dsth", pOutputData->height, 0);
    av_opt_set_int(img_convert_ctx, "dst_format", pOutputData->pixfmt, 0);
    av_opt_set_int(img_convert_ctx, "dst_range", 1, 0);
    if(0 != sws_init_context(img_convert_ctx, NULL, NULL))
    {
        DPrint(DPRINT_ERR, "sws_init_context failed, width:%d, height:%d, pixfmt:%d\n", pInputData->width, pInputData->height, pInputData->pixfmt);
        return -1;
	}

    uint8_t *src_data[4];
    int src_linesize[4];

    uint8_t *dst_data[4];
    int dst_linesize[4];
    av_image_fill_linesizes(src_linesize, pInputData->pixfmt, pInputData->width);
    av_image_fill_linesizes(dst_linesize, pOutputData->pixfmt, pOutputData->width);

    av_image_fill_pointers(src_data, pInputData->pixfmt, pInputData->height, pInputData->pDataBuffer, src_linesize);
    av_image_fill_pointers(dst_data, pOutputData->pixfmt, pOutputData->height, pOutputData->pDataBuffer, dst_linesize);

	double start = ms_time();	
    sws_scale(img_convert_ctx, src_data, src_linesize, 0, pInputData->height, dst_data, dst_linesize);	
	double end = ms_time();
	DPrint(DPRINT_INFO, "sws_scale takes %f ms\n", end-start);

    sws_freeContext(img_convert_ctx);

    return 0;
}
