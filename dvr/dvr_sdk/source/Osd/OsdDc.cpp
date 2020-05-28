#include "OsdDc.h"
#include <freetype.h>

DvrMutex	CTextDC::s_mutex;
char*		CTextDC::s_fontpath     = NULL;
FT_Library	CTextDC::s_lib          = NULL;
FT_Face		CTextDC::s_face         = NULL;
uint		CTextDC::s_txtsz        = 0;        //字符大小
uint        CTextDC::s_defaultsize  = 18;

#define MAX_CNT_SET1            (29)
#define MAX_CNT_SET2            (42)
#define MAX_CNT_SET3            (4)
#define	PERSIAN_AlPHABET_CNT    (7)

typedef enum
{
	ARBIC_FONT_POS_last,
	ARBIC_FONT_POS_first,
	ARBIC_FONT_POS_middle,
	ARBIC_FONT_POS_alone,
}ARBIC_FONT_POS_E;

const ushort Arbic_Position[][4]=/*last,first,midile,alone*/
{
	{ 0xfe80, 0xfe80, 0xfe80, 0xfe80},/*0x0621*/
	{ 0xfe82, 0xfe81, 0xfe82, 0xfe81},
	{ 0xfe84, 0xfe83, 0xfe84, 0xfe83},
	{ 0xfe86, 0xfe85, 0xfe86, 0xfe85},
	{ 0xfe88, 0xfe87, 0xfe88, 0xfe87},
	{ 0xfe8a, 0xfe8b, 0xfe8c, 0xfe89},/*26*/
	{ 0xfe8e, 0xfe8d, 0xfe8e, 0xfe8d},
	{ 0xfe90, 0xfe91, 0xfe92, 0xfe8f},
	{ 0xfe94, 0xfe93, 0xfe94, 0xfe93},
	{ 0xfe96, 0xfe97, 0xfe98, 0xfe95},
	{ 0xfe9a, 0xfe9b, 0xfe9c, 0xfe99},
	{ 0xfe9e, 0xfe9f, 0xfea0, 0xfe9d},
	{ 0xfea2, 0xfea3, 0xfea4, 0xfea1},
	{ 0xfea6, 0xfea7, 0xfea8, 0xfea5},
	{ 0xfeaa, 0xfea9, 0xfeaa, 0xfea9},
	{ 0xfeac, 0xfeab, 0xfeac, 0xfeab},
	{ 0xfeae, 0xfead, 0xfeae, 0xfead},
	{ 0xfeb0, 0xfeaf, 0xfeb0, 0xfeaf},
	{ 0xfeb2, 0xfeb3, 0xfeb4, 0xfeb1},
	{ 0xfeb6, 0xfeb7, 0xfeb8, 0xfeb5},
	{ 0xfeba, 0xfebb, 0xfebc, 0xfeb9},
	{ 0xfebe, 0xfebf, 0xfec0, 0xfebd},/*36*/
	{ 0xfec2, 0xfec3, 0xfec4, 0xfec1},/*37*/
	{ 0xfec6, 0xfec7, 0xfec8, 0xfec5},
	{ 0xfeca, 0xfecb, 0xfecc, 0xfec9},
	{ 0xfece, 0xfecf, 0xfed0, 0xfecd},
	{ 0x63b, 0x63b, 0x63b, 0x63b},
	{ 0x63c, 0x63c, 0x63c, 0x63c},
	{ 0x63d, 0x63d, 0x63d, 0x63d},
	{ 0x63e, 0x63e, 0x63e, 0x63e},
	{ 0x63f, 0x63f, 0x63f, 0x63f},
	{ 0x640, 0x640, 0x640, 0x640},/*'-'*/
	{ 0xfed2, 0xfed3, 0xfed4, 0xfed1},
	{ 0xfed6, 0xfed7, 0xfed8, 0xfed5},
	{ 0xfeda, 0xfedb, 0xfedc, 0xfed9},
	{ 0xfede, 0xfedf, 0xfee0, 0xfedd},
	/*{0x644, 0xfedd, 0xfedf, 0xfee0, 0xfede},dengx add*/
	{ 0xfee2, 0xfee3, 0xfee4, 0xfee1},
	{ 0xfee6, 0xfee7, 0xfee8, 0xfee5},
	{ 0xfeea, 0xfeeb, 0xfeec, 0xfee9},
	{ 0xfeee, 0xfeed, 0xfeee, 0xfeed},
	{ 0xfef0, 0xfef3, 0xfef4, 0xfeef},
	{0xfef2, 0xfef3, 0xfef4, 0xfef1},/*0x64a*/
};

const ushort Persian_AlphabetTab[][4]=  // last,first,midile,alone
{
	{0xfb57, 0xfb58, 0xfb59, 0xfb56},   // 0x067e
	{0xfb7b, 0xfb7c, 0xfb7d, 0xfb7a},   // 0x0686
	{0xfb8b, 0xfb8a, 0xfb8b, 0xfb8a},   // 0x0698
	{0xfb8f, 0xfb90, 0xfb91, 0xfb8e},   // 0x06a9
	{0xfb93, 0xfb94, 0xfb95, 0xfb92},   // 0x06af
	{0xfba5, 0xfba4, 0xfba5, 0xfba4},   // 0x06c0
	{0xfbfd, 0xfbfe, 0xfbff, 0xfbfc},   // 0x06cc   // 0xfbfd, 0xfef3, 0xfef4, 0xfbfc
};

