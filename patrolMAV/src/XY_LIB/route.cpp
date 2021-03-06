#include "route.h"
#include "thread_common_op.h"
#include "image_identify.h"
#include "sd_store_log.h"
#include "wireless_debug.h"
#include "control_law.h"
#include "http_chat_3g.h"
#include "range.h"
#define DEBUG_PRINT	1

Leg_Node *patrol_route_head = NULL;
Leg_Node *goback_route_head = NULL;
Leg_Node *cur_legn = NULL;
Leg task_info;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
extern struct debug_info debug_package;

int drone_goback = 0;

static void *drone_patrol_task_thread_func(void * arg);
static void *drone_patrol_up_thread_func(void * arg);
static void *drone_patrol_p2p_thread_func(void * arg);

static void *drone_goback_task_thread_func(void * arg);
static void *drone_goback_down_thread_func(void * arg);

int XY_Drone_Execute_Task(Autopilot_Interface &XYI_Brain)
{
	if( XY_Drone_Patrol_Task(XYI_Brain) < 0 )
		return -1;
	
	if( XY_Drone_Goback_Task(XYI_Brain) < 0 )
		return -1;
	
	return 0;
}

int XY_Drone_Patrol_Task(void)
{
	printf("--------patrol_task--------\n");
	if(XY_Create_Thread(drone_patrol_task_thread_func, NULL, THREADS_DELIVER, -1, SCHED_RR, 97) < 0)
	{
		printf("Create Drone Patrol Task Thread Error.\n");
		return -1;
	}
	pthread_join(get_tid(THREADS_DELIVER), NULL);
	return 0;
}

int XY_Drone_Goback_Task(void)
{

	if(XY_Create_Thread(drone_goback_task_thread_func, NULL, THREADS_GOBACK, -1, SCHED_RR, 97) < 0)
	{
		printf("Create Drone Goback Task Thread Error.\n");
		return -1;
	}
	pthread_join(get_tid(THREADS_GOBACK), NULL);

	return 0;
}

/* ============================================================================ */
static void *drone_patrol_task_thread_func(void * arg)
{
	int ret;
	pthread_t tid;
	printf("--------init_patrol-------\n");
	ret = init_patrol_route_list();
	if(ret == -1)
	{
		perror("init_patrol_route_list");
	}
	
	if(patrol_route_head->next == NULL)
	{
		goto error;
	}
	cur_legn = patrol_route_head->next;
	//cur_legn = patrol_route_head;
	
	printf("------------------ patrol task start -----------------\n");
	while(1)
	{		
		
		//Up
		if( pthread_create(&tid, 0, drone_patrol_up_thread_func, NULL) != 0  )
		{
			goto error;
		}
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond , &mutex);
		pthread_mutex_unlock(&mutex);

		//P2P on H3
		if( pthread_create(&tid, 0, drone_patrol_p2p_thread_func, NULL) != 0  )
		{
			goto error;
		}
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond , &mutex);
		pthread_mutex_unlock(&mutex);
		
		if(cur_legn->next == NULL)
		{
			goto exit;
		}
	}

error:
	XY_Release_List_Resource(patrol_route_head);
exit:
	printf("------------------ patrol task over ------------------\n");	
	pthread_exit(NULL);

}

/* ============================================================================ */
static void *drone_goback_task_thread_func(void * arg)
{
	int ret;
	pthread_t tid;

	printf("------------------ goback task start ------------------\n");
	XY_Debug_Send_At_Once("\n----------------- goback task start -----------------\n");
	while(1)
	{
		if( pthread_create(&tid, 0, drone_goback_down_thread_func, NULL) != 0  )
		{
			goto error;
		}
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&cond , &mutex);
		pthread_mutex_unlock(&mutex);

		goto exit;
	}

error:
	XY_Release_List_Resource(goback_route_head);
exit:
	printf("------------------ goback task over ------------------\n");
}


/* ============================================================================ */

