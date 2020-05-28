#include "OsdFont.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <vector>
#include "OsdDc.h"

#define __FREETYPE_SUPPORT__
int CLocales::m_lang_num = 0;
std::string CLocales::m_strings_file[];
#ifndef __FREETYPE_SUPPORT__
std::string CLocales::m_small_fontbin;
std::string CLocales::m_big_fontbin;
std::string CLocales::m_fontbin;
#endif

bool CLocales::s_m_bInit = false;

int CLocales::S_SetFontFile( c_schar* f_small_font, c_schar* f_font, c_schar* f_big_font, signed char** stringfile, int filenum )
{
#ifndef __FREETYPE_SUPPORT__
	if(f_small_font != NULL) m_small_fontbin = f_small_font;
	if(f_big_font != NULL) m_big_fontbin = f_big_font;
	if (f_font == NULL )
	{
		errorf("normal font bin must be exist!\n");
		return -1;
	}
	m_fontbin = f_font;
#endif

	assert(filenum < MAX_LANG_NUM);
	m_lang_num = filenum;
	for( int i = 0; i < m_lang_num; i++ )
	{
		m_strings_file[i] = (char*)stringfile[i];
	}

	s_m_bInit = true;
	return 0;
}

PATTERN_SINGLETON_IMPLEMENT(CLocales);

