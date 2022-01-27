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
// #define UARTPS_1_INTR 82U

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

/* Button interrupt */

void initButtonInterrupts()
{
	xil_printf("init_button_interrupts");
	int Status;

	// Initializes BTNS_SWTS as an XGPIO.
	Status = XGpio_Initialize(&BTNS_SWTS, BUTTONS_AXI_ID);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Buttons and switches error\n");
		return XST_FAILURE;
	}

	Status = XGpio_Initialize(&LEDS, LEDS_AXI_ID);
	if (Status != XST_SUCCESS)
	{
		xil_printf("LEDs error\n");
		return XST_FAILURE;
	}

	// xil_printf("Set data direction buttons \n");
	XGpio_SetDataDirection(&BTNS_SWTS, BUTTONS_channel, 0xF);
	// xil_printf("Set data direction switches \n");
	XGpio_SetDataDirection(&BTNS_SWTS, SWITCHES_channel, 0xF);
	// xil_printf("Set data direction switches \n");
	XGpio_SetDataDirection(&LEDS, LEDS_channel, 0x0);

	// xil_printf("Initialize interrupts \n");
	// Initializes interruptions.
	Status = IntcInitFunction(INTC_DEVICE_ID);
}


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

	Xil_ExceptionEnableMask(XIL_EXCEPTION_FIQ);

	return XST_SUCCESS;
}

void PushButtons_Intr_Handler(void *data)
{
	buttons = XGpio_DiscreteRead(&BTNS_SWTS, BUTTONS_channel);
	switch (buttons)
	{
	case LD0:
		xil_printf("Pressed button 0\n");
		ProcessEvent(0);
		break;
	case LD1:
		xil_printf("Pressed button 1\n");
		ProcessEvent(1);
		break;
	case LD2:
		processIncrementDecrementRequest(1);
		break;
	case LD3:
		processIncrementDecrementRequest(0);
		break;
	}
	XGpio_InterruptClear(&BTNS_SWTS, 0xF);
}
