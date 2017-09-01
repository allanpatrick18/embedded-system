/***********************************************************************//**
 * @file	: lpc13xx_clkpwr.h
 * @brief	: Contains all macro definitions and function prototypes
 * 				support for Clock and Power Control firmware library on LPC13xx
 * @version	: 1.0
 * @date	: 05. Nov. 2009
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
/** @defgroup ADC
 * @ingroup LPC1300CMSIS_FwLib_Drivers
 * @{
 */
#ifndef LPC13XX_CLKPWR_H_
#define LPC13XX_CLKPWR_H_

/* Includes ------------------------------------------------------------------- */
#include "LPC13xx.h"


/* Private Macros ------------------------------------------------------------- */
/** @defgroup ADC_Private_Macros
 * @{
 */

#define CLKPWR_AHBCLKCTRL_SYS		(1<<0)	/**< Enables clock for AHB to APB bridge, to AHB matrix,
												to Cortex-M3 FCLK and HCLK, to SysCon, and to
												PMU. This bit is read only */
#define CLKPWR_AHBCLKCTRL_ROM		(1<<1)	/**< Clock for ROM */
#define CLKPWR_AHBCLKCTRL_RAM		(1<<2)	/**< Clock for RAM */
#define CLKPWR_AHBCLKCTRL_FLASH1	(1<<3)	/**< Clock for Flash 1 */
#define CLKPWR_AHBCLKCTRL_FLASH2	(1<<4)	/**< Clock for Flash 2 */
#define CLKPWR_AHBCLKCTRL_I2C		(1<<5)	/**< Clock for I2C */
#define CLKPWR_AHBCLKCTRL_GPIO		(1<<6)	/**< Clock for GPIO */
#define CLKPWR_AHBCLKCTRL_CT16B0	(1<<7)	/**< Clock for 16 bit counter/timer 0 */
#define CLKPWR_AHBCLKCTRL_CT16B1	(1<<8)	/**< Clock for 16 bit counter/timer 1 */
#define CLKPWR_AHBCLKCTRL_CT32B0	(1<<9)	/**< Clock for 32 bit counter/timer 0 */
#define CLKPWR_AHBCLKCTRL_CT32B1	(1<<10)	/**< Clock for 32 bit counter/timer 1 */
#define CLKPWR_AHBCLKCTRL_SSP		(1<<11)	/**< Clock for SSP */
#define CLKPWR_AHBCLKCTRL_UART		(1<<12)	/**< Clock for UART */
#define CLKPWR_AHBCLKCTRL_ADC		(1<<13)	/**< Clock for ADC */
#define CLKPWR_AHBCLKCTRL_USB_REG	(1<<14)	/**< Clock for USB register */
#define CLKPWR_AHBCLKCTRL_WDT		(1<<15)	/**< Clock for WDT */
#define CLKPWR_AHBCLKCTRL_IOCON		(1<<16)	/**< Clock for IO configuration block */

/* Macro defines for Power-down configuration/ Deep-sleep mode configuration/
 * and Wake-up configuration register */
#define CLKPWR_PDRUNCFG_IRCOUT		(1<<0)	/**< IRC OSC output */
#define CLKPWR_PDRUNCFG_IRC			(1<<1)	/**< IRC OSC */
#define CLKPWR_PDRUNCFG_FLASH		(1<<2)	/**< Flash */
#define CLKPWR_PDRUNCFG_BOD			(1<<3)	/**< BOD */
#define CLKPWR_PDRUNCFG_ADC			(1<<4)	/**< ADC */
#define CLKPWR_PDRUNCFG_SYSOSC		(1<<5)	/**< System OSC */
#define CLKPWR_PDRUNCFG_WDTOSC		(1<<6)	/**< WDT OSC */
#define CLKPWR_PDRUNCFG_SYSPLL		(1<<7)	/**< System PLL */
#define CLKPWR_PDRUNCFG_USBPLL		(1<<8)	/**< USB PLL */
#define CLKPWR_PDRUNCFG_USBPAD		(1<<10)	/**< USB Pad */


/* Macro defines for Peripheral reset control register */
#define CLKPWR_PRESETCTRL_SSP_RST	(1<<0)	/**< SSP reset disabled */
#define CLKPWR_PRESETCTRL_I2C_RST	(1<<1)	/**< I2C reset disabled */


/* Power Management Unit section ------------------------------------------ */
/* Macro defines for Power control register */
#define PMU_PCON_DPDEN				(1<<1)	/**< Deep power down mode */
#define PMU_PCON_DPDFLAG			(1<<11)	/**< Deep power down flag */

/* System Low Power ----------------------------------------------------------*/
#define PMU_LP_SEVONPEND            (0x10)
#define PMU_LP_SLEEPDEEP            (0x04)
#define PMU_LP_SLEEPONEXIT          (0x02)


/**
 * @}
 */

/* Public Functions ----------------------------------------------------------- */

/** @defgroup ADC_Public_Functions
 * @{
 */

void CLKPWR_ConfigAHBClock(uint32_t AHBClk_Type, FunctionalState CmdState);
void CLKPWR_ConfigPowerDown(uint32_t PDtype, FunctionalState CmdState);
void CLKPWR_ConfigWakeUp(uint32_t WUtype, FunctionalState CmdState);
void CLKPWR_ConfigDeepSleep(uint32_t DStype, FunctionalState CmdState);
void CLKPWR_ResetPeripheralCmd(uint32_t PReset, FunctionalState CmdState);
void CLKPWR_SetSSPClockDiv(uint32_t div);
void CLKPWR_SetUARTClockDiv(uint32_t div);
void PMU_Sleep(uint32_t SleepMode, uint32_t SleepCtrl) ;
void PMU_PowerDown(void);

/**
 * @}
 */


#endif /*LPC13XX_CLKPWR_H_*/

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