int drone_patrol_takeoff(void)
{
	float max_vel, min_vel, t_height, threshold, kp_z;
	
	/* code just run one time */
	max_vel 	= DELIVER_MAX_VEL_UP_TO_H1;					//m/s
	min_vel		= DELIVER_MIN_VEL_UP_TO_H1;
	t_height 	= DIFF_HEIGHT_WHEN_TAKEOFF;					//m
	threshold 	= DELIVER_THRESHOLD_OF_UP_TO_H1_OUT;		//m
	kp_z 		= DELIVER_UP_TO_H1_KPZ;
	
	if( XY_Ctrl_Drone_Up_Has_NoGPS_Mode_And_Approach_Put_Point_DELIVER(max_vel, min_vel, t_height, threshold, kp_z) == 1)
		return 1;
	else 
		return -1;
}

int drone_patrol_up_to_h2(void)
{
	float max_vel, min_vel, t_height, threshold, kp_z;
	
	/* code just run one time */
	max_vel 	= DELIVER_MAX_VEL_UP_TO_H2;					//m/s
	min_vel		= DELIVER_MIN_VEL_UP_TO_H2;
	t_height 	= DELIVER_HEIGHT_OF_UPH2;					//m
	threshold 	= DELIVER_THRESHOLD_OF_UP_TO_H2_OUT;		//m
	kp_z 		= DELIVER_UP_TO_H2_KPZ;
	
	if( XY_Ctrl_Drone_Up_Has_NoGPS_Mode_And_Approach_Put_Point_DELIVER(max_vel, min_vel, t_height, threshold, kp_z) == 1)
		return 1;
	else 
		return -1;
}

int drone_patrol_up_to_h3(void)
{
	float max_vel, min_vel, t_height, threshold, kp_z;
	
	/* code just run one time */
	max_vel 	= DELIVER_MAX_VEL_UP_TO_H3;					//m/s
	min_vel		= DELIVER_MIN_VEL_UP_TO_H3;			
	t_height 	= DELIVER_HEIGHT_OF_UPH3;					//m
	threshold 	= DELIVER_THRESHOLD_OF_UP_TO_H3_OUT;		//m
	kp_z		= DELIVER_UP_TO_H3_KPZ;
	
	if( XY_Ctrl_Drone_To_Assign_Height_Has_MaxVel_And_FP_DELIVER(max_vel, min_vel, t_height, threshold, kp_z) == 1)
		return 1;
	else 
		return -1;
}

static void *drone_patrol_up_thread_func(void * arg)
{
	//Up to H1
	printf("----------------- take off -----------------\n");
	XY_Debug_Send_At_Once("\n----------------- Take off -----------------\n");
	while(1)
	{
		if( drone_patrol_takeoff() == 1)
			break;
	}
	XY_Start_Capture();
	printf("------------------ start up to h2 ------------------\n");
	XY_Debug_Send_At_Once("\n----------------- Up to H2 - %fm -----------------\n", DELIVER_HEIGHT_OF_UPH2);
	while(1)
	{
		if( drone_patrol_up_to_h2() == 1)
			break;
	}
	XY_Stop_Capture();

	printf("------------------ start up to h3 ------------------\n");
	XY_Debug_Send_At_Once("\n----------------- Up to H3 - %fm -----------------\n", DELIVER_HEIGHT_OF_UPH3);
	while(1)
	{
		if( drone_patrol_up_to_h3() == 1)
			break;
	}
	
}
/* ============================================================================ */




/* ============================================================================ */
int drone_patrol_p2p(void)
{
	float p2p_height;
	
	p2p_height = DELIVER_HEIGHT_OF_UPH3;
	if( XY_Ctrl_Drone_P2P_With_FP_COMMON(p2p_height, 0) == 1)
		return 1;
	else 
		return -1;
}


static void *drone_patrol_p2p_thread_func(void * arg)
{
	printf("------------------ start p2p ------------------\n");
	XY_Debug_Send_At_Once("\n----------------- Start P2P -----------------\n");
	while(1)
	{
		if( drone_patrol_p2p() == 1)
			break;
	}
	
}
/* ============================================================================ */



/* ============================================================================ */
int drone_patrol_down_to_h1(void)
{
	float max_vel, min_vel, t_height, threshold, kp_z;
	
	/* code just run one time */
	max_vel 	= DELIVER_MAX_VEL_DOWN_TO_H1;				//m/s
	min_vel		= DELIVER_MIN_VEL_DOWN_TO_H1;
	t_height	= DELIVER_HEIGHT_OF_DOWNH1;					//m
	threshold	= DELIVER_THRESHOLD_OF_DOWN_TO_H1_OUT;		//m
	kp_z		= DELIVER_DOWN_TO_H1_KPZ;
	
	if( XY_Ctrl_Drone_To_Assign_Height_Has_MaxVel_And_FP_DELIVER(max_vel, min_vel, t_height, threshold, kp_z) == 1)
		return 1;
	else 
		return -1;
}

