/**
 * @file	: lpc13xx_timer.c
 * @brief	: Contains all functions support for timer firmware library on LPC13xx
 * @version	: 1.0
 * @date	: 14. April. 2009
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
/** @addtogroup TIM
 * @{
 */

/* Includes ------------------------------------------------------------------- */
#include "LPC13xx.h"
#include "lpc13xx_timer.h"
#include "lpc13xx_clkpwr.h"

/* Private Functions ---------------------------------------------------------- */
/** @defgroup TIM_Private_Functions
 * @{
 */
/*********************************************************************//**
 * @brief 		Convert a time to a timer count value
 * @param[in]	usec Time in microseconds
 * @return 		The number of required clock ticks to give the time delay
 **********************************************************************/
uint32_t TIM_ConverUSecToVal (uint32_t usec)
{
	uint64_t clkdlycnt;

	// Get Pclock of timer
	clkdlycnt = (uint64_t) SystemCoreClock;

	clkdlycnt = (clkdlycnt * usec) / 1000000;
	return (uint32_t) clkdlycnt;
}
/**
 * @}
 */
/* Public Functions ----------------------------------------------------------- */
/** @addtogroup TIM_Public_Functions
 * @{
 */

/*********************************************************************//**
 * @brief 		Get Interrupt Status
 * @param[in]	TIMx Timer selection, should be LPC_TMR16B0, LPC_TMR16B1, LPC_TMR32B0, LPC_TMR32B1
 * @param[in]	IntFlag
 * @return 		FlagStatus
 * 				- SET : interrupt
 * 				- RESET : no interrupt
 **********************************************************************/
FlagStatus TIM_GetIntStatus(LPC_TMR_TypeDef *TIMx, uint8_t IntFlag)
{
	uint8_t temp = (TIMx->IR)& TIM_IR_CLR(IntFlag);
	if (temp)
		return SET;

	return RESET;

}
/*********************************************************************//**
 * @brief 		Get Capture Interrupt Status
 * @param[in]	TIMx Timer selection, should be TIM0, TIM1, TIM2, TIM3
 * @param[in]	IntFlag
 * @return 		FlagStatus
 * 				- SET : interrupt
 * 				- RESET : no interrupt
 **********************************************************************/
FlagStatus TIM_GetIntCaptureStatus(LPC_TMR_TypeDef *TIMx, uint8_t IntFlag)
{
	uint8_t temp = (TIMx->IR) & (1<<(4+IntFlag));
	if(temp)
		return SET;
	return RESET;
}
/*********************************************************************//**
 * @brief 		Clear Interrupt pending
 * @param[in]	TIMx Timer selection, should be TIM0, TIM1, TIM2, TIM3
 * @param[in]	IntFlag should be in TIM_INT_TYPE enum
 * @return 		None
 **********************************************************************/
void TIM_ClearIntPending(LPC_TMR_TypeDef *TIMx, uint8_t IntFlag)
{
	TIMx->IR |= TIM_IR_CLR(IntFlag);
}

/*********************************************************************//**
 * @brief 		Clear Capture Interrupt pending
 * @param[in]	TIMx Timer selection, should be TIM0, TIM1, TIM2, TIM3
 * @param[in]	IntFlag should be in TIM_INT_TYPE enum
 * @return 		None
 **********************************************************************/
void TIM_ClearIntCapturePending(LPC_TMR_TypeDef *TIMx, uint8_t IntFlag)
{
	TIMx->IR |= (1<<(4+IntFlag));
}

/*********************************************************************//**
* @brief 		Configuration for Timer at initial time
* @param[in] 	TimerCounterMode Should be :
* 					- PrescaleOption = TC_PRESCALE_USVAL,
* 					- PrescaleValue = 1
* 				Counter mode with
* 					- Caption channel 0
* @param[in] 	TIM_ConfigStruct pointer to TIM_TIMERCFG_Type or
* 				TIM_COUNTERCFG_Type
* @return 		None
 **********************************************************************/
void TIM_ConfigStructInit(uint8_t TimerCounterMode, void *TIM_ConfigStruct)
{
	if (TimerCounterMode == TIM_TIMER_MODE )
	{
		TIM_TIMERCFG_Type * pTimeCfg = (TIM_TIMERCFG_Type *)TIM_ConfigStruct;
		pTimeCfg->PrescaleOption = TIM_PRESCALE_USVAL;
		pTimeCfg->PrescaleValue = 1;
	}
	else
	{
		TIM_COUNTERCFG_Type * pCounterCfg = (TIM_COUNTERCFG_Type *)TIM_ConfigStruct;
		pCounterCfg->CountInputSelect = TIM_COUNTER_INCAP0;

	}
}

