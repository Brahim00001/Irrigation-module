

#ifndef _MCU_APPLICATIONS_IRRIGATION_APP_H_
#define _MCU_APPLICATIONS_IRRIGATION_APP_H_



#include "error.h"
#include "irr_app_basin.h"




/**
 *\enum T_eSx1280_ePacketType
 *\brief Enumeration containg the different packet types supported by sx1280.
 */
typedef enum {
	C_eIrrigationApp_eGetSlaveStatus = 0,   			/* Get Slave Status Command */
	/* Electrical Pumps */
	C_eIrrigationApp_eSetElectricalPumpsOn,       		/* Set All Electrical Pumps On */
	C_eIrrigationApp_eSetElectricalPumpsOff,        	/* Set All Electrical Pumps Off */
	/* Basin Related Electrical Pump and Solenoid Valves */
	C_eIrrigationApp_eSetBasinElectricalPumpOn,       	/* Set the Electrical Pump of the Basin On */
	C_eIrrigationApp_eSetBasinElectricalPumpOff,        /* Set the Electrical Pump of the Basin Off */
	C_eIrrigationApp_eSetPurgeSolenoidValveOn,    		/* Set Purge Solenoid Valve (Related to Basin) On */
	C_eIrrigationApp_eSetPurgeSolenoidValveOff,    		/* Set Purge Solenoid Valve (Related to Basin) Off */
	C_eIrrigationApp_eSetIrrigationSolenoidValveOn,    	/* Set Irrigation Solenoid Valve (Related to Basin) On */
	C_eIrrigationApp_eSetIrrigationSolenoidValveOff,    /* Set Irrigation Solenoid Valve (Related to Basin) Off */
	/* Solenoid Valves */
	C_eIrrigationApp_eSetSolenoidValve1On,     			/* Set Solenoid Valve #1 On */
	C_eIrrigationApp_eSetSolenoidValve1Off,     		/* Set Solenoid Valve #1 Off */
	C_eIrrigationApp_eSetSolenoidValve2On,     			/* Set Solenoid Valve #2 On */
	C_eIrrigationApp_eSetSolenoidValve2Off,     		/* Set Solenoid Valve #2 Off */
	C_eIrrigationApp_eSetSolenoidValve3On,     			/* Set Solenoid Valve #3 On */
	C_eIrrigationApp_eSetSolenoidValve3Off,     		/* Set Solenoid Valve #3 Off */
	C_eIrrigationApp_eSetSolenoidValve4On,     			/* Set Solenoid Valve #4 On */
	C_eIrrigationApp_eSetSolenoidValve4Off,     		/* Set Solenoid Valve #4 Off */
}T_eIrrigationApp_eGatewayRequestType;


/**
 *\enum T_eIrrigationNet_eBasinStatus
 *\brief Enumeration containg the state of the Basin.
 */
typedef enum {
	C_eIrrigationApp_eSlaveIsNotOk = 0,   	/* Slave is in NOT Ok State */
	C_eIrrigationApp_eSlaveIsOk,       		/* Slave is in Ok State */
}T_eIrrigationApp_eGeneralSlaveStatus;



/**
 *\struct T_stIrrigationApp_eSlaveResponse
 *\brief Slave Response.
 */
typedef struct __attribute__((__packed__)) {
	T_eIrrigationApp_eGeneralSlaveStatus m_eGeneralSlaveStatus:1; /* General Slave Status: Slave is Ok or Not */
	T_eIrrAppBasin_eLastBasinStatus m_eBasinSlaveLastStatus:1; /* Last Status reserved for a Basin Slave */
	uint8_t m_ucOnOffStatus:4;		/* Each bit is reserved for 1 equipement. */
									/* For Basin: Electrical Pump, Purge Solenoid valve and Irrigation Solenoid Valves ==> 3 bits */
									/* For Electrical Pumps: only 1 bit: On or Off */
									/* For Solenoid valve: 4 bits: 1 bit for each solenoid valves. Solenoid Valve Node supports up to 4 solenoid Valves */
	uint8_t m_ucSlaveResetType:2; 	/* Used for logging at Gateway: set when slave restarts and reset after the sending of the first ACK */
}T_stIrrigationApp_eSlaveResponse; /* Slave Response */



T_eError_eErrorType eIrrigationApp_eMainTask_Tsk(void);



#endif /* _MCU_APPLICATIONS_IRRIGATION_APP_H_ */
