/***********************************************************************//**
 * @file	: lpc13xx_uart.h
 * @brief	: Contains all macro definitions and function prototypes support for UART firmware library on LPC13xx
 * @version	: 1.0
 * @date	: 18. Mar. 2009
 * @author	: HieuNguyen.  Modify	: 19. August. 2009 by NguyenCao
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

#ifndef LPC13XX_UART_H_
#define LPC13XX_UART_H_

/* Includes ------------------------------------------------------------------- */
#include "lpc_types.h"

/*-------------------------Macro definition--------------------------------*/
/* UART interrupt identification register defines */
#define UART_IIR_INTSTAT_PEND	((uint32_t)(1<<0))	/*!<Interrupt Status - Active low */
#define UART_IIR_INTID_RLS		((uint32_t)(3<<1)) 	/*!<Interrupt identification: Receive line status*/
#define UART_IIR_INTID_RDA		((uint32_t)(2<<1)) 	/*!<Interrupt identification: Receive data available*/
#define UART_IIR_INTID_CTI		((uint32_t)(6<<1)) 	/*!<Interrupt identification: Character time-out indicator*/
#define UART_IIR_INTID_THRE		((uint32_t)(1<<1)) 	/*!<Interrupt identification: THRE interrupt*/
#define UART1_IIR_INTID_MODEM	((uint32_t)(0<<1)) 	/*!<Interrupt identification: Modem interrupt*/
#define UART_IIR_INTID_MASK		((uint32_t)(7<<1))	/*!<Interrupt identification: Interrupt ID mask */
#define UART_IIR_FIFO_EN		((uint32_t)(3<<6)) 	/*!<These bits are equivalent to UnFCR[0] */
#define UART_IIR_ABEO_INT		((uint32_t)(1<<8)) 	/*!< End of auto-baud interrupt */
#define UART_IIR_ABTO_INT		((uint32_t)(1<<9)) 	/*!< Auto-baud time-out interrupt */
#define UART_IIR_BITMASK		((uint32_t)(0x3CF))	/*!< UART interrupt identification register bit mask */

/* Macro defines for UART line status register */
#define UART_LSR_RDR		((uint8_t)(1<<0)) 	/*!<Line status register: Receive data ready*/
#define UART_LSR_OE			((uint8_t)(1<<1)) 	/*!<Line status register: Overrun error*/
#define UART_LSR_PE			((uint8_t)(1<<2)) 	/*!<Line status register: Parity error*/
#define UART_LSR_FE			((uint8_t)(1<<3)) 	/*!<Line status register: Framing error*/
#define UART_LSR_BI			((uint8_t)(1<<4)) 	/*!<Line status register: Break interrupt*/
#define UART_LSR_THRE		((uint8_t)(1<<5)) 	/*!<Line status register: Transmit holding register empty*/
#define UART_LSR_TEMT		((uint8_t)(1<<6)) 	/*!<Line status register: Transmitter empty*/
#define UART_LSR_RXFE		((uint8_t)(1<<7)) 	/*!<Error in RX FIFO*/
#define UART_LSR_BITMASK	((uint8_t)(0xFF)) 	/*!<UART Line status bit mask */

/* Macro defines for UART interrupt enable register */
#define UART_IER_RBRINT_EN		((uint32_t)(1<<0)) 	/*!< RBR Interrupt enable*/
#define UART_IER_THREINT_EN		((uint32_t)(1<<1)) 	/*!< THR Interrupt enable*/
#define UART_IER_RLSINT_EN		((uint32_t)(1<<2)) 	/*!< RX line status interrupt enable*/
#define UART1_IER_MSINT_EN		((uint32_t)(1<<3))	/*!< Modem status interrupt enable */
#define UART1_IER_CTSINT_EN		((uint32_t)(1<<7))	/*!< CTS1 signal transition interrupt enable */
#define UART_IER_ABEOINT_EN		((uint32_t)(1<<8)) 	/*!< Enables the end of auto-baud interrupt */
#define UART_IER_ABTOINT_EN		((uint32_t)(1<<9)) 	/*!< Enables the auto-baud time-out interrupt */
#define UART_IER_BITMASK		((uint32_t)(0x307)) /*!< UART interrupt enable register bit mask */
#define UART1_IER_BITMASK		((uint32_t)(0x38F)) /*!< UART1 interrupt enable register bit mask */

/* Macro defines for UART Auto baudrate control register */
#define UART_ACR_START				((uint32_t)(1<<0))	/**< UART Auto-baud start */
#define UART_ACR_MODE				((uint32_t)(1<<1))	/**< UART Auto baudrate Mode 1 */
#define UART_ACR_AUTO_RESTART		((uint32_t)(1<<2))	/**< UART Auto baudrate restart */
#define UART_ACR_ABEOINT_CLR		((uint32_t)(1<<8))	/**< UART End of auto-baud interrupt clear */
#define UART_ACR_ABTOINT_CLR		((uint32_t)(1<<9))	/**< UART Auto-baud time-out interrupt clear */
#define UART_ACR_BITMASK			((uint32_t)(0x307))	/**< UART Auto Baudrate register bit mask */

