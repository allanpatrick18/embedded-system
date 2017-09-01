/*****************************************************************************
 * $Id: lpc13xx_uart.c 28 2014-01-27 14:45:13Z danielru $
 *
 * Project: LPC13xx UART driver
 *
 * Description:
 *     This file contains the structure definitions and manifest
 *     constants for the LPC13xx chip family component: UART
 *
******************************************************************************/
#include "LPC13xx.h"
#include "lpc13xx_uart.h"

volatile uint32_t UARTStatus;
volatile uint8_t  UARTTxEmpty = 1;
volatile uint8_t  UARTBuffer[BUFSIZE];
volatile uint32_t UARTCount = 0;
volatile FlagStatus Synchronous;

/*********************************************************************//**
 * @brief		UART IRQ Handler
 * @param[in]	None
 * @return		None
 **********************************************************************/
void UART_IRQHandler(void)
{
	uint32_t IIRValue, LSRValue;
	uint8_t Dummy = Dummy;
	IIRValue = LPC_UART->IIR;

	if ((IIRValue & 0x0F) == UART_IIR_INTID_RLS)		/* Receive Line Status */
		{
		LSRValue = LPC_UART->LSR;
		/* Receive Line Status */
		if (LSRValue & (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE | UART_LSR_RXFE | UART_LSR_BI))
		{
			/* There are errors or break interrupt */
			/* Read LSR will clear the interrupt */
			UARTStatus = LSRValue;
			Dummy = LPC_UART->RBR;		/* Dummy read on RX to clear
							interrupt, then bail out */
			return;
		}
		if (LSRValue & UART_LSR_RDR)	/* Receive Data Ready */
		{
			/* If no error on RLS, normal ready, save into the data buffer. */
			/* Note: read RBR will clear the interrupt */
			UARTBuffer[UARTCount++] = LPC_UART->RBR;
			if (UARTCount == BUFSIZE)
			{
				UARTCount = 0;		/* buffer overflow */
			}
		}
	}
	else if ((IIRValue & 0x0F) == UART_IIR_INTID_RDA)	/* Receive Data Available */
	{
		/* Receive Data Available */
		UARTBuffer[UARTCount++] = LPC_UART->RBR;
		if (UARTCount == BUFSIZE)
		{
			UARTCount = 0;		/* buffer overflow */
		}
	}
	else if ((IIRValue & 0x0F) == UART_IIR_INTID_CTI)	/* Character timeout indicator */
	{
		/* Character Time-out indicator */
		UARTStatus |= 0x100;		/* Bit 9 as the CTI error */
	}
	else if ((IIRValue & 0x0F) == UART_IIR_INTID_THRE)	/* THRE, transmit holding register empty */
	{
		/* THRE interrupt */
		LSRValue = LPC_UART->LSR;		/* Check status in the LSR to see if valid data in U0THR or not */
		if (LSRValue & UART_LSR_THRE)
		{
			UARTTxEmpty = 1;
		}
		else
		{
			UARTTxEmpty = 0;
		}
	}
	if ((IIRValue >> 8)& 0x01) /* End of Auto baud */
	{
		UART_IntConfig(UART_INTCFG_ABEO, DISABLE);
		//clear bit ABEOInt in the U0IIR by set ABEOIntClr in the U0ACR register
		LPC_UART->ACR |=(1<<8);
		Synchronous = SET;
	}
	else if ((IIRValue >> 9)& 0x01)/* Auto baud time out */
	{
		UART_IntConfig(UART_INTCFG_ABEO, DISABLE);
		//clear bit ABTOInt in the U0IIR by set ABTOIntClr in the U0ACR register
		LPC_UART->ACR |=(1<<9);
	}
}

/*********************************************************************//**
 * @brief		Set data bit
 * @param[in]	databit: data bit will be set, should be:
 * 				- UART_DATABIT_5: UART 5 bit data mode
 * 				- UART_DATABIT_6: UART 6 bit data mode
 * 				- UART_DATABIT_7: UART 7 bit data mode
 * 				- UART_DATABIT_8: UART 8 bit data mode
 * @return		Status, will be:
 * 				- ERROR: setting unsuccessful
 * 				- SUCCESS: setting successful
 **********************************************************************/
Status UART_SetDatabit(UART_DATABIT_Type databit)
{
	LPC_UART->LCR &= ~0x00000003;
	switch(databit)
	{
	case UART_DATABIT_5:
		break;
	case UART_DATABIT_6:
		LPC_UART->LCR |= 0x01;
		break;
	case UART_DATABIT_7:
		LPC_UART->LCR |= 0x02;
		break;
	case UART_DATABIT_8:
		LPC_UART->LCR |= 0x03;
		break;
	default:
		return ERROR;
	}
	return SUCCESS;
}

