/**
 * @file	: lpc13xx_adc.c
 * @brief	: Contains all functions support for ADC firmware library on LPC13xx
 * @version	: 1.0
 * @date	: 20. Nov. 2009
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
/** @addtogroup ADC
 * @{
 */

/* Includes ------------------------------------------------------------------- */
#include "lpc13xx_adc.h"
#include "lpc_types.h"
#include "lpc13xx_clkpwr.h"


/* Public Functions ----------------------------------------------------------- */
/** @addtogroup ADC_Public_Functions
 * @{
 */


/***************************************************************************//**
** @brief 		Initialize ADC
** @param[in]	ADC_Clk ADC clock rate
** @param[in]	res 	ADC resolution, from 3 to 10
** @return		None
*****************************************************************************/
void ADC_Init( uint32_t ADC_Clk, uint32_t res )
{
	uint32_t tmp, temp;

	/* Disable Power down bit to the ADC block. */
	CLKPWR_ConfigPowerDown(CLKPWR_PDRUNCFG_ADC, DISABLE);

	/* Enable AHB clock to the ADC. */
	CLKPWR_ConfigAHBClock(CLKPWR_AHBCLKCTRL_ADC, ENABLE);

	//Enable PDN bit
	tmp = ADC_CR_PDN;
	// Set clock frequency
	temp = SystemCoreClock / ADC_Clk - 1 ;
	tmp |=  ADC_CR_CLKDIV(temp);
	// Add clocks/bit resolution
	tmp |= ADC_CR_CLKS(res);
	LPC_ADC->CR = tmp;

}


/**
* @brief 		Close ADC
* @param[in]	None
* @return 		None
*
 *********************************************************************/
void ADC_DeInit(void)
{
	// Turn on power and clock
	CLKPWR_ConfigAHBClock(CLKPWR_AHBCLKCTRL_ADC, DISABLE);
}


/**
* @brief 		Set start mode for ADC
* @param[in]	start_mode Start mode choose one of modes in
* 							'ADC_START_OPT' enumeration type definition
* @return 		None
 *********************************************************************/
void ADC_StartCmd(uint8_t start_mode)
{

	LPC_ADC->CR &= ~ADC_CR_START_MASK;
	LPC_ADC->CR |=ADC_CR_START_MODE_SEL((uint32_t)start_mode);
}


/**
* @brief 		ADC Burst mode setting
* @param[in]	NewState
* 				-	1: Set Burst mode
* 				-	0: reset Burst mode
* @return 		None
*
 *********************************************************************/
void ADC_BurstCmd(FunctionalState NewState)
{

	LPC_ADC->CR &= ~ADC_CR_BURST;
	if (NewState){
		LPC_ADC->CR |= ADC_CR_BURST;
	}

}

/**
* @brief 		Set AD conversion in power mode
* @param[in]	NewState
* 				-	1: AD converter is optional
* 				-	0: AD Converter is in power down mode
* @return 		None
*
 *********************************************************************/
void ADC_PowerdownCmd(FunctionalState NewState)
{

	LPC_ADC->CR &= ~ADC_CR_PDN;
	if (NewState){
		LPC_ADC->CR |= ADC_CR_PDN;
	}
}

/**
* @brief 		Set Edge start configuration
* @param[in]	EdgeOption is ADC_START_ON_RISING and ADC_START_ON_FALLING
* 					- 0:	ADC_START_ON_RISING
* 					- 1:	ADC_START_ON_FALLING
*
* @return 		None
*
 *********************************************************************/
void ADC_EdgeStartConfig(uint8_t EdgeOption)
{

	LPC_ADC->CR &= ~ADC_CR_EDGE;
	if (EdgeOption){
		LPC_ADC->CR |= ADC_CR_EDGE;
	}
}

/**
* @brief 		ADC interrupt configuration
* @param[in]	Channel		ADC channel
* @param[in]	NewState:
* 					- SET : enable ADC interrupt
* 					- RESET: disable ADC interrupt
*
* @return 		None
*
 *********************************************************************/
void ADC_IntConfig (uint32_t Channel, FunctionalState NewState)
{
	if (NewState){
		LPC_ADC->INTEN |= ADC_INTEN_CH(Channel);
	} else {
		LPC_ADC->INTEN &= ~ADC_INTEN_CH(Channel);
	}
}

/**
* @brief 		Enable/Disable ADC channel number
* @param[in]	Channel channel number
* @param[in]	NewState Enable or Disable
*
* @return 		None
*
 *********************************************************************/
void ADC_ChannelCmd (uint8_t Channel, FunctionalState NewState)
{

	if (NewState == ENABLE) {
		LPC_ADC->CR |= ADC_CR_CH_SEL(Channel);
	} else {
		LPC_ADC->CR &= ~ADC_CR_CH_SEL(Channel);
	}
}

/**
* @brief 		Get ADC result
* @param[in]	channel channel number
* @return 		Data conversion
*
 *********************************************************************/
uint16_t ADC_ChannelGetData(uint8_t channel)
{
	uint32_t adc_value;

	adc_value = *(uint32_t *) ((&LPC_ADC->DR[0]) + (channel));
	return ADC_DR_RESULT(adc_value);
}

/**
* @brief 		Get ADC Chanel status from ADC data register
* @param[in]	channel channel number
* @param[in]  	StatusType
*              		 	0:Burst status
*               		1:Done 	status
* @return 		SET / RESET
*
 *********************************************************************/
FlagStatus ADC_ChannelGetStatus(uint8_t channel, uint32_t StatusType)
{
	uint32_t temp;

	temp =  *(uint32_t *) ((&LPC_ADC->DR[0]) + channel);
	if (StatusType) {
		temp &= ADC_DR_DONE_FLAG;
	}else{
		temp &= ADC_DR_OVERRUN_FLAG;
	}

	if (temp) {
		return SET;
	} else {
		return RESET;
	}


}

/**
* @brief 		Get ADC Data from AD Global register
* @param[in]	channel channel number
* @return 		Result of conversion
*
 *********************************************************************/
uint16_t ADC_GlobalGetData(uint8_t channel)
{
	return (uint16_t)(ADC_GDR_RESULT(LPC_ADC->GDR));
}

/**
* @brief 		Get ADC Chanel status from AD global data register
* @param[in]  	StatusType
*              		 	0:Burst status
*               		1:Done 	status
* @return 		SET / RESET
*
 *********************************************************************/
FlagStatus	ADC_GlobalGetStatus(uint32_t StatusType)
{
	uint32_t temp;

	temp =  LPC_ADC->GDR;
	if (StatusType){
		temp &= ADC_DR_DONE_FLAG;
	}else{
		temp &= ADC_DR_OVERRUN_FLAG;
	}
	if (temp){
		return SET;
	}else{
		return RESET;
	}
}

/**
 * @}
 */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */

