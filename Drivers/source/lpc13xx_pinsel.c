/**
 * @file	: lpc13xx_pinsel.c
 * @brief	: Contains all functions support for Pin connect block firmware
 * 				library on LPC17xx
 * @version	: 1.0
 * @date	: 01. Dec. 2009
 * @author	: ThieuTrinh
 *----------------------------------------------------------------------------
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
 **********************************************************************/

/* Peripheral group ----------------------------------------------------------- */
/** @addtogroup PINSEL
 * @{
 */

/* Includes ------------------------------------------------------------------- */
#include "lpc13xx_pinsel.h"

/* Private Functions ---------------------------------------------------------- */
/** @defgroup GPIO_Private_Functions
 * @{
 */

/*********************************************************************//**
 * @brief		Get pointer to GPIO peripheral due to GPIO port
 * @param[in]	portNum		Port Number value, should be in range from 0 to 3.
 * @return		Pointer to GPIO peripheral
 **********************************************************************/


static uint32_t *PIN_GetPointer(uint8_t portnum, uint8_t pinnum)
{
	uint32_t *pPIN = NULL;
	if(portnum == 0){
		switch(pinnum){
		case 0:
			pPIN = (uint32_t *)&LPC_IOCON->RESET_PIO0_0;
			break;
		case 1:
			pPIN = (uint32_t *)&LPC_IOCON->PIO0_1;
			break;
		case 2:
			pPIN = (uint32_t *)&LPC_IOCON->PIO0_2;
			break;
		case 3:
			pPIN = (uint32_t *)&LPC_IOCON->PIO0_3;
			break;
		case 4:
			pPIN = (uint32_t *)&LPC_IOCON->PIO0_4;
			break;
		case 5:
			pPIN = (uint32_t *)&LPC_IOCON->PIO0_5;
			break;
		case 6:
			pPIN = (uint32_t *)&LPC_IOCON->PIO0_6;
			break;
		case 7:
			pPIN = (uint32_t *)&LPC_IOCON->PIO0_7;
			break;
		case 8:
			pPIN = (uint32_t *)&LPC_IOCON->PIO0_8;
			break;
		case 9:
			pPIN = (uint32_t *)&LPC_IOCON->PIO0_9;
			break;
		case 10:
			pPIN = (uint32_t *)&LPC_IOCON->SWCLK_PIO0_10;
			break;
		case 11:
			pPIN = (uint32_t *)&LPC_IOCON->R_PIO0_11;
			break;
		}

	}else if(portnum == 1){
		switch(pinnum){
		case 0:
			pPIN = (uint32_t *)&LPC_IOCON-> R_PIO1_0;
			break;
		case 1:
			pPIN = (uint32_t *)&LPC_IOCON->R_PIO1_1;
			break;
		case 2:
			pPIN = (uint32_t *)&LPC_IOCON->R_PIO1_2;
			break;
		case 3:
			pPIN = (uint32_t *)&LPC_IOCON->SWDIO_PIO1_3;
			break;
		case 4:
			pPIN = (uint32_t *)&LPC_IOCON->PIO1_4;
			break;
		case 5:
			pPIN = (uint32_t *)&LPC_IOCON->PIO1_5;
			break;
		case 6:
			pPIN = (uint32_t *)&LPC_IOCON->PIO1_6;
			break;
		case 7:
			pPIN = (uint32_t *)&LPC_IOCON->PIO1_7;
			break;
		case 8:
			pPIN = (uint32_t *)&LPC_IOCON->PIO1_8;
			break;
		case 9:
			pPIN = (uint32_t *)&LPC_IOCON->PIO1_9;
			break;
		case 10:
			pPIN = (uint32_t *)&LPC_IOCON->PIO1_10;
			break;
		case 11:
			pPIN = (uint32_t *)&LPC_IOCON->PIO1_11;
			break;
		}

	}else if (portnum == 2){
		switch(pinnum){
		case 0:
			pPIN = (uint32_t *)&LPC_IOCON->PIO2_0;
			break;
		case 1:
			pPIN = (uint32_t *)&LPC_IOCON->PIO2_1;
			break;
		case 2:
			pPIN = (uint32_t *)&LPC_IOCON->PIO2_2;
			break;
		case 3:
			pPIN = (uint32_t *)&LPC_IOCON->PIO2_3;
			break;
		case 4:
			pPIN = (uint32_t *)&LPC_IOCON->PIO2_4;
			break;
		case 5:
			pPIN = (uint32_t *)&LPC_IOCON->PIO2_5;
			break;
		case 6:
			pPIN = (uint32_t *)&LPC_IOCON->PIO2_6;
			break;
		case 7:
			pPIN = (uint32_t *)&LPC_IOCON->PIO2_7;
			break;
		case 8:
			pPIN = (uint32_t *)&LPC_IOCON->PIO2_8;
			break;
		case 9:
			pPIN = (uint32_t *)&LPC_IOCON->PIO2_9;
			break;
		case 10:
			pPIN = (uint32_t *)&LPC_IOCON->PIO2_10;
			break;
		case 11:
			pPIN = (uint32_t *)&LPC_IOCON->PIO2_11;
			break;
		}

	}else if(portnum == 3){
		switch(pinnum){
		case 0:
			pPIN = (uint32_t *)&LPC_IOCON->PIO3_0;
			break;
		case 1:
			pPIN = (uint32_t *)&LPC_IOCON->PIO3_1;
			break;
		case 2:
			pPIN = (uint32_t *)&LPC_IOCON->PIO3_2;
			break;
		case 3:
			pPIN = (uint32_t *)&LPC_IOCON->PIO3_3;
			break;
		case 4:
			pPIN = (uint32_t *)&LPC_IOCON->PIO3_4;
			break;
		case 5:
			pPIN = (uint32_t *)&LPC_IOCON->PIO3_5;
			break;
		}
	}
	return pPIN;
}

