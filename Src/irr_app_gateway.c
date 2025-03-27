
/**
 * \copyright Copyright (C) AGtek 2020 - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * proprietary and confidential. Modification of this file and the content
 * herein in any form is also strictly prohibited unless written consent has
 * been given.
 * \file irr_app_gateway.c
 * \author Brahim Ben Sedrine
 * \version 1.1
 * \date 15 april 2024
 *
 * Source file containing UART functions driver that can be used.
 *
 */
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "cmsis_os.h"
#include "irr_app_gateway.h"
#include "cmd.h"
#include "irrigation_net_sap.h"
#include "irrigation_app.h"
#include "irr_net_flash.h"
#include "irr_net_master.h"
#include "irr_app_basin.h"
#include "reset.h"
#include "irr_app_log.h"


#define D_uiIrrAppGateway_iBroadcastPacketId	(0x1F)







static T_stIrrNetPairig_iPairedSlaveInfos gs_pstIrrAppGateway_iPairedSlavesInfos[D_uiIrrigationNet_iMaxNbrSlavesPerNetwork];

static T_stIrrAppGateway_eSlavesInfos gs_pstIrrAppGateway_iSlavesInfos[D_uiIrrigationNet_iMaxNbrSlavesPerNetwork] = {0};

static T_stIrrigationApp_eSlaveResponse gs_pstIrrAppGateway_iSlavesResponses[D_uiIrrigationNet_iMaxNbrSlavesPerNetwork] = {0}; /* save last slaves responses: used only for logging purposes */

static uint32_t gs_uiIrrAppGateway_iIdleTimeinMs = 30000;

 uint32_t gs_uiBasinAbsentRounds = 0;

/**
 * T_eError_eErrorType eIrrAppGateway_eIdleTime_Set (uint32_t in_uiIdleTimeInSeconds)
 * See irrigation_net_sap.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppGateway_eIdleTime_Set (uint32_t in_uiIdleTimeInSeconds)
{
	gs_uiIrrAppGateway_iIdleTimeinMs = 1000 * in_uiIdleTimeInSeconds;

	return Success;
}


/**
 * T_eError_eErrorType eIrrigationNetSap_eSendGetStatusCommand_Req (uint32_t in_uiTargetDeviceId)
 * See irrigation_net_sap.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppGateway_eElectricalPumpsIdsBitMap_Get (uint32_t *out_puiElectricalPumpsIdsBitMap)
{
	T_eError_eErrorType l_eReturnValue;
	uint32_t l_uiNbrDevicesInNetwork;
	uint32_t l_uiCnt;
	uint32_t l_uiElectricalPumpId;
	uint32_t l_uiElectricalPumpsIdsBitMap;

	eIrrNetParameters_eNbrDevicesInNetwork_Get(&l_uiNbrDevicesInNetwork);

	l_eReturnValue = eIrrNetFlash_ePairedSlavesInfos_Get(gs_pstIrrAppGateway_iPairedSlavesInfos);
	if( l_eReturnValue != Success ) {
		return l_eReturnValue;
	}

	l_uiElectricalPumpsIdsBitMap = 0;
	for(l_uiCnt=0; l_uiCnt<(l_uiNbrDevicesInNetwork-1); l_uiCnt++) { /* Don't include Gateway Device */
		if(gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_eSlaveType == C_eIrrNetParameters_eElectricalPumpNode) {
			l_uiElectricalPumpId = gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_uiSlaveId;
			l_uiElectricalPumpsIdsBitMap = l_uiElectricalPumpsIdsBitMap | (1 << (l_uiElectricalPumpId-2));
		}
	}

	*out_puiElectricalPumpsIdsBitMap = l_uiElectricalPumpsIdsBitMap;

	return Success;
}

/**
* T_eError_eErrorType eIrrigationNetSap_eSendGetStatusCommand_Req (uint32_t in_uiTargetDeviceId)
* See irrigation_net_sap.h for details of how to use this function.
*/
T_eError_eErrorType eIrrAppGateway_eSolenoidValvesIdsBitMap_Get (uint32_t *out_puiSolenoidValvesIdsBitMap)
{
	T_eError_eErrorType l_eReturnValue;
	uint32_t l_uiNbrDevicesInNetwork;
	uint32_t l_uiCnt;
	uint32_t l_uiSolenoidValveId;
	uint32_t l_uiSolenoidValvesIdsBitMap;

	eIrrNetParameters_eNbrDevicesInNetwork_Get(&l_uiNbrDevicesInNetwork);

	l_eReturnValue = eIrrNetFlash_ePairedSlavesInfos_Get(gs_pstIrrAppGateway_iPairedSlavesInfos);
	if( l_eReturnValue != Success ) {
		return l_eReturnValue;
	}

	l_uiSolenoidValvesIdsBitMap = 0;
	for(l_uiCnt=0; l_uiCnt<(l_uiNbrDevicesInNetwork-1); l_uiCnt++) { /* Don't include Gateway Device */
		if(gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_eSlaveType == C_eIrrNetParameters_eSolenoidValveNode) {
			l_uiSolenoidValveId = gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_uiSlaveId;
			l_uiSolenoidValvesIdsBitMap = l_uiSolenoidValvesIdsBitMap | (1 << (l_uiSolenoidValveId-2));
		}
	}

	*out_puiSolenoidValvesIdsBitMap = l_uiSolenoidValvesIdsBitMap;

	return Success;
}

/**
 * \fn T_eError_eErrorType eBasin_eBasinBehavior_Tsk(void)
 * \brief see basin.h for more details on this function.
 */
T_eError_eErrorType eIrrAppGateway_iProcessSlavesStatus_Cmd(T_eParameters_eDeviceType in_eDeviceType, T_stIrrAppGateway_eRequestedCommand *out_pstRequestedCommand) {

	uint32_t l_uiNbrDevicesInNetwork;
	uint32_t l_uiCnt;
	T_eIrrAppBasin_eLastBasinStatus l_eBasinLastStatus;
	uint8_t l_ucSlaveResponse;
	uint8_t l_ucElectricalPumpStatus;
	uint32_t l_uiIsBasinFound;
	uint32_t l_uiElectricalPumpsIdsBitMap;


	/* Return Get status command by default */
	out_pstRequestedCommand->m_eRequestType = C_eIrrigationApp_eGetSlaveStatus;
	out_pstRequestedCommand->m_uiTargetIdsBitMap = D_uiIrrAppGateway_iBroadcastPacketId;

	/* Process Slaves Status */
	eIrrNetParameters_eNbrDevicesInNetwork_Get(&l_uiNbrDevicesInNetwork);
	//
	/* Get Basin Status */
	if(in_eDeviceType == C_eIrrNetParameters_eBasinGatewayNode) {
		/* Update Basin's status */
		eIrrAppBasin_eUpdateLastBasinStatus_Cmd();
		/* Get Basin Status */
		eIrrAppBasin_eBasinStatus_Get(&l_eBasinLastStatus);
	}
	else {
		l_uiIsBasinFound = 0;
		for(l_uiCnt=0; l_uiCnt<(l_uiNbrDevicesInNetwork-1); l_uiCnt++) { /* Don't include Gateway Device */
			if(gs_pstIrrAppGateway_iSlavesInfos[l_uiCnt].m_eSlaveType == C_eIrrNetParameters_eBasinSimpleNode) {
				l_ucSlaveResponse = gs_pstIrrAppGateway_iSlavesInfos[l_uiCnt].m_ucSlaveResponse;
				l_eBasinLastStatus = ((T_stIrrigationApp_eSlaveResponse*)(&l_ucSlaveResponse))->m_eBasinSlaveLastStatus;
				l_uiIsBasinFound = 1;
				gs_uiBasinAbsentRounds = 0;
				break;
			}
		}
		//
		if(l_uiIsBasinFound == 0) {
			gs_uiBasinAbsentRounds++;
			return Failure_NotFound;
		}
	}
	//
	/* Check status of Electrical Pumps */
	for(l_uiCnt=0; l_uiCnt<(l_uiNbrDevicesInNetwork-1); l_uiCnt++) { /* Don't include Gateway Device */
		eIrrAppGateway_eElectricalPumpsIdsBitMap_Get (&l_uiElectricalPumpsIdsBitMap);
		out_pstRequestedCommand->m_uiTargetIdsBitMap = l_uiElectricalPumpsIdsBitMap;
		if(gs_pstIrrAppGateway_iSlavesInfos[l_uiCnt].m_eSlaveType == C_eIrrNetParameters_eElectricalPumpNode) {
			l_ucSlaveResponse = gs_pstIrrAppGateway_iSlavesInfos[l_uiCnt].m_ucSlaveResponse;
			l_ucElectricalPumpStatus = (((T_stIrrigationApp_eSlaveResponse*)(&l_ucSlaveResponse))->m_ucOnOffStatus) & 0x01;
			//
			if((HAL_GPIO_ReadPin(D_uiGpio_eOnOffSwitch_GPIO_Port, D_uiGpio_eOnOffSwitch_Pin) == 0) || (l_uiIsBasinFound == 0)){
				out_pstRequestedCommand->m_eRequestType = C_eIrrigationApp_eSetElectricalPumpsOff;
				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Closing all electrical pumps");
				return Success;
			}
			else{
				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Gateway switch... ON: System works normally.");
				if( (l_ucElectricalPumpStatus == 1) && (l_eBasinLastStatus == C_eIrrAppBasin_eFullState) ) {
					out_pstRequestedCommand->m_eRequestType = C_eIrrigationApp_eSetElectricalPumpsOff;
					return Success;
				}
				else if(  (l_eBasinLastStatus == C_eIrrAppBasin_eEmptyState) ) {
					out_pstRequestedCommand->m_eRequestType = C_eIrrigationApp_eSetElectricalPumpsOn;
					return Success;
				}
				else {
					out_pstRequestedCommand->m_eRequestType = C_eIrrigationApp_eSetElectricalPumpsOff;
					return Success;
				}
			}
			break;
		}
		}

	return Success;
}

/**
 * \fn T_eError_eErrorType eBasin_eBasinBehavior_Tsk(void)
 * \brief see basin.h for more details on this function.
 */
T_eError_eErrorType eIrrAppGateway_iProcessSlave2Status_Cmd(T_eParameters_eDeviceType in_eDeviceType, T_stIrrAppGateway_eRequestedCommand *out_pstRequestedCommand) {

	uint32_t l_uiNbrDevicesInNetwork;
	uint32_t l_uiCnt;
	T_eIrrAppBasin_eLastBasinStatus l_eBasinLastStatus;
	uint8_t l_ucSlaveResponse;
	uint8_t l_ucElectricalPumpStatus;
	uint32_t l_uiIsBasinFound;
	uint32_t l_uiElectricalPumpsIdsBitMap;
	uint32_t l_uiSolenoidValvesIdsBitMap;


	/* Return Get status command by default */
	out_pstRequestedCommand->m_eRequestType = C_eIrrigationApp_eGetSlaveStatus;
	out_pstRequestedCommand->m_uiTargetIdsBitMap = D_uiIrrAppGateway_iBroadcastPacketId;

	/* Process Slaves Status */
	eIrrNetParameters_eNbrDevicesInNetwork_Get(&l_uiNbrDevicesInNetwork);
	//
	/* Get Basin Status */
	if(in_eDeviceType == C_eIrrNetParameters_eBasinGatewayNode) {
		/* Update Basin's status */
		eIrrAppBasin_eUpdateLastBasinStatus_Cmd();
		/* Get Basin Status */
		eIrrAppBasin_eBasinStatus_Get(&l_eBasinLastStatus);
	}
	else {
		l_uiIsBasinFound = 0;
		for(l_uiCnt=0; l_uiCnt<(l_uiNbrDevicesInNetwork-1); l_uiCnt++) { /* Don't include Gateway Device */
			if(gs_pstIrrAppGateway_iSlavesInfos[l_uiCnt].m_eSlaveType == C_eIrrNetParameters_eBasinSimpleNode) {
				l_ucSlaveResponse = gs_pstIrrAppGateway_iSlavesInfos[l_uiCnt].m_ucSlaveResponse;
				l_eBasinLastStatus = ((T_stIrrigationApp_eSlaveResponse*)(&l_ucSlaveResponse))->m_eBasinSlaveLastStatus;
				l_uiIsBasinFound = 1;
				break;
			}
		}
		//
		if(l_uiIsBasinFound == 0) {
			return Failure_NotFound;
		}
	}
	//
	/* Check status of Solenoid Valves */
	for(l_uiCnt=0; l_uiCnt<(l_uiNbrDevicesInNetwork-1); l_uiCnt++) { /* Don't include Gateway Device */
		eIrrAppGateway_eElectricalPumpsIdsBitMap_Get (&l_uiElectricalPumpsIdsBitMap);
		eIrrAppGateway_eSolenoidValvesIdsBitMap_Get (&l_uiSolenoidValvesIdsBitMap);
		if (gs_pstIrrAppGateway_iSlavesInfos[l_uiCnt].m_eSlaveType == C_eIrrNetParameters_eSolenoidValveNode) {
			l_ucSlaveResponse = gs_pstIrrAppGateway_iSlavesInfos[l_uiCnt].m_ucSlaveResponse;
			l_ucElectricalPumpStatus = (((T_stIrrigationApp_eSlaveResponse*)(&l_ucSlaveResponse))->m_ucOnOffStatus) & 0x01;
			//
			if(HAL_GPIO_ReadPin(D_uiGpio_eOnOffSwitch_GPIO_Port, D_uiGpio_eOnOffSwitch_Pin) == 0){
				HAL_GPIO_WritePin(D_uiGpio_eGreenLed_GPIO_Port, D_uiGpio_eGreenLed_Pin, 0);
				HAL_GPIO_WritePin(D_uiGpio_eRedLed_GPIO_Port,D_uiGpio_eRedLed_Pin, 1);
				out_pstRequestedCommand->m_eRequestType = C_eIrrigationApp_eSetIrrigationSolenoidValveOff;
				out_pstRequestedCommand->m_uiTargetIdsBitMap = l_uiSolenoidValvesIdsBitMap;
				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Gateway switch... OFF. Closing all selenoid valves.");
				return Success;
			}
			else{
				HAL_GPIO_WritePin(D_uiGpio_eGreenLed_GPIO_Port, D_uiGpio_eGreenLed_Pin, 1);
				HAL_GPIO_WritePin(D_uiGpio_eRedLed_GPIO_Port,D_uiGpio_eRedLed_Pin, 0);
				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Gateway switch... ON: System works normally.");
				if( (l_ucElectricalPumpStatus == 0) && (l_eBasinLastStatus == C_eIrrAppBasin_eFullState) ) {
					out_pstRequestedCommand->m_eRequestType = C_eIrrigationApp_eSetIrrigationSolenoidValveOff;
					out_pstRequestedCommand->m_uiTargetIdsBitMap = l_uiSolenoidValvesIdsBitMap;
					return Success;
				}
				else if( (l_eBasinLastStatus == C_eIrrAppBasin_eEmptyState) ) {
					out_pstRequestedCommand->m_eRequestType = C_eIrrigationApp_eSetIrrigationSolenoidValveOn;
					out_pstRequestedCommand->m_uiTargetIdsBitMap = l_uiSolenoidValvesIdsBitMap;
					return Success;
				}
			}
		}
			break;
	}

	return Success;
}