/*********************************************************************//**
 * @brief		Set parity
 * @param[in]	parity: parity will be set, should be:
 * 				- UART_PARITY_NONE: no parity
 * 				- UART_PARITY_ODD: odd parity
 * 				- UART_PARITY_EVEN: even parity
 * 				- UART_PARITY_SP_1: force '1' stick parity
 * 				- UART_PARITY_SP_0: force '0' stick parity
 * @return		Status, will be:
 * 				- ERROR: setting unsuccessful
 * 				- SUCCESS: setting successful
 **********************************************************************/
Status UART_SetParity(UART_PARITY_Type parity)
{
	LPC_UART->LCR &= ~(0x30); //clear bit LCR[5:4]
	switch(parity)
	{
	case UART_PARITY_NONE:
		//disable parity
		LPC_UART->LCR &= ~(1<<3);
		break;
	case UART_PARITY_ODD:
		LPC_UART->LCR |= (1<<3);
		break;
	case UART_PARITY_EVEN:
		LPC_UART->LCR |= (1<<4)|(1<<3);
		break;
	case UART_PARITY_SP_1:
		LPC_UART->LCR |= (2<<4)|(1<<3);
		break;
	case UART_PARITY_SP_0:
		LPC_UART->LCR |= (3<<4)|(1<<3);
		break;
	default:
		return ERROR;
	}
	return SUCCESS;
}

/*********************************************************************//**
 * @brief		Set stop bit
 * @param[in]	stopbit
 * @return		Status, will be:
 * 				- ERROR: setting unsuccessful
 * 				- SUCCESS: setting successful
 **********************************************************************/
Status UART_SetStopbit(UART_STOPBIT_Type stopbit)
{
	switch(stopbit)
	{
	case UART_STOPBIT_1:
		LPC_UART->LCR &= ~(1<<2);
		break;
	case UART_STOPBIT_2:
		LPC_UART->LCR |= (1<<2);
		break;
	default:
		return ERROR;
	}
	return SUCCESS;
}

/*********************************************************************//**
 * @brief		Set baudrate
 * @param[in]	baudrate: baudrate will be set, should be between:
 * 				600-115200 bps
 * @return		None
 **********************************************************************/
void UART_SetBaudrate(uint32_t baudrate)
{
	uint32_t Fdiv;
	uint32_t regVal;
	regVal = LPC_SYSCON->UARTCLKDIV;
	Fdiv = ((SystemCoreClock/regVal)/16)/baudrate ;	/*baud rate */

	LPC_UART->LCR |=(1<<7); //DLAB=1
	LPC_UART->DLM = Fdiv / 256;
	LPC_UART->DLL = Fdiv % 256;
	LPC_UART->LCR &=~(1<<7); //DLAB=0
	return;
}

/*********************************************************************//**
 * @brief		UART Init. UART will be set
 * @param[in]	None
 * @return		None
 **********************************************************************/

void UART_Init(void)
{
	/* Enable UART clock */
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<12);
//	LPC_SYSCON->PCLKCTRL |= (1<<1);   /* Enable the peripheral clock */
	LPC_SYSCON->UARTCLKDIV = 0x1;     /* divided by 1 */

	/* Setup pin select */
	LPC_IOCON->PIO1_6 &= ~0x07;    /*  UART I/O config */
	LPC_IOCON->PIO1_6 |= 0x01;     /* UART RXD */
	LPC_IOCON->PIO1_7 &= ~0x07;
	LPC_IOCON->PIO1_7 |= 0x01;     /* UART TXD */

	/* Default setup:
	 * baudrate = 9600 bps
	 * 8 bit data
	 * 1 stop bit
	 * no parity
	 */

	UART_SetDatabit(UART_DATABIT_8);
	UART_SetParity(UART_PARITY_NONE);
	UART_SetStopbit(UART_STOPBIT_1);
	/* setup baurate */
	UART_SetBaudrate(9600);
	LPC_UART->IER = 0;

	LPC_UART->FCR = 0x07;		/* Enable and reset TX and RX FIFO. */

}

/*********************************************************************//**
 * @brief		UART DeInit. Disable clock for UART peripheral
 * @param[in]	None
 * @return		None
 **********************************************************************/
void UART_DeInit(void)
{
	LPC_SYSCON->SYSAHBCLKCTRL &= ~(1<<11); /* Disable UART clock */
//	LPC_SYSCON->PCLKCTRL &= ~(1<<1);   /* Disable the peripheral clock */
}

