
#include "string.h"

#include "main.h"
#include "cmsis_os.h"
#include "irr_app_log.h"
#include "irr_app_flash.h"
#include "cmd.h"
#include "uart.h"
#include "ds1302.h"



static T_stIrrAppLog_eLogWord gs_pstIrrAppLog_iLogInRam[128];
static uint32_t gs_uiIrrAppLog_iNbrLogsInRam = 0;

static T_eIrrAppLog_eLoggingStatus gs_eIrrAppLog_iLoggingStatus = C_eIrrAppLog_eLoggingDisabled;

static char gs_pcIrrAppLog_iStr[128];

/**
 * T_eError_eErrorType eIrrAppLog_eInit_Cmd (void)
 * See irr_app_log.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppLog_eInit_Cmd (void)
{
	uint32_t l_uiLoggingStatus;

	eIrrAppFlash_eLoggingStatus_Get (&l_uiLoggingStatus);
	if(l_uiLoggingStatus == C_eIrrAppLog_eLoggingEnabled) {
		eIrrAppLog_eLoggingStatus_Set (C_eIrrAppLog_eLoggingEnabled);
	}
	else {
		eIrrAppLog_eLoggingStatus_Set (C_eIrrAppLog_eLoggingDisabled);
	}

	gs_uiIrrAppLog_iNbrLogsInRam = 0;
	memset((uint8_t*)gs_pstIrrAppLog_iLogInRam, 0xFF, 128*sizeof(T_stIrrAppLog_eLogWord));

	return Success;
}


/**
 * T_eError_eErrorType eIrrAppLog_eLoggingStatus_Set (T_eIrrAppLog_eLoggingStatus in_eLoggingStatus)
 * See irr_app_log.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppLog_eLoggingStatus_Set (T_eIrrAppLog_eLoggingStatus in_eLoggingStatus)
{
	gs_eIrrAppLog_iLoggingStatus = in_eLoggingStatus;

	return Success;
}


/**
 * T_eError_eErrorType eIrrAppLog_eLoggingStatus_Get (T_eIrrAppLog_eLoggingStatus *out_peLoggingStatus)
 * See irr_app_log.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppLog_eLoggingStatus_Get (T_eIrrAppLog_eLoggingStatus *out_peLoggingStatus)
{
	*out_peLoggingStatus = gs_eIrrAppLog_iLoggingStatus;

	return Success;
}


/**
 * T_eError_eErrorType eIrrAppLog_eAddEvent_Cmd (void)
 * See irr_app_log.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppLog_eAddEvent_Cmd (uint8_t in_ucDeviceId, T_eIrrAppLog_eEvent in_eLogEvent)
{
	T_eError_eErrorType l_eReturnResult;
	T_stIrrAppLog_eLogWord l_ePreviousWord;
	T_stIrrAppLog_eLogWord l_eReadWord;
	T_stIrrAppLog_eLogWord l_eWriteWord;
	uint32_t l_uiReadAddress;
	uint32_t l_uiWriteAddress;
	//
	uint8_t l_ucSeconds;
	uint8_t l_ucMinutes;
	uint8_t l_ucHours;
	uint8_t l_ucDay;
	uint8_t l_ucMonth;
	uint16_t l_usYear;
	//
	T_eIrrAppLog_eLoggingStatus l_eLoggingStatus;

	eIrrAppLog_eLoggingStatus_Get (&l_eLoggingStatus);
	if(l_eLoggingStatus == C_eIrrAppLog_eLoggingDisabled) {
		return Failure_NotInitialized;
	}


	eDs1302_eGetTime_Cmd(&l_usYear, &l_ucMonth, &l_ucDay, &l_ucHours, &l_ucMinutes, &l_ucSeconds);
	if ( (l_usYear < 2020) || (l_usYear > 2099) || (l_ucMonth < 1) || (l_ucMonth > 12) ||
		 (l_ucDay < 1) || (l_ucDay>31) || (l_ucHours < 0) || (l_ucHours > 23) || (l_ucMinutes > 59) || (l_ucSeconds > 59) ) {
		return Failure_NotAllowed;
	}

	l_eWriteWord.m_uiYear = l_usYear-2020;
	l_eWriteWord.m_uiMonth = l_ucMonth;
	l_eWriteWord.m_uiDay = l_ucDay;
	l_eWriteWord.m_uiHour = l_ucHours;
	l_eWriteWord.m_uiMinute = l_ucMinutes;
	//
	l_eWriteWord.m_uiDeviceID = in_ucDeviceId;
	l_eWriteWord.m_uiLogEvent = in_eLogEvent;

	(*((uint32_t*)(&l_ePreviousWord))) = 0xFFFFFFFF;

	l_uiWriteAddress = 0;

	for(l_uiReadAddress=D_uiIrrAppFlash_eLogFlashAddress; l_uiReadAddress<(D_uiIrrAppFlash_eLogFlashAddress+FLASH_BANK_SIZE); l_uiReadAddress=l_uiReadAddress+sizeof(T_stIrrAppLog_eLogWord)) {
		l_eReturnResult = eIrrAppFlash_eReadLogFromFlash_Cmd ((uint8_t*)(&l_eReadWord), sizeof(T_stIrrAppLog_eLogWord), l_uiReadAddress);
		if(l_eReturnResult == Success) {
			if( (*((uint32_t*)(&l_eReadWord))) == 0xFFFFFFFF) { /* empty case was found */
				l_uiWriteAddress = l_uiReadAddress;
				break;
			}
			(*((uint32_t*)(&l_ePreviousWord))) = (*((uint32_t*)(&l_eReadWord)));
		}
	}

	if(l_uiWriteAddress < D_uiIrrAppFlash_eLogFlashAddress) {
		return Failure_Memory;
	}

	if( memcmp((uint8_t*)(&l_eWriteWord), (uint8_t*)(&l_ePreviousWord), sizeof(T_stIrrAppLog_eLogWord)) == 0 ) { /* Don't log the same event more than once per minute */
		return Failure_Unsupported;
	}

	if( (l_eWriteWord.m_uiDeviceID == 1) && ((in_eLogEvent == C_eIrrAppLog_eHardReset) || (in_eLogEvent == C_eIrrAppLog_eSoftReset) || (in_eLogEvent == C_eIrrAppLog_eIwdgReset)) ) { /* Log Reset Immediately in Flash */
		if( (l_uiWriteAddress+sizeof(T_stIrrAppLog_eLogWord)) >= (D_uiIrrAppFlash_eLogFlashAddress+FLASH_BANK_SIZE) ) {
			return Failure_FullBuffers;
		}
		return eIrrAppFlash_eWriteLogInFlash_Cmd((uint8_t*)(&l_eWriteWord), sizeof(T_stIrrAppLog_eLogWord), l_uiWriteAddress);
	}
	else { /* Log Event In RAM */
		 memcpy((uint8_t*)(&(gs_pstIrrAppLog_iLogInRam[gs_uiIrrAppLog_iNbrLogsInRam])), (uint8_t*)(&l_eWriteWord), sizeof(T_stIrrAppLog_eLogWord));
		 gs_uiIrrAppLog_iNbrLogsInRam = gs_uiIrrAppLog_iNbrLogsInRam + 1;
		 if(gs_uiIrrAppLog_iNbrLogsInRam == 128) {
			 /* move Log from RAM to Flash */
			 return eIrrAppLog_eMoveLogToFlash_Cmd(0);
		 }
	}

	return Success;
}


