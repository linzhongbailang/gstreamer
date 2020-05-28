#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "RecorderUtils.h"
#include "osa.h"

#define BUFF_LEN 256
guint get_max_file_index(const char *dirname)
{
    int i;
    GDir *dir = NULL;
    guint64 max_file_index = 0;
    guint64 file_index = 0;

    dir = g_dir_open(dirname, 0, NULL);
    if (dir)
    {
        const gchar *dir_ent;

        while ((dir_ent = g_dir_read_name(dir)))
        {
            if (!g_str_has_suffix(dir_ent, ".MP4"))
                continue;

            gchar *nptr = g_strrstr(dir_ent, "_");
            gchar *endptr;

            if (nptr)
            {
                file_index = g_ascii_strtoull(nptr + 1, &endptr, 10);

                if (file_index > max_file_index)
                {
                    max_file_index = file_index;
                }
            }
        }

        g_dir_close(dir);
    }

    return (guint)max_file_index;
}

gchar *create_video_filename(gchar *format, guint file_index, gchar *location)
{
    gchar *dst = NULL, *sub_dst = NULL;
    gchar suffix[10] = { 0, };
	
#ifdef __linux__
	gint year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;	
	OSA_GetSystemTime(&year, &month, &day, &hour, &minute, &second);
	GDateTime *date = g_date_time_new_local(year, month, day, hour, minute, second);
	if(date == NULL)
		date = g_date_time_new_now_local();
#else
	GDateTime *date = g_date_time_new_now_local();
#endif
	if (date == NULL)
		return NULL;
	
    g_snprintf(suffix, sizeof(suffix), "%05d", file_index);
    sub_dst = g_date_time_format(date, format);
    gchar *tmp = g_strconcat(sub_dst, "_", suffix, ".MP4", NULL);
    g_free(sub_dst);
    dst = g_strjoin(NULL, location, tmp, NULL);
    g_free(tmp);

	g_date_time_unref(date);
    return dst;
}

gchar *create_thumbnail_filename(gchar *format, guint file_index, gchar *location)
{
    gchar *dst = NULL, *sub_dst = NULL;
    gchar suffix[10] = { 0, };

#ifdef __linux__
	gint year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;	
	OSA_GetSystemTime(&year, &month, &day, &hour, &minute, &second);
	GDateTime *date = g_date_time_new_local(year, month, day, hour, minute, second);
	if(date == NULL)
		date = g_date_time_new_now_local();	
#else
	GDateTime *date = g_date_time_new_now_local();
#endif
	if (date == NULL)
		return NULL;
	
    g_snprintf(suffix, sizeof(suffix), "%05d", file_index);
    sub_dst = g_date_time_format(date, format);
    gchar *tmp = g_strconcat(sub_dst, "_", suffix, ".BMP", NULL);
    g_free(sub_dst);

	if(location == NULL)
	{
		GST_ERROR("location is NULL!!!");
	}
	
    dst = g_strjoin(NULL, location, tmp, NULL);
    g_free(tmp);

	g_date_time_unref(date);
    return dst;
}

void get_memoccupy(void)
{
    typedef struct PACKED
    {
        char name[20];
        unsigned int total;
        char name2[20];
    }MEM_OCCUPY;

    FILE *fd = NULL;
    int n;
    char buff[BUFF_LEN];
    MEM_OCCUPY mem, *m;
    m = &mem;

    fd = fopen("/proc/meminfo", "r");
    if(fd == NULL)
    {
		GST_ERROR("open /proc/meminfo failed (%s)!!!",strerror(errno));
        return;
    }

    fgets(buff, BUFF_LEN, fd);
    sscanf(buff, "%s %u %s", m->name, &m->total, m->name2);
    g_print("%s %u %s\t ", m->name, m->total, m->name2);

    fgets(buff, BUFF_LEN, fd);
    sscanf(buff, "%s %u %s", m->name, &m->total, m->name2);
    g_print("%s %u %s\t ", m->name, m->total, m->name2);

    fgets(buff, BUFF_LEN, fd);
    sscanf(buff, "%s %u %s", m->name, &m->total, m->name2);
    g_print("%s %u %s\t ", m->name, m->total, m->name2);

    fgets(buff, BUFF_LEN, fd);
    sscanf(buff, "%s %u %s", m->name, &m->total, m->name2);
    g_print("%s %u %s\t ", m->name, m->total, m->name2);

    fgets(buff, BUFF_LEN, fd);
    sscanf(buff, "%s %u %s", m->name, &m->total, m->name2);
    g_print("%s %u %s\n", m->name, m->total, m->name2);

    fclose(fd);
}

bool CheckFrameIsIDR(unsigned char *buffer) 
{
    unsigned char* nalu = (unsigned char*)(buffer + H264_STARTCODE_SIZE);
    unsigned char spstype = nalu[0] & 0x1f;
    unsigned char ppstype = nalu[H264_SPS_DATA_LEN] & 0x1f;
    if(spstype == H264_NAL_TYPE_SPS && ppstype == H264_NAL_TYPE_PPS)
    {
        return true;
    }
    return false;
}

