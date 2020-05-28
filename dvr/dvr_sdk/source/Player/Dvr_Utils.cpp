#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#ifdef __linux__
#include <unistd.h>
#endif
#include <gst/gst.h>

#include "DVR_PLAYER_INTFS.h"

#ifdef _MSC_VER
#pragma warning(disable:4996)	// 4706: assignment within conditional expression
#endif

char* MakeURI(char* pFile)
{
	if (pFile == NULL)
	{
		return NULL;
	}

	int nLen;

	nLen = 2 * strlen(pFile) + 16;

	char* p = (char*)calloc(1, nLen);
	if (p == NULL)
	{
		return NULL;
	}
	char* pdst = (char*)calloc(1, nLen);
	if (pdst == NULL)
	{
		free(p);
		return NULL;
	}

	strcpy(p, pFile);

	if (!gst_uri_is_valid(p))
	{
		gchar* uri = g_uri_escape_string(p, G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, TRUE);

		if (uri != NULL)
		{
			sprintf(pdst, "file:///%s", uri);
			g_free(uri);
		}
	}

	if (p)
	{
		free(p);
		p = NULL;
	}

	return pdst;
}

gdouble ms_time(void)
{
	GTimeVal time;
	g_get_current_time(&time);
	return (double)time.tv_sec * (double)1000 + time.tv_usec / (double)1000;
}

int virtaddr_to_phyaddr(void* virt_addr, uintptr_t* phy_addr)
{
#ifdef linux
    const uint64_t pfn_mask = ((((uint64_t)1)<<55)-1);
    const uint64_t pfn_present_flag =  (((uint64_t)1)<<63);
    int ret = -1;
    int page_size = getpagesize(); //????????
    uint32_t vir_page_idx = (uintptr_t)virt_addr / page_size; //???????(????????????64??)
    uint32_t pfn_item_offset = vir_page_idx * sizeof(uint64_t); //??????????????
    uint64_t pfn_item;
    uint32_t round_pfn_offset = pfn_item_offset & (~1023); //1K????,??????????1K
    uint32_t delta = pfn_item_offset % 1024; //??????
    uint8_t  read_buf[1024];

    FILE* fi = fopen("/proc/self/pagemap", "rb");
    if(fi == NULL)
    {
        return -errno;
    }
   
    if(fseek(fi, round_pfn_offset, SEEK_SET) == 0 && fread(read_buf, 1024, 1, fi) == 1)
    {
        pfn_item = *(uint64_t*)(read_buf + delta);
        if ( 0 != (pfn_item & pfn_present_flag))
        {
            *phy_addr = (pfn_item & pfn_mask) * page_size + (uintptr_t)virt_addr % page_size;
            ret = 0;
        }
        else
        {
            ret = -ENODATA;
        }
    }
    else
    {
        ret = -errno;
    }
    
    fclose(fi);
    return ret;
#else
    return -1;
#endif
}

static gint pipeline_level = 0;

gboolean FindElement(GstElement *pEle, gchar *s, gboolean silent, GstElement **ppFound)
{
    GList *list;
    if (!GST_IS_BIN(pEle))
        return FALSE;

    list = (GList *)GST_BIN(pEle)->children;
    if (list) {
        if (!silent) 
            g_print("%s-%d's Children:\n", GST_ELEMENT_NAME(pEle), pipeline_level);
    }

    pipeline_level++;

    while (list) {
        gchar *name = GST_ELEMENT_NAME(GST_ELEMENT(list->data));
        if (!silent) 
            g_print("--%s-%d\n", name, pipeline_level);
        if (strstr(name, s))
        {
            if (ppFound)
                *ppFound = GST_ELEMENT(list->data);
            pipeline_level--;
            return TRUE;
        }
        if (FindElement(GST_ELEMENT(list->data), s, silent, ppFound))
        {
            pipeline_level--;
            return TRUE;
        }
        list = g_list_next(list);
    }
    pipeline_level--;
    return FALSE;
}