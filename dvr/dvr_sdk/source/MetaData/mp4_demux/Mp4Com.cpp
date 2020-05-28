#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#if !defined(__linux__) && !defined(__ADS__) && !defined(TI_ARM)
#include <windows.h>
#include <mmsystem.h>  // needed for timeGetTime()
#else
#include <ctype.h>
#endif

#include "Mp4Com.h"
#include "Mp4File.h"
#include "Mp4Navigator.h"

void Mp4Com::DumpXml()
{
	Mp4XmlDumper XmlDumper;
	XmlDumper.m_XmlFormatter.Print("<?xml version=\"1.0\" ?>\n");
	Navigate(&XmlDumper);
}

bool Mp4Com::Navigate(Mp4Navigator* pNavigator)
{
	unsigned int i;
	Mp4Com * pCom;
	if(!pNavigator->Visit(this) && m_bStopNavigateIfFail)
		return false;
	for(i=0;i<m_ulNumChildren;i++)
	{
		pCom = m_Children[i];
		if(!pCom->Navigate(pNavigator) && m_bStopNavigateIfFail)
			return false;
	}
	if(!pNavigator->VisitEnd(this) && m_bStopNavigateIfFail)
		return false;
	return true;
}

void Mp4Com::ReleaseChildren()
{
	unsigned int i;
	for(i=0;i<m_ulNumChildren;i++)
	{
		delete m_Children[i];
		m_Children[i] = 0;
	}
	m_ulNumChildren = 0;
}

void Mp4Com::Adopt(Mp4Com* pSon)
{
	if(m_ulNumChildren<MAX_NUM_CHILDREN)
	{
		m_Children[m_ulNumChildren] = pSon;
		m_ulNumChildren++;
	}
	else
	{
		//		DBG(("Mp4Com::Adopt() Error - exceeded max children\n"));
	}
}

void Mp4Com::Remove(Mp4Com* pSon, bool bDelete)
{
	unsigned long i,j;
	for(i=0;i<m_ulNumChildren;i++)
		if(m_Children[i]==pSon)
			break;
		if(i==m_ulNumChildren)
			return;	
		if(bDelete)
			delete m_Children[i];
		m_Children[i] = 0;
		for(j=i;j<m_ulNumChildren-1;j++)
			m_Children[j] = m_Children[j+1];
		m_ulNumChildren--;
}

void Mp4Com::DumpHeader(Formatter *pFormatter)
{
	pFormatter->Print("<com>\n");
}

void Mp4Com::DumpFooter(Formatter *pFormatter)
{
	pFormatter->Print("</com>\n");
}

void Formatter::PrintInt(char *buf, int size)
{
	int i;
	if(!UseDisplay())
		return;
	for(i=size-1;i>-1;i--)
	{
		fprintf(stdout,"%c",buf[i]);
	}
}

void Formatter::Print(char *szFormat, ...)
{
	char szBuffer[256]; 
	va_list vl; 
	va_start(vl, szFormat);
	if(!UseDisplay())
    {
        va_end(vl);
        return;
    }
#if !defined(_MSC_VER)
#ifndef TI_ARM
	vsnprintf(szBuffer,255,szFormat, vl);
#endif	
#endif	
	szBuffer[255]=0;
	fprintf(stdout,"%s",szBuffer);
	va_end(vl);
}

void XmlFormatter::Put(char * szFormat, long iVal)
{
	if(szFormat)
		fprintf(stdout,szFormat, iVal);
	else
		fprintf(stdout,"0x%lx%lx%lx%lx ", ((iVal>>8)&0xff) ,((iVal>>8)&0xff) ,((iVal>>8)&0xff) ,iVal&0xff);
}

void XmlFormatter::Put(char * szFormat, short iVal)
{
	if(szFormat)
		fprintf(stdout,szFormat, iVal);
	else
		fprintf(stdout,"0x%x%x ",((iVal>>8)&0xff),iVal&0xff);
}

void XmlFormatter::Put(char * szFormat, char cVal)
{
	if(szFormat)
		fprintf(stdout,szFormat, (cVal&0xff));
	else
	{
#if defined(__linux__)
		if(isprint(cVal&0xff))
			fprintf(stdout,"%c ",cVal);
		else
			fprintf(stdout,"0x%x ",(cVal&0xff));
#else
		fprintf(stdout,"0x%x ",(cVal&0xff));
#endif
	}
}

