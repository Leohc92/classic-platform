/* 
* Configuration of module Os (Os_Cfg.c)
* 
* Created by: 
* Configured for (MCU): MPC551x
* 
* Module editor vendor:  ArcCore
* Module editor version: 2.0.7
* 
* 
* Generated by Arctic Studio (http://arccore.com)
*           on Wed May 05 23:09:13 CEST 2010
*/

	

#include <stdlib.h>
#include <stdint.h>
#include "Platform_Types.h"
#include "Os.h"				// includes Os_Cfg.h
#include "os_config_macros.h"
#include "kernel.h"
#include "kernel_offset.h"
#include "alist_i.h"
#include "Mcu.h"

extern void dec_exception( void );

// Set the os tick frequency
OsTickType OsTickFreq = 1000;


// ###############################    DEBUG OUTPUT     #############################
uint32 os_dbg_mask = D_RESOURCE |D_SCHTBL |D_EVENT |D_TASK |D_ALARM;
 


// #################################    COUNTERS     ###############################
GEN_COUNTER_HEAD {
	GEN_COUNTER(	COUNTER_ID_OsTick,
					"OsTick",
					COUNTER_TYPE_HARD,
					COUNTER_UNIT_NANO,
					0xffff,
					1,
					1,
					0),
	GEN_COUNTER(	COUNTER_ID_soft_1,
					"soft_1",
					COUNTER_TYPE_SOFT,
					COUNTER_UNIT_NANO,
					65535,
					1,
					1,
					0),
	GEN_COUNTER(	COUNTER_ID_soft_2,
					"soft_2",
					COUNTER_TYPE_SOFT,
					COUNTER_UNIT_NANO,
					65535,
					1,
					1,
					0),
};

CounterType Os_Arc_OsTickCounter = COUNTER_ID_OsTick;

// ##################################    ALARMS     ################################

GEN_ALARM_HEAD {
	GEN_ALARM(	ALARM_ID_c_soft_1_inc_counter_2,
				"c_soft_1_inc_cou",
				COUNTER_ID_soft_1,
				NULL,
				ALARM_ACTION_INCREMENTCOUNTER,
				NULL,
				NULL,
				COUNTER_ID_soft_2 ),
	GEN_ALARM(	ALARM_ID_c_soft_1_setevent_etask_m,
				"c_soft_1_seteven",
				COUNTER_ID_soft_1,
				NULL,
				ALARM_ACTION_SETEVENT,
				TASK_ID_etask_sup_m,
				EVENT_MASK_notif,
				NULL ),
	GEN_ALARM(	ALARM_ID_c_sys_1_setevent_etask_m,
				"c_sys_1_setevent",
				COUNTER_ID_OsTick,
				NULL,
				ALARM_ACTION_SETEVENT,
				TASK_ID_etask_sup_m,
				EVENT_MASK_notif,
				NULL ),
	GEN_ALARM(	ALARM_ID_c_sys_activate_btask_h,
				"c_sys_activate_b",
				COUNTER_ID_OsTick,
				NULL,
				ALARM_ACTION_ACTIVATETASK,
				TASK_ID_btask_sup_h,
				NULL,
				NULL ),
};

// ################################    RESOURCES     ###############################
GEN_RESOURCE_HEAD {
	GEN_RESOURCE(
		RES_ID_int_1,
		RESOURCE_TYPE_INTERNAL,
		0
	),
	GEN_RESOURCE(
		RES_ID_std_prio_3,
		RESOURCE_TYPE_STANDARD,
		0
	),
	GEN_RESOURCE(
		RES_ID_std_prio_4,
		RESOURCE_TYPE_STANDARD,
		0
	),
	GEN_RESOURCE(
		RES_ID_std_prio_5,
		RESOURCE_TYPE_STANDARD,
		0
	),
};

// ##############################    STACKS (TASKS)     ############################
DECLARE_STACK(OsIdle,OS_OSIDLE_STACK_SIZE);
DECLARE_STACK(btask_sup_h,2048);
DECLARE_STACK(btask_sup_l,2048);
DECLARE_STACK(btask_sup_m,2048);
DECLARE_STACK(etask_master,2048);
DECLARE_STACK(etask_sup_h,2048);
DECLARE_STACK(etask_sup_l,2048);
DECLARE_STACK(etask_sup_m,2048);