CLocales::CLocales()
{
	//!ȷ��ʹ��֮��һ��Ҫ�ȳ������ļ�
	assert(s_m_bInit);
	printf("CLocales::CLocales() start init\n");
	m_iLanguage = -1;

#ifndef __FREETYPE_SUPPORT__
	m_bFontValid = FALSE;

#if 1//def ENC_SHOW_SMALL_FONT
  	m_bFontValidSmall = FALSE;           //zgzhit
    /*��ȡС�ֿ�ͷ��Ϣ, ��С��ASCII����: zgzhit*/
		//"/FontSmallEn.bin";
	if (m_FileFontSmall.Open(m_small_fontbin.c_str(), CFile::modeRead)
		&& m_FileFontSmall.Read(&m_UFHSmall, sizeof(UCS_FONT_HEADER)) == sizeof(UCS_FONT_HEADER)
		&& m_UFHSmall.size == m_FileFontSmall.GetLength()
       && m_UFHSmall.blocks
	   /*&& strcmp(m_UFHSmall.magic, UCS_FONT_MAGIC) == 0*/)
	{
		m_bFontValidSmall = TRUE;
		m_pUFBSmall = new UCS_FONT_BLOCK[m_UFHSmall.blocks];
		m_FileFontSmall.Read(m_pUFBSmall, sizeof(UCS_FONT_BLOCK) * m_UFHSmall.blocks);
		m_sizeFontSmall.w = m_pUFBSmall[0].width;	//��ʱֻȡ��һ��������Ŀ�Ⱥ͸߶�
		m_sizeFontSmall.h = m_pUFBSmall[0].height;
		m_nFontBytesSmall = m_sizeFontSmall.w * m_sizeFontSmall.h / 8;
		m_pASCIIFontSmall = new uchar[m_nFontBytesSmall * 128 + 128];
		memset(m_pASCIIFontSmall, 0, m_nFontBytesSmall * 128 + 128);

		/*����С��ASCII���忽������: zgzhit*/
		for(uint i = 0; i < m_UFHSmall.blocks; i++)
		{
			if(m_pUFBSmall[i].end <= 0x80)
			{
				m_FileFontSmall.Seek(m_pUFBSmall[i].roffs, CFile::begin);
				m_FileFontSmall.Read(&m_pASCIIFontSmall[m_pUFBSmall[i].start * m_nFontBytesSmall], (m_pUFBSmall[i].end - m_pUFBSmall[i].start) * m_nFontBytesSmall);
				m_FileFontSmall.Seek(m_pUFBSmall[i].xoffs, CFile::begin);
				m_FileFontSmall.Read(&m_pASCIIFontSmall[ 128 * m_nFontBytesSmall + m_pUFBSmall[i].start], m_pUFBSmall[i].end - m_pUFBSmall[i].start);
			}
		}
	}
	else
	{
		printf("CLocales::CLocales Open FontSmallEn.bin File Failed!\n");
	}
#endif

	m_bFontValidBig = FALSE;
	/*��ȡ�������ֿ�ͷ��Ϣ, ��С��ASCII����:*/
	//"/FontBigEn.bin";
	if (m_FileFontBig.Open(m_big_fontbin.c_str(), CFile::modeRead)
		&& m_FileFontBig.Read(&m_UFHBig, sizeof(UCS_FONT_HEADER)) == sizeof(UCS_FONT_HEADER)
		&& m_UFHBig.size == m_FileFontBig.GetLength()
		&& m_UFHBig.blocks
		/*&& strcmp(m_UFHBig.magic, UCS_FONT_MAGIC) == 0*/)
	{
		m_bFontValidBig = TRUE;
		m_pUFBBig = new UCS_FONT_BLOCK[m_UFHBig.blocks];
		m_FileFontBig.Read(m_pUFBBig, sizeof(UCS_FONT_BLOCK) * m_UFHBig.blocks);
		m_sizeFontBig.w = m_pUFBBig[0].width;	//��ʱֻȡ��һ��������Ŀ�Ⱥ͸߶�
		m_sizeFontBig.h = m_pUFBBig[0].height;
		m_nFontBytesBig = m_sizeFontBig.w * m_sizeFontBig.h / 8;
		m_pASCIIFontBig = new uchar[m_nFontBytesBig * 128 + 128];
		memset(m_pASCIIFontBig, 0, m_nFontBytesBig * 128 + 128);

		/*����С��ASCII���忽������: zgzhit*/
		for(uint i = 0; i < m_UFHBig.blocks; i++)
		{
			if(m_pUFBBig[i].end <= 0x80)
			{
				m_FileFontBig.Seek(m_pUFBBig[i].roffs, CFile::begin);
				m_FileFontBig.Read(&m_pASCIIFontBig[m_pUFBBig[i].start * m_nFontBytesBig],
					(m_pUFBBig[i].end - m_pUFBBig[i].start) * m_nFontBytesBig);
				m_FileFontBig.Seek(m_pUFBBig[i].xoffs, CFile::begin);
				m_FileFontBig.Read(&m_pASCIIFontBig[ 128 * m_nFontBytesBig + m_pUFBBig[i].start],
					m_pUFBBig[i].end - m_pUFBBig[i].start);
			}
		}
	}
	else
	{
		trace("CLocales::CLocales Open FontBigEn.bin File Failed!\n");
	}

	//��ȡ�ֿ�ͷ��Ϣ, ��ASCII����, �Ȳ���ȡ�����ַ�����
	if(m_FileFont.Open(m_fontbin.c_str(), CFile::modeRead)//FONT_BIN"/Font.bin
		&& m_FileFont.Read(&m_UFH, sizeof(UCS_FONT_HEADER)) == sizeof(UCS_FONT_HEADER)
		&& m_UFH.size == m_FileFont.GetLength()
		&& m_UFH.blocks
		/*&& strcmp(m_UFH.magic, UCS_FONT_MAGIC) == 0*/ )
		//&& strcmp(m_UFH.magic, UCS_FONT_MAGIC) != 0 )
	{
		//trace("CLocales::CLocales Open Font File %s OK\n", FONT_BIN"/Font.bin");
		m_bFontValid = TRUE;
		m_pUFB = new UCS_FONT_BLOCK[m_UFH.blocks];
		m_FileFont.Read(m_pUFB, sizeof(UCS_FONT_BLOCK) * m_UFH.blocks);
		m_sizeFont.w = m_pUFB[0].width;	//��ʱֻȡ��һ��������Ŀ�Ⱥ͸߶�
		m_sizeFont.h = m_pUFB[0].height;
		m_nFontBytes = m_sizeFont.w * m_sizeFont.h / 8;
		m_pASCIIFont = new uchar[m_nFontBytes * 128 + 128];
		memset(m_pASCIIFont, 0, m_nFontBytes * 128 + 128);

		//����ASCII���忽������
		for(uint i = 0; i < m_UFH.blocks; i++)
		{
			if(m_pUFB[i].end <= 0x80)
			{
				m_FileFont.Seek(m_pUFB[i].roffs, CFile::begin);
				m_FileFont.Read(&m_pASCIIFont[m_pUFB[i].start * m_nFontBytes],
					(m_pUFB[i].end - m_pUFB[i].start) * m_nFontBytes);
				m_FileFont.Seek(m_pUFB[i].xoffs, CFile::begin);
				m_FileFont.Read(&m_pASCIIFont[ 128 * m_nFontBytes + m_pUFB[i].start],
					m_pUFB[i].end - m_pUFB[i].start);
			}
		}
	}
	else
	{
		trace("CLocales::CLocales Open Font File Failed!\n");

		trace("##1.m_FileFont.Open:%d\n",m_FileFont.Open(m_fontbin.c_str(), CFile::modeRead));
		trace("##2.m_FileFont.Read:%d,size:%d\n",
			m_FileFont.Read(&m_UFH, sizeof(UCS_FONT_HEADER)),sizeof(UCS_FONT_HEADER));
		trace("##3.m_UFH.size:%d,m_FileFont.GetLength():%d \n",m_UFH.size,m_FileFont.GetLength());
		trace("##4.m_UFH.blocks:%d \n",m_UFH.blocks);
		trace("##5.m_UFH.magic:%s, UCS_FONT_MAGIC:%s \n",m_UFH.magic, UCS_FONT_MAGIC);
	}

	m_pCommonFont = NULL;

#endif

	m_iLanguage = -1;
	m_nCommonChars = 0;
	m_pCommonChars = NULL;

	//tracef("CConfigLocation::getLatest().iLanguage = %d.\n", CConfigLocation::getLatest().iLanguage);
	//��������, һ��Ҫ��֤��������CConfigLocation�Ѿ�����
	//SetLanguage(CConfigLocation::getLatest().iLanguage);
}

