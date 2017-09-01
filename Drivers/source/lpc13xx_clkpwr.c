/**
 * @file	: lpc13xx_clkpwr.c
 * @brief	: Contains all functions support for system clock and power control
 * 				firmware library on LPC13xx.
 * @version	: 1.0
 * @date	: 05. Nov. 2009
 * @author	: HieuNguyen
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

#include "lpc_types.h"
#include "lpc13xx_clkpwr.h"


/************************** GLOBAL/PUBLIC FUNCTIONS *************************/

/**
 * @brief 		Configures System AHB clock control
 * @param[in]	AHBClk_Type	AHB clock type
 * @param[in]	CmdState	Command State, should be:
 * 							- ENABLE: Enable clock for this AHB clock type
* 							- DISABLE: Disable clock for this AHB clock type
 * @return		None
 */
void CLKPWR_ConfigAHBClock(uint32_t AHBClk_Type, FunctionalState CmdState)
{
	if (CmdState == ENABLE){
		LPC_SYSCON->SYSAHBCLKCTRL |= (AHBClk_Type);
	} else {
		LPC_SYSCON->SYSAHBCLKCTRL &= ~(AHBClk_Type);
	}
}


/**
 * @brief 		Enter Power Down for a specified peripheral immediately
 * @param[in]	PDtype		Specified peripheral for power down configuration
 * @param[in]	CmdState	Command State, should be:
 * 							- ENABLE: Enable power down for this peripheral
* 							- DISABLE: Disable power down for this peripheral
 * @return		None
 */
void CLKPWR_ConfigPowerDown(uint32_t PDtype, FunctionalState CmdState)
{
	if (CmdState == ENABLE){
		LPC_SYSCON->PDRUNCFG |= (PDtype);
	} else {
		LPC_SYSCON->PDRUNCFG &= ~(PDtype);
	}
}


/**
 * @brief 		Configures wake up for specified peripheral to indicate
 * 				the state the chip must enter when it is waking up from
 * 				Deep-sleep mode.
 * @param[in]	WUtype		Specified peripheral for wake up configuration
 * @param[in]	CmdState	Command State, should be:
 * 							- ENABLE: Enable power down for this peripheral
 * 							after waking up
* 							- DISABLE: Disable power down for this peripheral
* 							after waking up
 * @return		None
 */
void CLKPWR_ConfigWakeUp(uint32_t WUtype, FunctionalState CmdState)
{
	if (CmdState == ENABLE){
		LPC_SYSCON->PDRUNCFG |= (WUtype);
	} else {
		LPC_SYSCON->PDRUNCFG &= ~(WUtype);
	}
}


/**
 * @brief 		Configures Deep sleep for specified peripheral to indicate
 * 				the state the chip must enter when Deep-sleep mode is
 * 				asserted by the ARM.
 * @param[in]	DStype		Specified peripheral for Deep sleep configuration
 * @param[in]	CmdState	Command State, should be:
 * 							- ENABLE: Enable power down for this peripheral
 * 							when the Deep sleep mode is entered
* 							- DISABLE: Disable power down for this peripheral
* 							when the Deep sleep mode is entered
 * @return		None
 */
void CLKPWR_ConfigDeepSleep(uint32_t DStype, FunctionalState CmdState)
{
	if (CmdState == ENABLE){
		LPC_SYSCON->PDRUNCFG |= (DStype);
	} else {
		LPC_SYSCON->PDRUNCFG &= ~(DStype);
	}
}


/**
 * @brief 		Executes a reset command to a specified peripheral.
 * @param[in]	PReset		Peripheral type.
 * @param[in]	CmdState	Command State, should be:
 * 							- ENABLE.
 * 							- DISABLE.
 * @return		None
 */
void CLKPWR_ResetPeripheralCmd(uint32_t PReset, FunctionalState CmdState)
{
	if (CmdState == ENABLE){
		LPC_SYSCON->PRESETCTRL &= ~(PReset);
	} else {
		LPC_SYSCON->PRESETCTRL |= (PReset);
	}
}


/**
 * @brief 		Set clock divider value for SSP peripheral.
 * @param[in]	div	Divider value, from 1 to 255, value '0' is used
 * 				to disable clock source for SSP
 * @return		None
 */
void CLKPWR_SetSSPClockDiv(uint32_t div)
{
	LPC_SYSCON->SSP0CLKDIV = div;
}


/**
 * @brief 		Set clock divider value for UART peripheral.
 * @param[in]	div	Divider value, from 1 to 255, value '0' is used
 * 				to disable clock source for UART
 * @return		None
 */
void CLKPWR_SetUARTClockDiv(uint32_t div)
{
	LPC_SYSCON->UARTCLKDIV = div;
}


/**
 * @brief 		Enter Sleep mode with co-operated instruction by the Cortex-M3.
 * @param[in]	SleepMode SleepMode: 1 is deep sleep, 0 is sleep,
 * @param[in]	SleepCtrl  Deep-sleep configuration register
 * @return		None
 */
void PMU_Sleep( uint32_t SleepMode, uint32_t SleepCtrl )
{
  LPC_SYSCON->PDAWAKECFG = LPC_SYSCON->PDRUNCFG;
  LPC_SYSCON->PDSLEEPCFG = SleepCtrl;
  if ( SleepMode )
  {
	SCB->SCR |= PMU_LP_SLEEPDEEP;
  }
  __WFI();
  return;
}


///**
// * @brief 		Enter Deep Sleep mode with co-operated instruction by the Cortex-M3.
// * @param[in]	None
// * @return		None
// */
//void CLKPWR_DeepSleep(void)
//{
//    /* Deep-Sleep Mode, set SLEEPDEEP bit */
//	SCB->SCR = 0x4;
//	SC->PCON = 0x00;
//	/* Sleep Mode*/
//	__WFI();
//}


///**
// * @brief 		Enter Power Down mode with co-operated instruction by the Cortex-M3.
// * @param[in]	None
// * @return		None
// */
//void CLKPWR_PowerDown(void)
//{
//    /* Deep-Sleep Mode, set SLEEPDEEP bit */
//	SCB->SCR = 0x4;
//	SC->PCON = 0x01;
//	/* Sleep Mode*/
//	__WFI();
//}


/**
 * @brief 		Enter Deep Power Down mode with co-operated instruction by the Cortex-M3.
 * @param[in]	None
 * @return		None
 */
void PMU_PowerDown( void )
{
  uint32_t regVal;

  if ( (LPC_PMU->PCON & ((0x1<<8) | (0x1<<11))) != 0x0 )
  {
	/* Check sleep and deep power down bits. If sleep and/or
	deep power down mode are entered, clear the PCON bits. */
    regVal = LPC_PMU->PCON;
	regVal |= ((0x1<<8) | (0x1<<11));
    LPC_PMU->PCON = regVal;

    if ( (LPC_PMU->GPREG0 != 0x12345678)||(LPC_PMU->GPREG1 != 0x87654321)
		||(LPC_PMU->GPREG2 != 0x56781234)||(LPC_PMU->GPREG3 != 0x43218765) )
    {
      while (1);
    }
  }
  else
  {
	/* If in neither sleep nor deep power mode, enter deep power
	down mode now. */
    LPC_PMU->GPREG0 = 0x12345678;
    LPC_PMU->GPREG1 = 0x87654321;
    LPC_PMU->GPREG2 = 0x56781234;
    LPC_PMU->GPREG3 = 0x43218765;
	SCB->SCR |= PMU_LP_SLEEPDEEP;
    LPC_PMU->PCON = 0x2;
    __WFI();
  }
  return;
}

