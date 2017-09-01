/**
 * @file	: lpc13xx_ssp.c
 * @brief	: Contains all functions support for SSP firmware library on LPC13xx
 * @version	: 1.0
 * @date	: 9. April. 2009
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

#include "lpc13xx_ssp.h"
//#include "lpc13xx_clkpwr.h"


/* If this source file built with example, the LPC13xx FW library configuration
 * file in each example directory ("lpc13xx_libcfg.h") must be included,
 * otherwise the default FW library configuration file must be included instead
 */



/************************** PRIVATE VARIABLES *************************/
/** @addtogroup Private_Variables
 * @{
 */

/** @defgroup 	SSP_Private_Variables
 * @{
 */

/* SSP device configuration structure type */
typedef struct
{
	int32_t 	dataword;				/* Current data word: 0 - 8 bit; 1 - 16 bit */
	uint32_t    txrx_setup; 			/* Transmission setup */
	void		(*inthandler)(LPC_SSP_TypeDef *ssp_dev);   	/* Transmission interrupt handler */
} SSP_CFG_T;

/* SSP configuration data */
static SSP_CFG_T sspdat;

/**
 * @}
 */

/**
 * @}
 */


/***************************** PRIVATE FUNCTION *****************************/
/** @addtogroup  Private_Functions
  * @{
  */

/** @defgroup 	SSP_Private_Functions
 * @{
 */


/*********************************************************************//**
 * @brief 		Standard Private SSP Interrupt handler
 * @param		SSP peripheral definition
 * @return 		None
 ***********************************************************************/
void SSP_IntHandler(LPC_SSP_TypeDef *SSP)
{
	SSP_DATA_SETUP_Type *xf_setup;
    uint16_t tmp;
    //int32_t sspnum;

    // Disable interrupt
    SSP->IMSC = 0;

    xf_setup = (SSP_DATA_SETUP_Type *)sspdat.txrx_setup;

    // save status
    tmp = SSP->RIS;
    xf_setup->status = tmp;

    // Check overrun error
    if (tmp & SSP_RIS_RORRIS){
    	// Clear interrupt
    	SSP->ICR = SSP_ICR_RORIC;
    	// update status
    	xf_setup->status |= SSP_STAT_ERROR;
    	// Callback
    	if (xf_setup->callback != NULL){
    		xf_setup->callback();
    	}
    	return;
    }

    if ((xf_setup->tx_cnt != xf_setup->length) || (xf_setup->rx_cnt != xf_setup->length)){
    	/* check if RX FIFO contains data */
		while ((SSP->SR & SSP_SR_RNE) && (xf_setup->rx_cnt != xf_setup->length)){
			// Read data from SSP data
			tmp = SSP_ReceiveData(SSP);

			// Store data to destination
			if (xf_setup->rx_data != NULL)
			{
				if (sspdat.dataword == 0){
					*(uint8_t *)((uint32_t)xf_setup->rx_data + xf_setup->rx_cnt) = (uint8_t) tmp;
				} else {
					*(uint16_t *)((uint32_t)xf_setup->rx_data + xf_setup->rx_cnt) = (uint16_t) tmp;
				}
			}
			// Increase counter
			if (sspdat.dataword == 0){
				xf_setup->rx_cnt++;
			} else {
				xf_setup->rx_cnt += 2;
			}
		}

		while ((SSP->SR & SSP_SR_TNF) && (xf_setup->tx_cnt != xf_setup->length)){
			// Write data to buffer
			if(xf_setup->tx_data == NULL){
				if (sspdat.dataword == 0){
					SSP_SendData(SSP, 0xFF);
					xf_setup->tx_cnt++;
				} else {
					SSP_SendData(SSP, 0xFFFF);
					xf_setup->tx_cnt += 2;
				}
			} else {
				if (sspdat.dataword == 0){
					SSP_SendData(SSP, (*(uint8_t *)((uint32_t)xf_setup->tx_data + xf_setup->tx_cnt)));
					xf_setup->tx_cnt++;
				} else {
					SSP_SendData(SSP, (*(uint16_t *)((uint32_t)xf_setup->tx_data + xf_setup->tx_cnt)));
					xf_setup->tx_cnt += 2;
				}
			}

		    // Check overrun error
		    if ((tmp = SSP->RIS) & SSP_RIS_RORRIS){
		    	// update status
		    	xf_setup->status |= SSP_STAT_ERROR;
		    	// Callback
		    	if (xf_setup->callback != NULL){
		    		xf_setup->callback();
		    	}
		    	return;
		    }

			// Check for any data available in RX FIFO
			while ((SSP->SR & SSP_SR_RNE) && (xf_setup->rx_cnt != xf_setup->length)){
				// Read data from SSP data
				tmp = SSP_ReceiveData(SSP);

				// Store data to destination
				if (xf_setup->rx_data != NULL)
				{
					if (sspdat.dataword == 0){
						*(uint8_t *)((uint32_t)xf_setup->rx_data + xf_setup->rx_cnt) = (uint8_t) tmp;
					} else {
						*(uint8_t *)((uint32_t)xf_setup->rx_data + xf_setup->rx_cnt) = (uint16_t) tmp;
					}
				}
				// Increase counter
				if (sspdat.dataword == 0){
					xf_setup->rx_cnt++;
				} else {
					xf_setup->rx_cnt += 2;
				}
			}
		}
    }

	// If there more data to sent or receive
	if ((xf_setup->rx_cnt != xf_setup->length) || (xf_setup->tx_cnt != xf_setup->length)){
		// Enable all interrupt
		SSP->IMSC = SSP_IMSC_BITMASK;
	} else {
		// Save status
		xf_setup->status = SSP_STAT_DONE;
		// Callback
		if (xf_setup->callback != NULL){
			xf_setup->callback();
		}
	}
}


 /**
 * @}
 */

