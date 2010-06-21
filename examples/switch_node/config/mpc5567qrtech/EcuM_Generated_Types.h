/* 
* Configuration of module EcuM (EcuM_Generated_Types.h)
* 
* Created by: ArcCore AB
* Configured for (MCU): MPC5567
* 
* Module editor vendor:  ArcCore
* Module editor version: 2.0.0
* 
* Copyright ArcCore AB 2010
* Generated by Arctic Studio (http://arccore.com)
*           on Fri Apr 30 15:43:38 CEST 2010
*/


#if (ECUM_SW_MAJOR_VERSION != 1) 
#error "EcuM: Configuration file version differs from BSW version."
#endif


#ifndef _ECUM_GENERATED_TYPES_H_
#define _ECUM_GENERATED_TYPES_H_

#if defined(USE_MCU)
#include "Mcu.h"
#endif
#if defined(USE_PORT)
#include "Port.h"
#endif
#if defined(USE_CAN)
#include "Can.h"
#endif
#if defined(USE_CANIF)
#include "CanIf.h"
#endif
#if defined(USE_PWM)
#include "Pwm.h"
#endif
#if defined(USE_COM)
#include "Com.h"
#endif
#if defined(USE_PDUR)
#include "PduR.h"
#endif
#if defined(USE_DMA)
#include "Dma.h"
#endif
#if defined(USE_ADC)
#include "Adc.h"
#endif
#if defined(USE_GPT)
#include "Gpt.h"
#endif


typedef struct
{
	EcuM_StateType EcuMDefaultShutdownTarget;
	uint8 EcuMDefaultShutdownMode;
	AppModeType EcuMDefaultAppMode;

#if defined(USE_MCU)
	const Mcu_ConfigType* McuConfig;
#endif
#if defined(USE_PORT)
	const Port_ConfigType* PortConfig;
#endif
#if defined(USE_CAN)
	const Can_ConfigType* CanConfig;
#endif
#if defined(USE_CANIF)
	const CanIf_ConfigType* CanIfConfig;
#endif
#if defined(USE_COM)
	const Com_ConfigType* ComConfig;
#endif
#if defined(USE_PDUR)
	const PduR_PBConfigType* PduRConfig;
#endif
#if defined(USE_PWM)
	const Pwm_ConfigType* PwmConfig;
#endif
#if defined(USE_DMA)
	const Dma_ConfigType* DmaConfig;
#endif
#if defined(USE_ADC)
    const Adc_ConfigType* AdcConfig;
#endif
#if defined(USE_GPT)
    const Gpt_ConfigType* GptConfig;
#endif
} EcuM_ConfigType;

#endif /*_ECUM_GENERATED_TYPES_H_*/