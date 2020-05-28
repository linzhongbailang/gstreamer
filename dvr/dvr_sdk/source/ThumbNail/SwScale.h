#ifndef _SW_SCALE_H_
#define _SW_SCALE_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif

typedef struct _tagInterfaceData
{
    unsigned char *pDataBuffer;
    int dataSize;
    int width;
    int height;
    AVPixelFormat pixfmt;
}SwScaleInterfaceData_t;

class SwScale
{
public:
    SwScale();
    ~SwScale();

    int imgProcess(void);

    SwScaleInterfaceData_t m_InputData;
    SwScaleInterfaceData_t m_OutputData;

private:

};

#endif