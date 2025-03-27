/**
 * \copyright Copyright (C) AGtek 2020 - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * proprietary and confidential. Modification of this file and the content
 * herein in any form is also strictly prohibited unless written consent has
 * been given.
 * \file irr_app_basin.c
 * \author Brahim Ben Sedrine
 * \version 1.1
 * \date 2 april 2024
 *
 * Source file containing UART functions driver that can be used.
 *
 */


#include "main.h"
#include "cmsis_os.h"
#include "irr_app_basin.h"
#include "cmd.h"
#include "irrigation_net.h"
#include "irr_app_electrical_pumps.h"
#include "irrigation_net_sap.h"
#include "irrigation_app.h"
#include "ds1302.h"
#include "irr_app_log.h"


/**
 *\enum T_eIrrAppBasin_iWaterLevelIndicatorStatus
 *\brief Enumeration containing the Water Level Indicator.
 */
typedef enum {
	C_eIrrAppBasin_iWaterLevelIndicatorOpen,   			/* Water Level Indicator is Open: Open is the most stable state */
	C_eIrrAppBasin_iWaterLevelIndicatorClose,   		/* Water Level Indicator is Close, otherwise (default state) */
}T_eIrrAppBasin_iWaterLevelIndicatorStatus;


static T_eIrrAppBasin_iWaterLevelIndicatorStatus gs_eIrrAppBasin_iLowIndicatorStatus = C_eIrrAppBasin_iWaterLevelIndicatorClose;
static T_eIrrAppBasin_iWaterLevelIndicatorStatus gs_eIrrAppBasin_iHighIndicatorStatus = C_eIrrAppBasin_iWaterLevelIndicatorClose;
static T_eIrrAppBasin_eLastBasinStatus gs_eIrrAppBasin_iLastBasinStatus = C_eIrrAppBasin_eFullState; /* By default the last state is full: to prevent the automatic power-on of motors at startup */


static uint32_t gs_uiIrrAppBasin_iWaterLevelSamplingTimeinMs = 5000;

/**
 * T_eError_eErrorType eIrrAppBasin_eWaterLevelSamplingTime_Set (uint32_t in_uiWaterLevelSamplingTimeInSeconds)
 * See irr_app_basin.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppBasin_eWaterLevelSamplingTime_Set (uint32_t in_uiWaterLevelSamplingTimeInSeconds)
{
	if(in_uiWaterLevelSamplingTimeInSeconds == 0) {
		in_uiWaterLevelSamplingTimeInSeconds = 1;
	}

	gs_uiIrrAppBasin_iWaterLevelSamplingTimeinMs = 1000 * in_uiWaterLevelSamplingTimeInSeconds;

	return Success;
}


/**
 * \fn T_eError_eErrorType eIrrAppBasin_eBasinStatus_Set(T_eIrrigationApp_eBasinStatus in_eBasinStatus)
 * \brief see basin.h for more details on this function.
 */
T_eError_eErrorType eIrrAppBasin_iBasinStatus_Set(T_eIrrAppBasin_iExactBasinStatus in_eExactBasinStatus) {

	static T_eIrrAppBasin_iExactBasinStatus ls_eExactBasinStatus = C_eIrrAppBasin_iTotalFullState; /* variable used for logging purposes only */

	if(in_eExactBasinStatus == C_eIrrAppBasin_iTotalFullState) {
		gs_eIrrAppBasin_iLastBasinStatus = C_eIrrAppBasin_eFullState;
	}
	else { /* totally empty or mid empty in night time */
		gs_eIrrAppBasin_iLastBasinStatus = C_eIrrAppBasin_eEmptyState;
	}


	/* Logging: */
	if(in_eExactBasinStatus != ls_eExactBasinStatus) { /* Log event: basin status change */
		if(in_eExactBasinStatus == C_eIrrAppBasin_iTotalFullState) {
			eIrrAppLog_eAddEvent_Cmd (1, C_eIrrAppLog_eBasinFull);
		}
		else if(in_eExactBasinStatus == C_eIrrAppBasin_iTotalEmptyState) {
			eIrrAppLog_eAddEvent_Cmd (1, C_eIrrAppLog_eBasinEmpty);
		}
		else if(in_eExactBasinStatus == C_eIrrAppBasin_iMidEmptyStateInNightTime) {
			eIrrAppLog_eAddEvent_Cmd (1, C_eIrrAppLog_eBasinNonFullInNightTime);
		}
		else {
			/* nothing to do, empty else */
		}
	}

	ls_eExactBasinStatus = in_eExactBasinStatus;


	return Success;
}


/**
 * \fn T_eError_eErrorType eIrrAppBasin_eBasinStatus_Set(T_eIrrigationApp_eBasinStatus in_eBasinStatus)
 * \brief see basin.h for more details on this function.
 */
T_eError_eErrorType eIrrAppBasin_eBasinStatus_Get(T_eIrrAppBasin_eLastBasinStatus *out_peLastBasinStatus) {

	*out_peLastBasinStatus = gs_eIrrAppBasin_iLastBasinStatus;

	return Success;
}


