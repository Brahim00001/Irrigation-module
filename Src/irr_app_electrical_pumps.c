/**
 * \copyright Copyright (C) AGtek 2020 - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * proprietary and confidential. Modification of this file and the content
 * herein in any form is also strictly prohibited unless written consent has
 * been given.
 * \file irr_app_electrical_pumps.c
 * \author Brahim Ben Sedrine
 * \version 1.1
 * \date 2 april 2024
 *
 * Source file containing UART functions driver that can be used.
 *
 */

#include "main.h"
#include "cmsis_os.h"
#include "irr_app_electrical_pumps.h"
#include "cmd.h"
#include "irrigation_net.h"
#include "irrigation_app.h"



#define D_uiIrrAppElectricalPumps_iTriggerOrder		100

#define D_uiIrrAppElectricalPumps_iMaxRunTimeInMs	43200000	// 12 Hour


static T_eIrrAppElectricalPumps_eElectricalPumpsStatus gs_eIrrAppElectricalPumps_iElectricalPumpsStatus;

static uint32_t gs_uiIrrAppElectricalPumps_iStartRunTimeInMs = 0;

static uint32_t gs_uiIrrAppElectricalPumps_iLastConnectionTime = 0;


/**
 * \fn T_eError_eErrorType eIrrAppElectricalPumps_eElectricalPumpsStatus_Get(T_eIrrAppElectricalPumps_eElectricalPumpsStatus *out_peElectricalPumpsStatus)
 * \brief see irr_app_electrical_pumps.h for more details on this function.
 */
T_eError_eErrorType eIrrAppElectricalPumps_eElectricalPumpsStatus_Get(T_eIrrAppElectricalPumps_eElectricalPumpsStatus *out_peElectricalPumpsStatus) {

	*out_peElectricalPumpsStatus = gs_eIrrAppElectricalPumps_iElectricalPumpsStatus;

	return Success;
}


/**
 * \fn T_eError_eErrorType eIrrAppElectricalPumps_eElectricalPumpsStatus_Get(T_eIrrAppElectricalPumps_eElectricalPumpsStatus *out_peElectricalPumpsStatus)
 * \brief see irr_app_electrical_pumps.h for more details on this function.
 */
T_eError_eErrorType eIrrAppElectricalPumps_iElectricalPumpsStatus_Set(void) {

	uint32_t l_uiCnt;
	uint32_t l_uiOnStateCnt;
	uint32_t l_uiOffStateCnt;

	l_uiOnStateCnt = 0;
	l_uiOffStateCnt = 0;
	for(l_uiCnt=0; l_uiCnt<D_uiIrrAppElectricalPumps_iTriggerOrder; l_uiCnt++) {
		if( (HAL_GPIO_ReadPin(D_uiGpio_eElectricalPump1Ctrl_GPIO_Port, D_uiGpio_eElectricalPump1Ctrl_Pin) == 0) && (HAL_GPIO_ReadPin(D_uiGpio_eElectricalPump2Ctrl_GPIO_Port, D_uiGpio_eElectricalPump2Ctrl_Pin) == 0) ) {
//		if (HAL_GPIO_ReadPin(D_uiGpio_eElectricalPump3Ctrl_GPIO_Port, D_uiGpio_eElectricalPump3Ctrl_Pin) == 0) {
			l_uiOffStateCnt = l_uiOffStateCnt + 1;
				}
//		if(HAL_GPIO_ReadPin(D_uiGpio_eElectricalPump3Ctrl_GPIO_Port, D_uiGpio_eElectricalPump3Ctrl_Pin) == 1){
		if( (HAL_GPIO_ReadPin(D_uiGpio_eElectricalPump1Ctrl_GPIO_Port, D_uiGpio_eElectricalPump1Ctrl_Pin) == 1) && (HAL_GPIO_ReadPin(D_uiGpio_eElectricalPump2Ctrl_GPIO_Port, D_uiGpio_eElectricalPump2Ctrl_Pin) == 1) ) {
			l_uiOnStateCnt = l_uiOnStateCnt + 1;
		}
		osDelay(5);
	}

	if( (l_uiOnStateCnt == D_uiIrrAppElectricalPumps_iTriggerOrder) && (l_uiOffStateCnt == 0) ) {
		gs_eIrrAppElectricalPumps_iElectricalPumpsStatus = C_eIrrAppElectricalPumps_eElectricalPumpsOn;
		HAL_GPIO_WritePin(D_uiGpio_eACticityLed_GPIO_Port,D_uiGpio_eActivityLed_Pin, SET);
	}
	else if( (l_uiOnStateCnt == 0) && (l_uiOffStateCnt == D_uiIrrAppElectricalPumps_iTriggerOrder) ) {
		gs_eIrrAppElectricalPumps_iElectricalPumpsStatus = C_eIrrAppElectricalPumps_eElectricalPumpsOff;
		HAL_GPIO_WritePin(D_uiGpio_eACticityLed_GPIO_Port,D_uiGpio_eActivityLed_Pin, RESET);

	}
	else {
		gs_eIrrAppElectricalPumps_iElectricalPumpsStatus = C_eIrrAppElectricalPumps_eUnknownElectricalPumpsStatus;
	}

	return Success;
}