CLocales::~CLocales()
{
}

void CLocales::SetLanguage(int index)
{
	char path[VD_MAX_PATH];
	int i, n;
	uint length;
	std::list<ushort> common_list;
	std::list<ushort>::iterator pi;
	ushort code = 0; //unicode
	char *buf;
	char *p;
	char *key;
	char *value;

	char *alias_key = NULL;

#ifdef __FREETYPE_SUPPORT__
	if (index == m_iLanguage)
	{
        return;
	}
#else
	if (!m_bFontValid || index == m_iLanguage)
	{
		return;
	}
#endif

	if(m_iLanguage >= 0) //if language already be set, release first.
	{
		m_FileStrings.UnLoad();
		delete []m_pCommonChars;
		delete []m_pCommonFont;
		m_mapStrings.clear();
	}

	m_iLanguage = index;

	// �ȼ���ָ��Ŀ¼���ټ�������Ŀ¼��Ϊ����������ʹ��
	//sprintf(path, "%s/%s", FONT_DIR, strings_file[index]);
	if( !(buf = (char *)m_FileStrings.Load(m_strings_file[index].c_str() ) ) )
	{
		printf("CLocales::SetLanguage Load Strings File Failed! file path %s, and Load from Default Directory\n",m_strings_file[index].c_str());
		assert(0);
		//sprintf(path, "%s/%s", FONT_DEFAULT_DIR, strings_file[index]);
		//if(!(buf = (char *)m_FileStrings.Load(path)))
		//{
		//	trace("CLocales::SetLanguage Load Strings File Failed! file path %s\n",path);
		//	return;
		//}
	}

	//����ļ���ʽ�Ƿ�UTF-8
	if(buf[0] != (char)0xEF || buf[1] != (char)0xBB || buf[2] != (char)0xBF)
	{
		printf("CLocales::SetLanguage Strings File Format Error!\n");
		m_FileStrings.UnLoad();
		return;
	}

	//ɨ���ַ����ļ�, ��ȡ���еĳ����ַ�
	length = m_FileStrings.GetLength();
	p = buf;
	while((code = GetCharCode(p, &n)))
	{
		p += n;
		if(code >= 0x80)//��ASCII��
		{
			common_list.push_back(code);
		}
		if(p >= buf + length)
		{
			break;
		}
	}
	if(!code) //��������
	{
		printf("CLocales::SetLanguage Parse Strings File Failed!\n");
	}

	//����,ȥ���ظ����ַ�.
	common_list.sort();
	common_list.unique();
	if((n = common_list.size()))
	{
		m_nCommonChars = n;
		m_pCommonChars = new ushort[n];
#ifndef __FREETYPE_SUPPORT__
		m_pCommonFont = new uchar[n * m_nFontBytes + n];
#endif
		for(i = 0, pi = common_list.begin(); pi != common_list.end(); pi++, i++)
		{
			m_pCommonChars[i] = *pi;
#ifndef __FREETYPE_SUPPORT__
			m_pCommonFont[n * m_nFontBytes + i] = GetOneFontFromFile(m_pCommonChars[i], &m_pCommonFont[i * m_nFontBytes]);
#endif
		}
	}

	//��ȡ�ַ���, ������ֵ����
	p = buf;
	key = p;
	value = NULL;
	while(*p)
	{
		if(*p == '=')
		{
			if(!value)
			{
				*p = '\0';
				value = p + 1;
			}
		}
		else if(*p == '\n')
		{
			if(value)
			{
				*p = '\0';
				if(*(p-1) == '\r') //windows format
				{
					*(p-1) = '\0';
				}
				if(alias_key && !strncmp(key, "titles", 6))//�����ODM�汾���ⲻͬ������
				{
					if(strncmp(key, alias_key, 9))
					{
						goto jump_over;
					}
					i = 6;
					while(1)
					{
						key[i] = key[i+3];
						if(!key[i])
						{
							break;
						}
						i++;
					}
				}
				m_mapStrings.insert(MAPSTRING::value_type(key,value));
jump_over:
				value = NULL;
			}
			key = p + 1;
		}
		p++;
		if(p >= buf + length)
		{
			break;
		}
	}
	return;
}

