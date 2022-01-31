/*
 * interrupts_buttons.c
 *
 *  Created on: Jan 27, 2022
 *  Authors: Bogdan Moroz &  Anssi Ronkainen
 */
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

#define LD0 0x1
#define LD1 0x2
#define LD2 0x4
#define LD3 0x8

XGpio BTNS_SWTS, LEDS;
static XScuGic INTCInstance;
u8 buttons = 0;


void initButtonInterrupts();
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

	XGpio_SetDataDirection(&BTNS_SWTS, BUTTONS_channel, 0xF);
	XGpio_SetDataDirection(&BTNS_SWTS, SWITCHES_channel, 0xF);
	XGpio_SetDataDirection(&LEDS, LEDS_channel, 0x0);

	// Initializes interruptions
	Status = IntcInitFunction(INTC_DEVICE_ID);
}

int IntcInitFunction(u16 DeviceId)
{
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
	int semaphoreState = acquireSemaphore(1);
	switch (buttons)
	{
	case LD0:
		// Pressed button zero
		// Check if the resource protected by semaphore is available
		if (semaphoreState == 1)
		{
			//Buttons have control over the system, send event 0 (i.e. "go to next system state")
			ProcessEvent(0);
		}
		break;
	case LD1:
		// Pressed button one
		// Check if the resource protected by semaphore is available
		if (semaphoreState == 1)
		{
			/*
			 * Buttons have control over the system, send event 1 (i.e.
			 * "go to CONFIGURATION_STATE_KI if in CONFIGURATION_STATE_KP" and go
			 * to CONFIGURATION_STATE_KP if in CONFIGURATION_STATE_KI)
			 */
			ProcessEvent(1);
		}
		break;
	case LD2:
		// Pressed button two
		// Check if the resource protected by semaphore is available
		if (semaphoreState == 1)
		{
			/*
			 * Buttons have control over the system, send increment or decrement request.
			 * The request increments Ki, Kp or the set point voltage, based on the state of the system.
			 * processIncrementDecrementRequest interprets 1 as an increment request, and 0 as a decrement request.
			 */
			processIncrementDecrementRequest(1);
		}
		break;
	case LD3:
		// Pressed button two
	    // Check if the resource protected by semaphore is available
		if (semaphoreState == 1)
		{
			/*
			 * Buttons have control over the system, send increment or decrement request.
			 * The request decrements Ki, Kp or the set point voltage, based on the state of the system.
			 * processIncrementDecrementRequest interprets 1 as an increment request, and 0 as a decrement request.
			 */
			processIncrementDecrementRequest(0);
		}
		break;
	}
	XGpio_InterruptClear(&BTNS_SWTS, 0xF);
}