static char gs_pcIrrAppGateway_iMasterCommand[128];

/**
 * \fn T_eError_eErrorType eBasin_eBasinBehavior_Tsk(void)
 * \brief see basin.h for more details on this function.
 */
T_eError_eErrorType eIrrAppGateway_eGatewayBehavior_Tsk(void) {

	T_eError_eErrorType l_eReturnValue;
	T_eParameters_eDeviceType l_eDeviceType;
//	T_eIrrigationNet_eCmdRequestStatus l_eCmdRequestStatus;
//	T_stIrrAppGateway_eRequestedCommand l_stRequestedCommand;
//	uint32_t l_uiWaitingSeconds;
//	uint32_t l_uiStartIdleTime;
	T_eReset_eType l_eResetType;
	T_eError_eErrorType l_eReturnValue1;
//	T_eParameters_eDeviceType l_eDeviceType1;
	T_eIrrigationNet_eCmdRequestStatus l_eCmdRequestStatus1;
	T_stIrrAppGateway_eRequestedCommand l_stRequestedCommand1;
	uint32_t l_uiWaitingSeconds1;
	uint32_t l_uiStartIdleTime1;
	bool exitLoop = false;


	/* Initialize Log Application */
	eIrrAppLog_eInit_Cmd ();


	/* Get Slave status is the starting Master Request Command */
//	l_stRequestedCommand.m_eRequestType = C_eIrrigationApp_eGetSlaveStatus;
//	l_stRequestedCommand.m_uiTargetIdsBitMap = D_uiIrrAppGateway_iBroadcastPacketId;
	l_stRequestedCommand1.m_eRequestType = C_eIrrigationApp_eGetSlaveStatus;
	l_stRequestedCommand1.m_uiTargetIdsBitMap = D_uiIrrAppGateway_iBroadcastPacketId;

	/* Logging: */
	/* Log Reset Event */
	eReset_eType_Get(&l_eResetType);
	if(l_eResetType == C_eReset_eSoftware) { /* Software Reset occurs at master */
		eIrrAppLog_eAddEvent_Cmd (1, C_eIrrAppLog_eSoftReset);
	}
	else if(l_eResetType == C_eReset_eWatchdog) { /* Watchdog Reset occurs at master */
		eIrrAppLog_eAddEvent_Cmd (1, C_eIrrAppLog_eIwdgReset);
	}
	else {
		eIrrAppLog_eAddEvent_Cmd (1, C_eIrrAppLog_eHardReset); /* Hardware Reset occurs at master */
	}


	while(1) {

		/* Check Device Type, if Changed then reset board */
		l_eReturnValue = eIrrNetFlash_eDeviceType_Get(&l_eDeviceType);
		if(l_eReturnValue != Success) {
			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irr_app_gateway.c Line %d: Device Type is not yet set", __LINE__);
			osDelay(1000);
		}
		if(HAL_GPIO_ReadPin(D_uiGpio_eOnOffSwitch_GPIO_Port, D_uiGpio_eOnOffSwitch_Pin) == 0){
			HAL_GPIO_WritePin(D_uiGpio_eGreenLed_GPIO_Port, D_uiGpio_eGreenLed_Pin, 0);
			HAL_GPIO_WritePin(D_uiGpio_eRedLed_GPIO_Port,D_uiGpio_eRedLed_Pin, 1);
			D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Commanding all nodes to close in this round...");
		}
		else{
			HAL_GPIO_WritePin(D_uiGpio_eGreenLed_GPIO_Port, D_uiGpio_eGreenLed_Pin, 1);
			HAL_GPIO_WritePin(D_uiGpio_eRedLed_GPIO_Port,D_uiGpio_eRedLed_Pin, 0);
			D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Commanding all nodes to work according to the water level in this round...");
		}
		/* TODO: Check the Device Type (Gateway only or Gateway Basin) according to the switch position */
//		if( (l_eDeviceType == C_eIrrNetParameters_eGatewayNode) && (BasinGatewaySwitch == ON) ) {
//			l_eReturnValue = eIrrNetFlash_eFlashDeviceType_Cmd(C_eIrrNetParameters_eBasinGatewayNode);
//			if(l_eReturnValue != Success) {
//				D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irr_app_gateway.c Line %d: Device Type cannot be flashed", __LINE__);
//			}
//			else {
//				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Change Device Type to Basin Gateway Node");
//			}
//			l_eReturnValue = eIrrNetFlash_eFlashDeviceId_Cmd(1);
//			if(l_eReturnValue != Success) {
//				D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irr_app_gateway.c Line %d: Device Id cannot be flashed", __LINE__);
//			}
//			else {
//				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Change Device ID to 1 for Basin Gateway Node");
//			}
//			/* Reset to update initializations */
//			osDelay(1000);
//			eIrrNetParameters_eSoftReset_Cmd();
//		}
//		if( (l_eDeviceType == C_eIrrNetParameters_eBasinGatewayNode) && (BasinGatewaySwitch == OFF) ) {
//			l_eReturnValue = eIrrNetFlash_eFlashDeviceType_Cmd(C_eIrrNetParameters_eGatewayNode);
//			if(l_eReturnValue != Success) {
//				D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irr_app_gateway.c Line %d: Device Type cannot be flashed", __LINE__);
//			}
//			else {
//				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Change Device Type to Gateway Node");
//			}
//			l_eReturnValue = eIrrNetFlash_eFlashDeviceId_Cmd(1);
//			if(l_eReturnValue != Success) {
//				D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irr_app_gateway.c Line %d: Device Id cannot be flashed", __LINE__);
//			}
//			else {
//				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Change Device ID to 1 for Gateway Node");
//			}
//			/* Reset to update initializations */
//			osDelay(1000);
//			eIrrNetParameters_eSoftReset_Cmd();
//		}





		/******
		 * The following commmented part is for the selenoid valve
		 *******/



		/* Initialize Slaves Infos */
//		memset(gs_pstIrrAppGateway_iSlavesInfos, 0, D_uiIrrigationNet_iMaxNbrSlavesPerNetwork*sizeof(T_stIrrAppGateway_eSlavesInfos));
//
//		eIrrNetParameters_eMasterCommandString_Get (l_stRequestedCommand.m_eRequestType, gs_pcIrrAppGateway_iMasterCommand);
//		D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "New Request: %s", gs_pcIrrAppGateway_iMasterCommand);
//
//		/* Send Get Status Command */
//		l_eReturnValue = eIrrigationNetSap_eSendCommand_Req (l_stRequestedCommand);
//		if(l_eReturnValue != Success) {
//			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Get Status Command is Rejected by Network");
//		}
//
//		/* Wait for the end of command processing */
//		l_uiWaitingSeconds = 0;
//		do {
//			eIrrNetMaster_eCmdRequestStatus_Get (&l_eCmdRequestStatus);
//			osDelay(100);
//			l_uiWaitingSeconds++;
//			if(l_uiWaitingSeconds > 300) { /* 5 minutes */
//				/* Log this event */
//				eIrrAppLog_eAddEvent_Cmd (1, C_eIrrAppLog_eTooMuchTimeToProcessCmd);
//				eIrrAppLog_eMoveLogToFlash_Cmd (1);
//				D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Timeout while waiting for the end of command processing ==> restart system");
//				osDelay(100);
//				eIrrNetParameters_eSoftReset_Cmd();
//			}
//		}while(l_eCmdRequestStatus == C_eIrrigationNet_iCmdReqInProgress);
//
//		D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "%s is processed with Success\r\n\r\n\r\n", gs_pcIrrAppGateway_iMasterCommand);
//
//		/* Process Slaves Responses */
//		eIrrAppGateway_iProcessSlave2Status_Cmd(l_eDeviceType, &l_stRequestedCommand);
//
//		/* Idle Time */
//		if(l_stRequestedCommand.m_eRequestType == C_eIrrigationApp_eGetSlaveStatus) {
//			if(l_eDeviceType == C_eIrrNetParameters_eBasinGatewayNode) {
//				l_uiStartIdleTime = osKernelSysTick();
//				do {
//					eIrrAppGateway_iProcessSlave2Status_Cmd(l_eDeviceType, &l_stRequestedCommand);
//					osDelay(100);
//				}while( ((osKernelSysTick() - l_uiStartIdleTime) < gs_uiIrrAppGateway_iIdleTimeinMs) && (l_stRequestedCommand.m_eRequestType == C_eIrrigationApp_eGetSlaveStatus) );
//			}
//			else {
//				osDelay(gs_uiIrrAppGateway_iIdleTimeinMs);
//			}
//		}
//		else {
//			osDelay(100);
//		}
//
//		/* Move Log from RAM to Flash if RAM is full or 2 hours are passed from last flashing */
//		eIrrAppLog_eMoveLogToFlash_Cmd (0);
//
//		osDelay(100);
//		l_eReturnValue1 = eIrrNetFlash_eDeviceType_Get(&l_eDeviceType1);
//		if(l_eReturnValue1 != Success) {
//			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irr_app_gateway.c Line %d: Device Type is not yet set", __LINE__);
//			osDelay(100);
//		}




		/*****
		 * End of the commented part
		 *****/




		/* Initialize Slaves Infos */
		memset(gs_pstIrrAppGateway_iSlavesInfos, 0, D_uiIrrigationNet_iMaxNbrSlavesPerNetwork*sizeof(T_stIrrAppGateway_eSlavesInfos));

		eIrrNetParameters_eMasterCommandString_Get (l_stRequestedCommand1.m_eRequestType, gs_pcIrrAppGateway_iMasterCommand);
		D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "New Request: %s", gs_pcIrrAppGateway_iMasterCommand);

		/* Send Get Status Command */
		l_eReturnValue1 = eIrrigationNetSap_eSendCommand_Req (l_stRequestedCommand1);
		if(l_eReturnValue1 != Success) {
			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Get Status Command is Rejected by Network");
		}

		/* Wait for the end of command processing */
		l_uiWaitingSeconds1 = 0;
		do {
			eIrrNetMaster_eCmdRequestStatus_Get (&l_eCmdRequestStatus1);
			osDelay(1000);
			l_uiWaitingSeconds1++;
			if(l_uiWaitingSeconds1 > 50) { /* 3 minutes */
				/* Log this event */
				eIrrAppLog_eAddEvent_Cmd (1, C_eIrrAppLog_eTooMuchTimeToProcessCmd);
				eIrrAppLog_eMoveLogToFlash_Cmd (1);
				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Timeout while waiting for the end of command processing, processing with received nodes.");
				osDelay(1000);
				exitLoop = true;
			}
		}while((l_eCmdRequestStatus1 == C_eIrrigationNet_iCmdReqInProgress)&&(exitLoop == false));

		D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "%s is processed with Success\r\n\r\n\r\n", gs_pcIrrAppGateway_iMasterCommand);

		/* Process Slaves Responses */
		eIrrAppGateway_iProcessSlavesStatus_Cmd(l_eDeviceType, &l_stRequestedCommand1);

		/* Idle Time */
		if(l_stRequestedCommand1.m_eRequestType == C_eIrrigationApp_eGetSlaveStatus) {
			if(l_eDeviceType == C_eIrrNetParameters_eBasinGatewayNode) {
				l_uiStartIdleTime1 = osKernelSysTick();
				do {
					eIrrAppGateway_iProcessSlavesStatus_Cmd(l_eDeviceType, &l_stRequestedCommand1);
					osDelay(500);
				}while( ((osKernelSysTick() - l_uiStartIdleTime1) < gs_uiIrrAppGateway_iIdleTimeinMs) && (l_stRequestedCommand1.m_eRequestType == C_eIrrigationApp_eGetSlaveStatus) );
			}
			else {
				osDelay(gs_uiIrrAppGateway_iIdleTimeinMs);
			}
		}
		else {
			osDelay(1000);
		}

		/* Move Log from RAM to Flash if RAM is full or 2 hours are passed from last flashing */
		eIrrAppLog_eMoveLogToFlash_Cmd (0);
	}

	return Success;
}




