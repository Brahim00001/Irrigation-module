/*
 * irr_app_flash.c
 *
 *  Created on: Jul 16, 2019
 *      Author: Codintek_3
 */



#include <string.h>

#include "irr_app_flash.h"
#include "main.h"
#include "cmd.h"
#include "irr_net_encoder.h"



static uint8_t gs_pucIrrAppFlash_iConfiguration[D_uiIrrAppFlash_iMaxConfigurationSize];




/**
 * eIrrAppFlash_eWriteConfigurationInFlash_Cmd (uint8_t *, uint32_t , uint32_t )
 * See irr_net_flash.h for details of how to use this function.
 * Write Configuration in the flash
 */
T_eError_eErrorType eIrrAppFlash_eWriteConfigurationInFlash_Cmd (uint8_t *in_pucConfigurationBody, uint32_t in_uiSize, uint32_t in_uiFlashAddress)
{
	uint8_t l_ucCrc;

	if(in_uiSize > D_uiIrrAppFlash_iMaxConfigurationBodySize) {
		D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irr_app_flash.c Line %d, Configuration Size, you need to increase the Configuration table Size or reduce the size of the current Configuration", __LINE__);
		return Failure_Memory;
	}

	/* Set the Identifier of configuration set */
	*((uint32_t*)gs_pucIrrAppFlash_iConfiguration) = D_eIrrAppFlash_eIsConfigurationSetIdentifier; /* 4 bytes for Configuration Identifier */

	/* Copy Configuration Body */
	memcpy(gs_pucIrrAppFlash_iConfiguration+4, in_pucConfigurationBody, in_uiSize);

	/* Add CRC */
	eIrrNetEncoder_eCrc8_Get(gs_pucIrrAppFlash_iConfiguration, in_uiSize+4, &l_ucCrc);
	*(gs_pucIrrAppFlash_iConfiguration+4+in_uiSize) = l_ucCrc;

	D_vCmd_eDebugPrint(C_eCmd_eDebugWarning, "############################ Flash Page at Address 0x%x is erased", (unsigned int)in_uiFlashAddress);

	return eFlash_eWrite_Cmd(in_uiFlashAddress, gs_pucIrrAppFlash_iConfiguration, in_uiSize+5); /* (1 byte for CRC + 4 bytes for Configuration Identifier) */

}


/**
 * eIrrAppFlash_eReadConfigurationFromFlash_Cmd (uint8_t *, uint32_t , uint32_t )
 * See irr_net_flash.h for details of how to use this function.
 * Read Configuration from Flash
 */
T_eError_eErrorType eIrrAppFlash_eReadConfigurationFromFlash_Cmd (uint8_t *out_pucConfigurationBody, uint32_t in_uiSize, uint32_t in_uiFlashAddress)
{
	T_eError_eErrorType l_eReturnResult;
	uint8_t l_ucCrc;


	l_eReturnResult = eFlash_eRead_Cmd(in_uiFlashAddress, in_uiSize+5, gs_pucIrrAppFlash_iConfiguration); /* 5 = (1 byte for CRC + 4 bytes for Configuration Identifier) */

	if(l_eReturnResult == Success) {
		/* Check CRC */
		eIrrNetEncoder_eCrc8_Get(gs_pucIrrAppFlash_iConfiguration, in_uiSize+4, &l_ucCrc);
		if(l_ucCrc == (*(gs_pucIrrAppFlash_iConfiguration+4+in_uiSize))) {
			/* Check the configuration Set Identifier */
			if(D_eFlashInt_eIsConfigurationSetIdentifier == (*((uint32_t*)gs_pucIrrAppFlash_iConfiguration))) {
				memcpy(out_pucConfigurationBody, gs_pucIrrAppFlash_iConfiguration+4, in_uiSize);
				return Success;
			}
			else {
				return Failure_BadConfig;
			}
		}
		else {
			return Failure_Memory;
		}
	}
	else {
		return l_eReturnResult;
	}

}


/**
 * eIrrAppFlash_eWriteConfigurationInFlash_Cmd (uint8_t *, uint32_t , uint32_t )
 * See irr_net_flash.h for details of how to use this function.
 * Write Configuration in the flash
 */
