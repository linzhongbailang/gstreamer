#ifndef MP4VE_H_
#define MP4VE_H_

#define VIDEO_OBJECT_START_CODE_BEGIN 			0x00
#define VIDEO_OBJECT_START_CODE_END 			0x1F
#define VIDEO_OBJECT_LAYER_START_CODE_BEGIN 	0x20
#define VIDEO_OBJECT_LAYER_START_CODE_END 	0x2F
#define VISUAL_OBJECT_SEQUENCE_START_CODE 	0xB0
#define VISUAL_OBJECT_SEQUENCE_END_CODE 		0xB1
#define USER_DATA_START_CODE 						0xB2
#define GROUP_OF_VOP_START_CODE					0xB3
#define VIDEO_SESSION_ERROR_CODE					0xB4
#define VISUAL_OBJECT_START_CODE					0xB5
#define VOP_START_CODE								0xB6
#define FBA_OBJECT_START_CODE						0xBA
#define FBA_OBJECT_PLANE_START_CODE				0xBB
#define MESH_OBJECT_START_CODE					0xBC
#define MESH_OBJECT_PLANE_START_CODE			0xBD
#define STILL_TEXTURE_OBJECT_START_CODE		0xBE
#define TEXTURE_SPATIAL_LAYER_START_CODE		0xBF
#define TEXTURE_SNR_LAYER_START_CODE			0xC0
#define TEXTURE_TILE_START_CODE					0xC1
#define TEXTURE_SHAPE_LAYER_START_CODE			0xC2
#define STUFFING_START_CODE						0xC3


// visual_object_type
#define VIDEO_ID				0x1
#define STILL_TEXTURE_ID	0x2
#define MESH_ID				0x3
#define FBA_ID					0x4
#define MESH_3D_ID			0x5


enum VeType
{
	VISUAL_OBJECT_SEQUENCE=VISUAL_OBJECT_SEQUENCE_START_CODE,
		VISUAL_OBJECT=VISUAL_OBJECT_START_CODE,
		VIDEO_OBJECT=VIDEO_OBJECT_START_CODE_BEGIN,
		VIDEO_OBJECT_LAYER=VIDEO_OBJECT_LAYER_START_CODE_BEGIN,
		USER_DATA=USER_DATA_START_CODE,
		VIDEO_SIGNAL_TYPE,
};

class Mp4Navigator;
class Mp4Com;
class Mp4Ve : public Mp4Com
{
public:
	Mp4Ve(VeType aVeType,char * pObjectName,int offset,Mp4Ve * aContainer);
	virtual int 	Load(Mp4File * pMp4File) = 0;
	virtual void 	Dump(Formatter *pFormatter) {};
	virtual void 	DumpHeader(Formatter *pFormatter)
	{
		pFormatter->Print("<%s",m_ObjectName);
		pFormatter->Print(" offset=\"%d\" >\n",m_iBeginOffset);
		Dump(pFormatter);
	}
	virtual void 	DumpFooter(Formatter *pFormatter)
	{
		pFormatter->Print("</%s>\n",m_ObjectName);
	}
	
protected:
	Mp4Ve() : Mp4Com(MP4VE) {};
	
	VeType 	m_VeType;	
	int		m_iBeginOffset;
	Mp4Ve *  m_pContainer;
	char		m_ObjectName[128];
};

class UserData : public Mp4Ve
{
public:
	UserData(int offset, Mp4Ve * aContainer=0) 
		: Mp4Ve(USER_DATA,"user_data",offset,aContainer)
	{
	}
	
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);
	
	long user_data_start_code;
};

class VideoSignalType : public Mp4Ve
{
public:
	VideoSignalType(int offset, Mp4Ve * aContainer=0) 
		: Mp4Ve(VIDEO_SIGNAL_TYPE,"video_signal_type",offset,aContainer)
	{
	}
	
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);
	
	char video_signal_type;
	char video_format;
	char video_range;
	char colour_description;
	char colour_primaries;
	char transfer_characteristics;
	char matrix_coefficients;
};

