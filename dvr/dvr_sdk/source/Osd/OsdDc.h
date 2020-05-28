//#ifndef _OSD_DC_H_
//#define _OSD_DC_H_
#include <string.h>
#include <list>
#include <vector>
#include <functional>
#include "OsdFont.h"
#include <ft2build.h>
#include <gst/gst.h>
#include FT_FREETYPE_H
#include "DvrMutexLocker.h"


typedef const char* VD_PCSTR;
#ifndef MAX
#define MAX(a,b)                (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)                (((a) < (b)) ? (a) : (b))
#endif

#define DC_FONT_SIZE 18

#define	TEXTDCPrintf(fmt, args ...)	printf("<CTextDC>%s(%d)::" fmt , __FUNCTION__, __LINE__, ## args)
#define MAX_TEXTOUT_LINES 16 //文本输出自动换行最大行数
int IsIncludeArbic(VD_PCSTR str);
ushort ArbicTransform(ushort code, ushort before_code, ushort after_code, int &bIsSkip);

class CTextDC// : public CObject
{
public:
    CTextDC(void);
    virtual ~CTextDC(void);
//public:
//	PATTERN_SINGLETON_DECLARE(CTextDC);
public:
    std::string GetText(void){return m_text;}
    int         SetText(const char* text);
    const char* GetTextPTR(void) { return m_text.c_str();}
    int         TextOut(const char* text, ushort txtsz = 16);
    int         GetTextExtent(uint* width, uint* height);
    ushort      GetWidth(void){ return m_pixeltxt;}
    ushort      GetHeight(void){ return m_linenum;}
    ushort      GetLineTop(void){ return m_linetop;}
    ushort      GetLineBtm(void){ return m_linebtm;}
    ushort      GetPitch(void){ return m_pitch;}
    DVR_U8*      GetBuffer(void){ return m_buffer;}
    ushort      GetCharCount() {return m_num;};
    DVR_U8*      GetCharWidth() {return m_pWidth;};
    int         SetFontSize(uint nSize);
    uint        GetFontSize() {return m_nSize;};
    int         GetCharBytes(int nCount);
    int         TextToRaster(DVR_U8 *pBuf, VD_PSIZE pSize);
	void        GetOutLines(VD_PCRECT ptRect, ushort* pusLineStartPos, DVR_U8& uchLineNum);
	DVR_U8  	GetOutLineNum(VD_PCRECT ptRect);
	int         GetFontOrder(DVR_U8 uchIndex);
	ushort      GetUniCode(DVR_U8 uchIndex);

    int         GetCharBytesEx(int nIndex);
    int         GetCharWidthEx(int nBegin, int nEnd);
    std::string GetText(int nIndex);
    void        RemoveChar(int nIndex);
    void        InsertChar(int nIndex, char* szValue, int nLength);

protected:
    int TextOut(ushort code, DVR_U8* width);
protected:
    std::string m_text;
	DVR_U8*	m_buffer;		        //字符图像缓冲区， 缓冲区大小为：m_linenum * m_pitch
	ushort	m_buflen;		        //字符图像缓冲区长度
	ushort	m_pitch;		        //字符图像缓冲区每行的字节数
	ushort	m_pixelnum;		        //最大像素数
	ushort	m_linenum;		        //最大线数
    ushort	m_pixeltxt;		        //字符串水平最大像素数
	ushort	m_linebase;		        //基线,	m_linebase < m_linenum
	ushort	m_linetop;		        //最高点线号, m_linetop<m_linebtm
	ushort	m_linebtm;		        //最低点线号, m_lisebtm<m_linenum
	size_t  m_num;                  //字符数
	DVR_U8  *m_pWidth;              //每个字符宽度
	DVR_U8  *m_pBytes;              //每个字符字节数
	ushort  *m_pusUniCode;          //utf-8转utf-16后的字符串
	DVR_U8  m_uchTextOutLineNum;    //字符输出行数;0表示未计算,计算后至少为1
	ushort  *m_pusLineStartPos;     //每行字符输出的开始
    uint    m_nSize;

public:
   	static	int		Load(const char* path);
    static  int     Unload(void);
    static  int     GetTextExtent(const char* text, uint* width, uint* height, int count, ushort txtsz = 24);
    static  size_t  ToUCS2(const char* src, size_t size, ushort* ucs, DVR_U8* bytes, int& arabic);
    static  size_t  ToArabic(ushort* ucs, size_t num);
    static  void    Dump(FT_GlyphSlot slot);
    static  void    Dump(FT_Glyph_Metrics* metrics);

private:
    static  int     SetTextSize(uint txtsz, ushort* linenum=NULL, ushort* linebase=NULL, ushort* pixelmax=NULL);
private:
	static  DvrMutex	s_mutex;
	static	char*		s_fontpath;
	static	FT_Library	s_lib;
	static	FT_Face		s_face;
	static	uint		s_txtsz;    //字符大小

public:
	static  uint        s_defaultsize;  // 默认字符大小
	static  void    SetDefaultFontSize(uint nSize) {s_defaultsize = nSize;};
};
//#define g_textdc (*CTextDC::instance())
//#endif