/**
 * @}
 */


/************************** GLOBAL/PUBLIC FUNCTIONS *************************/
/** @addtogroup Public_Functions
  * @{
  */

/** @defgroup SSP_Public_Functions
 * @{
 */

/*********************************************************************//**
 * @brief 		Setup clock rate for SSP device
 * @param[in] 	ssp_dev	SSP peripheral definition.
 * @param[in]	target_clock : clock of SSP (Hz)
 * @return 		None
 ***********************************************************************/
void SSP_SetClock (LPC_SSP_TypeDef *ssp_dev, uint32_t target_clock)
{
    uint32_t prescale, cr0_div, cmp_clk, ssp_clk;


    /* The SSP clock is derived from the (main system oscillator / 2),
       so compute the best divider from that clock */
    if (ssp_dev == LPC_SSP0)
    {
    	ssp_clk = SystemCoreClock / (LPC_SYSCON->SSP0CLKDIV);
    }
    else
    {
    	return;
    }

	/* Find closest divider to get at or under the target frequency.
	   Use smallest prescale possible and rely on the divider to get
	   the closest target frequency */
	cr0_div = 0;
	cmp_clk = 0xFFFFFFFF;
	prescale = 2;
	while (cmp_clk > target_clock)
	{
		cmp_clk = ssp_clk / ((cr0_div + 1) * prescale);
		if (cmp_clk > target_clock)
		{
			cr0_div++;
			if (cr0_div > 0xFF)
			{
				cr0_div = 0;
				prescale += 2;
			}
		}
	}

    /* Write computed prescaler and divider back to register */
    ssp_dev->CR0 &= (~ SSP_CR0_SCR (0xFF)) & SSP_CR0_BITMASK;
    ssp_dev->CR0 |= (SSP_CR0_SCR(cr0_div)) & SSP_CR0_BITMASK;
    ssp_dev->CPSR = prescale & SSP_CPSR_BITMASK;
}


/*********************************************************************//**
 * @brief		De-initializes the ssp_dev peripheral registers to their
*                  default reset values.
 * @param[in]	ssp_dev	SSP peripheral selected, should be SSP0 or SSP1
 * @return 		None
 **********************************************************************/
void SSP_DeInit(LPC_SSP_TypeDef* ssp_dev)
{
	LPC_SYSCON->SYSAHBCLKCTRL &= ~(1<<11);
}



/********************************************************************//**
 * @brief		Initializes the ssp_dev peripheral according to the specified
*               parameters in the SSP_ConfigStruct.
 * @param[in]	ssp_dev	SSP peripheral selected, should be SSP0 or SSP1
 * @param[in]	SSP_ConfigStruct Pointer to a SSP_CFG_Type structure
*                    that contains the configuration information for the
*                    specified SSP peripheral.
 * @return 		None
 *********************************************************************/
void SSP_Init(LPC_SSP_TypeDef *ssp_dev, SSP_CFG_Type *SSP_ConfigStruct)
{
	uint32_t tmp;

	LPC_SYSCON->PRESETCTRL |= (0x1<<0);
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<11);
	LPC_SYSCON->SSP0CLKDIV = 0x02;


	/* Configure SSP, interrupt is disable, LoopBack mode is disable,
	 * SSP is disable, Slave output is disable as default
	 */
	tmp = ((SSP_ConfigStruct->CPHA) | (SSP_ConfigStruct->CPOL) \
		| (SSP_ConfigStruct->FrameFormat) | (SSP_ConfigStruct->Databit))
		& SSP_CR0_BITMASK;
	// write back to SSP control register
	ssp_dev->CR0 = tmp;
	if (SSP_ConfigStruct->Databit > SSP_DATABIT_8){
		sspdat.dataword = 1;
	} else {
		sspdat.dataword = 0;
	}

	tmp = SSP_ConfigStruct->Mode & SSP_CR1_BITMASK;
	// Write back to CR1
	ssp_dev->CR1 = tmp;

	// Set clock rate for SSP peripheral
	SSP_SetClock(ssp_dev, SSP_ConfigStruct->ClockRate);
}



/*****************************************************************************//**
* @brief		Fills each SSP_InitStruct member with its default value:
* 				- CPHA = SSP_CPHA_FIRST
* 				- CPOL = SSP_CPOL_HI
* 				- ClockRate = 1000000
* 				- Databit = SSP_DATABIT_8
* 				- Mode = SSP_MASTER_MODE
* 				- FrameFormat = SSP_FRAME_SSP
* @param[in]	SSP_InitStruct Pointer to a SSP_CFG_Type structure
*                    which will be initialized.
* @return		None
*******************************************************************************/
void SSP_ConfigStructInit(SSP_CFG_Type *SSP_InitStruct)
{
	SSP_InitStruct->CPHA = SSP_CPHA_FIRST;
	SSP_InitStruct->CPOL = SSP_CPOL_HI;
	SSP_InitStruct->ClockRate = 1000000;
	SSP_InitStruct->Databit = SSP_DATABIT_8;
	SSP_InitStruct->Mode = SSP_MASTER_MODE;
	SSP_InitStruct->FrameFormat = SSP_FRAME_SPI;
}


/*********************************************************************//**
 * @brief		Enable or disable SSP peripheral's operation
 * @param[in]	ssp_dev	SSP peripheral, should be SSP0 or SSP1
 * @param[in]	NewState New State of ssp_dev peripheral's operation
 * @return 		none
 **********************************************************************/
void SSP_Cmd(LPC_SSP_TypeDef* ssp_dev, FunctionalState NewState)
{

	if (NewState == ENABLE)
	{
		ssp_dev->CR1 |= SSP_CR1_SSP_EN;
	}
	else
	{
		ssp_dev->CR1 &= (~SSP_CR1_SSP_EN) & SSP_CR1_BITMASK;
	}
}



/*********************************************************************//**
 * @brief		Enable or disable Loop Back mode function in SSP peripheral
 * @param[in]	ssp_dev	SSP peripheral selected, should be SSP0 or SSP1
 * @param[in]	NewState	New State of Loop Back mode, should be:
 * 							- ENABLE: Enable this function
 * 							- DISABLE: Disable this function
 * @return 		None
 **********************************************************************/
void SSP_LoopBackCmd(LPC_SSP_TypeDef* ssp_dev, FunctionalState NewState)
{

	if (NewState == ENABLE)
	{
		ssp_dev->CR1 |= SSP_CR1_LBM_EN;
	}
	else
	{
		ssp_dev->CR1 &= (~SSP_CR1_LBM_EN) & SSP_CR1_BITMASK;
	}
}



/*********************************************************************//**
 * @brief		Enable or disable Slave Output function in SSP peripheral
 * @param[in]	ssp_dev	SSP peripheral selected, should be SSP0 or SSP1
 * @param[in]	NewState	New State of Slave Output function, should be:
 * 							- ENABLE: Slave Output in normal operation
 * 							- DISABLE: Slave Output is disabled. This blocks
 * 							SSP controller from driving the transmit data
 * 							line (MISO)
 * Note: 		This function is available when SSP peripheral in Slave mode
 * @return 		None
 **********************************************************************/