/*********************************************************************//**
 * @brief 		Initial Timer/Counter device
 * 				 	Set Clock frequency for Timer
 * 					Set initial configuration for Timer
 * @param[in]	TIMx  Timer selection, should be LPC_TMR16B0, LPC_TMR16B1, LPC_TMR32B0, LPC_TMR32B1
 * @param[in]	TimerCounterMode TIM_MODE_OPT
 * @param[in]	TIM_ConfigStruct pointer to TIM_TIMERCFG_Type
 * 				that contains the configuration information for the
 *                    specified Timer peripheral.
 * @return 		None
 **********************************************************************/
void TIM_Init(LPC_TMR_TypeDef *TIMx, uint8_t TimerCounterMode, void *TIM_ConfigStruct)
{
	TIM_TIMERCFG_Type *pTimeCfg;
	TIM_COUNTERCFG_Type *pCounterCfg;
	//set clock enable
  if ( TIMx == LPC_TMR16B0 )
  {
	CLKPWR_ConfigAHBClock(CLKPWR_AHBCLKCTRL_CT16B0, ENABLE);
  }
  else if ( TIMx == LPC_TMR16B1 )
  {
	CLKPWR_ConfigAHBClock(CLKPWR_AHBCLKCTRL_CT16B1, ENABLE);
  }
  else if ( TIMx == LPC_TMR32B0 )
  {

	CLKPWR_ConfigAHBClock(CLKPWR_AHBCLKCTRL_CT32B0, ENABLE);
  }
  else if (TIMx == LPC_TMR32B1)
  {
	CLKPWR_ConfigAHBClock(CLKPWR_AHBCLKCTRL_CT32B1, ENABLE);
  }

	TIMx->CCR &= ~TIM_CTCR_MODE_MASK;
	TIMx->CCR |= TIM_TIMER_MODE;

	TIMx->TC =0;
	TIMx->PC =0;
	TIMx->PR =0;
	if (TimerCounterMode == TIM_TIMER_MODE )
	{
		pTimeCfg = (TIM_TIMERCFG_Type *)TIM_ConfigStruct;
		if (pTimeCfg->PrescaleOption  == TIM_PRESCALE_TICKVAL)
		{
			TIMx->PR   = pTimeCfg->PrescaleValue -1  ;
		}
		else
		{
			TIMx->PR   = TIM_ConverUSecToVal (pTimeCfg->PrescaleValue)-1;
		}
	}
	else
	{

		pCounterCfg = (TIM_COUNTERCFG_Type *)TIM_ConfigStruct;
		TIMx->CCR  &= ~TIM_CTCR_INPUT_MASK;
	}

	// Clear interrupt pending
	TIMx->IR = 0xFFFFFFFF;

}

/*********************************************************************//**
 * @brief 		Close Timer/Counter device
 * @param[in]	TIMx  Pointer to timer device
 * @return 		None
 **********************************************************************/
void TIM_DeInit(LPC_TMR_TypeDef *TIMx)
{
	// Disable timer/counter
	  TIMx->TCR = 0x00;
	  if(TIMx == LPC_TMR16B0)
	  {
	  	CLKPWR_ConfigAHBClock(CLKPWR_AHBCLKCTRL_CT16B0, DISABLE);
	  }
	  else if(TIMx == LPC_TMR16B1)
	  {
	  	CLKPWR_ConfigAHBClock(CLKPWR_AHBCLKCTRL_CT16B1, DISABLE);
	  }
	  else if(TIMx == LPC_TMR32B0)
	  {
	  	CLKPWR_ConfigAHBClock(CLKPWR_AHBCLKCTRL_CT32B0, DISABLE);
	  }
	  else if(TIMx == LPC_TMR32B1)
	  {
	  	CLKPWR_ConfigAHBClock(CLKPWR_AHBCLKCTRL_CT32B1, DISABLE);
	  }
}
/*********************************************************************//**
 * @brief	 	Start/Stop Timer/Counter device
 * @param[in]	TIMx Pointer to timer device
 * @param[in]	NewState
 * 				-	ENABLE  : set timer enable
 * 				-	DISABLE : disable timer
 * @return 		None
 **********************************************************************/
void TIM_Cmd(LPC_TMR_TypeDef *TIMx, FunctionalState NewState)
{
	if (NewState == ENABLE)
	{
		TIMx->TCR	|=  TIM_ENABLE;
	}
	else
	{
		TIMx->TCR &= ~TIM_ENABLE;
	}
}

/*********************************************************************//**
 * @brief 		Reset Timer/Counter device,
 * 					Make TC and PC are synchronously reset on the next
 * 					positive edge of PCLK
 * @param[in]	TIMx Pointer to timer device
 * @return 		None
 **********************************************************************/
