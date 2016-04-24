#include "XY_LIB/common/common.h"
#include "mavlink/include/mavlink/v1.0/common/common.h"

typedef struct 
{
	float lati;
	float longti;
	float alti;
	float height;
	unsigned char health_flag;
}XYI_global_position_float;
