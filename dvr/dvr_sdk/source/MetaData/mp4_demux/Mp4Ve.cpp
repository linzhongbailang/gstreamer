#include <stdio.h>
#include <string.h>
#if defined(__linux__)
#include <fcntl.h>
#endif
#include <stdlib.h>

#include "Mp4Com.h"
#include "Mp4File.h"
#include "Mp4Ve.h"

Mp4Ve::Mp4Ve(VeType aVeType,char * pObjectName,int offset,Mp4Ve * aContainer) :
Mp4Com(MP4VE),
m_VeType(aVeType), 
m_iBeginOffset(offset), 
m_pContainer(aContainer) 
{
	strcpy(m_ObjectName, pObjectName);
};

int Vos::Load(Mp4File * pMp4File)
{
	visual_object_sequence_start_code = pMp4File->GetStartCode();
	if(visual_object_sequence_start_code!=VISUAL_OBJECT_SEQUENCE_START_CODE)
		return -1;
	profile_and_level_indication = pMp4File->GetBits(8);
	if(pMp4File->NextBits(32)==USER_DATA_START_CODE)
	{
		if(pUserData)
			delete pUserData;
		pUserData = new UserData(pMp4File->GetPos(),this);
		if(NULL == pUserData)
		{
			return -2;
		}
		pUserData->Load(pMp4File);
		Adopt(pUserData);
	}
	if(pVisualObject)
		delete pVisualObject;
	pVisualObject = new VisualObject(pMp4File->GetPos(),this);
	if(NULL == pVisualObject)
	{
		return -2;
	}
	pVisualObject->Load(pMp4File);
	Adopt(pVisualObject);
	
	return 0;
}

void Vos::Dump(Formatter* pFormatter)
{
	pFormatter->Print("<visual_object_sequence_start_code val=\"0x%x\"/>\n",
		visual_object_sequence_start_code);
	pFormatter->Print("<profile_and_level_indication val=\"0x%x\"/>\n",profile_and_level_indication);
}

int UserData::Load(Mp4File * pMp4File)
{
	user_data_start_code = pMp4File->GetBits(32);
	while(pMp4File->NextBits(24)==0x1)
		;
	return 0;
}
void UserData::Dump(Formatter* pFormatter)
{
}

int VisualObject::Load(Mp4File * pMp4File)
{
	long start_code=0;
	
	visual_object_start_code = pMp4File->GetBits(32);
	if((visual_object_start_code&0xff)!=VISUAL_OBJECT_START_CODE)
		return -1;
	is_visual_object_identifier = pMp4File->GetBits(1);
	if(is_visual_object_identifier)
	{
		visual_object_verid = pMp4File->GetBits(4);
		visual_object_priority = pMp4File->GetBits(3);
	}
	else
		visual_object_verid = 0x1;
	visual_object_type = pMp4File->GetBits(4);
	
	if(visual_object_type==VIDEO_ID
		|| visual_object_type==STILL_TEXTURE_ID)
	{
		if(pVideoSignalType)
			delete pVideoSignalType;
		pVideoSignalType = new VideoSignalType(pMp4File->GetPos(),this);
		if(NULL == pVideoSignalType)
		{
			return -2;
		}
		pVideoSignalType->Load(pMp4File);
		Adopt(pVideoSignalType);
	}
	
	start_code = pMp4File->NextStartCode();
	
	if(start_code==USER_DATA_START_CODE)
	{
		if(pUserData)
			delete pUserData;
		pUserData = new UserData(pMp4File->GetPos(), this);
		if(NULL == pUserData)
		{
			return -2;
		}
		pUserData->Load(pMp4File);	
		Adopt(pUserData);
		start_code = pMp4File->GetBits(32);
	}
	
	if(visual_object_type==VIDEO_ID)
	{
		if(pVideoObjectLayer)
			delete pVideoObjectLayer;
		pVideoObjectLayer = new VideoObjectLayer(pMp4File->GetPos(),this);
		if(NULL == pVideoObjectLayer)
		{
			return -2;
		}
		pVideoObjectLayer->Load(pMp4File);
		Adopt(pVideoObjectLayer);
	}
	
	return 0;
}
void VisualObject::Dump(Formatter* pFormatter)
{
	pFormatter->Print("<visual_object_start_code val=\"0x%x\"/>\n",
		visual_object_start_code);
	pFormatter->Print("<is_visual_object_identifier val=\"0x%x\"/>\n",
		is_visual_object_identifier);
	
	if(is_visual_object_identifier)
	{
		pFormatter->Print("<visual_object_verid val=\"0x%x\"/>\n",
			visual_object_verid);
		pFormatter->Print("<visual_object_priority val=\"0x%x\"/>\n",
			visual_object_priority);
	}
	pFormatter->Print("<visual_object_type val=\"0x%x(%s)\"/>\n",
		visual_object_type,
		visual_object_type==0x1?"Video ID":"Unknown");
	
	//pFormatter->Print("<video_signal_type val=\"0x%x\"/>\n", video_signal_type);
}