// ##################################    TASKS     #################################
GEN_TASK_HEAD {
	GEN_ETASK(	OsIdle,
				0,
				FULL,
				TRUE,
				NULL,
				0 
	),
	GEN_BTASK(
		btask_sup_h,
		6,
		FULL,
		FALSE,
		NULL,
		RES_MASK_std_prio_3 | RES_MASK_std_prio_4 | RES_MASK_std_prio_5 | 0,
		1
	),
				
	GEN_BTASK(
		btask_sup_l,
		2,
		FULL,
		FALSE,
		NULL,
		RES_MASK_std_prio_3 | RES_MASK_std_prio_4 | RES_MASK_std_prio_5 | 0,
		1
	),
				
	GEN_BTASK(
		btask_sup_m,
		4,
		FULL,
		FALSE,
		NULL,
		RES_MASK_std_prio_3 | RES_MASK_std_prio_4 | RES_MASK_std_prio_5 | 0,
		1
	),
				
	GEN_ETASK(
		etask_master,
		1,
		FULL,
		TRUE,
		NULL,
		0
	),
		
				
	GEN_ETASK(
		etask_sup_h,
		6,
		FULL,
		FALSE,
		NULL,
		RES_MASK_std_prio_3 | RES_MASK_std_prio_4 | RES_MASK_std_prio_5 | 0
	),
		
				
	GEN_ETASK(
		etask_sup_l,
		2,
		FULL,
		FALSE,
		NULL,
		RES_MASK_std_prio_3 | RES_MASK_std_prio_4 | RES_MASK_std_prio_5 | 0
	),
		
				
	GEN_ETASK(
		etask_sup_m,
		4,
		FULL,
		FALSE,
		NULL,
		RES_MASK_std_prio_3 | RES_MASK_std_prio_4 | RES_MASK_std_prio_5 | 0
	),
		
				
};

// ##################################    HOOKS     #################################
GEN_HOOKS( 
	StartupHook, 
	NULL, 
	ShutdownHook, 
	ErrorHook,
	PreTaskHook, 
	PostTaskHook 
);

// ##################################    ISRS     ##################################


// ############################    SCHEDULE TABLES     #############################

// Table data 0

GEN_SCHTBL_TASK_LIST_HEAD( 0, 5 ) { 
	
	TASK_ID_etask_sup_m,
	
};




GEN_SCHTBL_EVENT_LIST_HEAD( 0, 7 ) {
	
	{ 
		EVENT_MASK_notif, 
		TASK_ID_etask_sup_m 
	},
	
};


GEN_SCHTBL_TASK_LIST_HEAD( 0, 11 ) { 
	
	TASK_ID_etask_sup_m,
	
};

GEN_SCHTBL_EVENT_LIST_HEAD( 0, 11 ) {
	
	{ 
		EVENT_MASK_notif, 
		TASK_ID_etask_sup_m 
	},
	
};


GEN_SCHTBL_EXPIRY_POINT_HEAD( 0 ) {
	GEN_SCHTBL_EXPIRY_POINT_W_TASK(0, 5),
	GEN_SCHTBL_EXPIRY_POINT_W_EVENT(0, 7),
	GEN_SCHTBL_EXPIRY_POINT_W_TASK_EVENT(0, 11),
	
};

GEN_SCHTBL_AUTOSTART(
	0,
	SCHTBL_AUTOSTART_ABSOLUTE,
	1, 
	OSDEFAULTAPPMODE
);

// Table data 1

GEN_SCHTBL_TASK_LIST_HEAD( 1, 2 ) { 
	
	TASK_ID_etask_sup_m,
	
};



GEN_SCHTBL_EXPIRY_POINT_HEAD( 1 ) {
	GEN_SCHTBL_EXPIRY_POINT_W_TASK(1, 2),
	
};


// Table heads
GEN_SCHTBL_HEAD {
	GEN_SCHEDULETABLE(
		0,
		"0",
	    COUNTER_ID_soft_1,
	    SINGLE_SHOT,
		15,
		GEN_SCHTBL_AUTOSTART_NAME(0)
	),
	GEN_SCHEDULETABLE(
		1,
		"1",
	    COUNTER_ID_soft_1,
	    SINGLE_SHOT,
		5,
		NULL
	),
};

GEN_PCB_LIST()

uint8_t os_interrupt_stack[OS_INTERRUPT_STACK_SIZE] __attribute__ ((aligned (0x10)));

GEN_IRQ_VECTOR_TABLE_HEAD {};
GEN_IRQ_ISR_TYPE_TABLE_HEAD {};
GEN_IRQ_PRIORITY_TABLE_HEAD {};

#include "os_config_funcs.h"