/*********************************************************************//**
 * @brief		UART FIFO Configuration
 * @param[in]	level: RX FIFO trigger level will be set, should be:
 * 				- UART_FIFO_TRGLEV0: FIFO trigger level 0
 * 				- UART_FIFO_TRGLEV1: FIFO trigger level 1
 * 				- UART_FIFO_TRGLEV2: FIFO trigger level 2
 * 				- UART_FIFO_TRGLEV3: FIFO trigger level 3
 * @return		None
 **********************************************************************/
void UART_FIFOConfig(UART_FITO_LEVEL_Type level)
{
	if(level == UART_FIFO_DISABLE)
	{
		LPC_UART->FCR &=~(0x01);
		return;
	}
	else
	{
		LPC_UART->FCR &= ~(3<<6); //clear bit FCR[7:6]
		switch (level)
		{
		case UART_FIFO_TRGLEV0:
			break;
		case UART_FIFO_TRGLEV1:
			LPC_UART->FCR |=(1<<6);
			break;
		case UART_FIFO_TRGLEV2:
			LPC_UART->FCR |=(2<<6);
			break;
		case UART_FIFO_TRGLEV3:
			LPC_UART->FCR |=(3<<6);
			break;
		default:
			break;
		}
	}
	//enable FIFOs
	LPC_UART->FCR |= 0x01;
}

/*********************************************************************//**
 * @brief		Enable/Disable Interrupt
 * @param[in]	IntType: Interrupt type, should be:
 * 				- UART_INTCFG_RBR: Receive Data Available Interrupt
 * 				- UART_INTCFG_THRE: THRE Interrupt
 * 				- UART_INTCFG_RLS: RX Line Interrupt
 * 				- UART_INTCFG_ABEO: End of Auto baud interrupt
 * 				- UART_INTCFG_ABTO: Auto baud time-out interrupt
 * @param[in]	NewState: new interrupt status, should be:
 * 				- ENABLE: enable interrupt
 * 				- DISABLE: disable interrupt
 * @return		None
 **********************************************************************/
void UART_IntConfig(UART_INT_Type IntType, FunctionalState NewState)
{
	uint32_t tmp;
	switch(IntType)
	{
	case UART_INTCFG_RBR:
		tmp = 1;
		break;
	case UART_INTCFG_THRE:
		tmp = (1<<1);
		break;
	case UART_INTCFG_RLS:
		tmp = (1<<2);
		break;
	case UART_INTCFG_ABEO:
		tmp = (1<<8);
		break;
	case UART_INTCFG_ABTO:
		tmp = (1<<9);
		break;
	}
	if(NewState == ENABLE)
	{
		LPC_UART->IER |=tmp;
	}
	else
		LPC_UART->IER &= ~tmp;
}

/*********************************************************************//**
 * @brief		Enable/Disable transmission
 * @param[in]	NewState: new transmit status, should be:
 * 				- ENABLE: enable transmit
 * 				- DISABLE: disable transmit
 * @return		None
 **********************************************************************/
void UART_TXCmd(FunctionalState NewState)
{
	if(NewState == ENABLE)
	{
		LPC_UART->TER |=(1<<7);
	}
	else
		LPC_UART->TER &= ~(1<<7);
}

/*********************************************************************//**
 * @brief		Get line status
 * @param[in]	None
 * @return		Line Status register value
 **********************************************************************/
uint8_t UART_GetLineStatus(void)
{
	return LPC_UART->LSR;
}

/*********************************************************************//**
 * @brief		Reset FIFO
 * @param[in]	None
 * @return		None
 **********************************************************************/
void UART_ResetFIFO(void)
{
	LPC_UART->FCR |=(1<<1); //Reset RX FIFO
	LPC_UART->FCR |=(1<<2); //Reset TX FIFO
}

/*********************************************************************//**
 * @brief		Check whether if UART is busy or not
 * @param[in]	None
 * @return		RESET if UART is not busy, otherwise return SET.
 **********************************************************************/
FlagStatus UART_CheckBusy(void)
{
	if (LPC_UART->LSR & (1<<6)){ //check bit TEMT
		return RESET;
	} else {
		return SET;
	}
}

/*********************************************************************//**
 * @brief		Enable autobaud mode
 * @param[in]	ABMode: Auto baud mode, should be:
 * 				- UART_AB_MODE0: Auto-baud mode 0
 * 				- UART_AB_MODE1: Auto-baud mode 1
 * @param[in]	ABRestart: restart mode, should be:
 * 				- ENABLE: enable restart mode
 * 				- DISABLE: disable restart mode
 * @return		None
 **********************************************************************/