void TIM_ResetCounter(LPC_TMR_TypeDef *TIMx)
{
	TIMx->TCR |= TIM_RESET;
	TIMx->TCR &= ~TIM_RESET;
}

/*********************************************************************//**
 * @brief 		Configuration for Match register
 * @param[in]	TIMx Pointer to timer device
 * @param[in]   TIM_MatchConfigStruct Pointer to TIM_MATCHCFG_Type
 * 					- MatchChannel : choose channel 0 or 1
 * 					- IntOnMatch	 : if SET, interrupt will be generated when MRxx match
 * 									the value in TC
 * 					- StopOnMatch	 : if SET, TC and PC will be stopped whenM Rxx match
 * 									the value in TC
 * 					- ResetOnMatch : if SET, Reset on MR0 when MRxx match
 * 									the value in TC
 * 					-ExtMatchOutputType: Select output for external match
 * 						 +	 0:	Do nothing for external output pin if match
 *						 +   1:	Force external output pin to low if match
 *						 + 	 2: Force external output pin to high if match
 *						 + 	 3: Toggle external output pin if match
 *					MatchValue: Set the value to be compared with TC value
 * @return 		None
 **********************************************************************/
void TIM_ConfigMatch(LPC_TMR_TypeDef *TIMx, TIM_MATCHCFG_Type *TIM_MatchConfigStruct)
{
	switch(TIM_MatchConfigStruct->MatchChannel)
	{
	case 0:
		TIMx->MR0 = TIM_MatchConfigStruct->MatchValue;
		break;
	case 1:
		TIMx->MR1 = TIM_MatchConfigStruct->MatchValue;
		break;
	}
	//interrupt on MRn
	TIMx->MCR &=~TIM_MCR_CHANNEL_MASKBIT(TIM_MatchConfigStruct->MatchChannel);

	if (TIM_MatchConfigStruct->IntOnMatch)
		TIMx->MCR |= TIM_INT_ON_MATCH(TIM_MatchConfigStruct->MatchChannel);

	//reset on MRn
	if (TIM_MatchConfigStruct->ResetOnMatch)
		TIMx->MCR |= TIM_RESET_ON_MATCH(TIM_MatchConfigStruct->MatchChannel);

	//stop on MRn
	if (TIM_MatchConfigStruct->StopOnMatch)
		TIMx->MCR |= TIM_STOP_ON_MATCH(TIM_MatchConfigStruct->MatchChannel);
//	TIMx->MCR = 0x02;

	// match output type

	TIMx->EMR 	&= ~TIM_EM_MASK(TIM_MatchConfigStruct->MatchChannel);
	TIMx->EMR    = TIM_EM_SET(TIM_MatchConfigStruct->MatchChannel,TIM_MatchConfigStruct->ExtMatchOutputType);
}
/*********************************************************************//**
 * @brief 		Configuration for Capture register
 * @param[in]	TIMx Pointer to timer device
 * 					- CaptureChannel: set the channel to capture data
 * 					- RisingEdge    : if SET, Capture at rising edge
 * 					- FallingEdge	: if SET, Capture at falling edge
 * 					- IntOnCaption  : if SET, Capture generate interrupt
 * @param[in]   TIM_CaptureConfigStruct	Pointer to TIM_CAPTURECFG_Type
 * @return 		None
 **********************************************************************/
void TIM_ConfigCapture(LPC_TMR_TypeDef *TIMx, TIM_CAPTURECFG_Type *TIM_CaptureConfigStruct)
{
	TIMx->CCR &= ~TIM_CCR_CHANNEL_MASKBIT(TIM_CaptureConfigStruct->CaptureChannel);

	if (TIM_CaptureConfigStruct->RisingEdge)
		TIMx->CCR |= TIM_CAP_RISING(TIM_CaptureConfigStruct->CaptureChannel);

	if (TIM_CaptureConfigStruct->FallingEdge)
		TIMx->CCR |= TIM_CAP_FALLING(TIM_CaptureConfigStruct->CaptureChannel);

	if (TIM_CaptureConfigStruct->IntOnCaption)
		TIMx->CCR |= TIM_INT_ON_CAP(TIM_CaptureConfigStruct->CaptureChannel);
}

/*********************************************************************//**
 * @brief 		Read value of capture register in timer/counter device
 * @param[in]	TIMx Pointer to timer/counter device
 * @return 		Value of capture register
 **********************************************************************/
uint32_t TIM_GetCaptureValue(LPC_TMR_TypeDef *TIMx)
{
		return TIMx->CR0;

}
/**
 * @}
 */


/**
 * @}
 */

/******************************************************************************
**                            End Of File
******************************************************************************/
