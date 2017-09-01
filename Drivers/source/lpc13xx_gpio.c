/**
 * @file	: lpc13xx_gpio.c
 * @brief	: Contains all functions support for GPIO firmware library on LPC13xx
 * @version	: 1.0
 * @date	: 30. Nov. 2009
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
/** @addtogroup GPIO
 * @{
 */

/* Includes ------------------------------------------------------------------- */
#include "lpc13xx_gpio.h"

/* Private Functions ---------------------------------------------------------- */
/** @defgroup GPIO_Private_Functions
 * @{
 */

/*********************************************************************//**
 * @brief		Get pointer to GPIO peripheral due to GPIO port
 * @param[in]	portNum		Port Number value, should be in range from 0 to 4.
 * @return		Pointer to GPIO peripheral
 **********************************************************************/
static LPC_GPIO_TypeDef *GPIO_GetPointer(uint8_t portNum)
{
	LPC_GPIO_TypeDef *pGPIO = NULL;

	switch (portNum) {
	case 0:
		pGPIO = LPC_GPIO0;
		break;
	case 1:
		pGPIO = LPC_GPIO1;
		break;
	case 2:
		pGPIO = LPC_GPIO2;
		break;
	case 3:
		pGPIO = LPC_GPIO3;
		break;
	default:
		break;
	}

	return pGPIO;
}
/**
 * @}
 */


/* Public Functions ----------------------------------------------------------- */
/** @addtogroup GPIO_Public_Functions
 * @{
 */


/*****************************************************************************
 * @brief		Initialize GPIO.
*****************************************************************************/
void GPIO_Init( void )
{
  /* Enable AHB clock to the GPIO domain. */
  LPC_SYSCON->SYSAHBCLKCTRL |= (1<<6);

}

/*********************************************************************//**
 * @brief		Set Direction for GPIO port.
 * @param[in]	portNum		Port Number value, should be in range from 0 to 4
 * @param[in]	bitValue	Value that contains all bits to set direction,
 * 							in range from 0 to 0xFFFFFFFF.
 * 							example: value 0x5 to set direction for bit 0 and bit 1.
 * @param[in]	dir			Direction value, should be:
 * 							- 0: Input.
 * 							- 1: Output.
 * Note: All remaining bits that are not activated in bitValue (value '0')
 * will not be effected by this function.
 **********************************************************************/
void GPIO_SetDir(uint8_t portNum, uint32_t bitValue, uint8_t dir)
{
	LPC_GPIO_TypeDef *pGPIO = GPIO_GetPointer(portNum);

	if (pGPIO != NULL) {
		// Enable Output
		if (dir) {
			pGPIO->DIR |= (0x1<<bitValue) ;
		}
		// Enable Input
		else {
			pGPIO->DIR &= ~(0x1<<bitValue);
		}
	}
}


/*********************************************************************//**
 * @brief		Set Value for bits that have output direction on GPIO port.
 * @param[in]	portNum		Port number value, should be in range from 0 to 3
 * @param[in]	bitValue	Value that contains all bits on GPIO to set,
 * 							in range from 0 to 11.
 **********************************************************************/
void GPIO_SetValue(uint8_t portNum, uint32_t bitValue)
{
	LPC_GPIO_TypeDef *pGPIO = GPIO_GetPointer(portNum);

	if (pGPIO != NULL) {
		pGPIO->DATA |= (0x1<<bitValue);

	}
}
/*********************************************************************//**
 * @brief		Clear Value for bits that have output direction on GPIO port.
 * @param[in]	portNum		Port number value, should be in range from 0 to 3
 * @param[in]	bitValue	Value that contains all bits on GPIO to clear,
 * 							in range from 0 to 11.
 **********************************************************************/
void GPIO_ClearValue(uint8_t portNum, uint32_t bitValue)
{
	LPC_GPIO_TypeDef *pGPIO = GPIO_GetPointer(portNum);

	if (pGPIO != NULL) {
		pGPIO->DATA &= ~(0x1<<bitValue);
	}
}