void UART_ABEnable(UART_AB_MODE_Type ABMode, FunctionalState ABRestart)
{
	if(ABMode == UART_AB_MODE0)
	{
		LPC_UART->ACR &=~(1<<1);
	}
	else
		LPC_UART->ACR |= (1<<1);
	if(ABRestart == ENABLE)
	{
		LPC_UART->ACR |= (1<<2);
	}
	else
		LPC_UART->ACR &= ~(1<<2);
	//set default fractional divider
	LPC_UART->FDR = 0x10;
	//Start Auto baud mode
	LPC_UART->ACR |=(1<<0);
}

/*********************************************************************//**
 * @brief		Disable autobaud mode
 * @param[in]	None
 * @return		None
 **********************************************************************/
void UART_ABDisable(void)
{
	LPC_UART->ACR = 0x00;
}

/*********************************************************************//**
 * @brief		Send a block of data via UART peripheral
 *
 * @param[in]	txbuf 	Pointer to Transmit buffer
 * @param[in]	buflen 	Length of Transmit buffer
 * @param[in] 	flag 	Flag used in  UART transfer, should be
 * 						NONE_BLOCKING or BLOCKING
 * @return 		Number of bytes sent.
 *
 * Note: when using UART in BLOCKING mode, a time-out condition is used
 * via defined symbol UART_BLOCKING_TIMEOUT.
 **********************************************************************/
uint32_t UART_Send(uint8_t *txbuf, uint32_t buflen, TRANSFER_BLOCK_Type flag)
{
	uint32_t bToSend, bSent, timeOut, fifo_cnt;
	uint8_t *pChar = txbuf;

	bToSend = buflen;

	// blocking mode
	if (flag == BLOCKING) {
		bSent = 0;
		while (bToSend){
			timeOut = UART_BLOCKING_TIMEOUT;
			// Wait for THR empty with timeout
			while (!(LPC_UART->LSR & (1<<5))) {
				if (timeOut == 0) break;
				timeOut--;
			}
			// Time out!
			if(timeOut == 0) break;
			fifo_cnt = UART_TX_FIFO_SIZE;
			while (fifo_cnt && bToSend){
				LPC_UART->THR = (*pChar++);
				fifo_cnt--;
				bToSend--;
				bSent++;
			}
		}
	}
	// None blocking mode
	else {
		bSent = 0;
		while (bToSend) {
			if (!(LPC_UART->LSR & (1<<5))){
				break;
			}
			fifo_cnt = UART_TX_FIFO_SIZE;
			while (fifo_cnt && bToSend) {
				LPC_UART->THR = (*pChar++);
				bToSend--;
				fifo_cnt--;
				bSent++;
			}
		}
	}
	return bSent;
}

/*********************************************************************//**
 * @brief		Receive a block of data via UART peripheral
 *
 * @param[out]	rxbuf 	Pointer to Received buffer
 * @param[in]	buflen 	Length of Received buffer
 * @param[in] 	flag 	Flag mode, should be NONE_BLOCKING or BLOCKING

 * @return 		Number of bytes received
 *
 * Note: when using UART in BLOCKING mode, a time-out condition is used
 * via defined symbol UART_BLOCKING_TIMEOUT.
 **********************************************************************/
uint32_t UART_Receive(uint8_t *rxbuf, uint32_t buflen, TRANSFER_BLOCK_Type flag)
{
	uint32_t bToRecv, bRecv, timeOut;
	uint8_t *pChar = rxbuf;

	bToRecv = buflen;

	// Blocking mode
	if (flag == BLOCKING) {
		bRecv = 0;
		while (bToRecv){
			timeOut = UART_BLOCKING_TIMEOUT;
			while (!(LPC_UART->LSR & 0x01)){ //check bit RDR
				if (timeOut == 0) break;
				timeOut--;
			}
			// Time out!
			if(timeOut == 0) break;
			// Get data from the buffer
			(*pChar++) = LPC_UART->RBR ;
			bToRecv--;
			bRecv++;
		}
	}
	// None blocking mode
	else {
		bRecv = 0;
		while (bToRecv) {
			if (!(LPC_UART->LSR & 0x01)) {
				break;
			} else {
				(*pChar++) = LPC_UART->RBR;
				bRecv++;
				bToRecv--;
			}
		}
	}
	return bRecv;
}
/******************************************************************************
**                            End Of File
******************************************************************************/