/**
 * @}
 */


/* Public Functions ----------------------------------------------------------- */
/** @addtogroup PINSEL_Public_Functions
 * @{
 */

/*********************************************************************//**
 * @brief 		Setup the pin selection function
 * @param[in]	portnum PORT number,
 * 				should be one of the following:
 * 				- PINSEL_PORT_0	: Port 0
 * 				- PINSEL_PORT_1	: Port 1
 * 				- PINSEL_PORT_2	: Port 2
 * 				- PINSEL_PORT_3	: Port 3
 *
 * @param[in]	pinnum	Pin number,
 * 				should be one of the following:
				- PINSEL_PIN_0 : Pin 0
				- PINSEL_PIN_1 : Pin 1
				- PINSEL_PIN_2 : Pin 2
				- PINSEL_PIN_3 : Pin 3
				- PINSEL_PIN_4 : Pin 4
				- PINSEL_PIN_5 : Pin 5
				- PINSEL_PIN_6 : Pin 6
				- PINSEL_PIN_7 : Pin 7
				- PINSEL_PIN_8 : Pin 8
				- PINSEL_PIN_9 : Pin 9
				- PINSEL_PIN_10 : Pin 10
				- PINSEL_PIN_11 : Pin 11

 * @param[in] 	funcnum Function number,
 * 				should be one of the following:
 *				- PINSEL_FUNC_0 : default function
 *				- PINSEL_FUNC_1 : first alternate function
 *				- PINSEL_FUNC_2 : second alternate function
 *				- PINSEL_FUNC_3 : third alternate function
 *
 * @return 		None
 **********************************************************************/
void PINSEL_SetPinFunc ( uint8_t portnum, uint8_t pinnum, uint8_t funcnum)
{
	uint32_t *pPIN = NULL;
	pPIN = PIN_GetPointer(portnum, pinnum);
	*(uint32_t *)pPIN &= ~0x00000007;//Clear function bits
	*(uint32_t *)pPIN |= funcnum;
}


