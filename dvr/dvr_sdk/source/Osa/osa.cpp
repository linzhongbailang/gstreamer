
#include <osa_fs.h>
#include <glib.h>
#include <stdio.h>

void OSA_Sleep(int miliseconds)
{
	g_usleep(miliseconds * 1000);
}

#if 0
int OSA_FormatDisplayName(char *srcName, char *dstName, unsigned int duration_in_ms)
{
	if (srcName == NULL || dstName == NULL)
		return DVR_RES_EPOINTER;

	int year, month, day, hour, minute, second;
	gchar **tokens, **walk;
	tokens = g_strsplit(srcName, "-", 0);
	if (g_strv_length(tokens) != 6)
	{
		g_strfreev(tokens);
		return DVR_RES_EFAIL;
	}

	walk = tokens;

    year = strtol(*walk++, NULL, 10);
    month = strtol(*walk++, NULL, 10);
    day = strtol(*walk++, NULL, 10);
    hour = strtol(*walk++, NULL, 10);
    minute = strtol(*walk++, NULL, 10);
    second = strtol(*walk++, NULL, 10);
	g_strfreev(tokens);

	GDateTime *date = g_date_time_new_local(year, month, day, hour, minute, second);
	if (date == NULL)
		return DVR_RES_EFAIL;
	gchar *date_name = g_date_time_format(date, "%Y/%m/%d %H:%M:%S");

	GDateTime *new_date = g_date_time_add_seconds(date, duration_in_ms/1000.0f);
	gchar *new_date_name = g_date_time_format(new_date, "%Y/%m/%d %H:%M:%S");

	if (g_strncasecmp(date_name, new_date_name, 10) != 0)
	{
		g_free(date_name);
		g_free(new_date_name);
		return DVR_RES_EFAIL;
	}

	gchar *slash = g_strrstr(new_date_name, " ");
	gchar *sub_dst = g_strconcat(date_name, "-", slash+1, NULL);
	strcpy(dstName, sub_dst);

	g_free(date_name);
	g_free(new_date_name);
	return DVR_RES_SOK;
}

#else

int OSA_FormatDisplayName(char *srcName, char *dstName, unsigned int duration_in_ms)
{
	if (srcName == NULL || dstName == NULL)
		return DVR_RES_EPOINTER;

	int year, month, day, hour, minute, second;
    gchar **tokens, **walk;
    tokens = g_strsplit(srcName, "_", 0);
    if (g_strv_length(tokens) != 5)
    {
        g_strfreev(tokens);
        return DVR_RES_EFAIL;
    }

    walk = tokens;

    sscanf(walk[1], "%4d%2d%2d", &year, &month, &day);
    sscanf(walk[2], "%2d%2d%2d", &hour, &minute, &second);

    g_strfreev(tokens);

	GDateTime *date = g_date_time_new_local(year, month, day, hour, minute, second);
	if (date == NULL)
		return DVR_RES_EFAIL;
	gchar *date_name = g_date_time_format(date, "%Y/%m/%d %H:%M:%S");
	g_date_time_unref(date);

	strcpy(dstName, date_name);

	g_free(date_name);
	return DVR_RES_SOK;
}

#endif

static int SystemTime_Year = 0;
static int SystemTime_Month = 0;
static int SystemTime_Day = 0;
static int SystemTime_Hour = 0;
static int SystemTime_Minute = 0;
static int SystemTime_Second = 0;

static GMutex DvrSystemTimeLock;
static int SystemTimeLockHasInit = 0;

int OSA_UpdateSystemTime(int year, int month, int day, int hour, int minute, int second)
{
	if(!SystemTimeLockHasInit)
	{
		g_mutex_init(&DvrSystemTimeLock);
		SystemTimeLockHasInit = 1;
	}

	g_mutex_lock(&DvrSystemTimeLock);

	SystemTime_Year = year;
	SystemTime_Month = month;
	SystemTime_Day = day;
	SystemTime_Hour = hour;
	SystemTime_Minute = minute;
	SystemTime_Second = second;
	
	g_mutex_unlock(&DvrSystemTimeLock);

	return 0;
}

void OSA_GetSystemTime(int *year, int *month, int *day, int *hour, int *minute, int *second)
{
	if(!SystemTimeLockHasInit)
		return;
	
	g_mutex_lock(&DvrSystemTimeLock);

	if(year)
		*year = SystemTime_Year;
	if(month)
		*month = SystemTime_Month;
	if(day)
		*day = SystemTime_Day;
	if(hour)
		*hour = SystemTime_Hour;
	if(minute)
		*minute = SystemTime_Minute;	
	if(second)
		*second = SystemTime_Second;
	
	g_mutex_unlock(&DvrSystemTimeLock);
}

