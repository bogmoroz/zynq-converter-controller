///*
// *	AXI_interrupts.c
// *  Originally created on: 16 Oct 2020
// *  Modified on: 11 Oct 2021
// */
//
///*
// * NOTICE: These button and switch interrupts have been routed as FIQs in the FPGA,
// * and can't be disabled with Xil_ExceptionDisable().
// *
// * Technical explanation:
// * ARM supports two kinds of hardware interrupts: the regular IRQ interrupts
// * and "fast" FIQ interrupts. These are enabled by clearing two separate bits
// * in current program status register (CPSR): i-bit for IRQs and f-bit for FIQs.
// * Xil_ExceptionEnable() works by invoking ARM assembly code that clears the i-bit
// * and Xil_ExceptionDisable() sets the i-bit. Xil_ExceptionEnableMask(XIL_EXCEPTION_FIQ)
// * clears the f-bit and thus enables FIQ interrupts but for reasons unknown,
// * Xil_ExceptionDisableMask(XIL_EXCEPTION_FIQ) doesn't set the f-bit. While all the other
// * interrupts used in the course are IRQs, the button interrupts were routed as FIQs in the
// * FPGA and so we currently don't know of a way to disable them once they've been enabled.
// *
// * */
//#include <xparameters.h>
//#include <xgpio.h>
//#include <xuartps.h>
//#include <xscugic.h>
//#include <xil_printf.h>
//
//#define BUTTONS_channel 2
//#define BUTTONS_AXI_ID XPAR_AXI_GPIO_SW_BTN_DEVICE_ID
//
//#define SWITCHES_channel 1
//#define SWITCHES_AXI_ID XPAR_AXI_GPIO_SW_BTN_DEVICE_ID
//
//#define LEDS_channel 1
//#define LEDS_AXI_ID XPAR_AXI_GPIO_LED_DEVICE_ID
//
//#define INTC_DEVICE_ID XPAR_PS7_SCUGIC_0_DEVICE_ID
//#define INT_PushButtons 61
//// #define INT_UARTInput 62
//
//#define LD0 0x1
//#define LD1 0x2
//#define LD2 0x4
//#define LD3 0x8
//
//XGpio BTNS_SWTS, LEDS;
//static XScuGic INTCInstance;
//u8 buttons = 0;
//
//int IntcInitFunction(u16 DeviceId);
//int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
//void PushButtons_Intr_Handler(void *data);
//
//int IntcInitFunction(u16 DeviceId)
//{
//	xil_printf("Hi\n");
//	xil_printf("Initializing interrupts\n");
//	int Status;
//
//	// Interrupt controller initialization
//	XScuGic_Config *IntcConfig;
//	IntcConfig = XScuGic_LookupConfig(DeviceId);
//
//	Status = XScuGic_CfgInitialize(&INTCInstance, IntcConfig, IntcConfig->CpuBaseAddress);
//	// xil_printf("Initialization status: %d\n", Status);
//	if (Status != XST_SUCCESS) {
//		xil_printf("Interrupt controller initialization failed\n");
//		return XST_FAILURE;
//	}
//
//	// Call to interrupt setup
//	Status = InterruptSystemSetup(&INTCInstance);
//	if (Status != XST_SUCCESS) {
//		xil_printf("Interrupt system setup failed\n");
//		return XST_FAILURE;
//	}
//
//	XScuGic_SetPriorityTriggerType(&INTCInstance, INT_PushButtons, 0, 3); //Max priority, rising edge.
//
//	Status = XScuGic_Connect(&INTCInstance, INT_PushButtons, (Xil_ExceptionHandler)PushButtons_Intr_Handler, (void *)0);
//	if (Status != XST_SUCCESS) {
//		xil_printf("Interrupt connection failed\n");
//		return XST_FAILURE;
//	}
//
//
//	XScuGic_Enable(&INTCInstance, INT_PushButtons);
//
//	return XST_SUCCESS;
//}
//
//int InterruptSystemSetup(XScuGic *XScuGicInstancePtr)
//{
//	/*
//	 * Initialize the interrupt controller driver so that it is ready to use.
//	 * */
//
//	XGpio_InterruptEnable(&BTNS_SWTS, 0xF);
//	XGpio_InterruptGlobalEnable(&BTNS_SWTS);
//
//	Xil_ExceptionInit();
//
//	// Enable interrupts
//	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_FIQ_INT,
//								 (Xil_ExceptionHandler)PushButtons_Intr_Handler,
//								 XScuGicInstancePtr);
//	Xil_ExceptionEnableMask(XIL_EXCEPTION_FIQ);
//
//	return XST_SUCCESS;
//}
//
//void PushButtons_Intr_Handler(void *data)
//{
//	buttons = XGpio_DiscreteRead(&BTNS_SWTS, BUTTONS_channel);
//	xil_printf("A button was pressed\n");
//	switch (buttons)
//	{
//	case LD0:
//		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD0); // LD0
//		xil_printf("Pressed button 0\n");
//		// ProcessEvent(0);
//		break;
//	case LD1:
//		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD1); // LD1.
//		break;
//	case LD2:
//		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD2); // LD2.
//		break;
//	case LD3:
//		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD3); // LD3.
//		break;
//	}
//	XGpio_InterruptClear(&BTNS_SWTS, 0xF);
//}
