/***********************************************************************//**
 * @file	: lpc13xx_nvic.h
 * @brief	: Contains all macro definitions and function prototypes
 * 				support for Nesting Vectored Interrupt firmware library
 * 				on LPC13xx
 * @version	: 1.0
 * @date	: 18. Mar. 2009
 * @author	: HieuNguyen
 **************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 **************************************************************************/

/* Peripheral group ----------------------------------------------------------- */
/** @defgroup NVIC
 * @ingroup LPC1300CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC13XX_NVIC_H_
#define LPC13XX_NVIC_H_

/* Includes ------------------------------------------------------------------- */
#include "LPC13xx.h"
#include "lpc_types.h"

#ifdef __cplusplus
extern "C"
{
#endif


/* Public Functions ----------------------------------------------------------- */
/** @defgroup NVIC_Public_Functions
 * @{
 */

void NVIC_DeInit(void);
void NVIC_SCBDeInit(void);
void NVIC_SetVTOR(uint32_t offset);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LPC13XX_NVIC_H_ */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