/**
 * \fn T_eError_eErrorType eIrrAppBasin_iBasinGlobalStatus_Get(T_eIrrAppBasin_iBasinGlobalStatus *out_peBasinGlobalStatus)
 * \brief see basin.h for more details on this function.
 */
T_eError_eErrorType eIrrAppBasin_iWaterLevelIndicatorsStatus_Set(void) {

	uint32_t l_uiLowLevelOpenCnt;
	uint32_t l_uiLowLevelCloseCnt;
	uint32_t l_uiHighLevelOpenCnt;
	uint32_t l_uiHighLevelCloseCnt;
	uint32_t l_uiCnt;
	uint32_t l_uiLoopSize;

	l_uiLowLevelOpenCnt = 0;
	l_uiLowLevelCloseCnt = 0;
	l_uiHighLevelOpenCnt = 0;
	l_uiHighLevelCloseCnt = 0;
	l_uiLoopSize = gs_uiIrrAppBasin_iWaterLevelSamplingTimeinMs / 10;



	for(l_uiCnt=0; l_uiCnt<l_uiLoopSize; l_uiCnt++) {
		/* Low Level Indicator */
		if(HAL_GPIO_ReadPin(D_uiGpio_eWaterLowLevelOpen_GPIO_Port, D_uiGpio_eWaterLowLevelOpen_Pin) == 0) {
			l_uiLowLevelOpenCnt++;
		}
		if(HAL_GPIO_ReadPin(D_uiGpio_eWaterLowLevelClose_GPIO_Port, D_uiGpio_eWaterLowLevelClose_Pin) == 0) {
			l_uiLowLevelCloseCnt++;
		}
		/* High Level Indicator */
		if(HAL_GPIO_ReadPin(D_uiGpio_eWaterHighLevelOpen_GPIO_Port, D_uiGpio_eWaterHighLevelOpen_Pin) == 0) {
			l_uiHighLevelOpenCnt++;
		}
		if(HAL_GPIO_ReadPin(D_uiGpio_eWaterHighLevelClose_GPIO_Port, D_uiGpio_eWaterHighLevelClose_Pin) == 0) {
			l_uiHighLevelCloseCnt++;
		}
		/*  */
		osDelay(5);
	}

	D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Low Indicator Status: Open Rate = %u / %u", (unsigned int)l_uiLowLevelOpenCnt, (unsigned int)l_uiLoopSize);
	D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Low Indicator Status: Close Rate = %u / %u", (unsigned int)l_uiLowLevelCloseCnt, (unsigned int)l_uiLoopSize);

	/* Low Level Indicator Status */
	if( (l_uiLowLevelOpenCnt == l_uiLoopSize) && (l_uiLowLevelCloseCnt == 0) ) {
		gs_eIrrAppBasin_iLowIndicatorStatus = C_eIrrAppBasin_iWaterLevelIndicatorOpen;
		D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Low Indicator is Open\r\n");
	}
	else {
		gs_eIrrAppBasin_iLowIndicatorStatus = C_eIrrAppBasin_iWaterLevelIndicatorClose;
		D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Low Indicator is Close\r\n");
	}

	D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "High Indicator Status: Open Rate = %u / %u", (unsigned int)l_uiHighLevelOpenCnt, (unsigned int)l_uiLoopSize);
	D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "High Indicator Status: Close Rate = %u / %u", (unsigned int)l_uiHighLevelCloseCnt, (unsigned int)l_uiLoopSize);

	/* High Level Indicator Status */
	if( (l_uiHighLevelOpenCnt == l_uiLoopSize) && (l_uiHighLevelCloseCnt == 0) ) {
		gs_eIrrAppBasin_iHighIndicatorStatus = C_eIrrAppBasin_iWaterLevelIndicatorOpen;
		D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "High Indicator is Open\r\n");
	}
	else {
		gs_eIrrAppBasin_iHighIndicatorStatus = C_eIrrAppBasin_iWaterLevelIndicatorClose;
		D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "High Indicator is Close\r\n");
	}

	return Success;
}



/**
 * \fn T_eError_eErrorType eIrrAppBasin_iBasinGlobalStatus_Get(T_eIrrAppBasin_iBasinGlobalStatus *out_peBasinGlobalStatus)
 * \brief see basin.h for more details on this function.
 */