T_eError_eErrorType eIrrAppFlash_eEraseConfigurationFromFlash_Cmd (uint32_t in_uiFlashAddress, uint32_t in_uiSize)
{
	D_vCmd_eDebugPrint(C_eCmd_eDebugWarning, "############################ Flash Page at Address 0x%x is erased", (unsigned int)in_uiFlashAddress);

	return eFlash_eErase_Cmd(in_uiFlashAddress, in_uiSize);
}


static uint8_t gs_pucIrrAppFlash_iPageContent[2048]; /* Size of flash page = 2048 */

/**
 * eIrrAppFlash_eWriteLogInFlash_Cmd (uint8_t *in_pucConfigurationBody, uint32_t in_uiSize, uint32_t in_uiFlashAddress)
 * See irr_app_flash.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppFlash_eWriteLogInFlash_Cmd (uint8_t *in_pucConfigurationBody, uint32_t in_uiSize, uint32_t in_uiFlashAddress)
{
	uint32_t l_uiStartPageNbr;
	uint32_t l_uiEndPageNbr;
	uint32_t l_uiPagesNbr;
	uint32_t l_uiStartPageAddr;
	uint32_t l_uiNbrBytesToWriteInPage1;
	uint32_t l_uiNbrBytesToWriteInPage2;

	/* Note: 512 bytes at maximum can be written in the same time => 2 pages at maximum can be written */
	/* If two pages will be written, write page by page */
	if(in_uiSize > 512) {
		return Failure_Unsupported;
	}

	/* Get the number of page containing the start address */
	eFlash_eNbrOfPage_Get(in_uiFlashAddress, &l_uiStartPageNbr);

	/* Get the number of page containing the end address */
	eFlash_eNbrOfPage_Get(in_uiFlashAddress+in_uiSize, &l_uiEndPageNbr);

	/* Get the number of pages that will be written */
	l_uiPagesNbr = (l_uiEndPageNbr - l_uiStartPageNbr) + 1;

	if( (l_uiPagesNbr < 1) || (l_uiPagesNbr > 2) ) {
		return Failure_Unsupported;
	}

	/* Get First Page Address */
	if( in_uiFlashAddress >= ((in_uiFlashAddress&0xFFFFF000)|0x800) ) {
		l_uiStartPageAddr = ((in_uiFlashAddress&0xFFFFF000)|0x800);
	}
	else {
		l_uiStartPageAddr = in_uiFlashAddress & 0xFFFFF000;
	}

	if( (in_uiFlashAddress+in_uiSize) <= (l_uiStartPageAddr+0x800) ) {
		l_uiNbrBytesToWriteInPage1 = in_uiSize;
		l_uiNbrBytesToWriteInPage2 = 0;
	}
	else {
		l_uiNbrBytesToWriteInPage1 = (l_uiStartPageAddr+0x800) - in_uiFlashAddress;
		l_uiNbrBytesToWriteInPage2 = in_uiSize - l_uiNbrBytesToWriteInPage1;
	}

	if(l_uiStartPageAddr >= (D_uiIrrAppFlash_eLogFlashAddress+FLASH_BANK_SIZE)) {
		return Failure_Unexpected;
	}
	if( (l_uiNbrBytesToWriteInPage2 > 0) && ((l_uiStartPageAddr+0x800) >= (D_uiIrrAppFlash_eLogFlashAddress+FLASH_BANK_SIZE)) ) {
		return Failure_Unexpected;
	}


	eFlash_eRead_Cmd(l_uiStartPageAddr, 2048, gs_pucIrrAppFlash_iPageContent);
	if(in_uiFlashAddress < l_uiStartPageAddr) {
		return Failure_Unexpected;
	}
	if( (in_uiFlashAddress-l_uiStartPageAddr) >= 2048 ) {
		return Failure_Unexpected;
	}
	if( (in_uiFlashAddress-l_uiStartPageAddr+l_uiNbrBytesToWriteInPage1) > 2048 ) {
		return Failure_Unexpected;
	}
	memcpy(gs_pucIrrAppFlash_iPageContent+in_uiFlashAddress-l_uiStartPageAddr, in_pucConfigurationBody, l_uiNbrBytesToWriteInPage1);
	D_vCmd_eDebugPrint(C_eCmd_eDebugWarning, "############################ Log Flash Page at Address 0x%x is erased, size = 2048", (unsigned int)l_uiStartPageAddr);
	eFlash_eWrite_Cmd(l_uiStartPageAddr, gs_pucIrrAppFlash_iPageContent, 2048);

	if(l_uiNbrBytesToWriteInPage2 > 0) { /* page 2 is then empty: no need to read it before */
		if( (l_uiNbrBytesToWriteInPage1+l_uiNbrBytesToWriteInPage2) > in_uiSize ) {
			return Failure_Unexpected;
		}
		D_vCmd_eDebugPrint(C_eCmd_eDebugWarning, "############################ Log Flash Page at Address 0x%x is erased, size = %u", (unsigned int)l_uiStartPageAddr+0x800, (unsigned int)l_uiNbrBytesToWriteInPage2);
		eFlash_eWrite_Cmd(l_uiStartPageAddr+0x800, in_pucConfigurationBody+l_uiNbrBytesToWriteInPage1, l_uiNbrBytesToWriteInPage2);
	}
	else {
		/* empty else: nothing to do */
	}

	return Success;

}


