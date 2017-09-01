/***********************************************************************//**
 * @file	: lpc13xx_gpio.h
 * @brief	: Contains all macro definitions and function prototypes
 * 				support for GPIO firmware library on LPC13xx
 * @version	: 1.0
 * @date	: 30. Nov. 2009
 * @author	: ThieuTrinh
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
/** @defgroup GPIO
 * @ingroup LPC1300CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC13XX_GPIO_H_
#define LPC13XX_GPIO_H_

/* Includes ------------------------------------------------------------------- */
#include "LPC13xx.h"
#include "lpc_types.h"


#ifdef __cplusplus
extern "C"
{
#endif

/* Public Functions ----------------------------------------------------------- */
/** @defgroup GPIO_Public_Functions
 * @{
 */
#define PORT0		0
#define PORT1		1
#define PORT2		2
#define PORT3		3

/* GPIO style ------------------------------- */
void GPIO_SetDir(uint8_t portNum, uint32_t bitValue, uint8_t dir);
void GPIO_SetValue(uint8_t portNum, uint32_t bitValue);
void GPIO_ClearValue(uint8_t portNum, uint32_t bitValue);
uint32_t GPIO_ReadValue(uint8_t portNum);
void GPIO_SetInterrupt( uint32_t portNum, uint32_t bitValue, uint32_t sense,uint32_t single, uint32_t event );
void GPIO_IntEnable( uint32_t portNum, uint32_t bitValue );
void GPIO_IntDisable( uint32_t portNum, uint32_t bitValue );
uint32_t GPIO_IntStatus( uint32_t portNum, uint32_t bitValue );
void GPIO_IntClear( uint32_t portNum, uint32_t bitValue );
void GPIO_Init( void );

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LPC13XX_GPIO_H_ */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