static uint32_t gs_uiIrrAppLog_iFlashLogTime = 0;


T_eError_eErrorType eIrrAppLog_eMoveLogToFlash_Cmd (uint32_t in_uiForceFlash)
{
	T_eError_eErrorType l_eReturnResult;
	T_stIrrAppLog_eLogWord l_eReadWord;
	uint32_t l_uiReadAddress;
	uint32_t l_uiWriteAddress;
	//
	uint8_t l_ucSeconds;
	uint8_t l_ucMinutes;
	uint8_t l_ucHours;
	uint8_t l_ucDay;
	uint8_t l_ucMonth;
	uint16_t l_usYear;
	//
	T_eIrrAppLog_eLoggingStatus l_eLoggingStatus;

	eIrrAppLog_eLoggingStatus_Get (&l_eLoggingStatus);
	if(l_eLoggingStatus == C_eIrrAppLog_eLoggingDisabled) {
		return Failure_NotInitialized;
	}

	if(gs_uiIrrAppLog_iNbrLogsInRam == 0) {
		return Failure_NotAllowed;
	}

	if(in_uiForceFlash == 0) {
		if( (gs_uiIrrAppLog_iNbrLogsInRam < 128) && (osKernelSysTick() < (gs_uiIrrAppLog_iFlashLogTime+7200000)) ) { /* enable logging only once every 2 hours */
			return Failure_InsufficientTime;
		}
	}

	eDs1302_eGetTime_Cmd(&l_usYear, &l_ucMonth, &l_ucDay, &l_ucHours, &l_ucMinutes, &l_ucSeconds);
	if ( (l_usYear < 2020) || (l_usYear > 2099) || (l_ucMonth < 1) || (l_ucMonth > 12) ||
		 (l_ucDay < 1) || (l_ucDay>31) || (l_ucHours < 0) || (l_ucHours > 23) || (l_ucMinutes > 59) || (l_ucSeconds > 59) ) {
		return Failure_NotAllowed;
	}

	l_uiWriteAddress = 0;

	for(l_uiReadAddress=D_uiIrrAppFlash_eLogFlashAddress; l_uiReadAddress<(D_uiIrrAppFlash_eLogFlashAddress+FLASH_BANK_SIZE); l_uiReadAddress=l_uiReadAddress+sizeof(T_stIrrAppLog_eLogWord)) {
		l_eReturnResult = eIrrAppFlash_eReadLogFromFlash_Cmd ((uint8_t*)(&l_eReadWord), sizeof(T_stIrrAppLog_eLogWord), l_uiReadAddress);
		if(l_eReturnResult == Success) {
			if( (*((uint32_t*)(&l_eReadWord))) == 0xFFFFFFFF) { /* empty case was found */
				l_uiWriteAddress = l_uiReadAddress;
				break;
			}
		}
	}

	if(l_uiWriteAddress < D_uiIrrAppFlash_eLogFlashAddress) {
		return Failure_Memory;
	}

	if( (l_uiWriteAddress+gs_uiIrrAppLog_iNbrLogsInRam*sizeof(T_stIrrAppLog_eLogWord)) >= (D_uiIrrAppFlash_eLogFlashAddress+FLASH_BANK_SIZE) ) {
		return Failure_FullBuffers;
	}

	l_eReturnResult = eIrrAppFlash_eWriteLogInFlash_Cmd((uint8_t*)gs_pstIrrAppLog_iLogInRam, gs_uiIrrAppLog_iNbrLogsInRam*sizeof(T_stIrrAppLog_eLogWord), l_uiWriteAddress);

	if(l_eReturnResult == Success) {
		gs_uiIrrAppLog_iFlashLogTime = osKernelSysTick();
	}

	gs_uiIrrAppLog_iNbrLogsInRam = 0;
	memset((uint8_t*)gs_pstIrrAppLog_iLogInRam, 0xFF, 128*sizeof(T_stIrrAppLog_eLogWord));

	return Success;
}



