/**
 * \copyright Copyright (C) AGtek 2020 - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * proprietary and confidential. Modification of this file and the content
 * herein in any form is also strictly prohibited unless written consent has
 * been given.
 * \file irr_app_solenoid_valves.c
 * \author Brahim Ben Sedrine
 * \version 1.1
 * \date 2 april 2024
 *
 * Source file containing UART functions driver that can be used.
 *
 */

#include "main.h"
#include "cmsis_os.h"
#include "irr_app_solenoid_valves.h"
#include "cmd.h"
#include "irrigation_net.h"
#include "irrigation_app.h"


#define D_uiIrrAppSolenoidValves_iTriggerOrder		3
#define D_uiIrrAppSolenoidValves_iMaxRunTimeInMs	3600000	// 1 Hour


static T_eIrrAppSolenoidValves_eSolenoidValvesStatus gs_eIrrAppSolenoidValves_iSolenoidValvesStatus = C_eIrrAppSolenoidValves_iSolenoidValvesOn;

static uint32_t gs_uiIrrAppSolenoidValves_iStartRunTimeInMs = 0;
static uint32_t gs_uiIrrAppSolenoidValves_iConnectionStatus = 0;

/**
 * \fn T_eError_eErrorType eIrrAppSolenoidValves_eSolenoidStatus_Get(T_eSolenoid_eSolenoidValvesStatus *out_peSolenoidValvesStatus)
 * \brief see irr_app_solenoid_valves.h for more details on this function.
 */
T_eError_eErrorType eIrrAppSolenoidValves_eSolenoidStatus_Get(T_eIrrAppSolenoidValves_eSolenoidValvesStatus *out_peSolenoidValvesStatus) {

	*out_peSolenoidValvesStatus = gs_eIrrAppSolenoidValves_iSolenoidValvesStatus;

	return Success;
}



/**
 * \fn T_eError_eErrorType eIrrAppSolenoidValves_eSolenoidStatus_Set(T_eIrrAppSolenoidValves_eSolenoidValvesStatus in_eSolenoidValvesStatus)
 * \brief see irr_app_solenoid_valves.h for more details on this function.
 */
T_eError_eErrorType eIrrAppSolenoidValves_eSolenoidValvesStatus_Cmd(T_eIrrAppSolenoidValves_eSolenoidValvesStatus in_eSolenoidValvesStatus) {


	/*  */
	if(in_eSolenoidValvesStatus == C_eIrrAppSolenoidValves_iSolenoidValvesOff) {
		HAL_GPIO_WritePin(D_uiGpio_eElectricalPump3Ctrl_GPIO_Port, D_uiGpio_eElectricalPump3Ctrl_Pin, 0);
		HAL_GPIO_WritePin(D_uiGpio_eSolenoidRelay1_GPIO_Port, D_uiGpio_eSolenoidRelay1_Pin, 1);
		osDelay(10);
		HAL_GPIO_WritePin(D_uiGpio_eSolenoidRelay1_GPIO_Port, D_uiGpio_eSolenoidRelay1_Pin, 0);
		gs_uiIrrAppSolenoidValves_iStartRunTimeInMs = osKernelSysTick();
		gs_eIrrAppSolenoidValves_iSolenoidValvesStatus = in_eSolenoidValvesStatus;

	}
	else if(in_eSolenoidValvesStatus == C_eIrrAppSolenoidValves_iSolenoidValvesOn) {
		HAL_GPIO_WritePin(D_uiGpio_eElectricalPump3Ctrl_GPIO_Port, D_uiGpio_eElectricalPump3Ctrl_Pin, 1);
		HAL_GPIO_WritePin(D_uiGpio_eSolenoidRelay2_GPIO_Port, D_uiGpio_eSolenoidRelay2_Pin, 1);
		osDelay(10);
		HAL_GPIO_WritePin(D_uiGpio_eSolenoidRelay2_GPIO_Port, D_uiGpio_eSolenoidRelay2_Pin, 0);
		gs_eIrrAppSolenoidValves_iSolenoidValvesStatus = in_eSolenoidValvesStatus;

	}
	else {
		/* Nothing to do */
	}

	return Success;
}


static uint32_t gs_uiIrrAppSolenoidValves_iLastConnectionTime = 0;


/**
 * \fn T_eError_eErrorType eIrrAppSolenoidValves_eSolenoidValvesBehavior_Tsk(void)
 * \brief see irr_app_solenoid_valves.h for more details on this function.
 */
T_eError_eErrorType eIrrAppSolenoidValves_eSolenoidValvesBehavior_Tsk(void) {

	while(1) {
		osDelay(1000);


		/* Check a Connection Loss */
		if( (gs_uiIrrAppSolenoidValves_iLastConnectionTime + D_uiIrrigationNet_iRoundTimeInMs + 60000) < osKernelSysTick()) {
			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Close Solenoid after Connection Loss");
			eIrrAppSolenoidValves_eSolenoidValvesStatus_Cmd(C_eIrrAppSolenoidValves_iSolenoidValvesOff);
		}

		/* Check Max Run Time
		if( (gs_uiIrrAppSolenoidValves_iStartRunTimeInMs + D_uiIrrAppSolenoidValves_iMaxRunTimeInMs) < osKernelSysTick()) {
			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Close Solenoid after Max Run Time");
			eIrrAppSolenoidValves_eSolenoidValvesStatus_Cmd(C_eIrrAppSolenoidValves_iSolenoidValvesOff);
		}*/
	}

	return Success;
}



/**
 * T_eError_eErrorType eIrrAppSolenoidValves_eCommandReceived_Cmd (T_eIrrigationNet_eCmdType in_eCmdType)
 * See irr_app_solenoid_valves.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppSolenoidValves_eCommandReceived_Cmd (uint8_t in_ucMasterRequest) {

	gs_uiIrrAppSolenoidValves_iLastConnectionTime = osKernelSysTick();
	if(in_ucMasterRequest == C_eIrrigationApp_eSetIrrigationSolenoidValveOn) {
		eIrrAppSolenoidValves_eSolenoidValvesStatus_Cmd(C_eIrrAppSolenoidValves_iSolenoidValvesOn);
	}
	else if(in_ucMasterRequest == C_eIrrigationApp_eSetIrrigationSolenoidValveOff) {
		eIrrAppSolenoidValves_eSolenoidValvesStatus_Cmd(C_eIrrAppSolenoidValves_iSolenoidValvesOff);
	}
	else {
		/* empty else: nothing to do */
	}
	return Success;
}

