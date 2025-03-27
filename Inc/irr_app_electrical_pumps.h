

#ifndef _MCU_APPLICATIONS_IRRIGATION_IRR_APP_ELECTRICAL_PUMPS_H_
#define _MCU_APPLICATIONS_IRRIGATION_IRR_APP_ELECTRICAL_PUMPS_H_


#include "error.h"


/**
 *\enum T_eMotor_eMotorStatus
 *\brief Enumeration containing the different status of the motor.
 */
typedef enum {
	C_eIrrAppElectricalPumps_eUnknownElectricalPumpsStatus = 0,	/* Electrical Pumps have unknown status */
	C_eIrrAppElectricalPumps_eElectricalPumpsOn,     			/* Electrical Pumps are on */
	C_eIrrAppElectricalPumps_eElectricalPumpsOff,          		/* Electrical Pumps are off */
}T_eIrrAppElectricalPumps_eElectricalPumpsStatus; /* Each electrical Pumps Node supports up to 2 electrical pumps */



T_eError_eErrorType eIrrAppElectricalPumps_eElectricalPumpsStatus_Get(T_eIrrAppElectricalPumps_eElectricalPumpsStatus *out_peElectricalPumpsStatus);


T_eError_eErrorType eIrrAppElectricalPumps_eElectricalPumpsBehavior_Tsk(void);

T_eError_eErrorType eIrrAppElectricalPumps_eCommandReceived_Cmd (uint8_t in_ucMasterRequest);

T_eError_eErrorType eIrrAppElectricalPumps_iSetElectricalPumpsStatus_Cmd(T_eIrrAppElectricalPumps_eElectricalPumpsStatus in_eElectricalPumpsStatus);



#endif /* _MCU_APPLICATIONS_IRRIGATION_IRR_APP_ELECTRICAL_PUMPS_H_ */