/**
 * T_eError_eErrorType eIrrigationNetSap_eSendGetStatusCommand_Req (uint32_t in_uiTargetDeviceId)
 * See irrigation_net_sap.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppGateway_iDisplaySlaveResponse_Cmd (uint8_t in_ucSlaveId, T_eParameters_eDeviceType in_eSlaveType, uint8_t in_ucSlaveResponse)
{
	T_eIrrigationApp_eGeneralSlaveStatus l_eGeneralSlaveStatus;
	T_eIrrAppBasin_eLastBasinStatus l_eBasinSlaveLastStatus;
	uint8_t l_ucOnOffStatus;

	l_eGeneralSlaveStatus = ((T_stIrrigationApp_eSlaveResponse*)(&in_ucSlaveResponse))->m_eGeneralSlaveStatus;
	l_eBasinSlaveLastStatus = ((T_stIrrigationApp_eSlaveResponse*)(&in_ucSlaveResponse))->m_eBasinSlaveLastStatus;
	l_ucOnOffStatus = ((T_stIrrigationApp_eSlaveResponse*)(&in_ucSlaveResponse))->m_ucOnOffStatus;

	switch(in_eSlaveType) {
	case C_eIrrNetParameters_eBasinSimpleNode:
		if( l_eGeneralSlaveStatus == C_eIrrigationApp_eSlaveIsNotOk ) {
			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Slave %d is Basin, State is NOT Ok", in_ucSlaveId);
		}
		else {
			if(l_eBasinSlaveLastStatus == C_eIrrAppBasin_eFullState) {
				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Slave %d is Basin, State is OK, Status = Full", in_ucSlaveId);
			}
			else if(l_eBasinSlaveLastStatus == C_eIrrAppBasin_eEmptyState) {
				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Slave %d is Basin, State is OK, Status = Empty", in_ucSlaveId);
			}
			else {
				/* empty else */
				D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Slave %d is Basin, State is OK, Status = Unknown", in_ucSlaveId);
			}

		}
		break;
	case C_eIrrNetParameters_eElectricalPumpNode:
		if( l_eGeneralSlaveStatus == C_eIrrigationApp_eSlaveIsNotOk ) {
			D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Slave %d is Electrical Pump, State is NOT NORMAL", in_ucSlaveId);
		}
		else {
			if(l_ucOnOffStatus & 0x01) {
				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Slave %d is Electrical Pump, State is OK, Status = ON", in_ucSlaveId);
			}
			else {
				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Slave %d is Electrical Pump, State is OK, Status = OFF", in_ucSlaveId);
			}
		}
		break;
	case C_eIrrNetParameters_eSolenoidValveNode:
			if( l_eGeneralSlaveStatus == C_eIrrigationApp_eSlaveIsNotOk ) {
				D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Slave %d is Solenoid Valve, State is NOT NORMAL", in_ucSlaveId);
			}
			else {
				D_vCmd_eDebugPrint(C_eCmd_eDebugInformation, "Slave %d is Solenoid Valve, State is OK", in_ucSlaveId);
			}
			break;
	default:
		D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Slave Type is not Known");
		break;
	}


	return Success;
}