void SSP_SlaveOutputCmd(LPC_SSP_TypeDef* ssp_dev, FunctionalState NewState)
{

	if (NewState == ENABLE)
	{
		ssp_dev->CR1 &= (~SSP_CR1_SO_DISABLE) & SSP_CR1_BITMASK;
	}
	else
	{
		ssp_dev->CR1 |= SSP_CR1_SO_DISABLE;
	}
}



/*********************************************************************//**
 * @brief		Transmit a single data through ssp_dev peripheral
 * @param[in]	ssp_dev	SSP peripheral selected, should be SSP
 * @param[in]	Data	Data to transmit (must be 16 or 8-bit long,
 * 						this depend on SSP data bit number configured)
 * @return 		none
 **********************************************************************/
void SSP_SendData(LPC_SSP_TypeDef* ssp_dev, uint16_t Data)
{

	ssp_dev->DR = SSP_DR_BITMASK(Data);
}



/*********************************************************************//**
 * @brief		Receive a single data from ssp_dev peripheral
 * @param[in]	ssp_dev	SSP peripheral selected, should be SSP
 * @return 		Data received (16-bit long)
 **********************************************************************/
uint16_t SSP_ReceiveData(LPC_SSP_TypeDef* ssp_dev)
{

	return ((uint16_t) (SSP_DR_BITMASK(ssp_dev->DR)));
}

/*********************************************************************//**
 * @brief 		SSP Read write data function
 * @param[in]	ssp_dev 	Pointer to SSP peripheral, should be SSP0 or SSP1
 * @param[in]	dataCfg	Pointer to a SSP_DATA_SETUP_Type structure that
 * 						contains specified information about transmit
 * 						data configuration.
 * @param[in]	xfType	Transfer type, should be:
 * 						- SSP_TRANSFER_POLLING: Polling mode
 * 						- SSP_TRANSFER_INTERRUPT: Interrupt mode
 * @return 		Actual Data length has been transferred in polling mode.
 * 				In interrupt mode, always return (0)
 * 				Return (-1) if error.
 * Note: This function can be used in both master and slave mode.
 ***********************************************************************/
int32_t SSP_ReadWrite (LPC_SSP_TypeDef *ssp_dev, SSP_DATA_SETUP_Type *dataCfg, \
						SSP_TRANSFER_Type xfType)
{
	uint8_t *rdata8;
    uint8_t *wdata8;
	uint16_t *rdata16;
    uint16_t *wdata16;
    uint32_t stat;
    uint32_t tmp;
//    int32_t sspnum;
    int32_t dataword;

    dataCfg->rx_cnt = 0;
    dataCfg->tx_cnt = 0;
    dataCfg->status = 0;


	/* Clear all remaining data in RX FIFO */
	while (ssp_dev->SR & SSP_SR_RNE){
		tmp = (uint32_t) SSP_ReceiveData(ssp_dev);
	}

	// Clear status
	ssp_dev->ICR = SSP_ICR_BITMASK;

	dataword = sspdat.dataword;

	// Polling mode ----------------------------------------------------------------------
	if (xfType == SSP_TRANSFER_POLLING){
		if (dataword == 0){
			rdata8 = (uint8_t *)dataCfg->rx_data;
			wdata8 = (uint8_t *)dataCfg->tx_data;
		} else {
			rdata16 = (uint16_t *)dataCfg->rx_data;
			wdata16 = (uint16_t *)dataCfg->tx_data;
		}
		while ((dataCfg->tx_cnt != dataCfg->length) || (dataCfg->rx_cnt != dataCfg->length)){
			if ((ssp_dev->SR & SSP_SR_TNF) && (dataCfg->tx_cnt != dataCfg->length)){
				// Write data to buffer
				if(dataCfg->tx_data == NULL){
					if (dataword == 0){
						SSP_SendData(ssp_dev, 0xFF);
						dataCfg->tx_cnt++;
					} else {
						SSP_SendData(ssp_dev, 0xFFFF);
						dataCfg->tx_cnt += 2;
					}
				} else {
					if (dataword == 0){
						SSP_SendData(ssp_dev, *wdata8);
						wdata8++;
						dataCfg->tx_cnt++;
					} else {
						SSP_SendData(ssp_dev, *wdata16);
						wdata16++;
						dataCfg->tx_cnt += 2;
					}
				}
			}

			// Check overrun error
			if ((stat = ssp_dev->RIS) & SSP_RIS_RORRIS){
				// save status and return
				dataCfg->status = stat | SSP_STAT_ERROR;
				return (-1);
			}

			// Check for any data available in RX FIFO
			while ((ssp_dev->SR & SSP_SR_RNE) && (dataCfg->rx_cnt != dataCfg->length)){
				// Read data from SSP data
				tmp = SSP_ReceiveData(ssp_dev);

				// Store data to destination
				if (dataCfg->rx_data != NULL)
				{
					if (dataword == 0){
						*(rdata8) = (uint8_t) tmp;
						rdata8++;
					} else {
						*(rdata16) = (uint16_t) tmp;
						rdata16++;
					}
				}
				// Increase counter
				if (dataword == 0){
					dataCfg->rx_cnt++;
				} else {
					dataCfg->rx_cnt += 2;
				}
			}
		}

		// save status
		dataCfg->status = SSP_STAT_DONE;

		if (dataCfg->tx_data != NULL){
			return dataCfg->tx_cnt;
		} else if (dataCfg->rx_data != NULL){
			return dataCfg->rx_cnt;
		} else {
			return (0);
		}
	}

	// Interrupt mode ----------------------------------------------------------------------
	else if (xfType == SSP_TRANSFER_INTERRUPT){
		sspdat.inthandler = SSP_IntHandler;
		sspdat.txrx_setup = (uint32_t)dataCfg;

		while ((ssp_dev->SR & SSP_SR_TNF) && (dataCfg->tx_cnt != dataCfg->length)){
			// Write data to buffer
			if(dataCfg->tx_data == NULL){
				if (sspdat.dataword == 0){
					SSP_SendData(ssp_dev, 0xFF);
					dataCfg->tx_cnt++;
				} else {
					SSP_SendData(ssp_dev, 0xFFFF);
					dataCfg->tx_cnt += 2;
				}
			} else {
				if (sspdat.dataword == 0){
					SSP_SendData(ssp_dev, (*(uint8_t *)((uint32_t)dataCfg->tx_data + dataCfg->tx_cnt)));
					dataCfg->tx_cnt++;
				} else {
					SSP_SendData(ssp_dev, (*(uint16_t *)((uint32_t)dataCfg->tx_data + dataCfg->tx_cnt)));
					dataCfg->tx_cnt += 2;
				}
			}

			// Check error
			if ((stat = ssp_dev->RIS) & SSP_RIS_RORRIS){
				// save status and return
				dataCfg->status = stat | SSP_STAT_ERROR;
				return (-1);
			}

			// Check for any data available in RX FIFO
			while ((ssp_dev->SR & SSP_SR_RNE) && (dataCfg->rx_cnt != dataCfg->length)){
				// Read data from SSP data
				tmp = SSP_ReceiveData(ssp_dev);

				// Store data to destination
				if (dataCfg->rx_data != NULL)
				{
					if (sspdat.dataword == 0){
						*(uint8_t *)((uint32_t)dataCfg->rx_data + dataCfg->rx_cnt) = (uint8_t) tmp;
					} else {
						*(uint16_t *)((uint32_t)dataCfg->rx_data + dataCfg->rx_cnt) = (uint16_t) tmp;
					}
				}
				// Increase counter
				if (sspdat.dataword == 0){
					dataCfg->rx_cnt++;
				} else {
					dataCfg->rx_cnt += 2;
				}
			}
		}

		// If there more data to sent or receive
		if ((dataCfg->rx_cnt != dataCfg->length) || (dataCfg->tx_cnt != dataCfg->length)){
			// Enable all interrupt
			ssp_dev->IMSC = SSP_IMSC_BITMASK;
		} else {
			// Save status
			dataCfg->status = SSP_STAT_DONE;
		}
		return (0);
	}

	return (-1);
}

/*********************************************************************//**
 * @brief		Checks whether the specified SSP status flag is set or not
 * @param[in]	ssp_dev	SSP peripheral selected, should be SSP0 or SSP1
 * @param[in]	FlagType	Type of flag to check status, should be one
 * 							of following:
 *							- SSP_STAT_TXFIFO_EMPTY: TX FIFO is empty
 *							- SSP_STAT_TXFIFO_NOTFULL: TX FIFO is not full
 *							- SSP_STAT_RXFIFO_NOTEMPTY: RX FIFO is not empty
 *							- SSP_STAT_RXFIFO_FULL: RX FIFO is full
 *							- SSP_STAT_BUSY: SSP peripheral is busy
 * @return		New State of specified SSP status flag
 **********************************************************************/