#ifndef __FREETYPE_SUPPORT__
//װ��һ���ַ�������, �������ΪUCS-2����, ����ֵΪ������
uchar CLocales::GetOneFontFromFile(ushort code, uchar *p)
{
	int start = 0;
	int end = m_UFH.blocks;
	int mid;
	uchar wx;

	while(1)
	{
		mid = (start + end)/2;
		if(mid == start)break;
		if(m_pUFB[mid].start <= code)
		{
			start = mid;
		}
		else
		{
			end = mid;
		}
	}
	if(code >= m_pUFB[mid].end) //�ֿ���Ҳû��, ��ʾδ֪�ַ�.
	{
		//trace("Unknown code = %04x\n", code);
		if(p)
		{
			memset(p, 0xff, m_nFontBytes);
		}
		return m_sizeFont.w / 2;
	}

	if(p)
	{
		m_FileFont.Seek(m_pUFB[mid].roffs + (code - m_pUFB[mid].start) * m_nFontBytes, CFile::begin);
		m_FileFont.Read(p, m_nFontBytes);
	}
	m_FileFont.Seek(m_pUFB[mid].xoffs + (code - m_pUFB[mid].start), CFile::begin);
	m_FileFont.Read(&wx, 1);
	return wx;
}
#endif

ushort CLocales::GetCharCode(VD_PCSTR pch, int *pn)
{
	DVR_U8 ch; //sigle char
	ushort code = 0; //unicode
	int flag = 0; //0 - empty, 1 - 1 char to finish unicode, 2 - 2 chars to finish unicode, -1 - error

	*pn = 0;
	while((ch = (DVR_U8)*pch))
	{
		pch++;
		if(ch & 0x80)
		{
			if((ch & 0xc0) == 0xc0)
			{
				if((ch & 0xe0) == 0xe0)
				{
					if((ch & 0xf0) == 0xf0) //ucs-4?
					{
						break;
					}
					if(flag)
					{
						break;
					}
					*pn = 3;
					flag = 2;
					code |= ((ch & 0x0f) << 12);
				}
				else
				{
					if(flag)
					{
						break;
					}
					*pn = 2;
					flag = 1;
					code |= ((ch & 0x1f) << 6);
				}
			}
			else
			{
				if(flag == 0)
				{
					break;
				}
				else if(flag == 1) //unicode finished
				{
					code |= (ch & 0x3f);
					break;
				}
				else
				{
					code |= ((ch & 0x3f) << 6);
				}
				flag--;
			}
		}
		else //ASCII
		{
			if(flag)
			{
				break;
			}
			*pn = 1;
			code = ch;
			break;
		}
	}
	if(ch == 0)
	{
		code = 0;
	}

	return code;
}

