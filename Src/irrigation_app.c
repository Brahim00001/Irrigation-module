/**
 * \copyright Copyright (C) AGtek 2020 - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * proprietary and confidential. Modification of this file and the content
 * herein in any form is also strictly prohibited unless written consent has
 * been given.
 * \file irrigation_app.c
 * \author Brahim Ben Sedrine
 * \version 1.1
 * \date 2 april 2024
 *
 * Source file containing UART functions driver that can be used.
 *
 */





#include "main.h"
#include "cmsis_os.h"
#include "cmd.h"
#include "irrigation_app.h"
#include "irr_net_parameters.h"
#include "irr_net_flash.h"
#include "irr_app_gateway.h"
#include "irr_app_basin.h"
#include "irr_app_electrical_pumps.h"
#include "irr_app_solenoid_valves.h"




/**
 * \fn T_eError_eErrorType eBasin_eMainTask_Tsk(void)
 * \brief see basin.h for more details on this function.
 */
T_eError_eErrorType eIrrigationApp_eMainTask_Tsk(void) {

	T_eError_eErrorType l_eReturnValue;
	T_eParameters_eDeviceType l_eDeviceType;
	uint32_t l_uiDeviceId;
	uint32_t l_uiPairingRequest;

	osDelay(300);

	do {
		/* Get Device Type */
		l_eReturnValue = eIrrNetFlash_eDeviceId_Get(&l_uiDeviceId);
		if(l_eReturnValue != Success) {
			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irrigation_app.c Line %d: Device Id is not yet set", __LINE__);
			osDelay(10000);
		}
	}while(l_eReturnValue != Success);

	do {
		/* Get Device Type */
		l_eReturnValue = eIrrNetFlash_eDeviceType_Get(&l_eDeviceType);
		if(l_eReturnValue != Success) {
			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irrigation_app.c Line %d: Device Type is not yet set", __LINE__);
			osDelay(1000);
		}
	}while(l_eReturnValue != Success);

	do {
		l_eReturnValue = eIrrNetFlash_ePairingRequest_Get(&l_uiPairingRequest);
		if(l_eReturnValue != Success) {
			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "in irrigation_net.c line %d: cannot read pairing request from flash", __LINE__);
		}
		if(l_uiPairingRequest == 1) {
			osDelay(1000);
		}
	}while(l_uiPairingRequest == 1);

	switch(l_eDeviceType) {

	case C_eIrrNetParameters_eGatewayNode:
		eIrrAppGateway_eGatewayBehavior_Tsk();
		break;

	case C_eIrrNetParameters_eBasinGatewayNode:
		eIrrAppGateway_eGatewayBehavior_Tsk();
		break;

	case C_eIrrNetParameters_eBasinSimpleNode:
		eIrrAppBasin_eBasinBehavior_Tsk();
		break;

	case C_eIrrNetParameters_eElectricalPumpNode:
		eIrrAppElectricalPumps_eElectricalPumpsBehavior_Tsk();
		break;

	case C_eIrrNetParameters_eSolenoidValveNode:
		eIrrAppSolenoidValves_eSolenoidValvesBehavior_Tsk();
		break;

	default:
		D_vCmd_eDebugPrint(C_eCmd_eDebugError, "in irrigation_app.c Line %d: FATAL ERROR: Device Type is not supported", __LINE__);
		break;
	}

	return Success;
}