static ushort theSet1[MAX_CNT_SET1]={
	0x626,
	0x628,
	0x62a,
	0x62b,
	0x62c,
	0x62d,
	0x62e,
	0x633,
	0x634,
	0x635,
	0x636,
	0x637,
	0x638,
	0x639,
	0x63a,
	0x640,
	0x641,
	0x642,
	0x643,
	0x644,
	0x645,
	0x646,
	0x647,
	0x64a,

	0x67e,  // Persian
	0x686,
	//0x698,
	0x6a9,
	0x6af,
	0x6cc,
};

static ushort theSet2[MAX_CNT_SET2]={
	0x622,
	0x623,
	0x624,
	0x625,
	0x626,
	0x627,
	0x628,
	0x629,
	0x62a,
	0x62b,
	0x62c,
	0x62d,
	0x62e,
	0x62f,
	0x630,
	0x631,
	0x632,
	0x633,
	0x634,
	0x635,
	0x636,
	0x637,
	0x638,
	0x639,
	0x63a,
	0x640,
	0x641,
	0x642,
	0x643,
	0x644,
	0x645,
	0x646,
	0x647,
	0x648,
	0x649,
	0x64a,

	0x67e,  // Persian
	0x686,
	0x698,  // <jx> 20150323 发现当遇到0x6cc 0x698时，变形不对，将此处的0x698打开，theSet1的0x698关掉
	0x6a9,
	0x6af,
	0x6cc,
};

static ushort arabic_specs[][2]=			//0x644 特殊字符处理
{
	{0xFEF5, 0xFEF6},
	{0xFEF7, 0xFEF8},
	{0xFEF9, 0xFEFA},
	{0xFEFB, 0xFEFC},
};


static int IsLinkPrev(const ushort wCode)
{
	if(wCode != 0)
	{
		ushort *pwData = (ushort*)theSet1;
		long low,high,mid;
		low = 0;
		high = MAX_CNT_SET1- 1;

		while(low <= high)
		{
			mid = (high+low)/2;
			if(pwData[mid] == wCode)
				return 1;
			else if(wCode > pwData[mid])
				low = mid + 1;
			else
				high = mid - 1;
		}
	}

	return 0;
}

/*   后连接判断
			返回值:  0. 非后连接	 1. 后连接
*/
static int IsLinkNext(const ushort wCode)
{
	if(wCode != 0)
	{
		ushort *pwData = (ushort*)theSet2;
		long low,high,mid;
		low = 0;
		high = MAX_CNT_SET2- 1;

		while(low <= high)
		{
			mid = (high+low)/2;
			if(pwData[mid] == wCode)
				return 1;
			else if(wCode > pwData[mid])
				low = mid + 1;
			else
				high = mid - 1;
		}
	}
	return 0;
}

static int IsArbic(const ushort word)
{
	if( (word >= 0x0600 && word <= 0x06FF)
		//|| (word >= 0x0750 && word <= 0x077F)
		|| (word >= 0xFB50 && word <= 0xFDFF)
		|| (word >= 0xFE70 && word <= 0xFEFF) )
		return 1;

	return 0;
}
int IsIncludeArbic(VD_PCSTR str)
{
	int i, len = strlen(str);
    int cl;
    ushort code;

	for (i = 0; i < len; i += cl)
	{
		code = CLocales::instance()->GetCharCode(&str[i], &cl);
		if (cl == 0)
			break;
		if (IsArbic(code))
			return 1;
	}

	return 0;
}

/*   规则1转换
     返回值:  成功, 返回转换后的编码；  失败, 返回 -1.
*/
static int ConvertRule1(const ushort wPrevCode, const ushort wCurCode, const ushort wNextCode)
{
	if(wCurCode >= 0x621 && wCurCode <= 0x64a)
	{
		int nPos = ARBIC_FONT_POS_alone;

		int bLinkPrev = IsLinkPrev(wPrevCode);
		int bLinkNext = IsLinkNext(wNextCode);

		if(bLinkPrev && bLinkNext)
			nPos = ARBIC_FONT_POS_middle;
		else if(bLinkPrev)
			nPos = ARBIC_FONT_POS_last;
		else if(bLinkNext)
			nPos = ARBIC_FONT_POS_first;

		return Arbic_Position[wCurCode-0x621][nPos];
	}

	return -1;
}
/*   规则2转换
     返回值： 成功, 返回转换后的编码；  失败, 返回 -1.
*/
static int ConvertRule2(const ushort wPrevCode, const ushort wCurCode, const ushort wNextCode)
{
	if(wCurCode == 0x644)
	{
		static ushort theSet3[MAX_CNT_SET3] = {0x622, 0x623, 0x625, 0x627};
		ushort i = 0;
		for(i = 0; i < MAX_CNT_SET3; i++)
		{
			if(theSet3[i] == wNextCode)
				break;
		}
		if(i != MAX_CNT_SET3)
			return arabic_specs[i][IsLinkPrev(wPrevCode)];
	}
	return -1;
}