int drone_patrol_find_put_point_with_image(void)
{
	if( XY_Ctrl_Drone_Spot_Hover_And_Find_Put_Point_DELIVER() == 1)
		return 1;
	else 
		return -1;
}

/*
 *             ---
 *			    |
 *			    |
 *			   ---	H1,Start to identify image
 *			    |
 *			    |  
 *			    |   Down with identift image  
 *			    |
 *			    |
 *			   ---  H2, 
 *				|
 *			   ---  H3, Hover and put goods
 */


/* ============================================================================ */


/* ============================================================================ */
int drone_goback_down_to_h1(void)
{
	float max_vel, min_vel, t_height, threshold, kp_z;
	
	/* code just run one time */
	max_vel 	= GOBACK_MAX_VEL_DOWN_TO_H1;				//m/s
	min_vel		= GOBACK_MIN_VEL_DOWN_TO_H1;
	t_height	= GOBACK_HEIGHT_OF_DOWNH1;					//m
	threshold	= GOBACK_THRESHOLD_OF_DOWN_TO_H1_OUT;		//m
	kp_z		= GOBACK_DOWN_TO_H1_KPZ;
	
	if( XY_Ctrl_Drone_To_Assign_Height_Has_MaxVel_And_FP_DELIVER(max_vel, min_vel, t_height, threshold, kp_z) == 1)
		return 1;
	else 
		return -1;
}

int drone_goback_find_put_point_with_image(void)
{
	if(drone_patrol_find_put_point_with_image() == 1)
		return 1;
	else 
		return -1;
}

int drone_goback_down_to_h2(void)
{
	float max_vel, min_vel, t_height, threshold, kp_z;
	
	/* code just run one time */
	max_vel 	= GOBACK_MAX_VEL_DOWN_TO_H2;					//m/s
	min_vel		= GOBACK_MIN_VEL_DOWN_TO_H2;
	t_height 	= GOBACK_HEIGHT_OF_DOWNH2;						//m
	threshold 	= GOBACK_THRESHOLD_OF_DOWN_TO_H2_OUT;			//m
	kp_z 		= GOBACK_DOWN_TO_H2_KPZ;
	if( XY_Ctrl_Drone_Down_Has_NoGPS_Mode_And_Approach_Put_Point_GOBACK(max_vel, min_vel, t_height, threshold, kp_z) == 1)
		return 1;
	else 
		return -1;
}

int drone_goback_down_land(void)
{
	float max_vel, min_vel, t_height, threshold, kp_z;
	
	/* code just run one time */
	max_vel 	= GOBACK_MAX_VEL_DOWN_LAND;					//m/s
	min_vel		= GOBACK_MIN_VEL_DOWN_LAND;
	t_height 	= GOBACK_HEIGHT_OF_LAND;						//m
	threshold 	= GOBACK_THRESHOLD_OF_DOWN_LAND_OUT;			//m
	kp_z 		= GOBACK_DOWN_LAND_KPZ;
	
	if( XY_Ctrl_Drone_To_Assign_Height_Has_MaxVel_And_FP_DELIVER(max_vel, min_vel, t_height, threshold, kp_z) == 1)
		return 1;
	else 
		return -1;
}
/*
 *             ---
 *			    |
 *			    |
 *			   ---	H1,Start to identify image
 *			    |
 *			    |  
 *			    |   Down with identift image to H2
 *			    |
 *			    |
 *			   ---  H2, Down to H3
 *				|
 *			   ---  H3, Take off
 */
