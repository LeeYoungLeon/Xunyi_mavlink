#include "Interface_Data.h"


void GPS_MAV_to_XYI_trans(mavlink_global_position_int_t *MAV_gps, mavlink_gps_raw_int_t *MAV_gps_raw, XYI_global_position_float *XYI_gps)
{
	mavlink_global_position_int_t mavgps = *MAV_gps;
	mavlink_gps_raw_int_t mavgps_raw = *MAV_gps_raw;
	XYI_global_position_float xyigps = { 0 };
	
	xyigps.lati = ( (float) mavgps.lat ) / 1E7;
	xyigps.longti = ( (float) mavgps.lon ) / 1E7;
	xyigps.alti= ( (float) mavgps.alt ) / 1000;
	xyigps.height= ( (float) mavgps.relative_alt ) / 1000;
	
	if (mavgps_raw.satellites_visible < 4)
	{
		xyigps.health_flag = 2;
	}
	else if (mavgps_raw.satellites_visible = 4)
	{ 
		xyigps.health_flag = 3;
	}
	else if (mavgps_raw.satellites_visible = 5)
	{
		xyigps.health_flag = 4;
	}
	else if (mavgps_raw.satellites_visible >= 6)
	{
		xyigps.health_flag = 5;
	}
	else
	{
		xyigps.health_flag = 1;
	}
	*XYI_gps = xyigps;	
}






