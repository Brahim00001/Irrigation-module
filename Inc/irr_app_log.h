

#ifndef _MCU_APPLICATIONS_IRRIGATION_IRR_APP_LOG_H_
#define _MCU_APPLICATIONS_IRRIGATION_IRR_APP_LOG_H_



#include "error.h"


/**
 *\enum T_eIrrAppLog_eLoggingStatus
 *\brief Enumeration containg the logging status
 */
typedef enum {
	C_eIrrAppLog_eLoggingDisabled = 0,     	/* Logging is disabled */
	C_eIrrAppLog_eLoggingEnabled,      		/* Logging is enabled */
}T_eIrrAppLog_eLoggingStatus;


/**
 *\enum T_eIrrAppLog_eEvent
 *\brief Enumeration containg the type of events that can occurs
 */
typedef enum {
	/* Reset Events */
	C_eIrrAppLog_eHardReset = 0,       			/* Hardware Reset Event */
	C_eIrrAppLog_eSoftReset,       				/* Software Reset Event */
	C_eIrrAppLog_eIwdgReset,       				/* IWDG Reset Event */
	/* Basin States Events */
	C_eIrrAppLog_eBasinFull,       				/* Basin becomes full */
	C_eIrrAppLog_eBasinEmpty,       			/* Basin becomes empty */
	C_eIrrAppLog_eBasinNonFullInNightTime,    	/* Basin becomes non full in night time */
	/* Electrical Pumps States Events */
	C_eIrrAppLog_eSlaveSetsElectricalPumpOn,   	/* Slave Sets Electrical Pump On */
	C_eIrrAppLog_eSlaveSetsElectricalPumpOff,  	/* Slave Sets Electrical Pump Off */
	/* Error Events */
	C_eIrrAppLog_eSlaveDisconnected,       		/* Slave Disconnected */
	C_eIrrAppLog_eTooMuchTimeToProcessCmd,      /* CMD takes too much time in progress state at master (> 5 minutes) */
	C_eIrrAppLog_eSlaveIsNotOk,      			/* Slave is Not Ok */
	/*Solenoid Valves States Events */
	C_eIrrAppLog_eSlaveSetsSolenoidValveOn,		/*Slave Sets Solenoid Valve On */
	C_eIrrAppLog_eSlaveSetsSolenoidValveOff,	/*Slave Sets Solenoid Valve Off */
}T_eIrrAppLog_eEvent;




/**
 *\struct T_stIrrAppLog_eLogWord
 *\brief structure containing the word to log for each occuring event.
 */
typedef struct __attribute__((__packed__)) {
	uint32_t m_uiLogEvent:4;	/* Log Event */
	//
	uint32_t m_uiDeviceID:5;
	//
	uint32_t m_uiYear:3; /* Year = 2020 + m_uiYear */
	uint32_t m_uiMonth:4; /* 1 to 12 */
	uint32_t m_uiDay:5; /* 1 to 31 */
	//
	uint32_t m_uiHour:5; /* 0 to 23 */
	uint32_t m_uiMinute:6; /* 0 to 59 */
}T_stIrrAppLog_eLogWord; /* 32 bits */



T_eError_eErrorType eIrrAppLog_eInit_Cmd (void);

T_eError_eErrorType eIrrAppLog_eLoggingStatus_Set (T_eIrrAppLog_eLoggingStatus in_eLoggingStatus);

T_eError_eErrorType eIrrAppLog_eLoggingStatus_Get (T_eIrrAppLog_eLoggingStatus *out_peLoggingStatus);

T_eError_eErrorType eIrrAppLog_eAddEvent_Cmd (uint8_t in_ucDeviceId, T_eIrrAppLog_eEvent in_eLogEvent);

T_eError_eErrorType eIrrAppLog_eMoveLogToFlash_Cmd (uint32_t in_uiForceFlash);

T_eError_eErrorType eIrrAppLog_eReadLog_Get(UART_HandleTypeDef* in_pstUartHandle);

T_eError_eErrorType eIrrAppLog_eReadLogInRam_Get(UART_HandleTypeDef* in_pstUartHandle);



#endif /* _MCU_APPLICATIONS_IRRIGATION_IRR_APP_LOG_H_ */