// 主要针对Persian
static int ConvertRule3(const ushort wPrevCode, const ushort wCurCode, const ushort wNextCode)
{
	if(wCurCode >= 0x600 && wCurCode <= 0x6ff)
	{
		int i;
		ushort Persian_Alphabets[] = {0x67e, 0x686, 0x698, 0x6a9, 0x6af, 0x6c0, 0x6cc};

		for( i = 0; i < PERSIAN_AlPHABET_CNT; i++)
			if(wCurCode == Persian_Alphabets[i])
				break;

		if(i < PERSIAN_AlPHABET_CNT)
		{
			int nPos = ARBIC_FONT_POS_alone;

			int bLinkPrev = IsLinkPrev(wPrevCode);
			int bLinkNext = IsLinkNext(wNextCode);

			if(bLinkPrev && bLinkNext)
				nPos = ARBIC_FONT_POS_middle;
			else if(bLinkPrev)
				nPos = ARBIC_FONT_POS_last;
			else if(bLinkNext)
				nPos = ARBIC_FONT_POS_first;
			return Persian_AlphabetTab[i][nPos];
		}
	}

	return -1;
}

ushort ArbicTransform(ushort code, ushort before_code, ushort after_code, int &bIsSkip)
{
    bIsSkip = FALSE;
    if (!IsArbic(code))
    {
        return code;
    }

    int nCode = code;
    nCode = ConvertRule2(before_code, code, after_code);
    if (nCode == -1)
    {
        nCode = ConvertRule1(before_code, code, after_code);
        if (nCode == -1)
        {
            nCode = ConvertRule3(before_code, code, after_code);
        }
    }
    else
    {
        bIsSkip = TRUE;
    }

    return (nCode != -1) ? (ushort)nCode : code;
}

//判断字符是否构成单词: 英文,西欧,俄文,常用字符等拼在一起认为构成单词
#define IN_WORD(x) ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') \
	|| x == '(' || x == ')' || (x >= '0' && x <= '9') \
	|| (x >= 0x0080 && x <= 0x00ff) \
	|| (x >= 0x0400 && x <= 0x04ff) \
	|| x == '"')  //added by wyf on 090909


int CTextDC::Load(const char* path)
{
    int     ret;
	DvrMutexLocker mutexLocker(&s_mutex);
    if( path == NULL )
        return -1;

   	if( s_lib != NULL )
	{
		FT_Done_FreeType(s_lib);
		s_lib	= NULL;
		s_face	= NULL;
	}
	if( s_fontpath != NULL )
	{
		free(s_fontpath);
        s_fontpath = NULL;
	}
	ret = FT_Init_FreeType(&s_lib);
    if( ret != 0 )
    {
        TEXTDCPrintf("FT_Init_FreeType Error(ret = %d)\n", ret);
        return -1;
    }
   	ret = FT_New_Face(s_lib, path, 0, &s_face);
    if( ret != 0 )
    {
        TEXTDCPrintf("FT_New_Face Error( ret = %d, path = %s)\n", ret, path);
        FT_Done_FreeType(s_lib);
        s_lib = NULL;
        return -1;
    }
    ret = FT_Set_Pixel_Sizes(s_face, 0, DC_FONT_SIZE);
    if( ret != 0 )
    {
        TEXTDCPrintf("FT_Set_Pixel_Sizes Error( ret = %d)\n", ret);
        return -1;
    }
    int len = strlen(path);
    s_fontpath = (char*)malloc( len+1 );
    memcpy(s_fontpath, path, len);
    s_fontpath[len] = '\0';
    SetTextSize(DC_FONT_SIZE);
    FT_Size	size = s_face->size;
	TEXTDCPrintf("text size = 16: ascender = %d, descender = %d, height = %d, width = %d\n",
        (int)(size->metrics.ascender>>6), (int)(size->metrics.descender>>6),
        (int)(size->metrics.height>>6),   (int)(size->metrics.max_advance>>6) );
    TEXTDCPrintf("Load Font(%s) OK\n", path);
    return 0;
}
int CTextDC::Unload(void)
{
	DvrMutexLocker mutexLocker(&s_mutex);
   	if( s_lib != NULL )
	{
		FT_Done_FreeType(s_lib);
		s_lib	= NULL;
		s_face	= NULL;
	}
	if( s_fontpath != NULL )
	{
		free(s_fontpath);
        s_fontpath = NULL;
	}
    return 0;
}