int VideoSignalType::Load(Mp4File * pMp4File)
{
	video_signal_type  = pMp4File->GetBits(1);
	if(video_signal_type)
	{
		video_format 		= pMp4File->GetBits(3);
		video_range  		= pMp4File->GetBits(1);
		colour_description= pMp4File->GetBits(1);
		if(colour_description)
		{
			colour_primaries = pMp4File->GetBits(8);
			transfer_characteristics = pMp4File->GetBits(8);
			matrix_coefficients = pMp4File->GetBits(8);
		}
	}
	
	return 0;
}

void VideoSignalType::Dump(Formatter* pFormatter)
{
	pFormatter->Print("<video_signal_type val=\"0x%x\"/>\n", video_signal_type);
	if(video_signal_type)
	{
		pFormatter->Print("<video_format val=\"0x%x\"/>\n", video_format);
		pFormatter->Print("<video_range val=\"0x%x\"/>\n", video_range);
		pFormatter->Print("<colour_description val=\"0x%x\"/>\n", colour_description);
		if(colour_description)
		{
			pFormatter->Print("<colour_primaries val=\"0x%x\"/>\n", 
				colour_primaries);
			pFormatter->Print("<transfer_characteristics val=\"0x%x\"/>\n", 
				transfer_characteristics);
			pFormatter->Print("<matrix_coefficients val=\"0x%x\"/>\n", 
				matrix_coefficients);
		}
	}
}