/**
 * eIrrAppFlash_eReadLogFromFlash_Cmd (uint8_t *out_pucConfigurationBody, uint32_t in_uiSize, uint32_t in_uiFlashAddress)
 * See irr_app_flash.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppFlash_eReadLogFromFlash_Cmd (uint8_t *out_pucConfigurationBody, uint32_t in_uiSize, uint32_t in_uiFlashAddress)
{

	return eFlash_eRead_Cmd(in_uiFlashAddress, in_uiSize, out_pucConfigurationBody);

}


/**
 * eIrrAppFlash_eEraseLogFromFlash_Cmd (uint32_t in_uiFlashAddress, uint32_t in_uiSize)
 * See irr_app_flash.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppFlash_eEraseLogFromFlash_Cmd (uint32_t in_uiFlashAddress, uint32_t in_uiSize)
{
	D_vCmd_eDebugPrint(C_eCmd_eDebugWarning, "############################ Log Flash Page at Address 0x%x is erased, size = %u", (unsigned int)in_uiFlashAddress, (unsigned int)in_uiSize);

	return eFlash_eErase_Cmd(in_uiFlashAddress, in_uiSize);
}




/* Logging Status */
/**
 * eIrrAppFlash_eFlashLoggingStatus_Cmd (uint32_t in_uiLoggingStatus)
 * See irr_net_flash.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppFlash_eFlashLoggingStatus_Cmd (uint32_t in_uiLoggingStatus) {

	T_eError_eErrorType l_eReturnValue;

	if( in_uiLoggingStatus > 1 ) {
		D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irr_app_flash.c Line %d, Logging status cannot exceed 1", __LINE__);
		return Failure_BadArgument;
	}

	l_eReturnValue = eIrrNetFlash_eWriteConfigurationInFlash_Cmd ((uint8_t*)(&in_uiLoggingStatus), 4, D_uiIrrAppFlash_eLoggingStatusFlashAddress);
	if(l_eReturnValue != Success) {
		D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irr_app_flash.c Line %d, Logging Status cannot be flashed", __LINE__);
	}

	return l_eReturnValue;
}

/**
 * eIrrAppFlash_eFlashMacAddress_Cmd (uint32_t in_uiMacAddress)
 * See irr_net_flash.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppFlash_eEraseLoggingStatus_Cmd (void) {

	T_eError_eErrorType l_eReturnValue;

	l_eReturnValue = eIrrNetFlash_eEraseConfigurationFromFlash_Cmd (D_uiIrrAppFlash_eLoggingStatusFlashAddress, 4);
	if(l_eReturnValue != Success) {
		D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irr_app_flash.c Line %d, Logging Status cannot be erased", __LINE__);
	}

	return l_eReturnValue;
}

/**
 * eIrrAppFlash_eFlashLoggingStatus_Cmd (uint32_t in_uiLoggingStatus)
 * See irr_net_flash.h for details of how to use this function.
 */
T_eError_eErrorType eIrrAppFlash_eLoggingStatus_Get (uint32_t *out_puiLoggingStatus) {

	T_eError_eErrorType l_eReturnValue;

	l_eReturnValue = eIrrNetFlash_eReadConfigurationFromFlash_Cmd ((uint8_t*)out_puiLoggingStatus, 4, D_uiIrrAppFlash_eLoggingStatusFlashAddress);
	if(l_eReturnValue != Success) {
		D_vCmd_eDebugPrint(C_eCmd_eDebugError, "Error in irr_net_flash.c Line %d, Logging Status cannot be get from Flash", __LINE__);
	}

	return l_eReturnValue;
}


