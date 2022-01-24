/*
 *	AXI_interrupts.c
 *  Originally created on: 16 Oct 2020
 *  Modified on: 11 Oct 2021
 */

/*
 * NOTICE: These button and switch interrupts have been routed as FIQs in the FPGA,
 * and can't be disabled with Xil_ExceptionDisable().
 *
 * Technical explanation:
 * ARM supports two kinds of hardware interrupts: the regular IRQ interrupts
 * and "fast" FIQ interrupts. These are enabled by clearing two separate bits
 * in current program status register (CPSR): i-bit for IRQs and f-bit for FIQs.
 * Xil_ExceptionEnable() works by invoking ARM assembly code that clears the i-bit
 * and Xil_ExceptionDisable() sets the i-bit. Xil_ExceptionEnableMask(XIL_EXCEPTION_FIQ)
 * clears the f-bit and thus enables FIQ interrupts but for reasons unknown,
 * Xil_ExceptionDisableMask(XIL_EXCEPTION_FIQ) doesn't set the f-bit. While all the other
 * interrupts used in the course are IRQs, the button interrupts were routed as FIQs in the
 * FPGA and so we currently don't know of a way to disable them once they've been enabled.
 *
 * */
#include <xparameters.h>
#include <xgpio.h>
#include <xuartps.h>
#include <xscugic.h>
#include <xil_printf.h>
#include <xuartps_hw.h>
#include <zynq_registers.h>

#define BUTTONS_channel 2
#define BUTTONS_AXI_ID XPAR_AXI_GPIO_SW_BTN_DEVICE_ID

#define SWITCHES_channel 1
#define SWITCHES_AXI_ID XPAR_AXI_GPIO_SW_BTN_DEVICE_ID

#define LEDS_channel 1
#define LEDS_AXI_ID XPAR_AXI_GPIO_LED_DEVICE_ID

#define INTC_DEVICE_ID XPAR_PS7_SCUGIC_0_DEVICE_ID
#define INT_PushButtons 61

// Interrupt (IRQ) ID# number for UART0 (XPAR_XUARTPS_1_INTR / XPS_UART1_INT_ID)
#define UARTPS_1_INTR 82U

#define LD0 0x1
#define LD1 0x2
#define LD2 0x4
#define LD3 0x8

XGpio BTNS_SWTS, LEDS;
static XScuGic INTCInstance;
u8 buttons = 0;

int IntcInitFunction(u16 DeviceId);
int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
void PushButtons_Intr_Handler(void *data);

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

void UartIRQHandler(void)
{
//	Xil_ExceptionDisable();
	xil_printf("UartIRQHandler() called!!!\r\n");

	char input = uart_receive(); // polling UART receive buffer
	if (input)
		set_leds(input); // if new data received call set_leds()

//	Xil_ExceptionEnable();

	//Xil_ExceptionDisable();
	//    UART_ISR = XUARTPS_IXR_MASK;
	//    xil_printf("UartIRQHandler() called!!!\r\n");
	//    AXI_LED_DATA ^= 0b1100; // Toggle (XOR operator (^)) two LEDs
	//    char temp;
	//    temp = uart_receive();
	//    int i = 0;
	//    while (temp)
	//    {
	//        i++;
	//        xil_printf("%d: %d\r\n", i, temp);
	//        temp = uart_receive();
	//    }
	//Xil_ExceptionEnable();
}

//void SetupUARTInterrupt(XScuGic *InterruptControllerInstancePtr)
//{
//	xil_printf("SetupUARTInterrupt() called!!!\r\n");
//
//	Xil_ExceptionDisable();
//
//	uint32_t r = 0; // Temporary value variable
//
//	r = UART_CTRL;
//	r &= ~(XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN); // Clear Tx & Rx Enable
//	r |= XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS;	 // Tx & Rx Disable
//	UART_CTRL = r;
//
//	// Connect to the interrupt controller (pointer to function)
//	//  InterruptControllerInstancePtr->Config->HandlerTable[UARTPS_1_INTR].Handler = (Xil_InterruptHandler)UartIRQHandler;
//
////	int Status;
////	Status = XScuGic_Connect(&InterruptControllerInstancePtr, UARTPS_1_INTR, (Xil_ExceptionHandler)UartIRQHandler, (void *)0);
////	if (Status != XST_SUCCESS)
////	{
////		xil_printf("UART interrupt connection failed\n");
////		return XST_FAILURE;
////	}
////
////	XScuGic_Enable(&InterruptControllerInstancePtr, UARTPS_1_INTR);
//
//  // Connect to the interrupt controller (pointer to function)
//	InterruptControllerInstancePtr->Config->HandlerTable[UARTPS_1_INTR].Handler = (Xil_InterruptHandler)UartIRQHandler;
//
//	// ICDISER1:  Interrupt Controller Distributor (ICD) - Interrupt Set-enable Register (ISER) 2
//	ICDISER1 = 1 << (UARTPS_1_INTR % 32); // Modulo operator (% 2^n) stripping off all but the n LSB bits (removes offset 32)
//
//	UART_IER = 0 | XUARTPS_IXR_RXOVR;
//	UART_IMR = 0 | XUARTPS_IXR_RXOVR;
//	UART_IDR = XUARTPS_IXR_MASK & ~(XUARTPS_IXR_RXOVR);
//
//	//UART_IER = XUARTPS_IXR_MASK;
//	//UART_IMR = XUARTPS_IXR_MASK;
//	//UART_IDR = 0;
//
//	UART_RXWM = XUARTPS_RXWM_MASK & 1U;
//
//	UART_CTRL |= (XUARTPS_CR_TXRST | XUARTPS_CR_RXRST); // TX & RX logic reset
//
//	r = UART_CTRL;
//	r |= XUARTPS_CR_RX_EN | XUARTPS_CR_TX_EN;	   // Set TX & RX enabled
//	r &= ~(XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS); // Clear TX & RX disabled
//	UART_CTRL = r;
//
//	UART_ISR = XUARTPS_IXR_MASK;
//	Xil_ExceptionEnable();
//}

int IntcInitFunction(u16 DeviceId)
{
	xil_printf("Hi\n");
	xil_printf("Initializing interrupts\n");
	int Status;

	// Interrupt controller initialization
	XScuGic_Config *IntcConfig;
	IntcConfig = XScuGic_LookupConfig(DeviceId);

	Status = XScuGic_CfgInitialize(&INTCInstance, IntcConfig, IntcConfig->CpuBaseAddress);
	// xil_printf("Initialization status: %d\n", Status);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Interrupt controller initialization failed\n");
		return XST_FAILURE;
	}

	// Call to interrupt setup
	Status = InterruptSystemSetup(&INTCInstance);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Interrupt system setup failed\n");
		return XST_FAILURE;
	}

	XScuGic_SetPriorityTriggerType(&INTCInstance, INT_PushButtons, 0, 3); //Max priority, rising edge.

	// Setup button interrupt
	Status = XScuGic_Connect(&INTCInstance, INT_PushButtons, (Xil_ExceptionHandler)PushButtons_Intr_Handler, (void *)0);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Button interrupt connection failed\n");
		return XST_FAILURE;
	}
	XScuGic_Enable(&INTCInstance, INT_PushButtons);

	// Setup UART
	xil_printf("Setting up UART \n");
	SetupUART();
//	SetupUARTInterrupt(&INTCInstance);

	// Setup UART interrupt
		Status = XScuGic_Connect(&INTCInstance, UARTPS_1_INTR, (Xil_ExceptionHandler)UartIRQHandler, (void *)0);
		if (Status != XST_SUCCESS) {
			xil_printf("UART interrupt connection failed\n");
			return XST_FAILURE;
		}
	XScuGic_Enable(&INTCInstance, UARTPS_1_INTR);


	return XST_SUCCESS;
}

int InterruptSystemSetup(XScuGic *XScuGicInstancePtr)
{
	/*
	 * Initialize the interrupt controller driver so that it is ready to use.
	 * */

	XGpio_InterruptEnable(&BTNS_SWTS, 0xF);
	XGpio_InterruptGlobalEnable(&BTNS_SWTS);

	Xil_ExceptionInit();

	// Enable interrupts
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_FIQ_INT,
								 (Xil_ExceptionHandler)PushButtons_Intr_Handler,
								 XScuGicInstancePtr);

	//	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_FIQ_INT,
	//									 (Xil_ExceptionHandler)UartIRQHandler,
	//									 XScuGicInstancePtr);
	Xil_ExceptionEnableMask(XIL_EXCEPTION_FIQ);

	// Xil_ExceptionEnable();

	return XST_SUCCESS;
}

void PushButtons_Intr_Handler(void *data)
{
	buttons = XGpio_DiscreteRead(&BTNS_SWTS, BUTTONS_channel);
	xil_printf("A button was pressed\n");
	switch (buttons)
	{
	case LD0:
		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD0); // LD0
		xil_printf("Pressed button 0\n");
		ProcessEvent(0);
		break;
	case LD1:
		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD1); // LD1.
		xil_printf("Pressed button 1\n");
		ProcessEvent(1);
		break;
	case LD2:
		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD2); // LD2.
		break;
	case LD3:
		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD3); // LD3.
		break;
	}
	XGpio_InterruptClear(&BTNS_SWTS, 0xF);
}