int CLocales::GetFontSize(VD_SIZE *pSize, FONTSIZE fontsize)
{
	if(!pSize)
	{
		return FALSE;
	}

	if (fontsize == FONTSIZE_NORMAL)
	{
		*pSize = m_sizeFont;
	}
	else if (fontsize == FONTSIZE_SMALL)
	{
		*pSize = m_sizeFontSmall;
	}
	else if (fontsize == FONTSIZE_BIG)
	{
		*pSize = m_sizeFontBig;
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}

//���ַ����ҳ��ú���
int CLocales::FindCommonChar(ushort code)
{
	if(!m_pCommonChars)
	{
		return -1;
	}

	int high = m_nCommonChars - 1, low = 0, mid;

	while(low <= high)
	{
		mid = (low + high) / 2;
		if(m_pCommonChars[mid] < code)
		{
			low = mid + 1;
		}
		else if(m_pCommonChars[mid] > code)
		{
			high = mid - 1;
		}
		else
		{
			return mid;
		}
	}
	return -1;
}

#ifndef __FREETYPE_SUPPORT__
//�õ��ַ��Ŀ��, ���p��ΪNULL, ���õ������
uchar CLocales::GetCharRaster(ushort code, uchar* p /* = NULL */, FONTSIZE fontsize)
{
	int index;

	if (fontsize == FONTSIZE_NORMAL)
	{
		if(!m_bFontValid)
		{
			return 0;
		}

		if(code < 0x80)//ASCII�ַ�
		{
			if(p)
			{
				memcpy(p, m_pASCIIFont + code * m_nFontBytes, m_nFontBytes);
			}
			return m_pASCIIFont[128 * m_nFontBytes + code];
		}

		//�����ַ�
		if((index = FindCommonChar(code)) >= 0) //�ڳ������б���,ֱ��ȡֵ
		{
			if(p)
			{
				memcpy(p, &m_pCommonFont[index * m_nFontBytes], m_nFontBytes);
			}
			return m_pCommonFont[m_nCommonChars * m_nFontBytes + index];
		}
		else //����,���ļ�
		{
			return GetOneFontFromFile(code, p);
		}
	}
	else if (fontsize == FONTSIZE_SMALL)
	{
		if(!m_bFontValidSmall)
		{
			return 0;
		}

		if(code < 0x80)//ASCII�ַ�
		{
			if(p)
			{
				memcpy(p, m_pASCIIFontSmall + code * m_nFontBytesSmall, m_nFontBytesSmall);
			}
			return m_pASCIIFontSmall[128 * m_nFontBytesSmall + code];
		}
	}
	else if (fontsize == FONTSIZE_BIG)
	{
		if(!m_bFontValidBig)
		{
			return 0;
		}

		if(code < 0x80)//ASCII�ַ�
		{
			if(p)
			{
				memcpy(p, m_pASCIIFontBig + code * m_nFontBytesBig, m_nFontBytesBig);
			}
			return m_pASCIIFontBig[128 * m_nFontBytesBig + code];
		}
	}
	return 0;
}
#endif

int CLocales::GetTextExtent(VD_PCSTR str, int len/*=1024*/, FONT_STYLE fs /*= FS_NORMAL*/)
{
#ifdef __FREETYPE_SUPPORT__
	uint    width1 = 0;
	CTextDC::GetTextExtent(str, &width1, NULL, len, CTextDC::s_defaultsize);
#else
	if (!str){
		return 0;
	}
	ushort code;//�ַ�unicode
	int n;
	int w;//�ַ����
	int l;//�ַ��ֽ���
	int width1 = 0;//�ַ������
	FONTSIZE	font_size = FONTSIZE_NORMAL;
	if (fs == FS_SMALL)
	{
		font_size = FONTSIZE_SMALL;
	}
	else if (fs == FS_BIG)
	{
		font_size = FONTSIZE_BIG;
	}

	if (len>(int)strlen(str))
	{
		len = strlen(str);
	}
	for (n = 0; n < len; n += l, width1 += w)
	{
		code = GetCharCode(&str[n], &l);
		if(l == 0)
		{
			break;
		}
		w = GetCharRaster(code, NULL, font_size);
	}
#endif
	return width1;
}

int CLocales::GetTextExtentEx(VD_PCSTR str, int len, int nFontSize)
{
#ifdef __FREETYPE_SUPPORT__
	uint    width1 = 0;
	CTextDC::GetTextExtent(str, &width1, NULL, len, (ushort) nFontSize);
    return width1;
#else
    return GetTextExtent(str, len);
#endif
}

int CLocales::GetLanuage()
{
	return m_iLanguage;
}

VD_PCSTR LOADSTR(VD_PCSTR key)
{
	MAPSTRING::iterator pi;
	pi = CLocales::instance()->m_mapStrings.find(key);
	if(pi != CLocales::instance()->m_mapStrings.end())
	{
		return (*pi).second;
	}
	else
	{
		printf("Load string '%s' failed!!!\n", key);
		return key;
	}
}

/*!
\b Description		:	ͨ��&�ַ������ַ��������������ڹ��캯����ʱ�򲻵��ú�LOADSTRING\n
\b Revisions		:
- 2007-04-06		wangqin		Create
- 2007-04-12		wangqin		modified
*/
VD_PCSTR GetParsedString(VD_PCSTR key)
{
	if (*key == '&')
	{
		return LOADSTR(key+1);
	}
	else
		return key;
}
FONT_ORDER_E CLocales::GetFontOrder()
{
	if ( m_iLanguage == 20
		|| m_iLanguage == 17
		|| m_iLanguage == 21 )
	{
		return FO_Right2Left;
	}

	return FO_Left2Right;
}

FONT_ORDER_E CLocales::GetFontOrderByUnicode( ushort code )
{
	if (
		(code >= 0x0590 && code <= 0x05ff)
		|| (code >= 0x0600 && code <= 0x06ff)
		|| (code >= 0xfb50 && code <= 0xfdff)
		|| (code >= 0xfe70 && code <= 0xfeff)
		)
	{
		return FO_Right2Left;
	}

	return FO_Left2Right;
}
/*!
    \b Description        :    �ַ���ת���ɵ���\n
    \b Argument            :    BYTE * pBuffer, PSIZE pSize, PCRECT pRect, PCSTR str, FONTSIZE sizetype
    \param    pBuffer        :    ��ŵ����ָ��
    \param    pRect        :    ���������С
    \param    pSize        :    ��ŵ���Ŀ�Ⱥ͸߶�
    \param    str            :    ת�����ַ���
    \param    sizetype    :    �������ͣ��������廹��С����
    \return

    \b Revisions        :
*/
void CLocales::TextToRaster(DVR_U8 * pBuffer, VD_PSIZE pSize, VD_PCRECT pRect, VD_PCSTR str, FONTSIZE sizetype)
{
#ifdef __FREETYPE_SUPPORT__
    CTextDC *pTextDC    = new CTextDC;
    pTextDC->TextOut(str, 16);
    DVR_U8   *pBuf   = pTextDC->GetBuffer();
    int     nPitch  = pTextDC->GetPitch();
    int     nWidth  = pTextDC->GetWidth();
    int     nHeight = pTextDC->GetHeight();
    int     nRow    = MIN(nHeight, pSize->h);

    for (int i = 0; i < nRow; i++)
    {
        memcpy(pBuffer, pBuf, MIN(pSize->w / 8, (nWidth + 7) / 8));
        pBuffer += (pSize->w / 8);
        pBuf    += nPitch;
    }

    delete pTextDC;

#else
    ushort code;//�ַ�unicode
    int n;//�ַ�����
    int cw;//�ַ����
    int cl;//�ַ��ֽ���
    int ox, ox_start, ox_end;//����ƫ��
    int oy, oy_start, oy_end;//����ƫ��

    int xoffset =0;//x����
    uchar * p;//���󻺳�
    uchar raster[128];

    VD_SIZE fontsize;

    int x,y; //�������

    int len = (int)strlen(str);

    x = pRect->left;
    y = pRect->top;

    if (x%8)
    {
        printf("the X offset  is not match\n");
        return;
    }

    //�������ͳһʹ�ô������õ㣬��Ҫ����
    GetFontSize(&fontsize, sizetype);

    oy_start = 0;

    if(y + fontsize.h >= pRect->bottom)
    {
        oy_end = pRect->bottom - y;
    }
    else
    {
        oy_end = fontsize.h;
    }


    for(n = 0; n < len; n += cl/*, x += cw*/)
    {
        code = GetCharCode(&str[n], &cl);
        if(cl == 0)
        {
            break;
        }
        if(code == '\t')
        {
            xoffset = 96;
        }

        cw = GetCharRaster(code, raster, sizetype);
        if ((cw == 0) && (sizetype == FONTSIZE_SMALL))
        {
            //С�������ʧ�ܣ������¼��ش�����
            cw = GetCharRaster(code, raster, FONTSIZE_NORMAL);
        }

        p = raster;

        ox_start = 0;
        ox_end = cw;
        if(xoffset + cw >(pRect->right - pRect->left) * 8)
        {
            ox_end = (pRect->right - pRect->left) * 8 - xoffset;
        }

        for (oy = oy_start; oy < oy_end; oy++)
        {
            for (ox = ox_start; ox < ox_end; ox++)
            {
                if (*(p + ox / 8) & (128 >> (ox % 8)))
                {
                    pBuffer[(y+oy)*pSize->w/8+(x+xoffset+ox)/8] |= BITMSK(7-(xoffset+ox)%8);
                }
            }
            p += fontsize.w/8;
        }
        xoffset += cw;
    }
#endif
}

#define IN_WORD(x) ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') \
	|| x == '(' || x == ')' || (x >= '0' && x <= '9') \
	|| (x >= 0x0080 && x <= 0x00ff) \
	|| (x >= 0x0400 && x <= 0x04ff) \
	|| x == '"')  //added by wyf on 090909
