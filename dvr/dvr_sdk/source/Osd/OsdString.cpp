#include <math.h>
//#include "DVR_GUI_OBJ.h"
#include "OsdString.h"
static const char* strings_file[25] = {"StringsEn.txt"};

#define THUMB_ACC "/opt/vision_sdk/project/svres/Dvr/ACC.bmp"
#define THUMB_IAC "/opt/vision_sdk/project/svres/Dvr/IAC.bmp"
#define THUMB_APA "/opt/vision_sdk/project/svres/Dvr/APA.bmp"
#define THUMB_AEB "/opt/vision_sdk/project/svres/Dvr/AEB.bmp"

extern gdouble ms_time(void);
extern unsigned char picpane[OSD_PANE_WIDTH*OSD_PANE_HEIGHT*3>>1];

COsdString::COsdString(void)
{
#ifndef THUMB_BMP
    CTextDC::Load("/opt/avm/times.ttf");
    CLocales::S_SetFontFile(NULL, NULL, NULL, (signed char**)strings_file, 1);
#endif
}

COsdString::~COsdString(void)
{
#ifndef THUMB_BMP
    CTextDC::Unload();
#endif
}

DVR_U8* COsdString::GetRasterBuf(const char* str, uint& width, uint& height)
{
    if(str == NULL)
        return NULL;
    CTextDC *pTextDC    = new CTextDC;
    pTextDC->SetFontSize(210);
    pTextDC->SetText(str);
    int swidth   = pTextDC->GetWidth();
    int sheight  = pTextDC->GetHeight();
    width        = (swidth + 7) / 8 * 8;
    height       = sheight;

    m_size.w = width;
    m_size.h = height;
    DVR_U8* RasterBuf = (DVR_U8*)malloc(width * height);
    pTextDC->TextToRaster(RasterBuf, &m_size);
    delete pTextDC;
    pTextDC = NULL;
    return RasterBuf;
}

