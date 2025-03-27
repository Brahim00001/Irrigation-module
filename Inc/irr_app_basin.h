

#ifndef _MCU_APPLICATIONS_IRRIGATION_IRR_APP_BASIN_H_
#define _MCU_APPLICATIONS_IRRIGATION_IRR_APP_BASIN_H_



#include "error.h"


/**
 *\enum T_eIrrAppBasin_eLastBasinStatus
 *\brief Enumeration containg the last state of the Basin. Only 2 Last states: Basin was Full or Basin was Empty
 */
typedef enum {
	C_eIrrAppBasin_eEmptyState = 0,       	/* Last state of Basin is: Empty */
	C_eIrrAppBasin_eFullState,       	/* Last state of Basin is: Full */
}T_eIrrAppBasin_eLastBasinStatus;


/**
 *\enum T_eIrrAppBasin_eLastBasinStatus
 *\brief Enumeration containg the exact state of the Basin. Only 3 states: Basin was Full, Basin was Empty or Basin is not full in Night Time
 */
typedef enum {
	C_eIrrAppBasin_iTotalEmptyState = 0,       	/* Last state of Basin is: Totally Empty */
	C_eIrrAppBasin_iTotalFullState,       		/* Last state of Basin is: Totally Full */
	C_eIrrAppBasin_iMidEmptyStateInNightTime,   /* Mid empty in night time */
}T_eIrrAppBasin_iExactBasinStatus;


T_eError_eErrorType eIrrAppBasin_eWaterLevelSamplingTime_Set (uint32_t in_uiWaterLevelSamplingTimeInSeconds);

T_eError_eErrorType eIrrAppBasin_eBasinStatus_Get(T_eIrrAppBasin_eLastBasinStatus *out_peLastBasinStatus);

T_eError_eErrorType eIrrAppBasin_eUpdateLastBasinStatus_Cmd(void);

T_eError_eErrorType eIrrAppBasin_eBasinBehavior_Tsk(void);



#endif /* _MCU_APPLICATIONS_IRRIGATION_IRR_APP_BASIN_H_ */