#ifndef __FREETYPE_SUPPORT__
void CLocales::TextToRaster_arbic(uchar * pBuffer, VD_PSIZE pSize, VD_PCRECT pRect, VD_PCSTR str, FONTSIZE sizetype)
{
    //printf("<jxlog> CDevCapture::TextToRaster_arbic str=%s\n", str);
    ushort code, code_before, code_after;//�ַ�unicode
    int n;//�ַ�����
    int cw;//�ַ����
    int cl;//�ַ��ֽ���
    int ox, ox_start, ox_end;//����ƫ��
    int oy, oy_start, oy_end;//����ƫ��

    int xoffset =0;//x����
    uchar * p;//���󻺳�
    uchar raster[128];

    VD_SIZE fontsize;

    int x,y; //�������

    int len = (int)strlen(str);

    x = pRect->left;
    y = pRect->top;

    if (x%8)
    {
        printf("the X offset  is not match\n");
        return;
    }

    //�������ͳһʹ�ô������õ㣬��Ҫ����
    GetFontSize(&fontsize, sizetype);


    std::vector<ushort> veccode;
    int cwtotal = 0;
    code_before = 0;
    for(n = 0; n < len; n += cl/*, x += cw*/)
    {
        code = GetCharCode(&str[n], &cl);
        if(cl == 0)
        {
            break;
        }
        if(code == '\t')
        {
            xoffset = 96;
        }

        int n_tmp = n+cl;
		int tmp_c1 = 0;
		if( n_tmp < len )
		{
			code_after = GetCharCode(&str[n_tmp], &tmp_c1);
			if(tmp_c1 == 0) { code_after = 0; }
		}
		else
		{
			code_after = 0;
		}

        //!��������ĸ����
		int bIsSkip = FALSE;
		ushort code_tmp = code;
		code = ArbicTransform(code, code_before, code_after, bIsSkip);
		code_before = code_tmp;
        //printf("<jxlog> CDevCapture::TextToRaster_arbic code_tmp=0x%x, code=0x%x\n", code_tmp, code);
        if (bIsSkip)
        {
            code_before = code_after;
            n += cl;
            cl = tmp_c1;
        }

        cw = GetCharRaster(code, raster, sizetype);
        if ((cw == 0) && (sizetype == FONTSIZE_SMALL))
        {
            //С�������ʧ�ܣ������¼��ش�����
            cw = GetCharRaster(code, raster, FONTSIZE_NORMAL);
        }

        veccode.push_back(code);
        cwtotal += cw;
    }

    xoffset += cwtotal;
    std::vector<ushort> veceng;
    for (size_t i = 0; i < veccode.size(); i++)
    {
        if (IN_WORD(veccode[i]))
        {
            veceng.push_back(veccode[i]);
            continue;
        }
        else
        {
            for (size_t j = veceng.size(); j > 0; j--)
            {
                cw = GetCharRaster(veceng[j-1], raster, sizetype);
                if ((cw == 0) && (sizetype == FONTSIZE_SMALL))
                {
                    //С�������ʧ�ܣ������¼��ش�����
                    cw = GetCharRaster(veceng[j-1], raster, FONTSIZE_NORMAL);
                }
                xoffset -= cw;
                WordOut(pBuffer, pSize, pRect, sizetype, veceng[j-1], xoffset, x, y);
            }
            veceng.clear();

            cw = GetCharRaster(veccode[i], raster, sizetype);
            if ((cw == 0) && (sizetype == FONTSIZE_SMALL))
            {
                //С�������ʧ�ܣ������¼��ش�����
                cw = GetCharRaster(veccode[i], raster, FONTSIZE_NORMAL);
            }
            xoffset -= cw;
            WordOut(pBuffer, pSize, pRect, sizetype, veccode[i], xoffset, x, y);
        }
    }
    for (size_t j = veceng.size(); j > 0; j--)
    {
        cw = GetCharRaster(veceng[j-1], raster, sizetype);
        if ((cw == 0) && (sizetype == FONTSIZE_SMALL))
        {
            //С�������ʧ�ܣ������¼��ش�����
            cw = GetCharRaster(veceng[j-1], raster, FONTSIZE_NORMAL);
        }
        xoffset -= cw;
        WordOut(pBuffer, pSize, pRect, sizetype, veceng[j-1], xoffset, x, y);
    }
    veceng.clear();
}

