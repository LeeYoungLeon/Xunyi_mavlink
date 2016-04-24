//============================================================================
// Name        : main.cpp
// Author      : XYI
// Version     :
// Copyright   : XYI 
// Description : XYI Onboard API test in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sched.h>R
#include <execinfo.h>
#include "DJI_Pro_Sample.h"
#include "XY_LIB/wireless_debug.h"
#include "XY_LIB/route.h"
#include "XY_LIB/common/common.h"
#include "XY_LIB/control_steer.h"
#include "XY_LIB/range.h"
#include "XY_LIB/image_identify.h"
#include "XY_LIB/thread_common_op.h"
#include "XY_LIB/status_led.h"
#include "XY_LIB/http_chat_3g.h"
#include "XY_LIB/sd_store_log.h"

// ----------------------------------------
//             MAVLink headers
// ----------------------------------------

#include "mavlink/include/mavlink/v1.0/common/mavlink.h"
#include "XY_LIB/autopilot_interface.h"
#include "XY_LIB/serial_port.h"

// ----------------------------------------

#include <cstdlib>
#include <cmath>
#include <inttypes.h>
#include <fstream>
#include <time.h>
#include <sys/time.h>

#define DEBUG_PRINTF 1

using std::string;
using namespace std;
extern Aircraft_Status aircraft_status;

/////////////////////////////////////////LLL Declaration for MAVLink LLL//////////////////////////////////////

char *control_uart_name = (char*)"/dev/ttyUSB0";
int control_baudrate = 921600;
Serial_Port control_serial_port(control_uart_name, control_baudrate);
Autopilot_Interface XYI_Brain(&serial_port);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void handle_signal(int signum)
{
	switch(signum)
	{
		case SIGINT:
			printf("SIGINT\n");		
			//raise(SIGKILL);
			exit(0);
			break;
		default:
			break;
	}
}

void handle_crash(int signum)
{
	void *array[30] = {0};
	size_t size;  
	char **strings = NULL;  
	size_t i;
	
	size = backtrace(array, 30);
	printf("Obtained %zd stack frames.\n", size);  
	strings = backtrace_symbols(array, size);  
	if (NULL == strings)  
	{  
		perror("backtrace_synbols");  
		exit(EXIT_FAILURE);  
	}  
	
	for (i = 0; i < size; i++)  
		printf("%s\n", strings[i]);  

	free(strings);  
	strings = NULL; 
	exit(0);
}



/* this function should be called by main first */
int setup_main_process_scheduler(int _policy, int _priority)
{
	struct sched_param param;
	int maxpri;
	
	maxpri = sched_get_priority_max(_policy);

	_priority = _priority < maxpri ? _priority : maxpri;
	
	param.sched_priority = _priority;		
	
	if(sched_setscheduler(getpid(), _policy, &param) == -1) //设置优先级
	{ 
		perror("sched_setscheduler() failed"); 
		return 1;
	} 

	return 0;
}

int process_binding_cpu(int cpu_seq)
{
	int cpu_core_num = get_nprocs();
	printf("CPU has %d core\n", cpu_core_num);
	cpu_set_t mask; 
 	cpu_set_t get; 

	cpu_seq = cpu_seq >= (cpu_core_num - 1) ? (cpu_core_num - 1) : cpu_seq;

	CPU_ZERO(&mask); 
	CPU_SET(cpu_seq, &mask); 
	if (sched_setaffinity(0, sizeof(mask), &mask) == -1) { 
		printf("warning: could not set CPU affinity, continuing\n"); 
		return 1;
	} 
	
	CPU_ZERO(&get); 
	if (sched_getaffinity(0, sizeof(get), &get) == -1) { 
		printf("warning: cound not get cpu affinity\n"); 
		return 1;
	} 
	for (int i = 0; i < cpu_core_num; i++) { 
		if (CPU_ISSET(i, &get)) { 
			printf("this process %d is running processor : %d\n",getpid(), i); 
		} 
	} 

	return 0;
	
}