T_eError_eErrorType eIrrAppBasin_iLastBasinStatus_Set(void) {

	T_eError_eErrorType l_eReturnResult;
	static uint32_t ls_uiConsecutiveOpenCnt = 0;
	static uint32_t ls_uiConsecutiveCloseCnt = 0;
	static uint32_t ls_uiHighIndConsecutiveOpenInNightCnt = 0; /* used in night time to fill basin if the high indicator is open */
	uint8_t l_ucHour;

	if( (gs_eIrrAppBasin_iHighIndicatorStatus == C_eIrrAppBasin_iWaterLevelIndicatorOpen) && (gs_eIrrAppBasin_iLowIndicatorStatus == C_eIrrAppBasin_iWaterLevelIndicatorOpen) ) {
		ls_uiConsecutiveOpenCnt = ls_uiConsecutiveOpenCnt + 1;
	}
	else {
		ls_uiConsecutiveOpenCnt = 0;
	}

	if( gs_eIrrAppBasin_iHighIndicatorStatus == C_eIrrAppBasin_iWaterLevelIndicatorClose ) {
		ls_uiConsecutiveCloseCnt = ls_uiConsecutiveCloseCnt + 1;
	}
	else {
		ls_uiConsecutiveCloseCnt = 0;
	}

	/* used in night time to fill basin if the high indicator is open */
	if( gs_eIrrAppBasin_iHighIndicatorStatus == C_eIrrAppBasin_iWaterLevelIndicatorOpen ) {
		l_eReturnResult = eDs1302_eGetHour_Cmd(&l_ucHour);
		if( (l_eReturnResult == Success) && ( (l_ucHour > 22) || (l_ucHour < 5) ) ) { /* it is night time ==> Fill Basin */
			ls_uiHighIndConsecutiveOpenInNightCnt = ls_uiHighIndConsecutiveOpenInNightCnt + 1;
		}
		else {
			ls_uiHighIndConsecutiveOpenInNightCnt = 0;
		}
	}
	else {
		ls_uiHighIndConsecutiveOpenInNightCnt = 0;
	}

	if( ls_uiConsecutiveCloseCnt > 2 ) {
		/* Set the New state (Total Full) of the Basin */
		eIrrAppBasin_iBasinStatus_Set(C_eIrrAppBasin_iTotalFullState);
		HAL_GPIO_WritePin(D_uiGpio_eACticityLed_GPIO_Port,D_uiGpio_eActivityLed_Pin, RESET);
	}
	else if( ls_uiConsecutiveOpenCnt > 2 ) {
		/* Set the New state (Total Empty) of the Basin */
		eIrrAppBasin_iBasinStatus_Set(C_eIrrAppBasin_iTotalEmptyState);
		HAL_GPIO_WritePin(D_uiGpio_eACticityLed_GPIO_Port,D_uiGpio_eActivityLed_Pin, SET);
	}
	else if( ls_uiHighIndConsecutiveOpenInNightCnt > 10 ) {
		/* Set the New state (Mid Empty in night time) of the Basin */
		eIrrAppBasin_iBasinStatus_Set(C_eIrrAppBasin_iMidEmptyStateInNightTime);
	}
	else {
		/* Nothing to do: keep the last state of the Basin unchanged */
	}

	/* Display Last Basin Status */
	if(gs_eIrrAppBasin_iLastBasinStatus == C_eIrrAppBasin_eFullState) {
		D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Last Basin State = Full\r\n\r\n");
	}
	else if(gs_eIrrAppBasin_iLastBasinStatus == C_eIrrAppBasin_eEmptyState) {
		D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Last Basin State = Empty\r\n\r\n");
	}
	else {
		D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Last Basin State = Unknown\r\n\r\n");
	}

	return Success;
}


/**
 * \fn T_eError_eErrorType eIrrAppBasin_eUpdateBasinStatus_Cmd(void)
 * \brief see basin.h for more details on this function.
 */
T_eError_eErrorType eIrrAppBasin_eUpdateLastBasinStatus_Cmd(void) {

	/* Set the Water Level Indicators Status */
	eIrrAppBasin_iWaterLevelIndicatorsStatus_Set();

	/* Set the Last Basin status according to the consecutive Water Level Indicators Status */
	eIrrAppBasin_iLastBasinStatus_Set();

	return Success;
}


/**
 * \fn T_eError_eErrorType eBasin_eBasinBehavior_Tsk(void)
 * \brief see basin.h for more details on this function.
 */
T_eError_eErrorType eIrrAppBasin_eBasinBehavior_Tsk(void) {


	while(1) {

		osDelay(3000);

		/* Update Last Basin Status */
		eIrrAppBasin_eUpdateLastBasinStatus_Cmd();

	}

	return Success;
}


/**
 * T_eError_eErrorType eIrrAppBasin_eSendCommand_Cnf (T_eIrrigationNet_eCmdType , uint32_t , T_eIrrigationNet_eNodeStatus )
 * See basin.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppBasin_eSendCommand_Cnf (uint8_t in_ucMasterRequest, uint32_t in_uiTargetDeviceId, uint8_t in_ucSlaveResponse)
{

//	if( (gs_eBasin_iMotorStatus == C_eMotor_iMotorOn) && (in_eNodeStatus == C_eIrrigationNet_iOffStatus) ) {
//		gs_uiBasin_iIsCmdRequested = 1;
//	}
//	else if( (gs_eBasin_iMotorStatus == C_eMotor_iMotorOff) && (in_eNodeStatus == C_eIrrigationNet_iOnStatus) ) {
//		gs_uiBasin_iIsCmdRequested = 1;
//	}

	return Success;
}
