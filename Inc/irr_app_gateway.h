

#ifndef _MCU_APPLICATIONS_IRRIGATION_IRR_APP_GATEWAY_H_
#define _MCU_APPLICATIONS_IRRIGATION_IRR_APP_GATEWAY_H_



#include "error.h"
#include "irr_net_parameters.h"
#include "irrigation_app.h"


/**
 *\struct T_stIrrAppGateway_iRequestedCommand
 *\brief Master Request Command.
 */
typedef struct __attribute__((__packed__)) {
	T_eIrrigationApp_eGatewayRequestType m_eRequestType; /* Gateway Request Type */
	uint32_t m_uiTargetIdsBitMap;
}T_stIrrAppGateway_eRequestedCommand;


/**
 *\struct T_stIrrAppGateway_iRequestedCommand
 *\brief Master Request Command.
 */
typedef struct __attribute__((__packed__)) {
	T_eParameters_eDeviceType m_eSlaveType; /* Slave Type */
	uint8_t m_ucSlaveId;
	uint8_t m_ucSlaveResponse;
	uint32_t m_uiResponseTime;
}T_stIrrAppGateway_eSlavesInfos;


T_eError_eErrorType eIrrAppGateway_eIdleTime_Set (uint32_t in_uiIdleTimeInSeconds);

T_eError_eErrorType eIrrAppGateway_eElectricalPumpsIdsBitMap_Get (uint32_t *out_puiElectricalPumpsIdsBitMap);

T_eError_eErrorType eIrrAppGateway_eSolenoidValvesIdsBitMap_Get (uint32_t *out_puiSolenoidValvesIdsBitMap);

T_eError_eErrorType eIrrAppGateway_iProcessSlavesStatus_Cmd(T_eParameters_eDeviceType in_eDeviceType, T_stIrrAppGateway_eRequestedCommand *out_pstRequestedCommand);

T_eError_eErrorType eIrrAppGateway_iProcessSlave2Status_Cmd(T_eParameters_eDeviceType in_eDeviceType, T_stIrrAppGateway_eRequestedCommand *out_pstRequestedCommand);

T_eError_eErrorType eIrrAppGateway_iDisplaySlaveResponse_Cmd (uint8_t in_ucSlaveId, T_eParameters_eDeviceType in_eSlaveType, uint8_t in_ucSlaveResponse);

T_eError_eErrorType eIrrAppGateway_eGatewayBehavior_Tsk(void);

T_eError_eErrorType eIrrAppGateway_eSendCommand_Cnf (uint8_t in_ucMasterRequest, uint32_t in_uiTargetSlaveId, uint8_t in_ucSlaveResponse);



#endif /* _MCU_APPLICATIONS_IRRIGATION_IRR_APP_GATEWAY_H_ */