#define BUFSIZE		0x40
#define UART_TX_FIFO_SIZE		(16)
#define UART_BLOCKING_TIMEOUT			(0xFFFFFFFFUL)


/*-------------------------Type definition--------------------------------*/
typedef enum {
	UART_DATABIT_5		= 0,     		/* UART 5 bit data mode */
	UART_DATABIT_6,		     			/* UART 6 bit data mode */
	UART_DATABIT_7,		     			/* UART 7 bit data mode */
	UART_DATABIT_8		     			/* UART 8 bit data mode */
} UART_DATABIT_Type;

typedef enum {
	UART_STOPBIT_1		= (0),   		/* UART 1 Stop Bits Select */
	UART_STOPBIT_2,		 				/* UART Two Stop Bits Select */
} UART_STOPBIT_Type;

typedef enum {
	UART_PARITY_NONE 	= 0,			/* No parity */
	UART_PARITY_ODD,	 				/* Odd parity */
	UART_PARITY_EVEN, 					/* Even parity */
	UART_PARITY_SP_1, 					/* Forced "1" stick parity */
	UART_PARITY_SP_0 					/* Forced "0" stick parity */
} UART_PARITY_Type;

typedef struct {
  uint32_t Baud_rate;   		/* UART baud rate */
  UART_PARITY_Type Parity;    	/* Parity selection, should be:
							   - UART_PARITY_NONE: No parity
							   - UART_PARITY_ODD: Odd parity
							   - UART_PARITY_EVEN: Even parity
							   - UART_PARITY_SP_1: Forced "1" stick parity
							   - UART_PARITY_SP_0: Forced "0" stick parity
							   */
  UART_DATABIT_Type Databits;   /* Number of data bits, should be:
							   - UART_DATABIT_5: UART 5 bit data mode
							   - UART_DATABIT_6: UART 6 bit data mode
							   - UART_DATABIT_7: UART 7 bit data mode
							   - UART_DATABIT_8: UART 8 bit data mode
							   */
  UART_STOPBIT_Type Stopbits;   /* Number of stop bits, should be:
							   - UART_STOPBIT_1: UART 1 Stop Bits Select
							   - UART_STOPBIT_2: UART 2 Stop Bits Select
							   */
} UART_CFG_Type;

typedef enum {

	UART_FIFO_TRGLEV0 = 0,	/* UART FIFO trigger level 0: 1 character */
	UART_FIFO_TRGLEV1, 		/* UART FIFO trigger level 1: 4 character */
	UART_FIFO_TRGLEV2,		/* UART FIFO trigger level 2: 8 character */
	UART_FIFO_TRGLEV3,		/* UART FIFO trigger level 3: 14 character */
	UART_FIFO_DISABLE	    /* Disable FIFO */
} UART_FITO_LEVEL_Type;

typedef enum {
	UART_INTCFG_RBR = 0,	/*!< RBR Interrupt enable*/
	UART_INTCFG_THRE,		/*!< THR Interrupt enable*/
	UART_INTCFG_RLS,		/*!< RX line status interrupt enable*/
	UART_INTCFG_ABEO,		/*!< Enables the end of auto-baud interrupt */
	UART_INTCFG_ABTO		/*!< Enables the auto-baud time-out interrupt */
} UART_INT_Type;

typedef enum {
	UART_AB_MODE0 = 0, 	/** Autobaud mode 0 */
	UART_AB_MODE1		/** Autobaud mode 1 */
}UART_AB_MODE_Type;

/*-------------------------Function definition--------------------------------*/
void UART_Init(void);
void UART_DeInit(void);

Status UART_SetDatabit(UART_DATABIT_Type databit);
Status UART_SetParity(UART_PARITY_Type parity);
Status UART_SetStopbit(UART_STOPBIT_Type stopbit);
void UART_SetBaudrate(uint32_t baudrate);

void UART_FIFOConfig(UART_FITO_LEVEL_Type level);
void UART_ResetFIFO(void);
void UART_IntConfig(UART_INT_Type IntType, FunctionalState NewState);
uint8_t UART_GetLineStatus(void);
void UART_TXCmd(FunctionalState NewState);
FlagStatus UART_CheckBusy();
void UART_ABEnable(UART_AB_MODE_Type ABMode, FunctionalState ABRestart);
void UART_ABDisable(void);

uint32_t UART_Send(uint8_t *txbuf, uint32_t buflen, TRANSFER_BLOCK_Type flag);
uint32_t UART_Receive(uint8_t *rxbuf, uint32_t buflen, TRANSFER_BLOCK_Type flag);

void UART_IRQHandler(void);

#endif /* LPC13XX_UART_H_ */

/**
 * @}
 */
/*****************************************************************************
**                            End Of File
******************************************************************************/
