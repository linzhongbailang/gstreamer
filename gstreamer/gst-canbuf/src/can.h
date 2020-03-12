
#ifndef _CAN_H_
#define _CAN_H_

#include <stdint.h>

typedef float float32_t;

#define CURRENT_CAN_DATA_VERSION    0x01000000
#define CAN_DATA_FIX_LENGTH         512u

typedef struct Ofilm_Vehicle_Data_Tag
{
    uint32_t    version;
    uint32_t    data_length;
    uint64_t    time_stamp;

    uint16_t    year;
    uint16_t    month;
    uint16_t    day;
    uint16_t    hour;
    uint16_t    minute;
    uint16_t    second;
    uint32_t    rsv_0;          //to match old can data struct,can use old record can data

    //real vehicle data body
    float32_t   right_wheel_speed;
    float32_t   left_wheel_speed;
    float32_t   right_wheel_speed_rear;
    float32_t   left_wheel_speed_rear;
    float32_t   vehicle_speed;
    float32_t   yaw_rate;
    float32_t   steer_angle;

    uint8_t     turn_signal;
    uint8_t     vehicle_movement_state;
    uint8_t     wiper_state;
    uint8_t     high_beam;

    uint8_t     low_beam;
    uint8_t     chainsSwitch;
    uint8_t     BSDNotifyState;
    uint8_t     AutoBoxState;
    
    //============================
    uint8_t     region_one;
    uint8_t     region_two;
    uint8_t     region_three;
    uint8_t     region_four;

    uint8_t     region_five;
    uint8_t     region_six;
    uint8_t     region_seven;
    uint8_t     region_eight;
    
    //==============================
    uint8_t     drive_door;       
    uint8_t     passenger_door;   
    uint8_t     rear_left_door;   
    uint8_t     rear_right_door;  
    
    //============================
    uint8_t     Record;
    uint8_t     APA;
    uint8_t     Online;
    uint8_t     LDW;

    uint8_t     BSD;
    uint8_t     rsv_1[3];
    
    //============================             
    float32_t   Steering_Wheel_Angle_Speed;
    float32_t   Lateral_Acceleration;
    
    float32_t   Driver_Applied_Torque;
    float32_t   Delivered_Delta_Torque;
    float32_t   EPS_Torque;
    float32_t   Required_Torque;

	uint8_t   	Accelerator_Actual_Position;
	uint8_t   	Brake_Pedal_Position; 

    uint8_t     Brake_Pedal_Driver_Applied_Pressure_Detected;
    uint8_t     EPS_Status;
    uint8_t     Requirement_Active;
    uint8_t     EmergencyLightstatus;

    uint32_t    positioning_system_latitude;
    uint32_t    positioning_system_longitude;

    uint8_t		radar_rear_left_side_distance;
    uint8_t		radar_rear_left_coner_distance;
    uint8_t		radar_rear_left_middle_distance;
	uint8_t		radar_rear_left_face_distance;
	
    uint8_t		radar_rear_right_side_distance;
    uint8_t		radar_rear_right_coner_distance;
    uint8_t		radar_rear_right_middle_distance;
	uint8_t		radar_rear_right_face_distance;
	
    uint8_t		radar_front_left_side_distance;
    uint8_t		radar_front_left_coner_distance;
    uint8_t		radar_front_left_middle_distance;
	uint8_t		radar_front_left_face_distance;
	
    uint8_t		radar_front_right_side_distance;
    uint8_t		radar_front_right_coner_distance;
    uint8_t		radar_front_right_middle_distance;
	uint8_t		radar_front_right_face_distance;

    uint16_t    WRSLDWhlDistPlsCntr;
    uint16_t    WRSRDWhlDistPlsCntr;
    uint16_t    WRSLNDWhlDistPCntr;
    uint16_t    WRSRNDWhlDistPCntr;
    
    int32_t     Electrical_Power_Steering_Availability_Status;//????????????????????

    uint8_t     nrcd_status;
    uint8_t     lcc_enable;
    uint8_t     auto_park_enable;
    uint8_t     DriverBuckleSwitchStatus; 
	uint8_t		SRS_CrashOutputStatus;
	float32_t 	Longitudinal_Acceleration;
	uint8_t		Vehicle_Speed_Validity;
	uint8_t     KeyAlarm_Status;
	uint8_t     LCDAL_CTAAlert;
    uint8_t     LCDAR_CTAAlert;
	uint8_t	  	APA_LSCAction;
	uint8_t	  	LAS_IACCTakeoverReq;
	uint8_t	  	ACC_TakeOverReq;
	uint8_t	  	ACC_AEBDecCtrlAvail; 
}Ofilm_Vehicle_Data_T;


typedef union Ofilm_Can_Data_Tag
{
    uint8_t              byte[CAN_DATA_FIX_LENGTH];
    Ofilm_Vehicle_Data_T vehicle_data;
    
}Ofilm_Can_Data_T;

#endif