void CLocales::WordOut(uchar * pBuffer, VD_PSIZE pSize, VD_PCRECT pRect, FONTSIZE sizetype, ushort code, int xoffset, int x, int y)
{
    int cw;
    int ox, ox_start, ox_end;//����ƫ��
    int oy, oy_start, oy_end;//����ƫ��

    uchar * p;//���󻺳�
    uchar raster[128];
    VD_SIZE fontsize;

    GetFontSize(&fontsize, sizetype);

    oy_start = 0;

    if(y + fontsize.h >= pRect->bottom)
    {
        oy_end = pRect->bottom - y;
    }
    else
    {
        oy_end = fontsize.h;
    }

    cw = GetCharRaster(code, raster, sizetype);
    if ((cw == 0) && (sizetype == FONTSIZE_SMALL))
    {
        //С�������ʧ�ܣ������¼��ش�����
        cw = GetCharRaster(code, raster, FONTSIZE_NORMAL);
    }
    p = raster;

    ox_start = 0;
    ox_end = cw;
    if(xoffset + cw >(pRect->right - pRect->left) * 8)
    {
        ox_end = (pRect->right - pRect->left) * 8 - xoffset;
    }

    for (oy = oy_start; oy < oy_end; oy++)
    {
        for (ox = ox_start; ox < ox_end; ox++)
        {
            if (*(p + ox / 8) & (128 >> (ox % 8)))
            {
                pBuffer[(y+oy)*pSize->w/8+(x+xoffset+ox)/8] |= BITMSK(7-(xoffset+ox)%8);
            }
        }
        p += fontsize.w/8;
    }
}

#endif
