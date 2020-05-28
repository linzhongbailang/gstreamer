#ifndef _OSD_DEF_H_
#define _OSD_DEF_H_

#define OSD_WIDTH            	  (1024)
#define OSD_HEIGHT            	  (52)
#define OSD_TIME_WIDTH            (16)
#define OSD_IACC

#ifdef OSD_IACC
#define OSD_FONE_SIZE  			  (20)
#define OSD_TIME_HEIGHT           (23)
#define OSD_TIME_START_Y          (15)
#define OSD_TIME_OFFSET_NUM       (10)
#define OSD_TIME_IACC_WIDTH       (60)//5*10+4+6
#define OSD_PIC_WIDTH             (360)//108 + 36*7
#else
#define OSD_FONE_SIZE  			  (24)
#define OSD_TIME_HEIGHT           (28)
#define OSD_TIME_START_Y          (13)
#define OSD_TIME_OFFSET_NUM       (13)
#define OSD_TIME_IACC_WIDTH       (77)//5*13+4+4+4
#define OSD_PIC_WIDTH             (324)//108 + 36*6
#endif
#define OSD_TIME_START_X(off)     (off)
#define OSD_TIME_END_X            (540)
#define OSD_TIME_END_Y            (OSD_TIME_START_Y+OSD_TIME_HEIGHT)

#define OSD_PIC_HEIGHT            (36)
#define OSD_PIC_START_X(off)      (OSD_TIME_END_X+off)
#define OSD_PIC_START_Y           (8)
#define OSD_PIC_END_X(off)        (OSD_PIC_START_X(off)+OSD_PIC_WIDTH)
#define OSD_PIC_END_Y             (OSD_PIC_START_Y+OSD_PIC_HEIGHT)

#define OSD_PANE_WIDTH 			  (482)
#define OSD_PANE_HEIGHT 		  (294)

typedef struct
{
	unsigned short lX;
	unsigned short lY;
}OSD_POINT;

typedef enum _OSD_TITLE_TYPE 
{
	OSD_TITLE_TIME,
	OSD_TITLE_PIC,
	OSD_TITLE_THUMB_IACC,
    OSD_TITLE_NUM
}OSD_TITLE_TYPE;

typedef enum _OSD_SRC_TYPE 
{
	OSD_SRC_VIDEO,
	OSD_SRC_PIC,
    OSD_SRC_NUM
}OSD_SRC_TYPE;

typedef enum _OSD_SRC_REC_STATE
{
    OSD_SRC_REC_STATE_IDLE,
    OSD_SRC_REC_STATE_RUNNING,
    OSD_SRC_REC_STATE_NUM
}OSD_SRC_REC_STATE;

typedef enum _OSD_SRC_REC_TYPE
{
    OSD_SRC_REC_TYPE_NONE,
	OSD_SRC_REC_TYPE_APA,
	OSD_SRC_REC_TYPE_IACC,
    OSD_SRC_REC_TYPE_ACC,
	OSD_SRC_REC_TYPE_AEB,
	OSD_SRC_REC_TYPE_NUM
}OSD_SRC_REC_TYPE;

typedef enum _OSD_PIC_TYPE 
{
#ifdef OSD_IACC
	OSD_PIC_TYPE_DAS,
#endif
	OSD_PIC_TYPE_SPPED,
    OSD_PIC_TYPE_GEAR,
	OSD_PIC_TYPE_ACC,
	OSD_PIC_TYPE_BRAKE,
	OSD_PIC_TYPE_TURN_LEFT,
	OSD_PIC_TYPE_TURN_RIGHT,
	OSD_PIC_TYPE_BUCKLE,
    OSD_PIC_NUM
}OSD_PIC_TYPE;