/*********************************************************************//**
 * @brief 		Setup resistor mode for each pin
 * @param[in]	portnum PORT number,
 * 				should be one of the following:
 * 				- PINSEL_PORT_0	: Port 0
 * 				- PINSEL_PORT_1	: Port 1
 * 				- PINSEL_PORT_2	: Port 2
 * 				- PINSEL_PORT_3	: Port 3
 * @param[in]	pinnum	Pin number,
 * 				should be one of the following:
				- PINSEL_PIN_0 : Pin 0
				- PINSEL_PIN_1 : Pin 1
				- PINSEL_PIN_2 : Pin 2
				- PINSEL_PIN_3 : Pin 3
				- PINSEL_PIN_4 : Pin 4
				- PINSEL_PIN_5 : Pin 5
				- PINSEL_PIN_6 : Pin 6
				- PINSEL_PIN_7 : Pin 7
				- PINSEL_PIN_8 : Pin 8
				- PINSEL_PIN_9 : Pin 9
				- PINSEL_PIN_10 : Pin 10
				- PINSEL_PIN_11 : Pin 11

 * @param[in] 	modenum: Mode number,
 * 				should be one of the following:
				- PINSEL_PINMODE_INACTIVE	: No pull-up or pull-down resistor
				- PINSEL_PINMODE_PULLUP	: Internal pull-up resistor
				- PINSEL_PINMODE_DOWN	: Internal pull-up resistor
				- PINSEL_PINMODE_REPEATER

 * @return 		None
 **********************************************************************/
void PINSEL_SetPinMode ( uint8_t portnum, uint8_t pinnum, uint8_t modenum)
{
	uint32_t *pPIN = NULL;
	pPIN = PIN_GetPointer(portnum, pinnum);
	*(uint32_t *)pPIN &= ~0x00000018;//Clear function bits
	*(uint32_t *)pPIN |= (modenum << 3);
}
/*********************************************************************//**
 * @brief 		Setup hysteresis for each pin
 * @param[in]	portnum PORT number,
 * 				should be one of the following:
 * 				- PINSEL_PORT_0	: Port 0
 * 				- PINSEL_PORT_1	: Port 1
 * 				- PINSEL_PORT_2	: Port 2
 * 				- PINSEL_PORT_3	: Port 3
 * @param[in]	pinnum	Pin number,
 * 				should be one of the following:
				- PINSEL_PIN_0 : Pin 0
				- PINSEL_PIN_1 : Pin 1
				- PINSEL_PIN_2 : Pin 2
				- PINSEL_PIN_3 : Pin 3
				- PINSEL_PIN_4 : Pin 4
				- PINSEL_PIN_5 : Pin 5
				- PINSEL_PIN_6 : Pin 6
				- PINSEL_PIN_7 : Pin 7
				- PINSEL_PIN_8 : Pin 8
				- PINSEL_PIN_9 : Pin 9
				- PINSEL_PIN_10 : Pin 10
				- PINSEL_PIN_11 : Pin 11

 * @param[in] 	hysnum: Mode number,
 * 				should be one of the following:
				- PINSEL_PINHYS_DISABLE : Disable
				- PINSEL_PINHYS_ENABLE : Enable

 * @return 		None
 **********************************************************************/
void PINSEL_SetHysMode ( uint8_t portnum, uint8_t pinnum, uint8_t hysnum)
{
	uint32_t *pPIN = NULL;
	pPIN = PIN_GetPointer(portnum, pinnum);
	if(hysnum == PINSEL_PINHYS_DISABLE){
	*(uint32_t *)pPIN &= ~(0x01 << 5);//Clear hys bits
	}
	else if(hysnum == PINSEL_PINHYS_ENABLE){
		*(uint32_t *)pPIN |= (0x01 << 5);
	}
}

/*********************************************************************//**
 * @brief 		Configure Pin corresponding to specified parameters passed
 * 				in the PinCfg
 * @param[in]	PinCfg	Pointer to a PINSEL_CFG_Type structure
 *                    that contains the configuration information for the
 *                    specified pin.
 * @return 		None
 **********************************************************************/
void PINSEL_ConfigPin(PINSEL_CFG_Type *PinCfg)
{
	PINSEL_SetPinFunc(PinCfg->Portnum, PinCfg->Pinnum, PinCfg->Funcnum);
	PINSEL_SetPinMode(PinCfg->Portnum, PinCfg->Pinnum, PinCfg->Funcmode);
	PINSEL_SetHysMode(PinCfg->Portnum, PinCfg->Pinnum, PinCfg->Hystereris);
}


/**
 * @}
 */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