int CTextDC::SetTextSize(uint txtsz, ushort* linenum, ushort* linebase, ushort* pixelmax)
{
    int ret;
    if( s_face == NULL )
        return -1;
    if( txtsz < 4 )
        txtsz = s_txtsz;
    if( txtsz != s_txtsz )
    {
		ret = FT_Set_Pixel_Sizes(s_face, 0, txtsz);
        if( ret != 0 )
        {
            TEXTDCPrintf("FT_Set_Pixel_Sizes Error( ret = %d, txtsz = %d)\n", ret, txtsz);
            return -1;
        }
        s_txtsz = txtsz;
    }
	FT_Size	size = s_face->size;
	if( linenum	!= NULL )
		*linenum	= (ushort)(size->metrics.height>>6);
	if( linebase!= NULL )
		*linebase	= (ushort)((size->metrics.height>>6) + (size->metrics.descender>>6) - 1);
   	if( pixelmax != NULL )
        *pixelmax   = (ushort)(size->metrics.max_advance>>6);
    return 0;
}
void CTextDC::Dump(FT_GlyphSlot slot)
{
	TEXTDCPrintf("<=================Dump FT_GlyphSlot ( slot = %p)================>\n", (void*)slot);
	if( slot == NULL )
		return;
	TEXTDCPrintf("\tbitmap_left = %d, bitmap_top = %d\n", slot->bitmap_left, slot->bitmap_top);
	TEXTDCPrintf("\tbitmap_width= %d, bitmap_rows= %d, bitmap_pitch = %d\n",
		slot->bitmap.width, slot->bitmap.rows, slot->bitmap.pitch);
	Dump(&slot->metrics);
	TEXTDCPrintf("\tlinearHoriAdvance = %f, linearVertAdvance = %f\n",
		slot->linearHoriAdvance/65536.0, slot->linearVertAdvance/65536.0);
	TEXTDCPrintf("\tadvance.x = %.3f, advance.y = %.3f\n", slot->advance.x/64.0, slot->advance.y/64.0);
	TEXTDCPrintf("\n");
}
void CTextDC::Dump(FT_Glyph_Metrics* metrics)
{
	TEXTDCPrintf("<FT_Glyph_Metrics>\n");
	if( metrics == NULL )
		return;
	TEXTDCPrintf("\twidth = %.3f, height = %.3f\n", metrics->width/64.0, metrics->height/64.0);
	TEXTDCPrintf("\thoriBearingX = %.3f, horiBearingY = %.3f, horiAdvance = %.3f\n",
		metrics->horiBearingX/64.0, metrics->horiBearingY/64.0, metrics->horiAdvance/64.0);
	TEXTDCPrintf("\tvertBearingX = %.3f, vertBearingY = %.3f, vertAdvance = %.3f\n",
		metrics->vertBearingX/64.0, metrics->vertBearingY/64.0, metrics->vertAdvance/64.0);
	TEXTDCPrintf("</FT_Glyph_Metrics>\n");
}
size_t CTextDC::ToUCS2(const char* src, size_t size, ushort* ucs, DVR_U8* bytes, int& arabic)
{
    CLocales*   locales = CLocales::instance();
    size_t      num    = 0;
    int         l      = 0;
    int         n;
    arabic      =  FALSE;
    if( ucs == NULL || size == 0 || src == NULL )
        return 0;
	for (n = 0; n < (int)size; n += l)
	{
		ucs[num] = locales->GetCharCode(&src[n], &l);
        bytes[num] = l;
		if(l == 0 || ucs[num] == 0)
		{
			break;
		}
        if( !arabic && IsArbic(ucs[num]) )
            arabic = TRUE;
		else if (('n' == ucs[num]) && (num > 0) && ('\\' == ucs[num - 1]))
		{
			ucs[num - 1] = '\n';
			continue;
		}
        num++;
        if( num>= size )
            break;
	}
    return num;
}
size_t CTextDC::ToArabic(ushort* ucs, size_t num)
{
    uint    idx;
    uint    cnt;
    //uint    arabnum;
    //uint    idxarab;
    int bSkip;
    ushort  code;
    ushort  codecurr;
    ushort  codenext;
    ushort  codeprev;
    //ushort* arabic;
    if( ucs == NULL || num == 0)
        return 0;
    //arabic = (ushort*)alloca( num * sizeof(ushort) );
    idx         = 0;
    cnt         = 0;
    codecurr    = 0;
    codenext    = 0;
    codeprev    = 0;
    //arabnum     = 0;
    bSkip       = FALSE;
    while( idx < num && cnt < num )
    {
        code = ucs[idx];
        /*if( !IsArbic(code)&&code>0x20&&code<0x600)
        {
            for( idxarab = 0; idxarab < arabnum && arabnum > 0; idxarab++)
            {
                ucs[cnt] = arabic[idxarab];//arabic[arabnum - 1 - idxarab];
                cnt++;
            }
            if( cnt < idx )
                ucs[cnt] = code;
            arabnum  = 0;
            codecurr = 0;
            codenext = 0;
            codeprev = 0;
            cnt++;
            idx++;
            continue;
        }*/
        if( idx + 1 < num )
            codenext = ucs[idx + 1];
        else
            codenext = 0;
        bSkip       = FALSE;
        codecurr    = ArbicTransform(code, codeprev, codenext, bSkip);
        codeprev    = code;
       if( bSkip )
       {
           codeprev = codenext;
           idx++;
       }
        //arabic[arabnum] = codecurr;
        //arabnum++;
        ucs[cnt] = codecurr;
	 cnt++;
        idx++;
    }
   /*for( idxarab = 0; idxarab < arabnum && arabnum > 0; idxarab++)
    {
        ucs[cnt] = arabic[idxarab];//arabic[arabnum - 1 - idxarab];
        cnt++;
    }*/

    return cnt;
}

int  CTextDC::GetFontOrder(DVR_U8 uchIndex)
{
	if (uchIndex < m_num)
	{
		return CLocales::instance()->GetFontOrderByUnicode(m_pusUniCode[uchIndex]);
	}

	return -1;
}

