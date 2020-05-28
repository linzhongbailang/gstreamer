#ifdef __linux__

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <log/log.h>
#include "DvrSDCardSpeedTest.h"
#include "DVR_SDK_INTFS.h"

#define FILENAME "file_speed_test.deleteme"
#define TESTCOUNT 8
#define LDW_SPEED_SDCARD_THRESHOLD 1.2f

DvrSDCardSpeedTest::DvrSDCardSpeedTest()
{
	memset(data, 0, sizeof(data));
}

DvrSDCardSpeedTest::~DvrSDCardSpeedTest()
{
		
}

void DvrSDCardSpeedTest::WriteTest(char *fileName)
{
	int i;
	int fd;
	ssize_t s;
    int count = 0;
	struct timespec StartTime,EndTime;
    float speed[TESTCOUNT] = {0.0f};

	fd = open(fileName, O_RDWR | O_CREAT);
	if(fd<0) {
		Log_Error("open %s failed\n", fileName);
		return ;
	}
	for(i = 0; i < TESTCOUNT; i++){
        system("echo 3 > /proc/sys/vm/drop_caches");
        clock_gettime(CLOCK_MONOTONIC, &StartTime);
		s = write(fd, &data, sizeof(data));
		if(s<0){
			Log_Error("write failed %d\n",i);
			close(fd);
			return ;
		}
        sync();
        clock_gettime(CLOCK_MONOTONIC, &EndTime);
        int deltat    = EndTime.tv_sec  - StartTime.tv_sec;
        long deltanano = EndTime.tv_nsec - StartTime.tv_nsec;
        
        if(deltanano < 0){
            deltanano += 1000000000;
            deltat -=1;
        }

        float dtime = deltat + deltanano/1000000000.;
        speed[i] = (DATASIZE >> 20) / dtime;
        Log_Message("WriteTest(%d) synctime %f\n",i,speed[i]);
        if(speed[i] < LDW_SPEED_SDCARD_THRESHOLD)
            count++;
	}
	close(fd);
}

void DvrSDCardSpeedTest::ReadTest(char *fileName)
{
	int i;
	int fd;
	ssize_t s;
	ssize_t total;
	
	fd = open(fileName,O_RDONLY);
	if(fd<0) {
		Log_Error("open %s failed\n", fileName);
		return;
	}

	total = TESTCOUNT * DATASIZE;

	while(total > 0){
		s = read(fd, &data, sizeof(data));
		if(s<0){
			Log_Error("read failed %d\n",i);
			close(fd);
			return ;
		}
		total -= s;
	}

	close(fd);
}

void DvrSDCardSpeedTest::syncAndDropCache()
{
	int fd;
	ssize_t s;

	sync();
	fd = open("/proc/sys/vm/drop_caches",O_WRONLY);
	if(fd<0){
		printf("open vm failed\n");
		return;
	}
	s = write(fd,"3",2);
	if(s < 0){
		printf("write vm failed\n");
		close(fd);
		return;
	}

	close(fd);
	sleep(2);
}

bool DvrSDCardSpeedTest::IsLowSpeedCheck(const char *path)
{
    float speed[TESTCOUNT] = {0.0f};
	struct timespec StartTime,EndTime;

    int count = 0;
    float averagespped = 0.0f;
	char fileName[128];
	memset(fileName, 0, sizeof(fileName));
	snprintf(fileName, 128, "%s/%s", path, FILENAME);	
	
	int fd = open(fileName, O_RDWR | O_CREAT);
	if(fd < 0)
    {
		Log_Error("open %s failed %s\n", fileName, strerror(errno));
		return false;
	}

	for(int i = 0; i < TESTCOUNT; i++)
    {
        system("echo 3 > /proc/sys/vm/drop_caches");
        clock_gettime(CLOCK_MONOTONIC, &StartTime);
		ssize_t s = write(fd, &data, sizeof(data));
		if(s < 0)
        {
			Log_Error("write failed %d %s\n", i, strerror(errno));
			close(fd);
            unlink(fileName);
			return false;
		}
        sync();
        clock_gettime(CLOCK_MONOTONIC, &EndTime);
        int deltat     = EndTime.tv_sec  - StartTime.tv_sec;
        long deltanano = EndTime.tv_nsec - StartTime.tv_nsec;

        if(deltanano < 0)
        {
            deltanano += 1000000000;
            deltat -= 1;
        }

        float dtime = deltat + deltanano/1000000000.;
        speed[i] = (DATASIZE >> 20) / dtime;
        Log_Message("WriteTest(%d) synctime %f\n",i,speed[i]);
        if(speed[i] <= LDW_SPEED_SDCARD_THRESHOLD)
            count++;
        averagespped += speed[i];
	}
	close(fd);
	unlink(fileName);
    averagespped = averagespped / TESTCOUNT;
    Log_Message("sd card wirte speed test result: %f %f %f %f %f %f %f %f %fMB/s!!!!!!",
        speed[0], speed[1], speed[2], speed[3], speed[4], speed[5], speed[6], speed[7], averagespped);
    if(count >= 5)
        return true;
    return false;
}

float DvrSDCardSpeedTest::GetWriteSpeed(const char *path)
{
	struct timespec StartTime,EndTime;

	char fileName[128];
	memset(fileName, 0, sizeof(fileName));
	snprintf(fileName, 128, "%s/%s", path, FILENAME);	
	
	clock_gettime(CLOCK_MONOTONIC, &StartTime);

	WriteTest(fileName);

	clock_gettime(CLOCK_MONOTONIC, &EndTime);
	
	unlink(fileName);

	int deltat    = EndTime.tv_sec  - StartTime.tv_sec;
	long deltanano = EndTime.tv_nsec - StartTime.tv_nsec;

	if(deltanano < 0){
		deltanano += 1000000000;
		deltat -=1;
	}

	float dtime = deltat + deltanano/1000000000.;
	float speed = TESTCOUNT / dtime;

	return speed;
}

bool DvrSDCardSpeedTest::IsLowSpeedSDCard(void)
{
	DVR_DEVICE CurDrive;
	memset(&CurDrive, 0, sizeof(DVR_DEVICE));
	
	Dvr_Sdk_GetActiveDrive(&CurDrive);
	if (strcmp(CurDrive.szMountPoint, ""))
	{
        DVR_U32 type = DVR_SD_SPEED_NONE;
        Dvr_Sdk_GetSDSpeekType(CurDrive.szDevicePath, CurDrive.szMountPoint, &type);
		Log_Message("SD Card Speed get type %d!!!!!!",type);
        if(type == DVR_SD_SPEED_NORMAL)
            return FALSE;
        else if(type == DVR_SD_SPEED_BAD)
            return TRUE;

        sync();
		Log_Message("SD Card Speed check Start!!!!!!");
        bool isLow = IsLowSpeedCheck(CurDrive.szMountPoint);
        Dvr_Sdk_MarkSDSpeed(CurDrive.szDevicePath, CurDrive.szMountPoint, isLow ? DVR_SD_SPEED_BAD : DVR_SD_SPEED_NORMAL);
        return isLow;
	}

	return FALSE;
}


#endif
