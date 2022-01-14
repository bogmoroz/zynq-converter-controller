
#include <stdio.h>
#include "platform.h"
#include <xil_printf.h>
#include <zynq_registers.h>
#include <xscugic.h> // Generic interrupt controller (GIC) driver
#include <xgpio.h>

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

/* The XScuGic driver instance data. The user is required to allocate a
 * variable of this type for every intc device in the system. A pointer
 * to a variable of this type is then passed to the driver API functions.
 */
// XScuGic InterruptControllerInstance; // Interrupt controller instance

#define NUMBER_OF_EVENTS 1
#define GO_TO_NEXT_STATE 0 // Switch to next state


#define NUMBER_OF_STATES 3
#define CONFIGURATION_STATE 0 // Configuration mode
#define IDLING_STATE 1 // Idling mode
#define MODULATING_STATE 2 // Modulating mode

int ProcessEvent(int Event);

const char StateChangeTable[NUMBER_OF_STATES][NUMBER_OF_EVENTS]=\
// event  GO_TO_NEXT_STATE
		{ IDLING_STATE,     // CONFIGURATION_STATE
	      MODULATING_STATE,     // IDLING_STATE
	      CONFIGURATION_STATE,  }; // MODULATING_STATE

static int CurrentState = 0;

int ProcessEvent(int Event){

	xil_printf("Processing event with number: %d\n", Event);

    if (Event<= NUMBER_OF_EVENTS){
        CurrentState=StateChangeTable[CurrentState][Event];
    }


    xil_printf("Switching to state: %d\n", CurrentState);

    return CurrentState; // we simply return current state if we receive event out of range
}


float convert(float u)
{
	unsigned int i, j;
	static const float A[6][6] = {
		{0.9652, -0.0172, 0.0057, -0.0058, 0.0052, -0.0251}, /* row 1 */

		{0.7732, 0.1252, 0.2315, 0.07, 0.1282, 0.7754}, /* row 2 */

		{0.8278, -0.7522, -0.0956, 0.3299, -0.4855, 0.3915}, /* row 3 */

		{0.9948, 0.2655, -0.3848, 0.4212, 0.3927, 0.2899}, /* row 4 */

		{0.7648, -0.4165, -0.4855, -0.3366, -0.0986, 0.7281}, /* row 5 */

		{1.1056, 0.7587, 0.1179, 0.0748, -0.2192, 0.1491} /* row 6 */

	};

	static const float B[6] = {
		0.0471,
		0.0377,
		0.0404,
		0.0485,
		0.0373,
		0.0539};

	static float states[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
	static float oldstates[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

	for (i = 0; i < 6; i++)
	{
		states[i] = 0.0;
		for (j = 0; j < 6; j++)
		{
			states[i] = states[i] + A[i][j] * oldstates[j];
		}
		states[i] = states[i] + B[i] * u;
	}

	for (i = 0; i < 6; i++)
	{
		oldstates[i] = states[i];
	}

	return states[5];
};

int main()
{
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

	// Initializes interruptions.
	Status = IntcInitFunction(INTC_DEVICE_ID);

	int i = 1;
	float u0, u, Ki, Kp;
	u0 = 1.5; //reference voltage
	u = 0;	  //actual voltage
	Ki = 0.2;
	Kp = 0.1;
	init_platform();

	while (i < 1000)
	{
		// xil_printf("Lost in  main() loop...\r\n");
		xil_printf("%f\n\r", u);
		u = PI(u0, u, Ki, Kp);
		i++;
	}

//	while (1) {
//		xil_printf("System currently in state: %d\n", CurrentState());
//
//	}

	cleanup_platform();
	return 0;


	// Old interrupt code, currently discarded

	//	xil_printf("Hello World\n\r");
	//	return 0;
	// AXI GPIO Initialization (LEDs)
	//	    AXI_LED_TRI &= ~(0b1111UL); // Direction mode - 0: output
	//	    AXI_LED_DATA = 0b1010UL;    // Initialize one led on - one led off
	//
	//	    // Connect the Intc to the interrupt subsystem such that interrupts can occur.  This function is application specific.
	//	    SetupInterruptSystem(&InterruptControllerInstance);
	//	    // Set up  the Ticker timer
	//	    SetupUART();
	//	    SetupUARTInterrupt(&InterruptControllerInstance);
	//	    SetupTimer();
	//	    SetupTicker(&InterruptControllerInstance);
}