/**
 * T_eError_eErrorType eIrrigationNetSap_eSendGetStatusCommand_Req (uint32_t in_uiTargetDeviceId)
 * See irrigation_net_sap.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppGateway_eSendCommand_Cnf (uint8_t in_ucMasterRequest, uint32_t in_uiTargetSlaveId, uint8_t in_ucSlaveResponse)
{
	T_eError_eErrorType l_eReturnValue;
	uint32_t l_uiNbrDevicesInNetwork;
	uint32_t l_uiCnt;

	eIrrNetParameters_eNbrDevicesInNetwork_Get(&l_uiNbrDevicesInNetwork);

	l_eReturnValue = eIrrNetFlash_ePairedSlavesInfos_Get(gs_pstIrrAppGateway_iPairedSlavesInfos);
	if( l_eReturnValue != Success ) {
		return l_eReturnValue;
	}

	for(l_uiCnt=0; l_uiCnt<(l_uiNbrDevicesInNetwork-1); l_uiCnt++) { /* Don't include Gateway Device */
		if(gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_uiSlaveId == in_uiTargetSlaveId) {
			/* Save Slave info */
			gs_pstIrrAppGateway_iSlavesInfos[in_uiTargetSlaveId-2].m_eSlaveType = gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_eSlaveType;
			gs_pstIrrAppGateway_iSlavesInfos[in_uiTargetSlaveId-2].m_ucSlaveId = in_uiTargetSlaveId;
			gs_pstIrrAppGateway_iSlavesInfos[in_uiTargetSlaveId-2].m_ucSlaveResponse = in_ucSlaveResponse;
			gs_pstIrrAppGateway_iSlavesInfos[in_uiTargetSlaveId-2].m_uiResponseTime = osKernelSysTick();
			eIrrAppGateway_iDisplaySlaveResponse_Cmd(in_uiTargetSlaveId, gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_eSlaveType, in_ucSlaveResponse);

			/* Logging: */
			/* Log reset type */
			if(((T_stIrrigationApp_eSlaveResponse*)(&(gs_pstIrrAppGateway_iSlavesInfos[in_uiTargetSlaveId-2].m_ucSlaveResponse)))->m_ucSlaveResetType == C_eReset_eHardware) { /* Hardware Reset occurs at slave */
				eIrrAppLog_eAddEvent_Cmd (gs_pstIrrAppGateway_iSlavesInfos[in_uiTargetSlaveId-2].m_ucSlaveId, C_eIrrAppLog_eHardReset);
			}
			else if(((T_stIrrigationApp_eSlaveResponse*)(&(gs_pstIrrAppGateway_iSlavesInfos[in_uiTargetSlaveId-2].m_ucSlaveResponse)))->m_ucSlaveResetType == C_eReset_eSoftware) { /* Software Reset occurs at slave */
				eIrrAppLog_eAddEvent_Cmd (gs_pstIrrAppGateway_iSlavesInfos[in_uiTargetSlaveId-2].m_ucSlaveId, C_eIrrAppLog_eSoftReset);
			}
			else if(((T_stIrrigationApp_eSlaveResponse*)(&(gs_pstIrrAppGateway_iSlavesInfos[in_uiTargetSlaveId-2].m_ucSlaveResponse)))->m_ucSlaveResetType == C_eReset_eWatchdog) { /* Watchdog Reset occurs at slave */
				eIrrAppLog_eAddEvent_Cmd (gs_pstIrrAppGateway_iSlavesInfos[in_uiTargetSlaveId-2].m_ucSlaveId, C_eIrrAppLog_eIwdgReset);
			}
			else {
				/* empty else, no reset occured at slave */
			}
			/* Log Slave Status if it is not OK */
			if(((T_stIrrigationApp_eSlaveResponse*)(&(gs_pstIrrAppGateway_iSlavesInfos[in_uiTargetSlaveId-2].m_ucSlaveResponse)))->m_eGeneralSlaveStatus == C_eIrrigationApp_eSlaveIsNotOk) { /* Slave is Not Ok */
				eIrrAppLog_eAddEvent_Cmd (gs_pstIrrAppGateway_iSlavesInfos[in_uiTargetSlaveId-2].m_ucSlaveId, C_eIrrAppLog_eSlaveIsNotOk);
			}
			/* Log change in Electrical Pumps states */
			if(gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_eSlaveType == C_eIrrNetParameters_eElectricalPumpNode) {
				if(in_uiTargetSlaveId >= 2) {
					if( ((gs_pstIrrAppGateway_iSlavesResponses[in_uiTargetSlaveId-2].m_ucOnOffStatus) & 0x01) != ((((T_stIrrigationApp_eSlaveResponse*)(&in_ucSlaveResponse))->m_ucOnOffStatus) & 0x01) ) {
						/* There is a change in electrical pump status */
						if( ((((T_stIrrigationApp_eSlaveResponse*)(&in_ucSlaveResponse))->m_ucOnOffStatus) & 0x01) == 1 ) { /* Current State of the Electrical Pump is ON: Electrical Pump passes from OFF to ON */
							eIrrAppLog_eAddEvent_Cmd (gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_uiSlaveId, C_eIrrAppLog_eSlaveSetsElectricalPumpOn);
						}
						else {
							eIrrAppLog_eAddEvent_Cmd (gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_uiSlaveId, C_eIrrAppLog_eSlaveSetsElectricalPumpOff);
						}
					}
					gs_pstIrrAppGateway_iSlavesResponses[in_uiTargetSlaveId-2] = (*((T_stIrrigationApp_eSlaveResponse*)(&in_ucSlaveResponse))); /* Save Last Slave Response */
				}
			}
			else if (gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_eSlaveType == C_eIrrNetParameters_eSolenoidValveNode) {
					if(in_uiTargetSlaveId >= 2) {
						if( ((gs_pstIrrAppGateway_iSlavesResponses[in_uiTargetSlaveId-2].m_ucOnOffStatus) & 0x01) != ((((T_stIrrigationApp_eSlaveResponse*)(&in_ucSlaveResponse))->m_ucOnOffStatus) & 0x01) ) {
							/* There is a change in Solenoid Valves status */
							if( ((((T_stIrrigationApp_eSlaveResponse*)(&in_ucSlaveResponse))->m_ucOnOffStatus) & 0x01) == 1 ) {/* Current State of the Solenoid Valves is ON: Solenoid Valves passes from OFF to ON */
								eIrrAppLog_eAddEvent_Cmd (gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_uiSlaveId,C_eIrrAppLog_eSlaveSetsSolenoidValveOn);
							}
							else {
								eIrrAppLog_eAddEvent_Cmd (gs_pstIrrAppGateway_iPairedSlavesInfos[l_uiCnt].m_uiSlaveId, C_eIrrAppLog_eSlaveSetsSolenoidValveOff);
							}
						}
						gs_pstIrrAppGateway_iSlavesResponses[in_uiTargetSlaveId-2] = (*((T_stIrrigationApp_eSlaveResponse*)(&in_ucSlaveResponse))); /* Save Last Slave Response */
					}
			}
		}
	}

	return Success;
}