class VideoObjectLayer : public Mp4Ve
{
public:
	VideoObjectLayer(int offset, Mp4Ve * aContainer=0) 
		: Mp4Ve(VIDEO_OBJECT_LAYER,"video_object_layer",offset,aContainer)
	{
		video_object_layer_width = 0;
		video_object_layer_height = 0;
	}
	
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);
	
	int video_object_layer_start_code;
	int random_accessible_vol;
	int video_object_type_indication;
	int is_object_layer_identifier;
	int aspect_ratio_info;
	int par_width;
	int par_height;
	int vol_control_parameters;
	int video_object_layer_shape;
	int marker_bit;
	int vop_time_increment_resolution;
	int fixed_vop_rate;
	int fixed_vop_time_increment;
	int video_object_layer_width;
	int video_object_layer_height;
	int interlaced;
	int obmc_disable;
	int sprite_enable;
	char not_8_bit;
	char quant_precision; 
	char bits_per_pixel; 
	int quant_type;
	char video_object_layer_verid;
	char video_object_layer_priority;
	
	char chroma_format;
	char low_delay;
	char vbv_parameters;
	long first_half_bit_rate;
	long latter_half_bit_rate;
	long first_half_vbv_buffer_size;
	long latter_half_vbv_buffer_size;
	long first_half_vbv_occupancy;
	long latter_half_vbv_occupancy;
	
	long sprite_width;	
	long sprite_height; 	
	long sprite_left_coordinate;	
	long sprite_top_coordinate;	
	
	char no_of_sprite_warping_pints;	
	char sprite_warping_accuracy;
	char sprite_brightness_change; 
	char low_latency_sprite_enable; 
	
	char sadct_disable; 
	
	char no_gray_quant_update;
	char composition_method;	
	char linear_composition;
	
	char video_object_layer_shape_extension;
	
	char load_intra_quant_mat;
	char intra_quant_mat[64];
	char load_intra_quant_mat_grayscale;
	char intra_quant_mat_grayscale[64];
	char load_nonintra_quant_mat_grayscale;
	char nonintra_quant_mat_grayscale[64];
	
	char aux_comp_count;
	
	char quarter_sample;
	char complexity_estimation_disable;
	char resync_marker_disable;
	char data_partitioned;
	char reversible_vlc;
	
	char newpred_enable;
	char requested_upstream_message_type;
	char newpred_segment_type;
	
	char reduced_resolution_vop_enable;
	
	char scalability;
	
	char hierachy_type;
	char ref_layer_id;
	char ref_layer_sampling_direc;
	char hor_sampling_factor_n;
	char hor_sampling_factor_m;
	char vert_sampling_factor_n;
	char vert_sampling_factor_m;
	char enhancement_type;
	char use_ref_shape;
	char use_ref_texture;
	char shape_hor_sampling_factor_n;
	char shape_hor_sampling_factor_m;
	char shape_vert_sampling_factor_n;
	char shape_vert_sampling_factor_m;
	
	char estimation_method;
	char shape_complexity_estimation_disable;
	char opaque;
	char transparent;
	char intra_cae;
	char inter_cae;
	char no_update;
	char upsampling;
	char texture_complexity_estimation_set_1_disable     ;
	char intra_blocks;
	char inter_blocks;
	char inter4v_blocks;
	char not_coded_blocks;
	char texture_complexity_estimation_set_2_disable ;
	char dct_coefs;
	char dct_lines;
	char vlc_symbols;
	char vlc_bits;
	char motion_compensation_complexity_disable;
	char apm;
	char npm;
	char interpolate_mc_q ;
	char forw_back_mc_q ;
	char halfpel2 ;
	char halfpel4;
	char version2_complexity_estimation_disable;
	char quarterpel;
	char sadct;
};

class VisualObject : public Mp4Ve
{
public:
	VisualObject(int offset, Mp4Ve * aContainer=0) 
		: Mp4Ve(VISUAL_OBJECT,"visual_object",offset,aContainer),
		pVideoSignalType(0),
		pUserData(0),
		pVideoObjectLayer(0)
	{
	}
	
	virtual int Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);
	
	long visual_object_start_code;
	char is_visual_object_identifier;
	char visual_object_verid;
	char visual_object_priority;
	char visual_object_type;
	
	VideoSignalType* pVideoSignalType;
	UserData* pUserData;
	VideoObjectLayer* pVideoObjectLayer;
};

class Vos : public Mp4Ve
{
public:
	Vos(int offset, Mp4Ve * aContainer=0) 
		: Mp4Ve(VISUAL_OBJECT_SEQUENCE,"visual_object_sequence",
		offset,aContainer),
		pUserData(0),
		pVisualObject(0)
	{
	}
	
	virtual int  Load(Mp4File * pMp4File);
	virtual void Dump(Formatter* pFormatter);
	
	long visual_object_sequence_start_code;
	char profile_and_level_indication;
	
	UserData* 		pUserData;
	VisualObject*	pVisualObject;
};

#endif