FlagStatus SSP_GetStatus(LPC_SSP_TypeDef* ssp_dev, uint32_t FlagType)
{

	return ((ssp_dev->SR & FlagType) ? SET : RESET);
}



/*********************************************************************//**
 * @brief		Enable or disable specified interrupt type in SSP peripheral
 * @param[in]	ssp_dev	SSP peripheral selected, should be SSP0 or SSP1
 * @param[in]	IntType	Interrupt type in SSP peripheral, should be:
 * 				- SSP_INTCFG_ROR: Receive Overrun interrupt
 * 				- SSP_INTCFG_RT: Receive Time out interrupt
 * 				- SSP_INTCFG_RX: RX FIFO is at least half full interrupt
 * 				- SSP_INTCFG_TX: TX FIFO is at least half empty interrupt
 * @param[in]	NewState New State of specified interrupt type, should be:
 * 				- ENABLE: Enable this interrupt type
 * 				- DISABLE: Disable this interrupt type
 * @return		None
 **********************************************************************/
void SSP_IntConfig(LPC_SSP_TypeDef *ssp_dev, uint32_t IntType, FunctionalState NewState)
{
	if (NewState == ENABLE)
	{
		ssp_dev->IMSC |= IntType;
	}
	else
	{
		ssp_dev->IMSC &= (~IntType) & SSP_IMSC_BITMASK;
	}
}


/*********************************************************************//**
 * @brief	Check whether the specified Raw interrupt status flag is
 * 			set or not
 * @param[in]	ssp_dev	SSP peripheral selected, should be SSP0 or SSP1
 * @param[in]	RawIntType	Raw Interrupt Type, should be:
 * 				- SSP_INTSTAT_RAW_ROR: Receive Overrun interrupt
 * 				- SSP_INTSTAT_RAW_RT: Receive Time out interrupt
 * 				- SSP_INTSTAT_RAW_RX: RX FIFO is at least half full interrupt
 * 				- SSP_INTSTAT_RAW_TX: TX FIFO is at least half empty interrupt
 * @return	New State of specified Raw interrupt status flag in SSP peripheral
 * Note: Enabling/Disabling specified interrupt in SSP peripheral does not
 * 		effect to Raw Interrupt Status flag.
 **********************************************************************/
IntStatus SSP_GetRawIntStatus(LPC_SSP_TypeDef *ssp_dev, uint32_t RawIntType)
{

	return ((ssp_dev->RIS & RawIntType) ? SET : RESET);
}


/*********************************************************************//**
 * @brief	Check whether the specified interrupt status flag is
 * 			set or not
 * @param[in]	ssp_dev	SSP peripheral selected, should be SSP0 or SSP1
 * @param[in]	IntType	Raw Interrupt Type, should be:
 * 				- SSP_INTSTAT_ROR: Receive Overrun interrupt
 * 				- SSP_INTSTAT_RT: Receive Time out interrupt
 * 				- SSP_INTSTAT_RX: RX FIFO is at least half full interrupt
 * 				- SSP_INTSTAT_TX: TX FIFO is at least half empty interrupt
 * @return	New State of specified interrupt status flag in SSP peripheral
 * Note: Enabling/Disabling specified interrupt in SSP peripheral effects
 * 			to Interrupt Status flag.
 **********************************************************************/
IntStatus SSP_GetIntStatus (LPC_SSP_TypeDef *ssp_dev, uint32_t IntType)
{

	return ((ssp_dev->MIS & IntType) ? SET :RESET);
}



/*********************************************************************//**
 * @brief				Clear specified interrupt pending in SSP peripheral
 * @param[in]	ssp_dev	SSP peripheral selected, should be SSP0 or SSP1
 * @param[in]	IntType	Interrupt pending to clear, should be:
 * 						- SSP_INTCLR_ROR: clears the "frame was received when
 * 						RxFIFO was full" interrupt.
 * 						- SSP_INTCLR_RT: clears the "Rx FIFO was not empty and
 * 						has not been read for a timeout period" interrupt.
 * @return		None
 **********************************************************************/
void SSP_ClearIntPending(LPC_SSP_TypeDef *ssp_dev, uint32_t IntType)
{

	ssp_dev->ICR = IntType;
}

/**
 * @brief		Standard SSP Interrupt handler
 * @param[in] 	None
 * @return		None
 */
void SSP_StdIntHandler(void)
{
	// Call relevant handler
	sspdat.inthandler(LPC_SSP0);
}


/**
 * @}
 */
/**
 * @}
 */

