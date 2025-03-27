/*
 * irr_app_flash.h
 *
 *  Created on: Jul 2, 2019
 *      Author: nizar
 */

#ifndef NET_INC_IRR_APP_FLASH_H_
#define NET_INC_IRR_APP_FLASH_H_


#include "flash.h"


#define D_uiIrrAppFlash_iMaxConfigurationBodySize		4 /* In Bytes */


/**
 *\def  D_eFlashInt_eIsConfigurationSetIdentifier
 *\brief Identifier to know if the configuration is set in the flash or no
 */
#define D_eIrrAppFlash_eIsConfigurationSetIdentifier		(0x55AA33CC)


#define D_uiIrrAppFlash_iMaxConfigurationSize		(D_uiIrrAppFlash_iMaxConfigurationBodySize+sizeof(D_eIrrAppFlash_eIsConfigurationSetIdentifier)+1)	/* In Bytes; 1 CRC Byte */



/**
 *\def  D_uiIrrAppFlash_eLoggingStatusFlashAddress
 *\brief Address of the Logging Status in the Flash
 */
#define D_uiIrrAppFlash_eLoggingStatusFlashAddress		D_uiFlashInt_ePage255Address // Page 255 in Bank 0: end page in Bank 0


/**
 *\def  D_uiIrrAppFlash_eLogFlashAddress
 *\brief Address of the Log in the Flash
 */
#define D_uiIrrAppFlash_eLogFlashAddress		D_uiFlashInt_eBank1Address // All Bank 1 is reserved for logging




T_eError_eErrorType eIrrAppFlash_eWriteConfigurationInFlash_Cmd (uint8_t *in_pucConfigurationBody, uint32_t in_uiSize, uint32_t in_uiFlashAddress);
T_eError_eErrorType eIrrAppFlash_eReadConfigurationFromFlash_Cmd (uint8_t *out_pucConfigurationBody, uint32_t in_uiSize, uint32_t in_uiFlashAddress);
T_eError_eErrorType eIrrAppFlash_eEraseConfigurationFromFlash_Cmd (uint32_t in_uiFlashAddress, uint32_t in_uiSize);


T_eError_eErrorType eIrrAppFlash_eWriteLogInFlash_Cmd (uint8_t *in_pucLog, uint32_t in_uiSize, uint32_t in_uiFlashAddress);
T_eError_eErrorType eIrrAppFlash_eReadLogFromFlash_Cmd (uint8_t *out_pucLog, uint32_t in_uiSize, uint32_t in_uiFlashAddress);
T_eError_eErrorType eIrrAppFlash_eEraseLogFromFlash_Cmd (uint32_t in_uiFlashAddress, uint32_t in_uiSize);


/* Logging Status */
T_eError_eErrorType eIrrAppFlash_eFlashLoggingStatus_Cmd (uint32_t in_uiLoggingStatus);
T_eError_eErrorType eIrrAppFlash_eEraseLoggingStatus_Cmd (void);
T_eError_eErrorType eIrrAppFlash_eLoggingStatus_Get (uint32_t *out_puiLoggingStatus);



#endif /* NET_INC_IRR_APP_FLASH_H_ */
