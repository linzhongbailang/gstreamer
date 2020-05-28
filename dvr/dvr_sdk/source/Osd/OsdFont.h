#ifndef __OSD_FONT_H__
#define __OSD_FONT_H__

#include <map>
#include <list>
#include <string.h>
#include "OsdFile.h"

#define UCS_FONT_MAGIC "ofilm ucs font"

#define NAME_LEN                 32
#define CHANNEL_NAME_SIZE        64
#define MUTLINT_TITLE_SIZE       320
#define LABEL_LEN                32
#define VD_MAX_PATH              220
#define EXPORT_LOG_LEN           512*1024

typedef const char* VD_PCSTR;
typedef const char	c_schar;
/////////////////////////////////////////////////
///////////////// 单件模式
#define PATTERN_SINGLETON_DECLARE(classname)	\
static classname * instance();

#define PATTERN_SINGLETON_IMPLEMENT(classname)	\
classname * classname::instance()		\
{												\
	static classname * _instance = NULL;		\
	if( NULL == _instance)						\
	{											\
		_instance = new classname;				\
	}											\
	return _instance;							\
}												
/////////////////////////////////////////////////

template<class T>
struct strless{
	inline bool operator()(const T& x, const T& y) const
	{
		return (strcmp(x ,y) < 0);
	}
};

typedef struct
{
	int left;
	int top;
	int right;
	int bottom;
}VD_RECT,*VD_PRECT;
typedef struct
{
	int lX;
	int lY;
}VD_POINT;

typedef struct {
    int w;
    int h;
}VD_SIZE,*VD_PSIZE;

typedef const VD_RECT* VD_PCRECT;
typedef const VD_POINT* VD_PCPOINT;
typedef const VD_SIZE* VD_PCSIZE;

struct UCS_FONT_HEADER
{
	char  magic[16];	//标志
	uint size;		//字库总大小
	uint blocks;	//块数
};

struct UCS_FONT_BLOCK
{
	ushort start;		//编码起始值
	ushort end;		//编码结束值
	ushort width;		//点阵宽度
	ushort height;	//点阵高度
	uint roffs;	//字体点阵数据偏移
	uint xoffs;	//字体扩展数据偏移
};

enum FONTSIZE
{
	FONTSIZE_SMALL, //!小字体
	FONTSIZE_NORMAL, //普通字体
	FONTSIZE_BIG,	//大字体
	FONTSIZE_ALL
};

enum FONT_STYLE
{
	FS_NORMAL = 0x0000,		///< 正常字体
	FS_BOLD = 0x0001,		///< 粗体字
	FS_SMALL = 0x0002,
	FS_ITALIC	= 0x0004,	///< 斜体字
	FS_OUTLINE	= 0x0008,	///< 环绕(勾边)效果
	FS_SCALING	= 0x0010,	///< 对不合适的点阵字体进行缩放
	FS_BIG		= 0x0020,
};

//!书写顺序
typedef enum FONT_ORDER
{
	FO_Left2Right,	//!从左向右
	FO_Right2Left, //!从右向左
}FONT_ORDER_E;

typedef std::map<VD_PCSTR, VD_PCSTR, strless<VD_PCSTR> > MAPSTRING;
class CLocales/*:public CObject*/
{
public:
	static int S_SetFontFile(c_schar* f_small_font, c_schar* f_font, c_schar* f_big_font, signed char** stringfile, int filenum);
public:
	PATTERN_SINGLETON_DECLARE(CLocales);
private:
	CLocales();
	~CLocales();
public:

#ifndef __FREETYPE_SUPPORT__
	//装载一个字符的字体, 传入参数为UCS-2编码, 返回值为字体宽度
	DVR_U8  GetOneFontFromFile(ushort code, DVR_U8 *p);
	DVR_U8   GetCharRaster(ushort code, DVR_U8* p = NULL, FONTSIZE fontsize = FONTSIZE_NORMAL);
    void    TextToRaster_arbic(DVR_U8 * pBuffer, VD_PSIZE pSize, VD_PCRECT pRect, VD_PCSTR str, FONTSIZE sizetype = FONTSIZE_NORMAL);
    void    WordOut(DVR_U8 * pBuffer, VD_PSIZE pSize, VD_PCRECT pRect, FONTSIZE sizetype, ushort code, int xoffset, int x, int y);
#endif

	//!enum ui_language_t 枚举
	void SetLanguage(int index);
	int  GetLanuage();
	ushort GetCharCode(VD_PCSTR pch, int *pn);
	int GetFontSize(VD_SIZE *pSize, FONTSIZE fontsize = FONTSIZE_NORMAL);
	//二分发查找常用汉字
	int FindCommonChar(ushort code);
	int GetTextExtent(VD_PCSTR str, int len=1024, FONT_STYLE fs = FS_NORMAL);
    int GetTextExtentEx(VD_PCSTR str, int len = 1024, int nFontSize = 18);

	//!得到当前语言书写顺序
	FONT_ORDER_E GetFontOrder();
	FONT_ORDER_E GetFontOrderByUnicode(ushort code);
    void TextToRaster(DVR_U8 * pBuffer, VD_PSIZE pSize, VD_PCRECT pRect, VD_PCSTR str, FONTSIZE fontsize = FONTSIZE_NORMAL);
	MAPSTRING m_mapStrings;
private:
	int m_iLanguage;

#ifndef __FREETYPE_SUPPORT__
	UCS_FONT_HEADER m_UFH;
	UCS_FONT_BLOCK *m_pUFB;
	int m_bFontValid;
	CFile m_FileFont;

//#ifdef ENC_SHOW_SMALL_FONT
	int m_bFontValidSmall;
	CFile m_FileFontSmall;
	UCS_FONT_BLOCK *m_pUFBSmall;
	UCS_FONT_HEADER m_UFHSmall;
	int m_nFontBytesSmall;
	VD_SIZE m_sizeFontSmall;
    DVR_U8* m_pASCIIFontSmall;
//#endif

	//!大字体
	int m_bFontValidBig;
	CFile m_FileFontBig;
	UCS_FONT_BLOCK *m_pUFBBig;
	UCS_FONT_HEADER m_UFHBig;
	int m_nFontBytesBig;
	VD_SIZE m_sizeFontBig;
	DVR_U8* m_pASCIIFontBig;

	VD_SIZE m_sizeFont;
	int m_nFontBytes;
	DVR_U8* m_pCommonFont;		//常用字字体
	DVR_U8* m_pASCIIFont;			//ASCII字符字体

	static std::string m_small_fontbin;
	static std::string m_fontbin;
	static std::string	m_big_fontbin;
#endif

	CFile m_FileStrings;
//	char* m_pbuf;
	int m_nCommonChars;			//常用字个数
	ushort* m_pCommonChars;		//常用字数组

#define MAX_LANG_NUM	200
	static std::string m_strings_file[MAX_LANG_NUM];
	static int	m_lang_num;
	static bool s_m_bInit;
};

//!暂行用法，因为gui内部使用了太多local相关的东西
//!全部改范围太大，暂时无使用这种方式

//!使用此函数前先要-InitLocalIns
VD_PCSTR GetParsedString(VD_PCSTR key);
VD_PCSTR LOADSTR(VD_PCSTR key);

#endif//__OSD_FONT_H__