ushort  CTextDC::GetUniCode(DVR_U8 uchIndex)
{
	if (uchIndex < m_num)
	{
		return m_pusUniCode[uchIndex];
	}

	return 0;
}

void  CTextDC::GetOutLines(VD_PCRECT ptRect, ushort* pusLineStartPos, DVR_U8& uchLineNum)
{
	DvrMutexLocker mutexLocker(&s_mutex);
	if (0 == m_uchTextOutLineNum && uchLineNum > 0 && pusLineStartPos) //计算总输出行数
	{
		int i = 0, n = 0;
//		ushort usNextStartPos = 0;
		DVR_U8  uchCurLineNum = 1;
		uint uiRectWidth = ptRect->right - ptRect->left;
		uint uiCurLineWidth = 0;
		uint uiWordWidth = 0;
		 //printf("11111111111111GetOutLines uchLineNum:%d m_num:%d\r\n", uchLineNum, m_num);
		pusLineStartPos[uchCurLineNum - 1] = 0;
		for (i = 0; i < (int) m_num; i++)
		{
			if (uchCurLineNum < uchLineNum)
			{
				//英文单词长度计算
				uiWordWidth = 0;
				n = i;
				if (!IN_WORD(m_pusUniCode[n]) && n < (int) (m_num - 1))
				{
					n++;
					while (n < (int) m_num && IN_WORD(m_pusUniCode[n]))
					{
						uiWordWidth += m_pWidth[n];
						n++;
					}
				}

				if ('\n' == m_pusUniCode[i])
				{
					uchCurLineNum++;
					pusLineStartPos[uchCurLineNum - 1] = i;
					uiCurLineWidth = 0;
				}
				else if ((uiCurLineWidth + m_pWidth[i] + uiWordWidth) <= uiRectWidth)
				{
					uiCurLineWidth += m_pWidth[i];
				}
				else if (uiCurLineWidth + m_pWidth[i] > uiRectWidth)
				{
					uchCurLineNum++;
					pusLineStartPos[uchCurLineNum - 1] = i;
					uiCurLineWidth = m_pWidth[i];
				}
				else
				{
					uchCurLineNum++;
					pusLineStartPos[uchCurLineNum - 1] = i + 1;
					uiCurLineWidth = 0;
				}
			}
		}
		uchLineNum = uchCurLineNum;
		m_uchTextOutLineNum = uchLineNum;
		m_pusLineStartPos = new ushort[m_uchTextOutLineNum];
		memcpy(m_pusLineStartPos, pusLineStartPos, sizeof(ushort) * m_uchTextOutLineNum);
		return;
	}

	if (m_uchTextOutLineNum <= uchLineNum)
	{
		uchLineNum = m_uchTextOutLineNum;
	}

	memcpy(pusLineStartPos, m_pusLineStartPos,  sizeof(ushort) * uchLineNum);
}
DVR_U8  CTextDC::GetOutLineNum(VD_PCRECT ptRect)
{
	if (0 == m_uchTextOutLineNum)
	{
		ushort usLineStartPos[MAX_TEXTOUT_LINES];
		DVR_U8 uchLineNum = MAX_TEXTOUT_LINES;
		uchLineNum = MAX_TEXTOUT_LINES;
		GetOutLines(ptRect, usLineStartPos, uchLineNum);
		m_uchTextOutLineNum = uchLineNum;
	}
	return m_uchTextOutLineNum;
}