void XmlFormatter::Put24(char * szFormat, long cVal)
{
	if(szFormat)
		fprintf(stdout,szFormat, (cVal&0xffffff));
	else
	{
		fprintf(stdout,"0x%lx ",(cVal&0xffffff));
	}
}

void XmlFormatter::Put(char * szFormat, char *  String)
{
	if(szFormat)
		fprintf(stdout, szFormat, String);
	else
	{
		unsigned i;
		for(i=0;i<strlen(String);i++)
			fprintf(stdout,"%c",String[i]);
	}
}

#ifndef TI_ARM
#if defined(__linux__)
void XmlFormatter::Put(char * szFormat, long long iVal)
#else
void XmlFormatter::Put(char * szFormat, __int64 iVal)
#endif
{
	if(szFormat)
		fprintf(stdout, szFormat, iVal);
}
#else
void XmlFormatter::Put(char * szFormat, int64s iVal)
{
    if(szFormat)
	{
        fprintf(stdout, szFormat, iVal.low);
	}
}
#endif    	

MpegFileFormatter::MpegFileFormatter(char *pathname)
{
	m_pFileCtrl = CreateFileCtrl();
	m_pFileCtrl->Open(pathname, 0, FILECTRL_OPEN_WRITE);
	m_NextTrackId = 0;
	m_FileFormat = 0;
}

MpegFileFormatter::MpegFileFormatter(FileCtrl *pFileCtrl)
{
	m_pFileCtrl = pFileCtrl;
	m_NextTrackId = 0;
	m_FileFormat = 0;
}

MpegFileFormatter::~MpegFileFormatter()
{
	if(m_pFileCtrl)
	{
		m_pFileCtrl->Close();
		m_pFileCtrl = 0;
	}
}

int MpegFileFormatter::Open(char *pathname)
{
	if(m_pFileCtrl)
		m_pFileCtrl->Close();
	else
		m_pFileCtrl = CreateFileCtrl();
	return m_pFileCtrl->Open(pathname, 0, FILECTRL_OPEN_WRITE);
}

void MpegFileFormatter::Put(char * szFormat, long iVal)
{
	if(m_bConvertToBig)
		PutBuf((char *)&iVal, 4);
	else
		m_pFileCtrl->Write((char *)&iVal, 4);
}

void MpegFileFormatter::Put(char * szFormat, short iVal)
{
	if(m_bConvertToBig)
		PutBuf((char *)&iVal, 2);
	else
		m_pFileCtrl->Write((char *)&iVal, 2);
}

void MpegFileFormatter::Put(char * szFormat, char cVal)
{
	m_pFileCtrl->Write((char *)&cVal, 1);
}

void MpegFileFormatter::Put24(char * szFormat, long cVal)
{
	if(m_bConvertToBig)
		PutBuf((char *)&cVal, 3);
	else
		m_pFileCtrl->Write((char *)&cVal, 3);
}

void MpegFileFormatter::Put(char * szFormat, char *  String)
{
	m_pFileCtrl->Write(String, strlen(String));
}

#ifndef TI_ARM
#if defined(__linux__)
void MpegFileFormatter::Put(char * szFormat, long long iVal)
#else
void MpegFileFormatter::Put(char * szFormat, __int64 iVal)
#endif
{
	if(m_bConvertToBig)
		PutBuf((char *)&iVal, 8);
	else
		m_pFileCtrl->Write((char *)&iVal, 8);
}
#else	
void MpegFileFormatter::Put(char * szFormat, int64s iVal)
{
	if(m_bConvertToBig)
		PutBuf((char *)&iVal, 8);
	else
		m_pFileCtrl->Write((char *)&iVal, 8);
}
#endif	

void MpegFileFormatter::PutSizeOfInstance(long soi)
{
	int ilen = 4,i;
	char buf[4];
	
	i = ilen-1;
	while(i > -1)
	{
		buf[i] = (char)(soi&0x7f); 
		soi >>= 7;
		if(i!=(ilen-1))
			buf[i] |= 0x80;
		i--;
	}
	PutStream(buf, 4);
}

void MpegFileFormatter::PutBuf(char *buf, int size)
{
	int i;
	for(i=size-1;i>-1;i--)
	{
		m_pFileCtrl->Write(buf+i, 1);
	}
}

long MpegFileFormatter::GetPos() 
{ 
	if(m_pFileCtrl)
		return m_pFileCtrl->Tell();
	return 0; 
}

void MpegFileFormatter::PutStream(char * buf, int size)
{
	m_pFileCtrl->Write(buf, size);
}
