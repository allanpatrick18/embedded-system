/***********************************************************************//**
 * @file	: lpc13xx_adc.h
 * @brief	: Contains all macro definitions and function prototypes
 * 				support for ADC firmware library on LPC13xx
 * @version	: 1.0
 * @date	: 3. April. 2009
 * @author	: NgaDinh
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
/** @defgroup ADC
 * @ingroup LPC1300CMSIS_FwLib_Drivers
 * @{
 */

#ifndef LPC13XX_ADC_H_
#define LPC13XX_ADC_H_

/* Includes ------------------------------------------------------------------- */
#include "LPC13xx.h"
#include "lpc_types.h"


#ifdef __cplusplus
extern "C"
{
#endif


/* Private Macros ------------------------------------------------------------- */
/** @defgroup ADC_Private_Macros
 * @{
 */


/** @defgroup ADC_REGISTER_BIT_DEFINITIONS
 * @{
 */

/*********************************************************************//**
 * Macro defines for ADC  control register
 **********************************************************************/
/**  Selects which of the AD0.0:7 pins is (are) to be sampled and converted */
#define ADC_CR_CH_SEL(n)	((1UL << n))
/**  The APB clock (PCLK) is divided by (this value plus one)
* to produce the clock for the A/D */
#define ADC_CR_CLKDIV(n)	((n<<8))
/**  Repeated conversions A/D enable bit */
#define ADC_CR_BURST		((1UL<<16))
/** This field selects the number of clocks used for each conversion in
 * Burst mode, and the number of bits of accuracy of the result in the
 * LS bits of ADDR, between 11 clocks (10 bits) and 4 clocks (3 bits) */
#define ADC_CR_CLKS(n)		(((10-n)&0x07)<<17)
/**  ADC convert in power down mode */
#define ADC_CR_PDN			((1UL<<21))
/**  Start mask bits */
#define ADC_CR_START_MASK	((7UL<<24))
/**  Select Start Mode */
#define ADC_CR_START_MODE_SEL(SEL)	((SEL<<24))
/**  Start conversion now */
#define ADC_CR_START_NOW	((1UL<<24))
/**  Start conversion when the edge selected by bit 27 occurs on PIO0_2/SSEL/CT16B0_CAP0 */
#define ADC_CR_START_CT16B0_CAP0	((2UL<<24))
/** Start conversion when the edge selected by bit 27 occurs on PIO1_5/DIR/CT32B0_CAP0 */
#define ADC_CR_START_CT32B0_CAP0	((3UL<<24))
/**  Start conversion when the edge selected by bit 27 occurs on CT32B0_MAT0 */
#define ADC_CR_START_CT32B0_MAT0	((4UL<<24))
/**  Start conversion when the edge selected by bit 27 occurs on CT32B0_MAT1 */
#define ADC_CR_START_CT32B0_MAT1	((5UL<<24))
/**  Start conversion when the edge selected by bit 27 occurs on CT16B0_MAT0 */
#define ADC_CR_START_CT16B0_MAT0	((6UL<<24))
/**  Start conversion when the edge selected by bit 27 occurs on CT16B0_MAT1 */
#define ADC_CR_START_CT16B0_MAT1	((7UL<<24))
/**  Start conversion on a falling edge on the selected CAP/MAT signal */
#define ADC_CR_EDGE			((1UL<<27))

/*********************************************************************//**
 * Macro defines for ADC Global Data register
 **********************************************************************/
/** When DONE is 1, this field contains result value of ADC conversion */
#define ADC_GDR_RESULT(n)		(((n>>6)&0x3FF))
/** These bits contain the channel from which the LS bits were converted */
#define ADC_GDR_CH(n)			(((n>>24)&0x7))
/** This bit is 1 in burst mode if the results of one or
 * more conversions was (were) lost */
#define ADC_GDR_OVERRUN_FLAG	((1UL<<30))
/** This bit is set to 1 when an A/D conversion completes */
#define ADC_GDR_DONE_FLAG		((1UL<<31))

/*********************************************************************//**
 * Macro defines for ADC Interrupt register
 **********************************************************************/
/** These bits allow control over which A/D channels generate
 * interrupts for conversion completion */
#define ADC_INTEN_CH(n)			((1UL<<n))
/** When 1, enables the global DONE flag in ADDR to generate an interrupt */
#define ADC_INTEN_GLOBAL		((1UL<<8))

/*********************************************************************//**
 * Macro defines for ADC Data register
 **********************************************************************/
/** When DONE is 1, this field contains result value of ADC conversion */
#define ADC_DR_RESULT(n)		(((n>>6)&0x3FF))
/** These bits mirror the OVERRRUN status flags that appear in the
 * result register for each A/D channel */
#define ADC_DR_OVERRUN_FLAG		((1UL<<30))
/** This bit is set to 1 when an A/D conversion completes. It is cleared
 * when this register is read */
#define ADC_DR_DONE_FLAG		((1UL<<31))

/*********************************************************************//**
 * Macro defines for ADC Status register
**********************************************************************/
/** These bits mirror the DONE status flags that appear in the result
 * register for each A/D channel */
#define ADC_STAT_CH_DONE_FLAG(n)		((n&0xFF))
/** These bits mirror the OVERRRUN status flags that appear in the
 * result register for each A/D channel */
#define ADC_STAT_CH_OVERRUN_FLAG(n)		(((n>>8)&0xFF))
/** This bit is the A/D interrupt flag */
#define ADC_STAT_INT_FLAG				((1UL<<16))


/**
 * @}
 */

/**
 * @}
 */


/* Public Types --------------------------------------------------------------- */
/** @defgroup ADC_Public_Types
 * @{
 */

/*********************************************************************//**
 * @brief ADC enumeration
 **********************************************************************/
/** @brief Channel Selection */
typedef enum
{
	ADC_CHANNEL_0  = 0, /*!<  Channel 0 */
	ADC_CHANNEL_1,		/*!<  Channel 1 */
	ADC_CHANNEL_2,		/*!<  Channel 2 */
	ADC_CHANNEL_3,		/*!<  Channel 3 */
	ADC_CHANNEL_4,		/*!<  Channel 4 */
	ADC_CHANNEL_5,		/*!<  Channel 5 */
	ADC_CHANNEL_6,		/*!<  Channel 6 */
	ADC_CHANNEL_7		/*!<  Channel 7 */
}ADC_CHANNEL_SELECTION;



/** @brief Type of start option */

/** @brief Type of start option */

typedef enum
{
	ADC_START_CONTINUOUS =0,	/*!< Continuous mode */
	ADC_START_NOW,				/*!< Start conversion now */
	ADC_START_ON_CT16B0_CAP0,			/*!< Start conversion when the edge selected
								 		* by bit 27 occurs on PIO0_2/SSEL/CT16B0_CAP0 */
	ADC_START_ON_CT32B0_CAP0,			/*!< Start conversion when the edge selected
								 		* by bit 27 occurs on PIO1_5/DIR/CT32B0_CAP0 */
	ADC_START_ON_CT32B0_MAT0,			/*!< Start conversion when the edge selected
								 		* by bit 27 occurs on CT32B0_MAT0 */
	ADC_START_ON_CT32B0_MAT1,			/*!< Start conversion when the edge selected
								 		* by bit 27 occurs on CT32B0_MAT1 */
	ADC_START_ON_CT16B0_MAT0,			/*!< Start conversion when the edge selected
								  		* by bit 27 occurs on CT16B0_MAT0 */
	ADC_START_ON_CT16B0_MAT1			/*!< Start conversion when the edge selected
								  		* by bit 27 occurs on CT16B0_MAT1 */
} ADC_START_OPT;


/** @brief Type of edge when start conversion on the selected CAP/MAT signal */

typedef enum
{
	ADC_START_ON_RISING = 0,	/*!< Start conversion on a rising edge
								*on the selected CAP/MAT signal */
	ADC_START_ON_FALLING		/*!< Start conversion on a falling edge
								*on the selected CAP/MAT signal */
} ADC_START_ON_EDGE_OPT;


/** @brief ADC Data  status */
typedef enum
{
	ADC_DATA_OVERRUN = 0,		/* Overrun bit */
	ADC_DATA_DONE		 		/* Done bit */
}ADC_DATA_STATUS;


/**
 * @}
 */



/* Public Functions ----------------------------------------------------------- */

/** @defgroup ADC_Public_Functions
 * @{
 */

void ADC_Init( uint32_t ADC_Clk, uint32_t res );
void ADC_DeInit(void);
void ADC_StartCmd(uint8_t start_mode);
void ADC_BurstCmd(FunctionalState NewState);
void ADC_PowerdownCmd(FunctionalState NewState);
void ADC_EdgeStartConfig(uint8_t EdgeOption);
void ADC_IntConfig (uint32_t Channel, FunctionalState NewState);
void ADC_ChannelCmd (uint8_t Channel, FunctionalState NewState);
uint16_t ADC_ChannelGetData(uint8_t channel);
FlagStatus ADC_ChannelGetStatus(uint8_t channel, uint32_t StatusType);
uint16_t ADC_GlobalGetData(uint8_t channel);
FlagStatus	ADC_GlobalGetStatus(uint32_t StatusType);


/**
 * @}
 */


#ifdef __cplusplus
}
#endif


#endif /* LPC13XX_ADC_H_ */

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