CTextDC::CTextDC(void)
{
   	m_buffer    = NULL;		//字符图像缓冲区， 缓冲区大小为：m_linenum * m_pitch
	m_buflen    = 0;		//字符图像缓冲区长度
	m_pitch     = 0;		//字符图像缓冲区每行的字节数
	m_pixelnum  = 0;		//最大像素数
	m_linenum   = 0;		//最大线数
   	m_pixeltxt  = 0;		//字符串总的水平像素数
	m_linebase  = 0;		//基线,	m_linebase < m_linenum
	m_linetop   = 0;		//最高点线号, m_linetop<m_linebtm
	m_linebtm   = 0;		//最低点线号, m_lisebtm<m_linenum
	m_num       = 0;
    m_pWidth    = NULL;
    m_pBytes    = NULL;
	m_pusUniCode = NULL;
	m_uchTextOutLineNum = 0;
	m_pusLineStartPos = NULL;
    m_nSize     = 18;
}
CTextDC::~CTextDC(void)
{
    if( m_buffer != NULL )
    {
        free(m_buffer);
        m_buffer = NULL;
    }
    if (m_pWidth != NULL)
    {
        delete m_pWidth;
        m_pWidth    = NULL;
    }
    if (m_pBytes != NULL)
    {
        delete m_pBytes;
        m_pBytes    = NULL;
    }
	if (m_pusUniCode)
	{
		delete m_pusUniCode;
		m_pusUniCode = NULL;
	}
	if (m_pusLineStartPos)
	{
		delete m_pusLineStartPos;
		m_pusLineStartPos = NULL;
	}
}
int CTextDC::SetText(const char* text)
{
    return TextOut(text);
}
int	CTextDC::TextOut(ushort code, DVR_U8* width)
{
    if( m_pixeltxt >= m_pixelnum )
        return 0;
    FT_UInt idx = FT_Get_Char_Index(s_face, code);
	int     ret = FT_Load_Glyph(s_face, idx, FT_LOAD_MONOCHROME | FT_LOAD_RENDER);
    if( ret != 0 )
    {
   		TEXTDCPrintf("FT_Load_Glyph Error( code = 0x%04x, idx = %u, ret = %d)\n", code, idx, ret);
		return -1;
    }
    FT_GlyphSlot  slot = s_face->glyph;
	if( slot == NULL )
    {
   		TEXTDCPrintf("FT_Load_Glyph Error(slot == NULL)\n");
        return -1;
    }   
    int startx  = m_pixeltxt + slot->bitmap_left;
    m_pixeltxt  += (slot->advance.x >> 6);
    *width      = slot->advance.x >> 6;
    if( m_pixeltxt >= m_pixelnum )
        m_pixeltxt = m_pixelnum;
    if( startx >= m_pixelnum )
        return 0;
    int starty = m_linebase - slot->bitmap_top;
    int row;
    int col;
    int x;
    for( row = starty; row < (int)(starty + slot->bitmap.rows); row++)
    {
        if( row < 0 )
            continue;
        if( row >= m_linenum )
            break;
        DVR_U8* dst = m_buffer + row * m_pitch;
        DVR_U8* src = slot->bitmap.buffer + (row - starty) * slot->bitmap.pitch;
        for( col = 0; col < (int)slot->bitmap.width; col++)
        {
            x = startx + col;
            if( x < 0 )
                continue;
            if( x >= m_pixeltxt )
                break;
   			if( src[col>>3] & (0x80>>(col&0x07) ) )
    		{
                dst[x>>3] |= (0x80 >> (x&0x07) );
		    }
        }
    }
    if( m_linebtm == 0 )
    {
        m_linetop   = starty;
        m_linebtm   = starty + slot->bitmap.rows;
    }
    else
    {
        if( starty < m_linetop )
        {
            if( starty > 0 )
                m_linetop = starty;
            else
                m_linetop = 0;
        }
        if( starty + slot->bitmap.rows > m_linebtm )
            m_linebtm = starty + slot->bitmap.rows;
    }
    return 0;
}
int CTextDC::TextOut(const char* text, ushort txtsz)
{
    int         ret;
    size_t      idx;
    size_t      len     = 0;
    size_t      num     = 0;
    ushort*     code    = NULL;
    DVR_U8*      bytes   = NULL;
    int     arabic  = FALSE;

    txtsz   = m_nSize;
    if( text == NULL )
    {
        m_text.erase();
       	m_linetop	= 0;
    	m_linebtm	= 0;
	    m_pixeltxt	= 0;
        m_num = 0;
        return 0;
    }
    if( m_text.compare(text) == 0 )
    {
        return 0;
    }
   	m_linetop	= 0;
   	m_linebtm	= 0;
    m_pixeltxt	= 0;
    m_num = 0;
    len = strlen(text);
    if( len == 0 )
    {
        m_text.erase();
        return 0;
    }
    m_text      = text;
    code    = (ushort*)alloca( len*sizeof(ushort) );
    bytes   = (DVR_U8*)alloca(len*sizeof(DVR_U8));

    num   = ToUCS2( text, len, code, bytes, arabic);
    //printf("<ofilm> CTextDC::TextOut() : text = %s, num = %d\n", text, num);
    if( arabic )
        num = ToArabic( code, num );
    if (m_num != num)
    {
        if (m_pWidth != NULL)
        {
            delete m_pWidth;
            m_pWidth    = NULL;
        }

        if (m_pBytes != NULL)
        {
            delete m_pBytes;
            m_pBytes = NULL;
        }

    	if (m_pusUniCode)
    	{
    		delete m_pusUniCode;
    		m_pusUniCode = NULL;
    	}

        m_num   = num;
    }
    if( m_num == 0 )
    {
        TEXTDCPrintf("ToUCS2 Error(num = %d)\n", m_num);
        return 0;
    }
    else
    {
        if (NULL == m_pWidth)
        {
            m_pWidth = new DVR_U8[m_num];
            memset(m_pWidth, 0, sizeof(DVR_U8) * m_num);
        }

        if (NULL == m_pBytes)
        {
            m_pBytes = new DVR_U8[m_num];
            memset(m_pBytes, 0, sizeof(DVR_U8) * m_num);
        }

    	if (NULL == m_pusUniCode)
    	{
    		m_pusUniCode = new ushort[m_num];
    		memset(m_pusUniCode, 0, sizeof(ushort) * m_num);
    	}
    }
    
	DvrMutexLocker mutexLocker(&s_mutex);
    if( txtsz < 4 )
        txtsz = s_txtsz;
    ret = SetTextSize(txtsz, &m_linenum, &m_linebase, &m_pixelnum);
    if( ret != 0 || m_linenum == 0 || m_pixelnum == 0)
    {
        printf("(%s)%d ofilm-------error ret %d\n",__FILE__,__LINE__,ret);
        return -1;
    }
    m_pixelnum = m_pixelnum * m_num;
    m_pitch = ((m_pixelnum - 1)>>3) + 1;
    if( m_buflen < m_pitch * m_linenum )
    {
        if( m_buffer != NULL )
        {
            free(m_buffer);
            m_buffer = NULL;
        }
        m_buflen = m_pitch * m_linenum;
    }
    if( m_buffer == NULL )
    {
        m_buffer = (DVR_U8*)malloc( m_buflen );
        if( m_buffer == NULL )
            return -1;
    }
    memset( m_buffer, 0, m_buflen);
    for( idx = 0; idx < m_num; idx++)
    {
    	m_pusUniCode[idx] = code[idx];
        m_pBytes[idx] = bytes[idx];
        TextOut(code[idx], &m_pWidth[idx]);
    }

	if (m_pusLineStartPos)
	{
		delete m_pusLineStartPos;
		m_pusLineStartPos = NULL;
	}
	m_uchTextOutLineNum = 0;
    return 0;
}

int CTextDC::SetFontSize(uint nSize)
{
    if (m_nSize == nSize)
    {
        return 0;
    }

    m_nSize = nSize;
    const char  *text   = m_text.c_str();
    size_t  len = strlen(text);
    if (len == 0)
    {
        return 0;
    }

    int         ret;
    size_t      idx;
    size_t      num     = 0;
    ushort*     code    = NULL;
    DVR_U8*      bytes   = NULL;
    int     arabic  = FALSE;

   	m_linetop	= 0;
   	m_linebtm	= 0;
    m_pixeltxt	= 0;

    code    = (ushort*)alloca( len*sizeof(ushort) );
    bytes   = (DVR_U8*)alloca(len*sizeof(DVR_U8));
    num     = ToUCS2( text, len, code, bytes, arabic);
    if( arabic )
        num = ToArabic( code, num );
    if (m_num != num)
    {
        if (m_pWidth != NULL)
        {
            delete m_pWidth;
            m_pWidth    = NULL;
        }

        if (m_pBytes != NULL)
        {
            delete m_pBytes;
            m_pBytes = NULL;
        }

    	if (m_pusUniCode)
    	{
    		delete m_pusUniCode;
    		m_pusUniCode = NULL;
    	}

        m_num   = num;
    }
    if( m_num == 0 )
    {
        TEXTDCPrintf("ToUCS2 Error(num = %d)\n", m_num);
        return 0;
    }
    else
    {
        if (NULL == m_pWidth)
        {
            m_pWidth = new DVR_U8[m_num];
            memset(m_pWidth, 0, sizeof(DVR_U8) * m_num);
        }

        if (NULL == m_pBytes)
        {
            m_pBytes = new DVR_U8[m_num];
            memset(m_pBytes, 0, sizeof(DVR_U8) * m_num);
        }

    	if (NULL == m_pusUniCode)
    	{
    		m_pusUniCode = new ushort[m_num];
    		memset(m_pusUniCode, 0, sizeof(ushort) * m_num);
    	}
    }
    DvrMutexLocker mutexLocker(&s_mutex);
    ret = SetTextSize(m_nSize, &m_linenum, &m_linebase, &m_pixelnum);
    if( ret != 0 || m_linenum == 0 || m_pixelnum == 0)
    {
        printf("(%s)%d ofilm-------error ret %d\n",__FILE__,__LINE__,ret);
        return -1;
    }
    m_pixelnum = m_pixelnum * m_num;
    m_pitch = ((m_pixelnum - 1)>>3) + 1;
    if( m_buflen < m_pitch * m_linenum )
    {
        if( m_buffer != NULL )
        {
            free(m_buffer);
            m_buffer = NULL;
        }
        m_buflen = m_pitch * m_linenum;
    }
    if( m_buffer == NULL )
    {
        m_buffer = (DVR_U8*)malloc( m_buflen );
        if( m_buffer == NULL )
            return -1;
    }
    memset( m_buffer, 0, m_buflen);
    for( idx = 0; idx < m_num; idx++)
    {
    	m_pusUniCode[idx] = code[idx];
        m_pBytes[idx] = bytes[idx];
        TextOut(code[idx], &m_pWidth[idx]);
    }

	if (m_pusLineStartPos)
	{
		delete m_pusLineStartPos;
		m_pusLineStartPos = NULL;
	}
	m_uchTextOutLineNum = 0;
    return 0;
}

int CTextDC::GetCharBytes(int nCount)
{
#if 1

    if (NULL == m_pBytes)
    {
        return 0;
    }

    if (nCount >= (int) m_num)
    {
        return strlen(m_text.c_str());
    }

    int len = 0;
    for (int i = 0; i < nCount; i++)
    {
        len += m_pBytes[i];
    }

    return len;

#else

    ushort* 	code	= NULL;
    int     arabic  = FALSE;
    size_t      num	    = 0;

    int len = strlen(m_text.c_str());
    code    = (ushort*) alloca(len * sizeof(ushort));

    if (len == 0)
    {
        return -1;
    }

    for(int i = 1; i <= len; i++)
    {
    	num = ToUCS2( m_text.c_str(), i, code, arabic);
    	if ((int) num > nCount)
    	{
    		return (i-1);
    	}
    }

    return len;

#endif
}

