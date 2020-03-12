#ifndef __GST_COMPATIBLE_H__
#define __GST_COMPATIBLE_H__

#include <gst/gst.h>

#ifndef GST_CLOCK_STIME_NONE
#define GST_CLOCK_STIME_NONE             ((GstClockTimeDiff)G_MININT64)
#endif

#ifndef GST_STIME_FORMAT
#define GST_STIME_FORMAT "c%" GST_TIME_FORMAT
#endif

#ifndef GST_PAD_PROBE_HANDLED
#define GST_PAD_PROBE_HANDLED (GST_PAD_PROBE_PASS + 1)
#endif

#define GST_CLOCK_STIME_IS_VALID(time)   (((GstClockTimeDiff)(time)) != GST_CLOCK_STIME_NONE)

#define GST_STIME_ARGS(t)                                               \
  ((t) == GST_CLOCK_STIME_NONE || (t) >= 0) ? '+' : '-',                \
    GST_CLOCK_STIME_IS_VALID (t) ?                                      \
    (guint) (((GstClockTime)(ABS(t))) / (GST_SECOND * 60 * 60)) : 99,   \
    GST_CLOCK_STIME_IS_VALID (t) ?                                      \
    (guint) ((((GstClockTime)(ABS(t))) / (GST_SECOND * 60)) % 60) : 99, \
    GST_CLOCK_STIME_IS_VALID (t) ?                                      \
    (guint) ((((GstClockTime)(ABS(t))) / GST_SECOND) % 60) : 99,        \
    GST_CLOCK_STIME_IS_VALID (t) ?                                      \
    (guint) (((GstClockTime)(ABS(t))) % GST_SECOND) : 999999999

#endif