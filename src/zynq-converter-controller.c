
#include <stdio.h>
#include "platform.h"
#include <xil_printf.h>
#include <zynq_registers.h>
#include <xscugic.h> // Generic interrupt controller (GIC) driver
#include <xgpio.h>
#include <xttcps.h>
#include <xuartps_hw.h>
#include <xparameters.h>
#include <xuartps_hw.h>
#include <xscugic.h>

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

XScuGic InterruptControllerInstance; // Interrupt controller instance

/* The XScuGic driver instance data. The user is required to allocate a
 * variable of this type for every intc device in the system. A pointer
 * to a variable of this type is then passed to the driver API functions.
 */
// XScuGic InterruptControllerInstance; // Interrupt controller instance

#define NUMBER_OF_EVENTS 1
#define GO_TO_NEXT_STATE 0 // Switch to next state

#define NUMBER_OF_STATES 3
#define CONFIGURATION_STATE 0 // Configuration mode
#define IDLING_STATE 1		  // Idling mode
#define MODULATING_STATE 2	  // Modulating mode

int ProcessEvent(int Event);

const char StateChangeTable[NUMBER_OF_STATES][NUMBER_OF_EVENTS] = // event  GO_TO_NEXT_STATE
	{
		IDLING_STATE,	  // CONFIGURATION_STATE
		MODULATING_STATE, // IDLING_STATE
		CONFIGURATION_STATE,
}; // MODULATING_STATE

static int CurrentState = 0;

// Send one character through UART interface
void uart_send(char c)
{
	while (UART_STATUS & XUARTPS_SR_TNFUL)
		;
	UART_FIFO = c;
	while (UART_STATUS & XUARTPS_SR_TACTIVE)
		;
}

//#define BUFFER_SIZE 20
//static char str[] = "\tHello World\r";

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

// Set LED outputs based on character value '1', '2', '3', '4'
void set_leds(uint8_t input)
{
	if (input < '0' || '4' < input)
		return;
	uint8_t mask = 0;
	// In a character table, '0' is the first out of the numbers.
	// Its integer value can be something like 48 (ASCII).
	// By subtracting it from input, we arrive at the actual
	// number it's representing.
	char c = input - '0';
	for (uint8_t i = 0; i < c; i++)
	{
		mask <<= 1;	 // Bitwise left shift assignment
		mask |= 0b1; // Set first bit true / led on
	}
	AXI_LED_DATA = mask; // LEDS LD3..0 - AXI LED DATA GPIO register bits [3:0]
}

int ProcessEvent(int Event)
{

	xil_printf("Processing event with number: %d\n", Event);

	if (Event <= NUMBER_OF_EVENTS)
	{
		CurrentState = StateChangeTable[CurrentState][Event];
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

void init_button_interrupts()
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

int main()
{
	xil_printf("Line of main method: %d\n", 232);
	init_button_interrupts();

	/* PWM input */
	// Setup timer
	TTC0_CLK_CNTRL = (0 << XTTCPS_CLK_CNTRL_PS_VAL_SHIFT) | XTTCPS_CLK_CNTRL_PS_EN_MASK;
	TTC0_CNT_CNTRL = XTTCPS_CNT_CNTRL_RST_MASK | XTTCPS_CNT_CNTRL_DIS_MASK | XTTCPS_CNT_CNTRL_MATCH_MASK | XTTCPS_CNT_CNTRL_POL_WAVE_MASK;
	TTC0_MATCH_0 = 0;
	TTC0_CNT_CNTRL &= ~XTTCPS_CNT_CNTRL_DIS_MASK;
	//
	uint16_t match_value = 0;
	uint8_t state = 0;
	volatile u32 *ptr_register = NULL;
	uint16_t rounds = 0;
	// Initializing PID controller and converter values
	float u0, u1, u2, Ki, Kp;
	u0 = 50; //reference voltage - what we want
	u1 = 0;	 //actual voltage out of the controller
	u2 = 0;	 // process variable - voltage out of the converter
	// PID parameters
	// TODO protect them with semaphores
	Ki = 0.001;
	Kp = 0.01;

	//
	//setup_uart_comms();

	while (1)
	{
		// Count to big number before exiting for loop - delay loop
		//		for (uint32_t i = 1; i < (1 << 20); i++)
		//		{
		//			char input = uart_receive(); // polling UART receive buffer
		//			if (input)
		//				set_leds(input); // if new data received call set_leds()
		//		}
		//		static char c = '0';
		//		uart_send(c++); // Send and increment character variable c
		//		// If incremented over 'Z', initialize to '0' (ref: see ASCII character table)
		//		if (c > 'Z')
		//			c = '0';
		//		uart_send_string(str); // Send character array variable str
	}

	while (1)
	{
		// Notthing
	}

	while (rounds < 30000)
	{

		if (match_value == 0)
		{
			switch (state)
			{
			case 0:
				ptr_register = &TTC0_MATCH_0;
				break;
			case 1:
				*ptr_register = match_value++;
				break;
			case 2:
				*ptr_register = match_value--;
				break;
			}

			state == 2 ? state = 0 : state++; // change state
			// Send reference voltage and current voltage to controller
			u1 = PI(u0, u2, Ki, Kp);
			u2 = convert(u1);
			char c[50]; //size of the number
			sprintf(c, "%f", u2);
			xil_printf(c);
			xil_printf("\n");
		}

		rounds = rounds + 1;
	}

	cleanup_platform();
	return 0;
}