/*********************************************************************//**
 * @brief		Read Current state on port pin that have input direction of GPIO
 * @param[in]	portNum		Port number to read value, in range from 0 to 3
 * @return		Current value of GPIO port.
 **********************************************************************/
uint32_t GPIO_ReadValue(uint8_t portNum)
{
	LPC_GPIO_TypeDef *pGPIO = GPIO_GetPointer(portNum);

	if (pGPIO != NULL) {
		return pGPIO->DATA;
	}
	return (0);
}

/*****************************************************************************
 * @brief		Set GPIO interrupt.
 * @param[in]	portNum		Port Number value, should be in range from 0 to 3
 * @param[in]	bitValue	Value that contains all bits on GPIO to clear,
 * 							in range from 0 to 11.
 * @param[in]	sense		0: interrupt on pin PIOn_x is configured as edge sensitive
 * 							      1: interrupt on pin PIOn_x is configured as level sensitive
 * @param[in]	single	0: interrupt on pin PIOn_x is controlled through register GPIOnIEV
 * 							      1: interrupt edges on pin PIOn_x trigger an interrupt
 * @param[in]	event		0: falling adges or LOW level on pin PIOn_x
 * 							      1: rising edges or HIGH level on pin PIOn_x
*****************************************************************************/
void GPIO_SetInterrupt( uint32_t portNum, uint32_t bitValue, uint32_t sense,
			uint32_t single, uint32_t event )
{
  switch ( portNum )
  {
	case PORT0:
	  if ( sense == 0 )
	  {
		  LPC_GPIO0->IS &= ~(0x1<<bitValue);
  		/* single or double only applies when sense is 0(edge trigger). */
  		if ( single == 0 )
  		  LPC_GPIO0->IBE &= ~(0x1<<bitValue);
  		else
  		  LPC_GPIO0->IBE |= (0x1<<bitValue);
	  }
	  else
	  	LPC_GPIO0->IS |= (0x1<<bitValue);
	  if ( event == 0 )
		  LPC_GPIO0->IEV &= ~(0x1<<bitValue);
	  else
		  LPC_GPIO0->IEV |= (0x1<<bitValue);
	  break;
 	case PORT1:
	  if ( sense == 0 )
	  {
  		LPC_GPIO1->IS &= ~(0x1<<bitValue);
  		/* single or double only applies when sense is 0(edge trigger). */
  		if ( single == 0 )
  		  LPC_GPIO1->IBE &= ~(0x1<<bitValue);
  		else
  		  LPC_GPIO1->IBE |= (0x1<<bitValue);
	  }
	  else
	  	LPC_GPIO1->IS |= (0x1<<bitValue);
	  if ( event == 0 )
		  LPC_GPIO1->IEV &= ~(0x1<<bitValue);
	  else
		  LPC_GPIO1->IEV |= (0x1<<bitValue);
	  break;
	case PORT2:
	  if ( sense == 0 )
	  {
  		LPC_GPIO2->IS &= ~(0x1<<bitValue);
  		/* single or double only applies when sense is 0(edge trigger). */
  		if ( single == 0 )
  		  LPC_GPIO2->IBE &= ~(0x1<<bitValue);
  		else
  		  LPC_GPIO2->IBE |= (0x1<<bitValue);
	  }
	  else
	  	LPC_GPIO2->IS |= (0x1<<bitValue);
	  if ( event == 0 )
		  LPC_GPIO2->IEV &= ~(0x1<<bitValue);
	  else
		  LPC_GPIO2->IEV |= (0x1<<bitValue);
	  break;
	case PORT3:
	  if ( sense == 0 )
	  {
  		LPC_GPIO3->IS &= ~(0x1<<bitValue);
  		/* single or double only applies when sense is 0(edge trigger). */
  		if ( single == 0 )
  		  LPC_GPIO3->IBE &= ~(0x1<<bitValue);
  		else
  		  LPC_GPIO3->IBE |= (0x1<<bitValue);
	  }
	  else
	  	LPC_GPIO3->IS |= (0x1<<bitValue);
	  if ( event == 0 )
		  LPC_GPIO3->IEV &= ~(0x1<<bitValue);
	  else
		  LPC_GPIO3->IEV |= (0x1<<bitValue);
	  break;
	default:
	  break;
  }
  return;
}

