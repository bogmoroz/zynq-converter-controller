/*
 * uart_comms.c
 *
 *  Created on: Jan 27, 2022
 *      Authors: Anssi & Bogdan
 */

#include <xuartps_hw.h>
#include <zynq_registers.h>

/*
* This function initializes the UART - serial connection to PC (from Exercise 4)
*/
void SetupUART()
{
	uint32_t r = 0;			// Temporary value variable
							// Initialize AXI GPIO (LEDS LD3..0 - AXI_LED_DATA[3:0])
	AXI_LED_TRI &= ~0b1111; // Direction Mode (0 = Output)
	AXI_LED_DATA = 0;		// Output value
	r = UART_CTRL;
	r &= ~(XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN); // Clear Tx & Rx Enable
	r |= XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS;	 // Tx & Rx Disable
	UART_CTRL = r;
	UART_MODE = 0;
	UART_MODE &= ~XUARTPS_MR_CLKSEL;					// Clear "Input clock selection" - 0: clock source is uart_ref_clk
	UART_MODE |= XUARTPS_MR_CHARLEN_8_BIT;				// Set "8 bits data"
	UART_MODE |= XUARTPS_MR_PARITY_NONE;				// Set "No parity mode"
	UART_MODE |= XUARTPS_MR_STOPMODE_1_BIT;				// Set "1 stop bit"
	UART_MODE |= XUARTPS_MR_CHMODE_NORM;				// Set "Normal mode"
														// baud_rate = sel_clk / (CD * (BDIV + 1) (ref: UG585 - TRM - Ch. 19 UART)
	UART_BAUD_DIV = 6;									// ("BDIV")
	UART_BAUD_GEN = 124;								// ("CD")
														// Baud Rate = 100Mhz / (124 * (6 + 1)) = 115200 bps
	UART_CTRL |= (XUARTPS_CR_TXRST | XUARTPS_CR_RXRST); // TX & RX logic reset
	r = UART_CTRL;
	r |= XUARTPS_CR_RX_EN | XUARTPS_CR_TX_EN;	   // Set TX & RX enabled
	r &= ~(XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS); // Clear TX & RX disabled
	UART_CTRL = r;
}

// Send one character through UART interface
void uart_send(char c)
{
	while (UART_STATUS & XUARTPS_SR_TNFUL)
		;
	UART_FIFO = c;
	while (UART_STATUS & XUARTPS_SR_TACTIVE)
		;
}


// Send string (character array) through UART interface
void uart_send_string(char str[20])
{
	char *ptr = str;
	while (*ptr != '\0')
	{
		uart_send(*ptr);
		ptr++;
	}
}

// Check if UART receive FIFO is not empty and return the new data
char uart_receive()
{
	if ((UART_STATUS & XUARTPS_SR_RXEMPTY) == XUARTPS_SR_RXEMPTY)
		return 0;
	return UART_FIFO;
}