int COsdString::SetString(OSD_SRC_PARAM* srcparam, BMPIMAGE* bmpimg)
{
    if(srcparam == NULL)
        return DVR_RES_EFAIL;

    if(srcparam->rectype >= OSD_SRC_REC_TYPE_NUM)
        return DVR_RES_EFAIL;

    double start = ms_time();
#ifndef THUMB_BMP
    uint width = 0;
    uint height = 0;

    const char* string[] = {"APA","IAC","ACC","AEB"};
    DVR_U8* pImage = srcparam->data;
    DVR_U8* RasterBuf   = GetRasterBuf(string[srcparam->rectype - OSD_SRC_REC_TYPE_APA], width, height);

    if(RasterBuf == NULL)
        return DVR_RES_EFAIL;
    int space = 10;
    int circlelen = 7;
    int startypos = 100;
    int startxpos = (srcparam->imagewidth - 48 - OSD_PANE_WIDTH + 7) / 2 * 2;
    //printf("startxpos = %d width === %d height == %d\n",startxpos, width, height);
	//FILE *fp_out=fopen("output.yuv","wb+");
    /*for(int n = 0; n < circlelen; n++)
    {
        memset(pImage + (startypos - (space + circlelen) + n) * srcparam->imagewidth + startxpos - space + n, 235, width + (space + n)*2);
        memset(pImage + (startypos + height + space + n) * srcparam->imagewidth + startxpos - (space + circlelen) + n, 235, width + (space + n)*2);
        //memset(pImage + (startypos - (space + circlelen) + n) * srcparam->imagewidth + startxpos - (space + circlelen), 235, width + (space + circlelen)*2);
        //memset(pImage + (startypos + height + space + n) * srcparam->imagewidth + startxpos - (space + circlelen), 235, width + (space + circlelen)*2);
    }*/
    int starty = startypos - ((OSD_PANE_HEIGHT - height)>>1);
    int startx = startxpos - ((OSD_PANE_WIDTH - width)>>1);
    for(int m = starty;m < OSD_PANE_HEIGHT + starty; m++)
    {
        for (int n = 0; n < OSD_PANE_WIDTH; n++)
        {
            if(m >= startypos && m <= height + startypos && 
                n + startx >= startxpos && n + startx <= width + startxpos)
                continue;

            int value = picpane[(m -starty)*OSD_PANE_WIDTH + n];
            if(value <= 0x12)
                continue;

            pImage[m * srcparam->imagewidth + startx + n] = value;
        }
    }

    for(int i = startypos; i < height + startypos; i++)
    {
        /*for(int n = 0; n < circlelen; n++)
        {
            memset(pImage + (i - space) * srcparam->imagewidth + startxpos - (space + circlelen) + n, 235, 1 + n);
            memset(pImage + (i - space) * srcparam->imagewidth + startxpos + width + space + n, 235, 1 + n);
            //memset(pImage + (i - space) * srcparam->imagewidth + startxpos - (space + circlelen) + n, 235, circlelen);
            //memset(pImage + (i - space) * srcparam->imagewidth + startxpos + width + space + n, 235, circlelen);
        }*/

        for (int k = 0; k < width; k++)
        {
            int byteIndex = (width * (i - startypos) + k) >> 3;
            int bitIndex = k % 8;
            int shift = 8 - bitIndex - 1;
            if ((RasterBuf[byteIndex] >> shift) & 0x1)
            {
                pImage[i * srcparam->imagewidth + startxpos + k] = 235;
            }
        }
    }
    free(RasterBuf);
    //fwrite(pImage,1,srcparam->imagewidth*srcparam->imageheight*3/2,fp_out);
	//fclose(fp_out);
#else
    if(bmpimg == NULL)
        return DVR_RES_EFAIL;

    BMPIMAGE srcbmpimg;
    srcbmpimg.data      = NULL;
    srcbmpimg.channels  = 3;
    srcbmpimg.width     = BMP_WIGTH;
    srcbmpimg.height    = BMP_HEIGHT;
    const char* thumbname = NULL;
    if(srcparam->rectype  == OSD_SRC_REC_TYPE_APA)
        thumbname = THUMB_APA;
    else if(srcparam->rectype  == OSD_SRC_REC_TYPE_IACC)
        thumbname = THUMB_IAC;
    else if(srcparam->rectype  == OSD_SRC_REC_TYPE_ACC)
        thumbname = THUMB_ACC;
    else if(srcparam->rectype  == OSD_SRC_REC_TYPE_AEB)
        thumbname = THUMB_AEB;

    int ret = LoadBMP(thumbname, &srcbmpimg, BIT24);
    if(ret == 0)
    {
        unsigned char *rgb24_buffer = srcbmpimg.data;
        //int temp = bmpimg->height;
        //printf("R %x G %x B %x\n",bmpimg->data[2],bmpimg->data[1],bmpimg->data[0]);
        //printf("R %x G %x B %x\n",bmpimg->data[(temp*bmpimg->width+0)*3+2],bmpimg->data[(temp*bmpimg->width+0)*3+1],bmpimg.data[(temp*bmpimg->width+0)*3+0]);
        int startx = bmpimg->width - srcbmpimg.width - 5;
        int starty = 5;
        #if 0
        //��ȡʱ�ǵ��Ŷ���
        for(int j = 0;j < bmpimg->height;j++)
        {
            for(int i= 0;i< bmpimg->width;i++)
            {
                if(i > startx && i < startx + srcbmpimg.width && j > starty && j < starty + srcbmpimg.height)
                {
                    unsigned char tempr = bmpimg->data[((temp - j)*bmpimg->width+i)*3+2];//R
                    unsigned char tempg = bmpimg->data[((temp - j)*bmpimg->width+i)*3+1];//G
                    unsigned char tempb = bmpimg->data[((temp - j)*bmpimg->width+i)*3+0];//B
                    //printf("i = %d j = %d\n",i,j);
                    bmpimg->data[((temp - j)*bmpimg.width+i)*3+2]= rgb24_buffer[((srcbmpimg.height - (j - starty))*srcbmpimg.width + i - startx)*3 + 0]/2 + tempr/2;//R
                    bmpimg->data[((temp - j)*bmpimg.width+i)*3+0]= rgb24_buffer[((srcbmpimg.height - (j - starty))*srcbmpimg.width + i - startx)*3 + 2]/2 + tempb/2;//B
                    bmpimg->data[((temp - j)*bmpimg.width+i)*3+1]= rgb24_buffer[((srcbmpimg.height - (j - starty))*srcbmpimg.width + i - startx)*3 + 1]/2 + tempg/2;//G
                }
            }
        }
        #else
        for(int j = 0;j < bmpimg->height;j++)
        {
            for(int i = 0;i< bmpimg->width;i++)
            {
                if(i >= startx && i < startx + srcbmpimg.width && j >= starty && j < starty + srcbmpimg.height)
                {
                    int offset0 = (j * bmpimg->width + i) * 3;
                    unsigned char tempr = bmpimg->data[offset0 + 2];//R
                    unsigned char tempg = bmpimg->data[offset0 + 1];//G
                    unsigned char tempb = bmpimg->data[offset0 + 0];//B
                    int offset1 = ((j - starty) * srcbmpimg.width + i - startx) * 3;
                    //printf("(%d,%d)------------(%u,%u,%u)\n",i - startx,j - starty,rgb24_buffer[offset1 + 0],rgb24_buffer[offset1 + 1],rgb24_buffer[offset1 + 2]);
                    unsigned char rgb24r = rgb24_buffer[offset1 + 0];//R
                    unsigned char rgb24g = rgb24_buffer[offset1 + 1];//G
                    unsigned char rgb24b = rgb24_buffer[offset1 + 2];//B
                    //corner
                    if((rgb24r == 255 && rgb24g == 0 && rgb24b == 0) ||
                        (rgb24r > 250 && rgb24g < 40 && rgb24b < 40))
                        continue;

                    if(rgb24r == 219 && rgb24g == 142 && rgb24b == 152)
                    {
                        rgb24r = 181;
                        rgb24g = 193;
                        rgb24b = 213;
                    }

                    //grey---to black (translucence)
                    if(rgb24r == 73 && rgb24g == 73 && rgb24b == 74)
                    {
                        rgb24r = 0;
                        rgb24g = 0;
                        rgb24b = 0;
                        bmpimg->data[offset0 + 2] = (rgb24r + tempr) >> 1;//R
                        bmpimg->data[offset0 + 0] = (rgb24b + tempb) >> 1;//B
                        bmpimg->data[offset0 + 1] = (rgb24g + tempg) >> 1;//G
                        continue;
                    }

                    //character (opacification)
                    //if(i > startx + 5 && i < startx + srcbmpimg.width - 5 && j >= starty + 5 && j < starty + srcbmpimg.height - 5)
                    {
                        bmpimg->data[offset0 + 2] = rgb24r;//R
                        bmpimg->data[offset0 + 0] = rgb24b;//B
                        bmpimg->data[offset0 + 1] = rgb24g;//G
                        //continue;
                    }

                    //translucence
                    /*bmpimg->data[offset0 + 2] = (rgb24r + tempr) >> 1;//R
                    bmpimg->data[offset0 + 0] = (rgb24b + tempb) >> 1;//B
                    bmpimg->data[offset0 + 1] = (rgb24g + tempg) >> 1;//G*/
                }
            }
        }
        #endif
    }
    freeImage(&srcbmpimg);
#endif
    double end = ms_time();
    DPrint(DPRINT_INFO, "SetString(%p type %d) imagewidth %d takes %lf ms\n",this,srcparam->type,srcparam->imagewidth,end-start);

    return DVR_RES_SOK;
}