/*****************************************************************************
 * @brief		Enable GPIO interrupt
 * @param[in]	portNum		Port number value, should be in range from 0 to 3
 * @param[in]	bitValue	Value that contains all bits on GPIO to clear,
 * 							in range from 0 to 11.

*****************************************************************************/
void GPIO_IntEnable( uint32_t portNum, uint32_t bitValue )
{
  switch ( portNum )
  {
	case PORT0:
	  LPC_GPIO0->IE |= (0x1<<bitValue);
	break;
 	case PORT1:
	  LPC_GPIO1->IE |= (0x1<<bitValue);
	break;
	case PORT2:
	  LPC_GPIO2->IE |= (0x1<<bitValue);
	break;
	case PORT3:
	  LPC_GPIO3->IE |= (0x1<<bitValue);
	break;
	default:
	  break;
  }
  return;
}

/*****************************************************************************
 * @brief		Disable GPIO interrupt
 * @param[in]	portNum		Port number value, should be in range from 0 to 3
 * @param[in]	bitValue	Value that contains all bits on GPIO to clear,
 * 							in range from 0 to 11.
*****************************************************************************/
void GPIO_IntDisable( uint32_t portNum, uint32_t bitValue )
{
  switch ( portNum )
  {
	case PORT0:
	  LPC_GPIO0->IE &= ~(0x1<<bitValue);
	break;
 	case PORT1:
	  LPC_GPIO1->IE &= ~(0x1<<bitValue);
	break;
	case PORT2:
	  LPC_GPIO2->IE &= ~(0x1<<bitValue);
	break;
	case PORT3:
	  LPC_GPIO3->IE &= ~(0x1<<bitValue);
	break;
	default:
	  break;
  }
  return;
}

/*****************************************************************************
 * @brief		Get GPIO interrupt status
 * @param[in]	portNum		Port number value, should be in range from 0 to 3
 * @param[in]	bitValue	Value that contains all bits on GPIO to clear,
 * 							in range from 0 to 11.
 *
 * @return	 	status
*****************************************************************************/
uint32_t GPIO_IntStatus( uint32_t portNum, uint32_t bitValue )
{
  uint32_t regVal = 0;

  switch ( portNum )
  {
	case PORT0:
	  if ( LPC_GPIO0->MIS & (0x1<<bitValue) )
		regVal = 1;
	break;
 	case PORT1:
	  if ( LPC_GPIO1->MIS & (0x1<<bitValue) )
		regVal = 1;
	break;
	case PORT2:
	  if ( LPC_GPIO2->MIS & (0x1<<bitValue) )
		regVal = 1;
	break;
	case PORT3:
	  if ( LPC_GPIO3->MIS & (0x1<<bitValue) )
		regVal = 1;
	break;
	default:
	  break;
  }
  return ( regVal );
}

/*****************************************************************************
 * @brief		Clear GPIO interrupt status
 * @param[in]	portNum		Port number value, should be in range from 0 to 3
 * @param[in]	bitValue	Value that contains all bits on GPIO to clear,
 * 							in range from 0 to 11.
*****************************************************************************/
void GPIO_IntClear( uint32_t portNum, uint32_t bitValue )
{
  switch ( portNum )
  {
	case PORT0:
	  LPC_GPIO0->IC |= (0x1<<bitValue);
	break;
 	case PORT1:
	  LPC_GPIO1->IC |= (0x1<<bitValue);
	break;
	case PORT2:
	  LPC_GPIO2->IC |= (0x1<<bitValue);
	break;
	case PORT3:
	  LPC_GPIO3->IC |= (0x1<<bitValue);
	break;
	default:
	  break;
  }
  return;
}

/**
 * @}
 */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