int CTextDC::TextToRaster(DVR_U8 *pBuf, VD_PSIZE pSize)
{
    if (m_buffer == NULL)
    {
        return -1;
    }

    int     nRow        = MIN(m_linenum, pSize->h);
    DVR_U8   *pBuffer    = m_buffer;
    //printf("<ofilm> CTextDC::TextToRaster() : m_buffer = %p, pitch = %d, w = %d, h = %d, nRow = %d\n",
    //    m_buffer, m_pitch, m_pixeltxt, m_linenum, nRow);
    for (int i = 0; i < nRow; i++)
    {
        memcpy(pBuf, pBuffer, MIN(pSize->w / 8, (m_pixeltxt + 7) / 8));
        pBuf        += (pSize->w / 8);
        pBuffer     += m_pitch;
    }

    return 0;
}

int CTextDC::GetTextExtent(const char* text, uint* width, uint* height, int count, ushort txtsz)
{
    int         ret;
    size_t      idx;
    size_t      len     = 0;
    size_t      num     = 0;
    ushort*     code    = NULL;
    DVR_U8*      bytes   = NULL;
    int     arabic  = FALSE;
    CLocales*   locales = CLocales::instance();
    if( text == NULL || locales == NULL)
        return -1;
    len = strlen(text);
    if( len == 0 )
        return -1;
    DvrMutexLocker mutexLocker(&s_mutex);
    ushort  linenum = 0;
    if( txtsz < 4 )
        txtsz = s_txtsz;
    ret = SetTextSize(txtsz, &linenum);
    if( ret != 0 )
        return -1;

    if( height != NULL )
        *height = linenum;
    if( width == NULL )
        return 0;

    *width  = 0;
    code    = (ushort*)alloca( len*sizeof(ushort) );
    bytes   = (DVR_U8*) alloca(len*sizeof(DVR_U8));
    num     = ToUCS2( text, len, code, bytes, arabic);
    if( arabic )
    {
        num = ToArabic( code, num );
    }
    if( num == 0 )
    {
//        printf("<ofilm> text = %s, 111111111111111111111\n", text);
        TEXTDCPrintf("ToUCS2 Error(num = %d)\n", num);
        return -1;
    }
    FT_UInt         idft;
    FT_GlyphSlot    slot;
    num = MIN((int) num, count);
    for( idx = 0; idx < num && num > 0; idx++)
    {
        idft    = FT_Get_Char_Index(s_face, code[idx]);
    	ret     = FT_Load_Glyph(s_face, idft, FT_LOAD_MONOCHROME | FT_LOAD_RENDER);
        if( ret != 0 )
        {
   		    TEXTDCPrintf("FT_Load_Glyph Error( code = 0x%04x, idx = %u, ret = %d)\n", code[idx], idft, ret);
    		continue;
        }
        slot = s_face->glyph;
      	if( slot == NULL )
            continue;
        *width += (slot->advance.x>>6);
    }
    return 0;
}
int CTextDC::GetTextExtent(uint* width, uint* height)
{
    if( m_linebtm == 0 )
        return -1;
    if( width != NULL )
        *width = m_pixeltxt;
    if( height!= NULL )
        *height= m_linenum;
    return 0;
}

int CTextDC::GetCharBytesEx(int nIndex)
{
    if (NULL == m_pBytes)
    {
        return 0;
    }

    if (nIndex >= (int) m_num)
    {
        nIndex = m_num;
    }

    return m_pBytes[nIndex];
}

int CTextDC::GetCharWidthEx(int nBegin, int nEnd)
{
    if (NULL == m_pWidth)
    {
        return 0;
    }

    if (nBegin < 0)
    {
        nBegin = 0;
    }

    if (nEnd > (int) m_num)
    {
        nEnd = m_num;
    }

    if (nBegin >= nEnd)
    {
        return 0;
    }

    int nWidth = 0;
    for (int i = nBegin; i < nEnd; i++)
    {
        nWidth += m_pWidth[i];
    }

    return nWidth;
}

std::string CTextDC::GetText(int nIndex)
{
    if (nIndex >= (int) m_num)
    {
        return std::string("");
    }

    if (nIndex <= 0)
    {
        return m_text;
    }

    int nBytes = GetCharBytes(nIndex);
    return m_text.substr(nBytes, m_text.size() - nBytes);
}

void CTextDC::RemoveChar(int nIndex)
{
    if (nIndex >= (int) m_num)
    {
        return;
    }

    std::string szText = m_text;
    int         nBytes = GetCharBytes(nIndex + 1);
    int         nCharBytes = GetCharBytesEx(nIndex);

    szText.erase(nBytes - nCharBytes, nCharBytes);
    SetText(szText.c_str());
}

void CTextDC::InsertChar(int nIndex, char* szValue, int nLength)
{
    if (nIndex < 0)
    {
        nIndex = 0;
    }
    else if (nIndex > (int) m_num)
    {
        nIndex = m_num;
    }

    int nBytes = GetCharBytes(nIndex);

    std::string szText = m_text;
    for (int i = 0; i < nLength; i++)
    {
        szText.insert(szText.begin() + nBytes + i, szValue[i]);
    }
    SetText(szText.c_str());
}