/**
 * T_eError_eErrorType eIrrAppLog_eReadLog_Get(UART_HandleTypeDef* in_pstUartHandle)
 * See irr_app_log.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppLog_eReadLog_Get(UART_HandleTypeDef* in_pstUartHandle)
{
	T_eError_eErrorType l_eReturnResult;
	T_stIrrAppLog_eLogWord l_eReadWord;
	uint32_t l_uiReadAddress;

	eUart_eSend_Cmd(in_pstUartHandle, "Log:\r\n");

	for(l_uiReadAddress=D_uiIrrAppFlash_eLogFlashAddress; l_uiReadAddress<(D_uiIrrAppFlash_eLogFlashAddress+FLASH_BANK_SIZE); l_uiReadAddress=l_uiReadAddress+sizeof(T_stIrrAppLog_eLogWord)) {
		l_eReturnResult = eIrrAppFlash_eReadLogFromFlash_Cmd ((uint8_t*)(&l_eReadWord), sizeof(T_stIrrAppLog_eLogWord), l_uiReadAddress);
		if(l_eReturnResult == Success) {
			if( (*((uint32_t*)(&l_eReadWord))) != 0xFFFFFFFF) {
				switch(l_eReadWord.m_uiLogEvent) {
				case C_eIrrAppLog_eHardReset:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Hard Reset\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				case C_eIrrAppLog_eSoftReset:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Soft Reset\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				case C_eIrrAppLog_eIwdgReset:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = IWDG Reset\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				case C_eIrrAppLog_eBasinFull:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Basin Becomes Full\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				case C_eIrrAppLog_eBasinEmpty:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Basin Becomes Empty\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				case C_eIrrAppLog_eBasinNonFullInNightTime:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Basin Becomes Empty In Night Time\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				case C_eIrrAppLog_eSlaveSetsElectricalPumpOn:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Set Electrical Pump ON\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				case C_eIrrAppLog_eSlaveSetsElectricalPumpOff:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Set Electrical Pump OFF\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				case C_eIrrAppLog_eSlaveDisconnected:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = NOT Connected\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				case C_eIrrAppLog_eTooMuchTimeToProcessCmd:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Too Much Time To Process Cmd\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				case C_eIrrAppLog_eSlaveIsNotOk:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Slave is NOT Ok\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				case C_eIrrAppLog_eSlaveSetsSolenoidValveOn:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Set Solenoid Valve ON\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				case C_eIrrAppLog_eSlaveSetsSolenoidValveOff:
					sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Set Solenoid Valve OFF\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
					break;
				default:
					break;
				}
				eUart_eSend_Cmd(in_pstUartHandle, gs_pcIrrAppLog_iStr);
			}
		}
		else {
			break;
		}
	}


	return Success;
}





/**
 * T_eError_eErrorType eIrrAppLog_eReadLog_Get(UART_HandleTypeDef* in_pstUartHandle)
 * See irr_app_log.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppLog_eReadLogInRam_Get(UART_HandleTypeDef* in_pstUartHandle)
{
	T_stIrrAppLog_eLogWord l_eReadWord;
	uint32_t l_uiReadAddress;

	eUart_eSend_Cmd(in_pstUartHandle, "Log in RAM:\r\n");

	for(l_uiReadAddress=0; l_uiReadAddress<gs_uiIrrAppLog_iNbrLogsInRam; l_uiReadAddress++) {
		(*((uint32_t*)(&l_eReadWord))) = (*((uint32_t*)(&(gs_pstIrrAppLog_iLogInRam[l_uiReadAddress]))));
		if( (*((uint32_t*)(&l_eReadWord))) != 0xFFFFFFFF) {
			switch(l_eReadWord.m_uiLogEvent) {
			case C_eIrrAppLog_eHardReset:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Hard Reset\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			case C_eIrrAppLog_eSoftReset:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Soft Reset\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			case C_eIrrAppLog_eIwdgReset:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = IWDG Reset\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			case C_eIrrAppLog_eBasinFull:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Basin Becomes Full\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			case C_eIrrAppLog_eBasinEmpty:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Basin Becomes Empty\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			case C_eIrrAppLog_eBasinNonFullInNightTime:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Basin Becomes Empty In Night Time\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			case C_eIrrAppLog_eSlaveSetsElectricalPumpOn:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Set Electrical Pump ON\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			case C_eIrrAppLog_eSlaveSetsElectricalPumpOff:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Set Electrical Pump OFF\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			case C_eIrrAppLog_eSlaveDisconnected:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = NOT Connected\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			case C_eIrrAppLog_eTooMuchTimeToProcessCmd:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Too Much Time To Process Cmd\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			case C_eIrrAppLog_eSlaveIsNotOk:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Slave is NOT Ok\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			case C_eIrrAppLog_eSlaveSetsSolenoidValveOn:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Set SetsSolenoid Valve ON\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			case C_eIrrAppLog_eSlaveSetsSolenoidValveOff:
				sprintf(gs_pcIrrAppLog_iStr, "%u-%u-%u %u:%u  ID = %u   Event = Set SetsSolenoid Valve OFF\r\n", l_eReadWord.m_uiYear+2020, l_eReadWord.m_uiMonth, l_eReadWord.m_uiDay, l_eReadWord.m_uiHour, l_eReadWord.m_uiMinute, l_eReadWord.m_uiDeviceID);
				break;
			default:
				break;
			}
			eUart_eSend_Cmd(in_pstUartHandle, gs_pcIrrAppLog_iStr);
		}
	}


	return Success;
}