int main(int argc,char **argv)
{
	activate_data_t user_act_data; 
	char temp_buf[65];
	int i = 0;
	int wait_time = 0;

	printf("\nXunyi Drone Test,Ver 1.1.0\n\n");

	setup_main_process_scheduler(SCHED_RR, 99);		//如无另行设置，后面创建的线程将继承这里的属性(调度策略+优先级)
	//process_binding_cpu(GENERAL_JOB_CPU);							
	
	signal(SIGINT, handle_signal);
	signal(SIGSEGV, handle_crash);
	
	if(XY_Debug_Setup() < 0)
	{
		printf("Debug Function Open ERROR...\n");
	}
	printf("Debug Function Open SUCCESS...\n");
	XY_Debug_Send_At_Once("Debug Function Open SUCCESS.\n");

  	if(XY_Status_Led_Setup() < 0)
	{
		printf("Led Function Open ERROR...\n");
		XY_Debug_Send_At_Once("Led Function Open ERROR.\n");
	}
	printf("Led Function Open SUCCESS...\n");
	XY_Debug_Send_At_Once("Led Function Open SUCCESS.\n");

	ioctl_led(2);

	if(XY_Capture_Setup() < 0)
	{
		printf("Capture function Open ERROR...\n");
		XY_Debug_Send_At_Once("Capture function Open ERROR.\n");
	}
	printf("Capture function Open SUCCESS...\n");
	XY_Debug_Send_At_Once("Capture function Open SUCCESS.\n");

#if 1
_relink:
	if(XY_Http_Chat_Setup() < 0)
	{
		printf("Http Chat Function Open ERROR...\n");
		XY_Debug_Send_At_Once("Http Chat Function Open ERROR.\n");
	}
	printf("Http Chat Function Open SUCCESS...\n");
	XY_Debug_Send_At_Once("Http Chat Function Open SUCCESS.\n");
	
	pthread_join(get_tid(THREADS_WAIT_ORDER), NULL );

	if( get_order_status() == 0)
	{
		goto _relink;
	}

	if( XY_Http_Reported_Setup() < 0)
	{
		printf("Http Reported Function Open ERROR...\n");
		XY_Debug_Send_At_Once("Http Reported Function Open ERROR.\n");
	}
	printf("Http Reported Function Open SUCCESS...\n");
	XY_Debug_Send_At_Once("Http Reported Function Open SUCCESS.\n");
	
	ioctl_led(2);
	if(argc == 2)
	{
		wait_time = atoi(argv[1]);

		wait_time = wait_time<5 ? 5 : wait_time;
		wait_time = wait_time>90 ? 90 : wait_time;
			
		printf("Please wait %ds\n", wait_time);
		XY_Debug_Send_At_Once("Please wait %ds\n", wait_time);
		
		while(i < wait_time)
		{
			sleep(1);
			i++;
			if(i % 2 != 0)
				continue;
			
			printf("Last: %ds\n", wait_time-i);
			XY_Debug_Send_At_Once("Last: %ds\n", wait_time-i);
		}
	}
	

/////////////////////////////////////////LLL open serial port  instance autopilot object LLL//////////////////////////////////////////

	control_serial_port.start();
	XYI_Brain.start();

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	if(XY_Ultra_Setup("/dev/ttyTHS2", 9600) < 0)
	{
		printf("Ultra Function Open ERROR.\n");
		XY_Debug_Send_At_Once("Ultra Function Open ERROR.\n");
	}
	sleep(5);
	int count = 0;	


////////////////////////////////////////LLL enable offboard control LLL//////////////////////////////////////////////////////////////

	XYI_TK1.enable_offboard_control();
	mavlink_heartbeat_t hbt;
	while(1)
	{
		hbt = XYI_TK1.current_messages.heartbeat;
		if ( (hbt.base_mode & MAV_MODE_FLAG_AUTO_ENABLED) )
		{
			break;
		}
		usleep(0.2 * 1E6);
	}
	printf("Autonomous mode is set, start to flight.\n");
	XY_Debug_Send_At_Once("Autonomous mode is set, start to flight.\n");
	
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	enable_report_drone_pos();

	if(XY_SD_Log_Setup() < 0)
	{
		printf("Log Function Open ERROR.\n");
		XY_Debug_Send_At_Once("Log Function Open ERROR.\n");
	}
	XY_Start_Store_Log();
	
	XY_Drone_Execute_Task(XYI_TK1);
	
	XY_Stop_Store_Log();

	XY_close_Capture();
	
	sleep(50);
	pause();
	
	return 0;
}