static void *drone_goback_down_thread_func(void * arg)
{
	printf("------------------ start down to h1 ------------------\n");
	XY_Debug_Send_At_Once("\n----------------- Down to H1 - %fm -----------------\n", GOBACK_HEIGHT_OF_DOWNH1);
	while(1)
	{
		if( drone_goback_down_to_h1() == 1)
			break;
	}

#if 1
	printf("------------------ start find put point ------------------\n");
	XY_Debug_Send_At_Once("\n----------------- Start Find Put Point -----------------\n");
	XY_Start_Capture();
	while(1)
	{
		if( drone_goback_find_put_point_with_image() == 1 )
			break;	
	}
#endif

	XY_Start_Capture();
	printf("------------------ start down to h2 ------------------\n");
	XY_Debug_Send_At_Once("\n----------------- Down to H2 - %fm -----------------\n", GOBACK_HEIGHT_OF_DOWNH2);
	while(1)
	{
		if( drone_goback_down_to_h2() == 1)
			break;
	}
	XY_Stop_Capture();
	printf("------------------ start landing ------------------\n");
	XY_Debug_Send_At_Once("\n----------------- landing -----------------\n");
	while(1)
	{
		if( drone_goback_down_land() == 1)
			break;
	}
	
}


void XY_Release_List_Resource(Leg_Node *_head)
{
	if(_head)
	{
		delete_leg_list(_head);
		printf("Memory of list is free.\n");
	}
}


int init_patrol_route_list(void)
{
	int ret = 0;
	static XYI_global_position_float start_pos;
	//int route_seq =0;
	int i=0;
	int route_count=0;
	double arrLng[255] = {};
	double arrLat[255] = {};
	double arrHeight[255] = {};
	extern cJSON *json;
	cJSON *taskArry;
	int arrySize;
	cJSON *tasklist;
	

	patrol_route_head = create_head_node();
	if(patrol_route_head == NULL)
	{
		printf("Create Patrol Route Head ERROR.\n");
		return -1;
	}

	do{
		GPS_MAV_to_XYI_trans(XYI_Brain.current_messages.global_position_int, XYI_Brain.current_messages.gps_raw, start_pos);		
		XY_Debug_Send_At_Once("Getting start pos\n");
	}while(start_pos.longti == 0);

	set_leg_start_pos(&task_info, start_pos.longti, start_pos.lati, 0.100000);
	arrLng[0] = task_info.start._longti;
	arrLat[0] = task_info.start._lati;
	arrHeight[0] = task_info.start._alti;
	printf("[%.8lf,%.8lf]\n", arrLng[0], arrLat[0]);
	route_count ++;
	
	taskArry=cJSON_GetObjectItem(json,"data");//取数组  
	arrySize=cJSON_GetArraySize(taskArry);//数组大小 
	printf("arrySize is %d\n",arrySize);
	tasklist=taskArry->child;//子对象  
	
	while(tasklist!=NULL)  
	{  	
		printf("[%lf,%lf,%lf]\n",cJSON_GetArrayItem(tasklist,0)->valuedouble,
			                     cJSON_GetArrayItem(tasklist,1)->valuedouble,
			                     cJSON_GetArrayItem(tasklist,2)->valuedouble); 
		printf("[%lf,%lf,%lf]\n", ((cJSON_GetArrayItem(tasklist,0)->valuedouble - GD2GE_LONGTI_DIFF)/180) * PI,
			                     ((cJSON_GetArrayItem(tasklist,1)->valuedouble - GD2GE_LATI_DIFF)/180) * PI,
			                     cJSON_GetArrayItem(tasklist,2)->valuedouble);
		
		arrLng[route_count] = ((cJSON_GetArrayItem(tasklist,0)->valuedouble - GD2GE_LONGTI_DIFF)/180) * PI;
		arrLat[route_count] = ((cJSON_GetArrayItem(tasklist,1)->valuedouble - GD2GE_LATI_DIFF)/180) * PI;
		arrHeight[route_count] = cJSON_GetArrayItem(tasklist,2)->valuedouble;
		
		tasklist=tasklist->next; 
		route_count++;
	} 	 
	arrLng[route_count] = start_pos.longti;
	arrLat[route_count] = start_pos.lati;
	arrHeight[route_count] = start_pos.height;
	
	for(i = 0; i <= route_count; i++)
	{
		set_leg_seq(&task_info, i);
		set_leg_end_pos(&task_info, arrLng[i], arrLat[i], arrHeight[i]);
		task_info.criFlag = 1;
		ret = insert_new_leg_into_route_list(patrol_route_head, task_info);
		printf("----add route %d-------\n",i);
		if(ret != 0)
		{
			printf("Add Patrol Route Node ERROR.\n");
			return -1;
		}
	}
	
	return 0;
}