/**
 * \fn T_eError_eErrorType eIrrAppElectricalPumps_eElectricalPumpsStatus_Set(T_eIrrAppElectricalPumps_eElectricalPumpsStatus in_eElectricalPumpsStatus)
 * \brief see irr_app_electrical_pumps.h for more details on this function.
 */
T_eError_eErrorType eIrrAppElectricalPumps_iSetElectricalPumpsStatus_Cmd(T_eIrrAppElectricalPumps_eElectricalPumpsStatus in_eElectricalPumpsStatus) {

	/*  */
	if(in_eElectricalPumpsStatus == C_eIrrAppElectricalPumps_eElectricalPumpsOn) {
		HAL_GPIO_WritePin(D_uiGpio_eElectricalPump1Ctrl_GPIO_Port, D_uiGpio_eElectricalPump1Ctrl_Pin, 1);
		HAL_GPIO_WritePin(D_uiGpio_eElectricalPump2Ctrl_GPIO_Port, D_uiGpio_eElectricalPump2Ctrl_Pin, 1);
//		HAL_GPIO_WritePin(D_uiGpio_eElectricalPump3Ctrl_GPIO_Port, D_uiGpio_eElectricalPump3Ctrl_Pin, 1);
		gs_uiIrrAppElectricalPumps_iStartRunTimeInMs = osKernelSysTick();
		gs_eIrrAppElectricalPumps_iElectricalPumpsStatus = C_eIrrAppElectricalPumps_eElectricalPumpsOn;
	}
	else { /* Power Off Electrical Pumps for any other state */
		HAL_GPIO_WritePin(D_uiGpio_eElectricalPump1Ctrl_GPIO_Port, D_uiGpio_eElectricalPump1Ctrl_Pin, 0);
		HAL_GPIO_WritePin(D_uiGpio_eElectricalPump2Ctrl_GPIO_Port, D_uiGpio_eElectricalPump2Ctrl_Pin, 0);
//		HAL_GPIO_WritePin(D_uiGpio_eElectricalPump3Ctrl_GPIO_Port, D_uiGpio_eElectricalPump3Ctrl_Pin, 0);
		gs_eIrrAppElectricalPumps_iElectricalPumpsStatus = C_eIrrAppElectricalPumps_eElectricalPumpsOff;
	}

	return Success;
}




/**
 * \fn T_eError_eErrorType eIrrAppElectricalPumps_eElectricalPumpsBehavior_Tsk(void)
 * \brief see basin.h for more details on this function.
 */
T_eError_eErrorType eIrrAppElectricalPumps_eElectricalPumpsBehavior_Tsk(void) {

	while(1) {

		osDelay(1000);

		/* Set Electrical Pumps Status */
		eIrrAppElectricalPumps_iElectricalPumpsStatus_Set();
		/*check high pressure*/
/*		if (HAL_GPIO_ReadPin(D_uiGpio_ePresostat_GPIO_Port, D_uiGpio_ePresostat_Pin) == 0)  {
			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "High Pressure!! Close Electrical Pumps");
			eIrrAppElectricalPumps_iSetElectricalPumpsStatus_Cmd(C_eIrrAppElectricalPumps_eElectricalPumpsOff);
			HAL_GPIO_WritePin(D_uiGpio_eRedLed_GPIO_Port,D_uiGpio_eRedLed_Pin,1);
			HAL_GPIO_WritePin(D_uiGpio_eGreenLed_GPIO_Port,D_uiGpio_eGreenLed_Pin,0);
			gs_eIrrAppElectricalPumps_iElectricalPumpsStatus = C_eIrrAppElectricalPumps_eElectricalPumpsOff;
		}
		else{
			HAL_GPIO_WritePin(D_uiGpio_eRedLed_GPIO_Port,D_uiGpio_eRedLed_Pin,0);
			HAL_GPIO_WritePin(D_uiGpio_eGreenLed_GPIO_Port,D_uiGpio_eGreenLed_Pin,1);
			D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Optimum pressure...")
		}*/
		/* Check a Connection Loss */
		if( (gs_uiIrrAppElectricalPumps_iLastConnectionTime + D_uiIrrigationNet_iRoundTimeInMs + 60000) < osKernelSysTick()) {
			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Close Electrical Pumps after Connection Loss");
			eIrrAppElectricalPumps_iSetElectricalPumpsStatus_Cmd(C_eIrrAppElectricalPumps_eElectricalPumpsOff);
		}

		/* Check Max Run Time*/
		if( (gs_uiIrrAppElectricalPumps_iStartRunTimeInMs + D_uiIrrAppElectricalPumps_iMaxRunTimeInMs) < osKernelSysTick()) {
			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Close Electrical Pumps after Max Run Time");
			eIrrAppElectricalPumps_iSetElectricalPumpsStatus_Cmd(C_eIrrAppElectricalPumps_eElectricalPumpsOff);
		}
		/*Reset Max Run TIme after 24 hours*/
		if ((osKernelSysTick() - gs_uiIrrAppElectricalPumps_iStartRunTimeInMs) >  (2 * D_uiIrrAppElectricalPumps_iMaxRunTimeInMs))
		gs_uiIrrAppElectricalPumps_iStartRunTimeInMs = 0;
	}

	return Success;
}



/**
 * T_eError_eErrorType eIrrAppElectricalPumps_eCommandReceived_Cmd (T_eIrrigationNet_eCmdType in_eCmdType)
 * See motor.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppElectricalPumps_eCommandReceived_Cmd (uint8_t in_ucMasterRequest) {

	gs_uiIrrAppElectricalPumps_iLastConnectionTime = osKernelSysTick();

	if(in_ucMasterRequest == C_eIrrigationApp_eSetElectricalPumpsOn) {
		if (gs_eIrrAppElectricalPumps_iElectricalPumpsStatus == C_eIrrAppElectricalPumps_eElectricalPumpsOff) {
		eIrrAppElectricalPumps_iSetElectricalPumpsStatus_Cmd(C_eIrrAppElectricalPumps_eElectricalPumpsOn);
		}
		else {

		}
	}
	else if(in_ucMasterRequest == C_eIrrigationApp_eSetElectricalPumpsOff) {
		eIrrAppElectricalPumps_iSetElectricalPumpsStatus_Cmd(C_eIrrAppElectricalPumps_eElectricalPumpsOff);
		gs_uiIrrAppElectricalPumps_iStartRunTimeInMs = 0;
	}
	else {
		/* empty else: nothing to do */
	}

	return Success;
}

