

#ifndef _MCU_APPLICATIONS_IRRIGATION_IRR_APP_SOLENOID_VALVES_H_
#define _MCU_APPLICATIONS_IRRIGATION_IRR_APP_SOLENOID_VALVES_H_


#include "error.h"


/**
 *\enum T_eMotor_eMotorStatus
 *\brief Enumeration containing the different status of the motor.
 */
typedef enum {
	C_eIrrAppSolenoidValves_iSolenoidValvesOff = 0,     /* Solenoid is off */
	C_eIrrAppSolenoidValves_iSolenoidValvesOn,
}T_eIrrAppSolenoidValves_eSolenoidValvesStatus;



T_eError_eErrorType eIrrAppSolenoidValves_eSolenoidValvesStatus_Get(T_eIrrAppSolenoidValves_eSolenoidValvesStatus *out_peSolenoidValvesStatus);

T_eError_eErrorType eIrrAppSolenoidValves_eSolenoidValvesStatus_Cmd(T_eIrrAppSolenoidValves_eSolenoidValvesStatus in_eSolenoidValvesStatus);

T_eError_eErrorType eIrrAppSolenoidValves_eSolenoidValvesBehavior_Tsk(void);

T_eError_eErrorType eIrrAppSolenoidValves_eCommandReceived_Cmd (uint8_t in_ucMasterRequest);



#endif /* _MCU_APPLICATIONS_IRRIGATION_IRR_APP_SOLENOID_VALVES_H_ */
