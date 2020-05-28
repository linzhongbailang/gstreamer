#ifndef _RECORDER_UTILS_H_
#define _RECORDER_UTILS_H_

#include <gst/gst.h>
#include <glib.h>

#define H264_NAL_TYPE_SPS                (7)
#define H264_NAL_TYPE_PPS                (8)
#define H264_STARTCODE_SIZE              (4)
#define H264_SPS_DATA_LEN                (66)
#define H264_IDR_NALU_SIZE               (75)
#define H264_COPY_HEAD_SIZE              (H264_IDR_NALU_SIZE + H264_STARTCODE_SIZE)

guint get_max_file_index(const char *dirname);
gchar *create_video_filename(gchar *format, guint file_index, gchar *location);
gchar *create_thumbnail_filename(gchar *format, guint file_index, gchar *location);
void get_memoccupy(void);
bool CheckFrameIsIDR(unsigned char *buffer);


#endif