int init_goback_route_list(void)
{
	int ret = 0;
	static XYI_global_position_float start_pos;

	goback_route_head = create_head_node();
	if(goback_route_head == NULL)
	{
		printf("Create Goback Route Head ERROR.\n");
		return -1;
	}

	set_leg_seq(&task_info, 1);
	set_leg_num(&task_info, 1);
	
	set_leg_end_pos(&task_info,
					patrol_route_head->next->leg.start._longti, 
					patrol_route_head->next->leg.start._lati,
					patrol_route_head->next->leg.start._alti);


	start_pos.longti = 0;
	do{
		GPS_MAV_to_XYI_trans(XYI_Brain.current_messages.global_position_int, XYI_Brain.current_messages.gps_raw, start_pos);
		XY_Debug_Send_At_Once("Getting start pos\n");
	}while(start_pos.longti == 0);	
	set_leg_start_pos(&task_info, start_pos.longti, start_pos.lati, 0.100000);

	printf("Initial information: (%.8lf, %.8lf) to (%.8lf, %.8lf)\n", 	task_info.start._longti, 
																		task_info.start._lati,
																		task_info.end._longti,
																		task_info.end._lati);

	ret = insert_new_leg_into_route_list(goback_route_head, task_info);
	if(ret != 0)
	{
		printf("Add Goback Route Node ERROR.\n");
		return -1;
	}
	return 0;
}




int setup_route_list_head_node(Leg_Node *head)
{
	head = create_head_node();
	if(head == NULL)
		return -1;
	return 0;
}


int insert_new_leg_into_route_list(Leg_Node *head, struct Leg leg)
{
	return add_leg_node(head, leg);
}


Leg_Node *create_head_node(void)
{
	Leg_Node *_head = NULL;

	_head = (Leg_Node *)calloc(1, sizeof(Leg_Node));
	if(_head == NULL)
	{
		return NULL;
	}
	_head->leg.leg_seq = 0;
	_head->next = NULL;
	_head->prev = NULL;
	return _head;
}

int add_leg_node(Leg_Node *_head, struct Leg _leg)
{
	Leg_Node *pcur, *pnew;
	//int valid_seq;
	
	pcur = _head;
	if(_leg.leg_seq == 0)
	{
		set_leg_end_pos(&(pcur->leg), _leg.start._longti, _leg.start._lati, _leg.start._alti);
		set_leg_seq(&(pcur->leg), _leg.leg_seq);
		pcur->leg.criFlag = _leg.criFlag;
	}
	else
	{
		while(pcur->next)
		{
			pcur = pcur->next;
		}

		/*
		valid_seq = pcur->leg.leg_seq+1;
		if(_leg.leg_seq != valid_seq)
		{
			return -1;
		}
		*/
		pnew = (Leg_Node *)calloc(1, sizeof(Leg_Node));
		if(pnew == NULL)
			return -1;
		
		set_leg_seq(&(pnew->leg), _leg.leg_seq);
		set_leg_start_pos(&(pnew->leg), _leg.start._longti, _leg.start._lati, _leg.start._alti);
		set_leg_end_pos(&(pnew->leg), _leg.end._longti, _leg.end._lati, _leg.end._alti);
		pnew->leg.criFlag = _leg.criFlag;
		//set_leg_start_xyz();
		//set_leg_end_xyz();
		
		pcur->next = pnew;
		pnew->prev = pcur;
		printf("pcur->next is %.8lf,%.8lf\n",pcur->next->leg.end._longti,pcur->next->leg.end._lati);
		pnew->next = NULL;
	}

	return 0;
}

void delete_leg_list(Leg_Node *_head)
{
	Leg_Node *p;

	while(_head->next)
	{
		p = _head;
		_head = _head->next;
		free(p);
	}
	free(_head);
	_head = p = NULL;

}

int update_cur_legn_data(double _longti, double _lati)
{
	if(!cur_legn)
		return -1;

	cur_legn->leg.end._longti = _longti;
	cur_legn->leg.end._alti = _lati;
	return 0;
}