typedef enum _OSD_FONT_TYPE 
{
	OSD_FONT_TYPE_0,
    OSD_FONT_TYPE_1,
	OSD_FONT_TYPE_2,
	OSD_FONT_TYPE_3,
	OSD_FONT_TYPE_4,
	OSD_FONT_TYPE_5,
	OSD_FONT_TYPE_6,
	OSD_FONT_TYPE_7,
	OSD_FONT_TYPE_8,
	OSD_FONT_TYPE_9,
	OSD_FONT_TYPE_DOT,//.
	OSD_FONT_TYPE_COMMA,//,
	OSD_FONT_TYPE_DQM,//"
	OSD_FONT_TYPE_PLUS,//+
	OSD_FONT_TYPE_H,//-
	OSD_FONT_TYPE_M,//:
	OSD_FONT_TYPE_S,
	OSD_FONT_TYPE_N,
	OSD_FONT_TYPE_E,
	OSD_FONT_TYPE_W,
	OSD_FONT_TYPE_X,
	OSD_FONT_TYPE_Y,
	OSD_FONT_TYPE_NUM
}OSD_FONT_TYPE;

typedef enum _OSD_PIC_STATE 
{
	OSD_PIC_STATE_SPPED_G = 0,
    OSD_PIC_STATE_SPPED_W,
    OSD_PIC_STATE_SPPED,

	OSD_PIC_STATE_DAS_BLACK = 0,
	OSD_PIC_STATE_DAS_APA,
	OSD_PIC_STATE_DAS_IACC,
	OSD_PIC_STATE_DAS_ACC,
	OSD_PIC_STATE_DAS_AEB,
	OSD_PIC_STATE_DAS,

	OSD_PIC_STATE_IACC_LATRERL = 0,
	OSD_PIC_STATE_IACC_LONG,
	OSD_PIC_STATE_IACC,

	OSD_PIC_STATE_GEAR_D = 0,
	OSD_PIC_STATE_GEAR_G,
	OSD_PIC_STATE_GEAR_I,
    OSD_PIC_STATE_GEAR_M,
    OSD_PIC_STATE_GEAR_N,
    OSD_PIC_STATE_GEAR_P,
    OSD_PIC_STATE_GEAR_R,
    OSD_PIC_STATE_GEAR_W,
    OSD_PIC_STATE_GEAR,
    
    OSD_PIC_STATE_ACC_G = 0,
    OSD_PIC_STATE_ACC_W,
    OSD_PIC_STATE_ACC_R,
	OSD_PIC_STATE_ACC_1,
	OSD_PIC_STATE_ACC_2,
	OSD_PIC_STATE_ACC_3,
	OSD_PIC_STATE_ACC_4,
	OSD_PIC_STATE_ACC_5,
	OSD_PIC_STATE_ACC_GREEN,
    OSD_PIC_STATE_ACC,

	OSD_PIC_STATE_BRAKE_G = 0,
	OSD_PIC_STATE_BRAKE_W,
	OSD_PIC_STATE_BRAKE_R,
	OSD_PIC_STATE_BRAKE_GREEN,
    OSD_PIC_STATE_BRAKE,

	OSD_PIC_STATE_TURN_LEFT_G = 0,
	OSD_PIC_STATE_TURN_LEFT_W,
	OSD_PIC_STATE_TURN_LEFT_R,
	OSD_PIC_STATE_TURN_LEFT_GREEN,
    OSD_PIC_STATE_TURN_LEFT,

    OSD_PIC_STATE_TURN_RIGHT_G = 0,
	OSD_PIC_STATE_TURN_RIGHT_W,
	OSD_PIC_STATE_TURN_RIGHT_R,
	OSD_PIC_STATE_TURN_RIGHT_GREEN,
    OSD_PIC_STATE_TURN_RIGHT,

    OSD_PIC_STATE_BUCKLE_G = 0,
    OSD_PIC_STATE_BUCKLE_GREEN,
    OSD_PIC_STATE_BUCKLE_R,
    OSD_PIC_STATE_BUCKLE_W,
    OSD_PIC_STATE_BUCKLE
}OSD_PIC_STATE;

typedef struct _CAPTURE_TITLE_PARAM
{
	int		index;
	int		enable;
	unsigned short	x;
	unsigned short	y;
	unsigned short	width;
	unsigned short	height;
	unsigned int	fg_color;
	unsigned int	bg_color;
	unsigned char	*raster;
}CAPTURE_TITLE_PARAM;

typedef struct _OSD_SRC_PARAM
{
	OSD_SRC_TYPE		type;
	OSD_SRC_REC_TYPE	rectype;
	unsigned short		imagewidth;
	unsigned short		imageheight;
	unsigned char		*data;
}OSD_SRC_PARAM;

#endif//_OSD_DEF_H_