int VideoObjectLayer::Load(Mp4File * pMp4File)
{
	int i;
	long start_code;
	start_code = pMp4File->NextBits(32);
	/*need to initialize it. add by roger ma
	seems the code below that use aux_comp_count isn't correct,
	so, if we do need to use these code, we should fix it.*/
	aux_comp_count = 0; 
	if(start_code>0x12f)
	{
		start_code = pMp4File->NextBits(22);
		return -1;
	}
	
	video_object_layer_start_code = start_code = pMp4File->GetBits(32);
	random_accessible_vol = pMp4File->GetBits(1);
	video_object_type_indication = pMp4File->GetBits(8);
	is_object_layer_identifier = pMp4File->GetBits(1);
	if (is_object_layer_identifier)
	{
		video_object_layer_verid 	 = pMp4File->GetBits(4);
		video_object_layer_priority = pMp4File->GetBits(3);
	}
	else
	{
		VisualObject * pVisual = (VisualObject *)m_pContainer;
		if(pVisual)
			video_object_layer_verid 	 = pVisual->visual_object_verid;
		else
			video_object_layer_verid 	 = 0;
	}
	aspect_ratio_info = pMp4File->GetBits(4);
	if(aspect_ratio_info==0xf) // extended PAR
	{
		par_width = pMp4File->GetBits(8);
		par_height = pMp4File->GetBits(8);
	}
	
	vol_control_parameters = pMp4File->GetBits(1);
	
	if(vol_control_parameters)
	{
		chroma_format = pMp4File->GetBits(2);
		low_delay = pMp4File->GetBits(1);
		vbv_parameters = pMp4File->GetBits(1);
		if(vbv_parameters)
		{
			first_half_bit_rate = pMp4File->GetBits(15);
			marker_bit = pMp4File->GetBits(1);
			latter_half_bit_rate = pMp4File->GetBits(15);
			marker_bit = pMp4File->GetBits(1);
			first_half_vbv_buffer_size = pMp4File->GetBits(15);
			marker_bit = pMp4File->GetBits(1);
			latter_half_vbv_buffer_size = pMp4File->GetBits(15);
			marker_bit = pMp4File->GetBits(1);
			first_half_vbv_occupancy = pMp4File->GetBits(15);
			marker_bit = pMp4File->GetBits(1);
			latter_half_vbv_occupancy = pMp4File->GetBits(15);
			marker_bit = pMp4File->GetBits(1);
		}
	}
	
	video_object_layer_shape = pMp4File->GetBits(2);
				
	if(video_object_layer_shape==0x3
		&& video_object_layer_verid==0x1)
	{
		video_object_layer_shape_extension = pMp4File->GetBits(4);
	}
	
	marker_bit = pMp4File->GetBits(1);
	vop_time_increment_resolution = pMp4File->GetBits(16);
	marker_bit = pMp4File->GetBits(1);
	fixed_vop_rate = pMp4File->GetBits(1);
	
	if(fixed_vop_rate)
	{
		int num_bits=0,num=1;
		for(num_bits=0,num=1;num<vop_time_increment_resolution;num*=2,num_bits++);
		fixed_vop_time_increment = pMp4File->GetBits(num_bits);
	}
	if(video_object_layer_shape!=0x1)
	{
		if(video_object_layer_shape==0x0)
		{
			marker_bit = pMp4File->GetBits(1);
			video_object_layer_width = pMp4File->GetBits(13);
			marker_bit = pMp4File->GetBits(1);
			video_object_layer_height = pMp4File->GetBits(13);
			marker_bit = pMp4File->GetBits(1);
		}
		
		interlaced = pMp4File->GetBits(1);
		obmc_disable = pMp4File->GetBits(1);
		
		// 00 sprite not used
		// 01 static
		// 10 GMC
		// 11 reserved
		if(video_object_layer_verid==0x1)
			sprite_enable = pMp4File->GetBits(1);
		else
			sprite_enable = pMp4File->GetBits(2);
		
		if(sprite_enable==0x1 || sprite_enable==0x2)
		{
			if(sprite_enable!=0x2) //!=GMC
			{
				sprite_width 	= pMp4File->GetBits(13);
				marker_bit 	 	= pMp4File->GetBits(1);
				sprite_height 	= pMp4File->GetBits(13);
				marker_bit 	 	= pMp4File->GetBits(1);
				sprite_left_coordinate 	= pMp4File->GetBits(13);
				marker_bit 	 	= pMp4File->GetBits(1);
				sprite_top_coordinate 	= pMp4File->GetBits(13);
				marker_bit 	 	= pMp4File->GetBits(1);
			}
			no_of_sprite_warping_pints	= pMp4File->GetBits(6);
			sprite_warping_accuracy = pMp4File->GetBits(2);
			sprite_brightness_change = pMp4File->GetBits(1);
			if(sprite_enable!=0x2) //!=GMC
				low_latency_sprite_enable = pMp4File->GetBits(1);
		}
		if(video_object_layer_verid!=0x1 &&
			video_object_layer_shape!=0x0)
		{
			sadct_disable = pMp4File->GetBits(1);
		}
		not_8_bit = pMp4File->GetBits(1);
		if(not_8_bit)
		{
			quant_precision = pMp4File->GetBits(4); 
			bits_per_pixel = pMp4File->GetBits(4); 
		}
		if(video_object_layer_shape==0x3)
		{
			no_gray_quant_update = pMp4File->GetBits(1); 
			composition_method 	= pMp4File->GetBits(1); 
			linear_composition 	= pMp4File->GetBits(1); 
		}
		quant_type = pMp4File->GetBits(1);
		if(quant_type)
		{
			load_intra_quant_mat = pMp4File->GetBits(1);
			if(load_intra_quant_mat)
			{
				for(i=0;i<64;i++)
				{
					intra_quant_mat[i] = pMp4File->GetBits(8);
					if(intra_quant_mat[i]==0)
						break;
				}
			}
			if(video_object_layer_shape==0x3) // ==grayscale
			{
				for(i=0;i<aux_comp_count;i++)
				{
					load_intra_quant_mat_grayscale = pMp4File->GetBits(1);
					if(load_intra_quant_mat_grayscale)
					{
						for(i=0;i<64;i++)
						{
							intra_quant_mat_grayscale[i] = pMp4File->GetBits(8);
							if(intra_quant_mat_grayscale[i]==0) break;
						}
					}
					load_nonintra_quant_mat_grayscale = pMp4File->GetBits(1);
					if(load_nonintra_quant_mat_grayscale)
					{
						for(i=0;i<64;i++)
						{
							nonintra_quant_mat_grayscale[i] = pMp4File->GetBits(8);
							if(nonintra_quant_mat_grayscale[i]==0) break;
						}
					}
				} //for
			} //if grayscale
		} //if qant_type
		
		if(video_object_layer_verid!=0x1)
			quarter_sample = pMp4File->GetBits(1);
		complexity_estimation_disable = pMp4File->GetBits(1);
		if(complexity_estimation_disable==0)
		{
			estimation_method = pMp4File->GetBits(2);
			if(estimation_method==0 || estimation_method==1)
			{
				shape_complexity_estimation_disable = pMp4File->GetBits(1);
				if(shape_complexity_estimation_disable)
				{
					opaque = pMp4File->GetBits(1);
					transparent = pMp4File->GetBits(1);
					intra_cae = pMp4File->GetBits(1);
					inter_cae = pMp4File->GetBits(1);
					no_update = pMp4File->GetBits(1);
					upsampling = pMp4File->GetBits(1);
				}
				
				texture_complexity_estimation_set_1_disable = pMp4File->GetBits(1);
				if(texture_complexity_estimation_set_1_disable==0)
				{
					intra_blocks = pMp4File->GetBits(1);
					inter_blocks = pMp4File->GetBits(1);
					inter4v_blocks = pMp4File->GetBits(1);
					not_coded_blocks = pMp4File->GetBits(1);
				}
				
				marker_bit = pMp4File->GetBits(1);
				
				texture_complexity_estimation_set_2_disable  = pMp4File->GetBits(1);
				if(texture_complexity_estimation_set_2_disable==0)
				{
					dct_coefs = pMp4File->GetBits(1);
					dct_lines = pMp4File->GetBits(1);
					vlc_symbols = pMp4File->GetBits(1);
					vlc_bits = pMp4File->GetBits(1);
				}
				motion_compensation_complexity_disable = pMp4File->GetBits(1);
				if(motion_compensation_complexity_disable==0)
				{
					apm = pMp4File->GetBits(1);
					npm = pMp4File->GetBits(1);
					interpolate_mc_q  = pMp4File->GetBits(1);
					forw_back_mc_q  = pMp4File->GetBits(1);
					halfpel2  = pMp4File->GetBits(1);
					halfpel4 = pMp4File->GetBits(1);
				}
				
				marker_bit = pMp4File->GetBits(1);
				
				if(estimation_method==0x1)
				{
					version2_complexity_estimation_disable = pMp4File->GetBits(1);
					if(version2_complexity_estimation_disable!=0)
					{
						sadct = pMp4File->GetBits(1);
						quarterpel = pMp4File->GetBits(1);
					}
				}
			}
		}
		resync_marker_disable = pMp4File->GetBits(1);
		data_partitioned = pMp4File->GetBits(1);
		
		if(data_partitioned)
			reversible_vlc = pMp4File->GetBits(1);
		if(video_object_layer_verid!=0x1)
		{
			newpred_enable = pMp4File->GetBits(1);
			if(newpred_enable)
			{
				requested_upstream_message_type = pMp4File->GetBits(2);
				newpred_segment_type = pMp4File->GetBits(1);
			}
			reduced_resolution_vop_enable = pMp4File->GetBits(1);
		}
		scalability = pMp4File->GetBits(1);
		
		if(scalability)
		{
			hierachy_type = pMp4File->GetBits(1);
			ref_layer_id = pMp4File->GetBits(4);
			ref_layer_sampling_direc = pMp4File->GetBits(1);
			hor_sampling_factor_n = pMp4File->GetBits(5);
			hor_sampling_factor_m = pMp4File->GetBits(5);
			vert_sampling_factor_n = pMp4File->GetBits(5);
			vert_sampling_factor_m = pMp4File->GetBits(5);
			enhancement_type = pMp4File->GetBits(1);
			if(video_object_layer_shape==0x1 && //binary
				hierachy_type==0)
			{
				use_ref_shape = pMp4File->GetBits(1);
				use_ref_texture = pMp4File->GetBits(1);
				shape_hor_sampling_factor_n = pMp4File->GetBits(5);
				shape_hor_sampling_factor_m = pMp4File->GetBits(5);
				shape_vert_sampling_factor_n = pMp4File->GetBits(5);
				shape_vert_sampling_factor_m = pMp4File->GetBits(5);
			}
		} // scalibility
		}
		else
		{
			if(video_object_layer_verid !=0x1) 
			{
				scalability = pMp4File->GetBits(1);
				if(scalability)
				{
					ref_layer_id = pMp4File->GetBits(4);
					shape_hor_sampling_factor_n = pMp4File->GetBits(5);
					shape_hor_sampling_factor_m = pMp4File->GetBits(5);
					shape_vert_sampling_factor_n = pMp4File->GetBits(5);
					shape_vert_sampling_factor_m = pMp4File->GetBits(5);
				}
			}
			resync_marker_disable = pMp4File->GetBits(1);
		}
		
		return 0;
	}
	
	
	void VideoObjectLayer::Dump(Formatter* pFormatter)
	{
		int i=0;
		pFormatter->Print("<video_object_layer_start_code val=\"0x%x\"/>\n",
			video_object_layer_start_code);
		pFormatter->Print("<random_accessible_vol val=\"0x%x\"/>\n",
			random_accessible_vol);
		pFormatter->Print("<video_object_type_indication val=\"0x%x\"/>\n",
			video_object_type_indication);
		pFormatter->Print("<is_object_layer_identifier val=\"0x%x\"/>\n",
			is_object_layer_identifier);
		if(is_object_layer_identifier)
		{
			pFormatter->Print("<object_layer_identifier>\n");
			pFormatter->Print("<video_object_layer_verid val=\"0x%x\"/>\n",
				video_object_layer_verid);
			pFormatter->Print("<video_object_layer_priority val=\"0x%x\"/>\n",
				video_object_layer_priority);
			pFormatter->Print("</object_layer_identifier>\n");
		}
		static const char * const ratio_conv_tab[] =
		{
			"forbidden",
				"1:1,square",
				"12:11",
				"10:11",
				"16:11",
				"40:33",
		};
		pFormatter->Print("<aspect_ratio_info val=\"0x%x(%s)\"/>\n",
			aspect_ratio_info,
			ratio_conv_tab[aspect_ratio_info%6]);
		// 1: 1:1(Square)
		// 2: 12:11
		// 3: 10:11
		// 4: 16:11
		// 5: 40:33
		// ff: extended PAR.
		if(aspect_ratio_info==0xff)
		{
			pFormatter->Print("<par_width val=\"0x%x\"/>\n",
				par_width);
			pFormatter->Print("<par_height val=\"0x%x\"/>\n",
				par_height);
		}
		pFormatter->Print("<vol_control_parameters val=\"0x%x\"/>\n",
			vol_control_parameters);
		
		if(vol_control_parameters)
		{
			pFormatter->Print("<chroma_format val=\"0x%x\"/>\n",
				chroma_format);
			pFormatter->Print("<low_delay val=\"0x%x\"/>\n",
				low_delay);
			pFormatter->Print("<vbv_parameters val=\"0x%x\"/>\n",
				vbv_parameters);
			if(vbv_parameters)
			{
			}
		}
		
		pFormatter->Print("<video_object_layer_shape val=\"0x%x\"/>\n",
			video_object_layer_shape);
		
		if(video_object_layer_shape==0x3
			&& video_object_layer_verid==0x1)
		{
			pFormatter->Print("<video_object_layer_shape_extension val=\"%d\"/>\n",
				video_object_layer_shape_extension);
		}
		
		pFormatter->Print("<vop_time_increment_resolution val=\"%d\"/>\n",
			vop_time_increment_resolution);
		
		pFormatter->Print("<fixed_vop_rate val=\"%d\"/>\n",
			fixed_vop_rate);
		if(fixed_vop_rate)
		{
			pFormatter->Print("<fixed_vop_time_increment val=\"%d\"/>\n",
				fixed_vop_time_increment);
		}
		if(video_object_layer_shape!=0x1)
		{
			if(video_object_layer_shape==0x0)
			{
				pFormatter->Print("<video_object_layer_width val=\"%d\"/>\n",
					video_object_layer_width);
				pFormatter->Print("<video_object_layer_height val=\"%d\"/>\n",
					video_object_layer_height);
			}
			pFormatter->Print("<interlaced val=\"%d\"/>\n",
				interlaced);
			pFormatter->Print("<obmc_disable val=\"%d\"/>\n",
				obmc_disable);
			pFormatter->Print("<sprite_enable val=\"%d\"/>\n",
				sprite_enable);
			if(sprite_enable==0x1 || sprite_enable==0x2)
			{
				if(sprite_enable!=0x2) //!=GMC
				{
					pFormatter->Print("<sprite_width val=\"%d\"/>\n",
						sprite_width);
					pFormatter->Print("<sprite_height val=\"%d\"/>\n",
						sprite_height);
					pFormatter->Print("<sprite_left_coordinate val=\"%d\"/>\n",
						sprite_left_coordinate);
					pFormatter->Print("<sprite_top_coordinate val=\"%d\"/>\n",
						sprite_top_coordinate);
				}
				pFormatter->Print("<no_of_sprite_warping_pints val=\"%d\"/>\n",
					no_of_sprite_warping_pints);
				pFormatter->Print("<sprite_warping_accuracy val=\"%d\"/>\n",
					sprite_warping_accuracy);
				pFormatter->Print("<sprite_brightness_change val=\"%d\"/>\n",
					sprite_brightness_change);
				if(sprite_enable!=0x2) //!=GMC
				{
					pFormatter->Print("<low_latency_sprite_enable val=\"%d\"/>\n",
						low_latency_sprite_enable);
				}
			}
			if(video_object_layer_verid!=0x1 &&
				video_object_layer_shape!=0x0)
			{
				pFormatter->Print("<sadct_disable val=\"%d\"/>\n",
					sadct_disable);
			}
			pFormatter->Print("<not_8_bit val=\"%d\"/>\n", not_8_bit);
			if(not_8_bit)
			{
				pFormatter->Print("<not_8_bit>\n");
				pFormatter->Print("<quant_precision val=\"%d\"/>\n",quant_precision);
				pFormatter->Print("<bits_per_pixel val=\"%d\"/>\n",bits_per_pixel);
				pFormatter->Print("</not_8_bit>\n");
			}
			if(video_object_layer_shape==0x3)
			{
				pFormatter->Print("<no_gray_quant_update val=\"%d\"/>\n",no_gray_quant_update);
				pFormatter->Print("<composition_method val=\"%d\"/>\n",composition_method);
				pFormatter->Print("<linear_composition val=\"%d\"/>\n",linear_composition);
				//pFormatter->Print("<FIXME>%s,%d</FIXME>\n",__FILE__,__LINE__);
			}
			pFormatter->Print("<quant_type val=\"%d\"/>\n", quant_type);
			
			if(quant_type)
			{
				pFormatter->Print("<quant_type>\n");
				pFormatter->Print("<load_intra_quant_mat val=\"%d\"/>\n", 
					load_intra_quant_mat);
				if(load_intra_quant_mat)
				{
					pFormatter->Print("<data>\n");
					for(i=0;i<64;i++)
					{
						if(intra_quant_mat[i]==0) break;
						pFormatter->Print("0x%x", intra_quant_mat[i]);
					}
					pFormatter->Print("\n</data>\n");
				}
				if(video_object_layer_shape==0x3) // ==grayscale
				{
					for(i=0;i<aux_comp_count;i++)
					{
						pFormatter->Print("<load_intra_quant_mat_grayscale val=\"%d\"/>\n", 
							load_intra_quant_mat_grayscale);
							/*
							if(load_intra_quant_mat_grayscale)
							{
							for(i=0;i<2*64;i++)
							intra_quant_mat_grayscale[i]
							}
						*/
						pFormatter->Print("<load_nonintra_quant_mat_grayscale val=\"%d\"/>\n", 
							load_nonintra_quant_mat_grayscale);
							/*
							if(load_nonintra_quant_mat_grayscale)
							{
							for(i=0;i<2*64;i++)
							nonintra_quant_mat_grayscale[i]
							}
						*/
					} //for
				} //if grayscale
				pFormatter->Print("</quant_type>\n");
			} //if qant_type
			
			if(video_object_layer_verid!=0x1)
				pFormatter->Print("<quarter_sample val=\"%d\"/>\n", 
				quarter_sample);
			pFormatter->Print("<complexity_estimation_disable val=\"%d\"/>\n", 
				quarter_sample);
			if(complexity_estimation_disable==0)
			{
				pFormatter->Print("<estimation_method val=\"%d\"/>\n", 
					estimation_method);
				if(estimation_method==0 || estimation_method==1)
				{
					pFormatter->Print("<shape_complexity_estimation_disable val=\"%d\"/>\n", 
						shape_complexity_estimation_disable);
					if(shape_complexity_estimation_disable==0)
					{
						pFormatter->Print("<opaque val=\"%d\"/>\n", 
							opaque);
						pFormatter->Print("<transparent val=\"%d\"/>\n", 
							transparent);
						pFormatter->Print("<intra_cae val=\"%d\"/>\n", 
							intra_cae);
						pFormatter->Print("<inter_cae val=\"%d\"/>\n", 
							inter_cae);
						pFormatter->Print("<no_update val=\"%d\"/>\n", 
							no_update);
						pFormatter->Print("<upsampling val=\"%d\"/>\n", 
							upsampling);
					}
					
					pFormatter->Print("<texture_complexity_estimation_set_1_disable val=\"%d\"/>\n", texture_complexity_estimation_set_1_disable);
					
					if(texture_complexity_estimation_set_1_disable==0)
					{
						pFormatter->Print("<intra_blocks val=\"%d\"/>\n", 
							intra_blocks);
						pFormatter->Print("<inter_blocks val=\"%d\"/>\n", 
							inter_blocks);
						pFormatter->Print("<inter4v_blocks val=\"%d\"/>\n", 
							inter4v_blocks);
						pFormatter->Print("<not_coded_blocks val=\"%d\"/>\n", 
							not_coded_blocks);
					}
					
					pFormatter->Print("<texture_complexity_estimation_set_2_disable val=\"%d\"/>\n", texture_complexity_estimation_set_2_disable);
					
					if(texture_complexity_estimation_set_2_disable==0)
					{
						pFormatter->Print("<dct_coefs val=\"%d\"/>\n", 
							dct_coefs);
						pFormatter->Print("<dct_lines val=\"%d\"/>\n", 
							dct_lines);
						pFormatter->Print("<vlc_symbols val=\"%d\"/>\n", 
							vlc_symbols);
						pFormatter->Print("<vlc_bits val=\"%d\"/>\n", 
							vlc_bits);
					}
					pFormatter->Print("<motion_compensation_complexity_disable val=\"%d\"/>\n", motion_compensation_complexity_disable);
					if(motion_compensation_complexity_disable==0)
					{
						pFormatter->Print("<apm val=\"%d\"/>\n", 
							apm);
						pFormatter->Print("<npm val=\"%d\"/>\n", 
							npm);
						pFormatter->Print("<interpolate_mc_q val=\"%d\"/>\n", 
							interpolate_mc_q);
						pFormatter->Print("<forw_back_mc_q val=\"%d\"/>\n", 
							forw_back_mc_q);
						pFormatter->Print("<halfpel2 val=\"%d\"/>\n", 
							halfpel2);
						pFormatter->Print("<halfpel4 val=\"%d\"/>\n", 
							halfpel4);
					}
					
					if(estimation_method==0x1)
					{
						pFormatter->Print("<version2_complexity_estimation_disable val=\"%d\"/>\n", version2_complexity_estimation_disable);
						if(version2_complexity_estimation_disable!=0)
						{
							pFormatter->Print("<sadct val=\"%d\"/>\n", 
								sadct);
							pFormatter->Print("<quarterpel val=\"%d\"/>\n", 
								quarterpel);
						}
					}
				}
			}
			pFormatter->Print("<resync_marker_disable val=\"%d\"/>\n", 
				resync_marker_disable);
			pFormatter->Print("<data_partitioned val=\"%d\"/>\n", 
				data_partitioned);
			if(data_partitioned)
				pFormatter->Print("<reversible_vlc val=\"%d\"/>\n", 
				reversible_vlc);
			if(video_object_layer_verid!=0x1)
			{
				pFormatter->Print("<newpred_enable val=\"%d\"/>\n", 
					newpred_enable);
				if(newpred_enable)
				{
					pFormatter->Print("<requested_upstream_message_type val=\"%d\"/>\n", 
						requested_upstream_message_type);
					pFormatter->Print("<newpred_segment_type val=\"%d\"/>\n", 
						newpred_segment_type);
				}
				pFormatter->Print("<reduced_resolution_vop_enable val=\"%d\"/>\n", 
					reduced_resolution_vop_enable);
			}
			pFormatter->Print("<scalability val=\"%d\"/>\n", 
				scalability);
			
			if(scalability)
			{
				pFormatter->Print("<hierachy_type val=\"%d\"/>\n", 
					hierachy_type);
				pFormatter->Print("<ref_layer_id val=\"%d\"/>\n", 
					ref_layer_id);
				pFormatter->Print("<ref_layer_sampling_direc val=\"%d\"/>\n", 
					ref_layer_sampling_direc);
				pFormatter->Print("<hor_sampling_factr_n val=\"%d\"/>\n", 
					hor_sampling_factor_n);
				pFormatter->Print("<hor_sampling_factr_m val=\"%d\"/>\n", 
					hor_sampling_factor_m);
				pFormatter->Print("<vert_sampling_factr_n val=\"%d\"/>\n", 
					vert_sampling_factor_n);
				pFormatter->Print("<vert_sampling_factr_m val=\"%d\"/>\n", 
					vert_sampling_factor_m);
				pFormatter->Print("<enhancement_type val=\"%d\"/>\n", 
					enhancement_type);
				if(video_object_layer_shape==0x1 && //binary
					hierachy_type==0)
				{
					pFormatter->Print("<use_ref_shape val=\"%d\"/>\n", 
						use_ref_shape);
					pFormatter->Print("<use_ref_texture val=\"%d\"/>\n", 
						use_ref_texture);
					pFormatter->Print("<shape_hor_sampling_factor_n val=\"%d\"/>\n", 
						shape_hor_sampling_factor_n);
					pFormatter->Print("<shape_hor_sampling_factor_m val=\"%d\"/>\n", 
						shape_hor_sampling_factor_m);
					pFormatter->Print("<shape_vert_sampling_factor_n val=\"%d\"/>\n", 
						shape_vert_sampling_factor_n);
					pFormatter->Print("<shape_vert_sampling_factor_m val=\"%d\"/>\n", 
						shape_vert_sampling_factor_m);
				}
			} // scalibility
		}
		else
		{
			if(video_object_layer_verid !=0x1) 
			{
				pFormatter->Print("<scalability val=\"%d\"/>\n", scalability);
				if(scalability)
				{
					pFormatter->Print("<ref_layer_id val=\"%d\"/>\n", 
						ref_layer_id);
					pFormatter->Print("<shape_hor_sampling_factor_n val=\"%d\"/>\n", 
						shape_hor_sampling_factor_n);
					pFormatter->Print("<shape_hor_sampling_factor_m val=\"%d\"/>\n", 
						shape_hor_sampling_factor_m);
					pFormatter->Print("<shape_vert_sampling_factor_n val=\"%d\"/>\n", 
						shape_vert_sampling_factor_n);
					pFormatter->Print("<shape_vert_sampling_factor_m val=\"%d\"/>\n", 
						shape_vert_sampling_factor_m);
				}
			}
		}
	}
	
