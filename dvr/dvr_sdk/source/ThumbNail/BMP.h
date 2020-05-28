#ifndef _BMP_H_
#define _BMP_H_

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum _BIT_DATA_TYPE_{
        BIT32 = 1,                      //��ȡ��洢��32λ
        BIT24 = 2,                      //��ȡ��洢��24λ
    }BITDATATYPE;

    typedef struct _BMPFILEHEAD_{
        unsigned char   type[2];        //�洢 'B' 'M'                    2�ֽ�
        unsigned int    size;           //λͼ�ļ���С                    4�ֽ�
        unsigned short  reserved1;      //������                          2�ֽ�
        unsigned short  reserved2;      //������                          2�ֽ�
        unsigned int    offBits;        //λͼ������ʼλ��                4�ֽ�
    }BMPHEAD;

    typedef struct _BMPFILEINFOHEAD_{
        unsigned int    selfSize;       //λͼ��Ϣͷ�Ĵ�С                 4�ֽ�
        long            bitWidth;       //λͼ�Ŀ��,������Ϊ��λ          4�ֽ�
        long            bitHeight;      //λͼ�ĸ߶�,������Ϊ��λ          4�ֽ�
        unsigned short  bitPlanes;      //Ŀ���豸�ļ���,����Ϊ1           2�ֽ�
        unsigned short  pixelBitCount;  //ÿ�����������λ��               2�ֽ�
        unsigned int    compression;    //λͼѹ������,0(��ѹ��)           4�ֽ�
        unsigned int    sizeImage;      //λͼ�Ĵ�С,���ֽ�Ϊ��λ          4�ֽ�
        long            pixelXPerMeter; //λͼ��ˮƽ�ֱ���,ÿ��������      4�ֽ�
        long            pixelYPerMeter; //λͼ�Ĵ�ֱ�ֱ���,ÿ��������      4�ֽ�
        unsigned int    colorUsed;      //λͼʵ��ʹ�õ���ɫ���е���ɫ��   4�ֽ�
        unsigned int    colorImportant; //λͼ��ʾ��������Ҫ����ɫ��       4�ֽ�
    }BMPINFOHEAD;

    typedef struct _IMAGE_{
        int width;
        int height;
        int channels;
        unsigned char * data;
    }BMPIMAGE;

    int LoadBMP(const char * file, BMPIMAGE* out_img, BITDATATYPE bit_data_type);
    int WriteBMP(const char * file, BMPIMAGE * in_img, BITDATATYPE bit_data_type);
    int freeImage(BMPIMAGE * img);

#ifdef __cplusplus
}
#endif

#endif