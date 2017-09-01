/***********************************************************************//**
 * @file	: lpc13xx_pinsel.h
 * @brief	: Contains all macro definitions and function prototypes
 * 				support for Pin connect block firmware library on LPC17xx
 * @version	: 1.0
 * @date	: 01. Nov. 2009
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
/** @defgroup PINSEL
 * @ingroup LPC1300CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC13XX_PINSEL_H_
#define LPC13XX_PINSEL_H_

/* Includes ------------------------------------------------------------------- */
#include "lpc13xx.h"
#include "lpc_types.h"

#ifdef __cplusplus
extern "C"
{
#endif



/* Public Macros -------------------------------------------------------------- */
/** @defgroup PINSEL_Public_Macros
 * @{
 */

/*********************************************************************//**
*!< Macro defines for I/O configuration registers
***********************************************************************/
	#define PINSEL_PIO_FUNC(n)		(n&0x07)		/**< Pin function selected */
	#define PINSEL_PIO_MODE(n)		((n&0x03)<<3)	/**< Mode function selected */
	#define PINSEL_PIO_HYS			(1<<5)			/**< Hysteresis */
	#define PINSEL_PIO_DMODE			(1<<7)			/**< Digital function mode, applied for pin that
														has ADC functionality */
	#define PINSEL_PIO_I2CMODE(n)	((n&0x03)<<8)	/**< I2C mode selected, applied for pin that
														has I2C functionality */
	#define PINSEL_PIO_MASK			(0x3FF)			/**< IOCON_PIOn mask value */


/*********************************************************************//**
 *!< Macros define for PORT Selection
 ***********************************************************************/
#define PINSEL_PORT_0 	((0))	/**< PORT 0*/
#define PINSEL_PORT_1 	((1))	/**< PORT 1*/
#define PINSEL_PORT_2 	((2))	/**< PORT 2*/
#define PINSEL_PORT_3 	((3))	/**< PORT 3*/


/***********************************************************************
 * Macros define for Pin Function selection
 **********************************************************************/
#define PINSEL_FUNC_0	((0))	/**< default function*/
#define PINSEL_FUNC_1	((1))	/**< first alternate function*/
#define PINSEL_FUNC_2	((2))	/**< second alternate function*/
#define PINSEL_FUNC_3	((3))	/**< third or reserved alternate function*/


/***********************************************************************
 * Macros define for Pin mode
 **********************************************************************/
 #define PINSEL_PINMODE_INACTIVE	((0))	/**< No pull-up or pull-down resistor*/
#define PINSEL_PINMODE_PULLUP		((1))	/**< Internal pull-up resistor*/
#define PINSEL_PINMODE_PULLDOWN 	((2)) 	/**< Internal pull-down resistor */
#define PINSEL_PINMODE_REPEATER 	((3))

/***********************************************************************
 * Macros define for Pin Hysteresis
 **********************************************************************/

#define PINSEL_PINHYS_DISABLE		((0)) /**< Hysteresis disable*/
#define PINSEL_PINHYS_ENABLE		((1)) /**< Hysteresis enable*/





/***********************************************************************
 * Macros define for Pin Number of Port
 **********************************************************************/
#define PINSEL_PIN_0 	((0)) 	/**< Pin 0 */
#define PINSEL_PIN_1 	((1)) 	/**< Pin 1 */
#define PINSEL_PIN_2 	((2)) 	/**< Pin 2 */
#define PINSEL_PIN_3 	((3)) 	/**< Pin 3 */
#define PINSEL_PIN_4 	((4)) 	/**< Pin 4 */
#define PINSEL_PIN_5 	((5)) 	/**< Pin 5 */
#define PINSEL_PIN_6 	((6)) 	/**< Pin 6 */
#define PINSEL_PIN_7 	((7)) 	/**< Pin 7 */
#define PINSEL_PIN_8 	((8)) 	/**< Pin 8 */
#define PINSEL_PIN_9 	((9)) 	/**< Pin 9 */
#define PINSEL_PIN_10 	((10)) 	/**< Pin 10 */
#define PINSEL_PIN_11 	((11)) 	/**< Pin 11 */

/**
 * @}
 */


/* Public Types --------------------------------------------------------------- */
/** @defgroup PINSEL_Public_Types
 * @{
 */

/** @brief Pin configuration structure */
typedef struct
{
	uint8_t Portnum;	/**< Port Number, should be PINSEL_PORT_x,
						where x should be in range from 0 to 3 */
	uint8_t Pinnum;		/**< Pin Number, should be PINSEL_PIN_x,
						where x should be in range from 0 to 11 */
	uint8_t Funcnum;	/**< Function Number, should be PINSEL_FUNC_x,
						where x should be in range from 0 to 3 */
	uint8_t Funcmode;	/**< Mode Number, should be PINSEL_MODE_x,
						where x should be in range from 0 to 3 */
	uint8_t Hystereris;	/**< Hysterris, should be ENABLE/DISABLE */

} PINSEL_CFG_Type;

/**
 * @}
 */


/* Public Functions ----------------------------------------------------------- */
/** @defgroup PINSEL_Public_Functions
 * @{
 */

void PINSEL_SetPinFunc ( uint8_t portnum, uint8_t pinnum, uint8_t funcnum);
void PINSEL_SetPinMode ( uint8_t portnum, uint8_t pinnum, uint8_t modenum);
void PINSEL_SetHysMode ( uint8_t portnum, uint8_t pinnum, uint8_t hysnum);
void PINSEL_ConfigPin(PINSEL_CFG_Type *PinCfg);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LPC13XX_PINSEL_H_ */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */

