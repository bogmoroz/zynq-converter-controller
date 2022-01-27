
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
#include <xil_exception.h>

#define ENTER_CRITICAL Xil_ExceptionDisable() // Disable Interrupts
#define EXIT_CRITICAL Xil_ExceptionEnable()	  // Enable Interrupts

#define getName(var) #var

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

#define NUMBER_OF_EVENTS 2
#define GO_TO_NEXT_STATE 0 // Switch to next state
#define GO_TO_NEXT_K 1	   // Switch to next state

#define NUMBER_OF_STATES 4
#define CONFIGURATION_STATE_KI 0 // Configuration mode for Ki
#define CONFIGURATION_STATE_KP 1 // Configuration mode for Kp
#define IDLING_STATE 2			 // Idling mode
#define MODULATING_STATE 3		 // Modulating mode

int ProcessEvent(int Event);

static float Ki = 0.001;
static float Kp = 0.01;
static int voltageSetPoint = 50;

void setKi(float n)
{
	Ki = n;
}

float getKi(void)
{
	return Ki;
}

void setKp(float n)
{
	Kp = n;
}

float getKp(void)
{
	return Kp;
}

void setVoltageSetPoint(int n)
{
	voltageSetPoint = n;
}

int getVoltageSetPoint(void)
{
	return voltageSetPoint;
}

const char StateChangeTable[NUMBER_OF_STATES][NUMBER_OF_EVENTS] =
	// event  GO_TO_NEXT_STATE    GO_TO_NEXT_K
	{
		IDLING_STATE, CONFIGURATION_STATE_KP,	   // CONFIGURATION_STATE_KI
		IDLING_STATE, CONFIGURATION_STATE_KI,	   // CONFIGURATION_STATE_KP
		MODULATING_STATE, IDLING_STATE,			   // IDLING_STATE
		CONFIGURATION_STATE_KI, MODULATING_STATE}; // MODULATING_STATE

static int CurrentState = 0;

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

	if (CurrentState == 0)
	{
		xil_printf("Switching to state: %s\n", "CONFIGURATION_STATE_KI");
	}

	if (CurrentState == 1)
	{
		xil_printf("Switching to state: %s\n", "CONFIGURATION_STATE_KP");
	}

	if (CurrentState == 2)
	{
		xil_printf("Switching to state: %s\n", "IDLING_STATE");
	}

	if (CurrentState == 3)
	{
		xil_printf("Switching to state: %s\n", "MODULATING_STATE");
	}

	return CurrentState; // we simply return current state if we receive event out of range
}

int ProcessIncrementRequest()
{
	switch (CurrentState)
	{
	case CONFIGURATION_STATE_KP:
		// increment Kp
		setKp(getKp() + 0.01);
		char outputStringKp[50];
		sprintf(outputStringKp, "%f", getKp());
		xil_printf(outputStringKp);
		xil_printf("\n");
		return;
	case CONFIGURATION_STATE_KI:
		// increment Ki
		setKi(getKi() + 0.01);
		char outputStringKi[50];
		sprintf(outputStringKi, "%f", getKi());
		xil_printf(outputStringKi);
		xil_printf("\n");
		return;
	case MODULATING_STATE:
		// increment voltage set point
		setVoltageSetPoint(getVoltageSetPoint() + 1);
		char outputStringVoltage[50];
		sprintf(outputStringVoltage, "%d", getVoltageSetPoint());
		xil_printf(outputStringVoltage);
		xil_printf("\n");
		return;
	default:
		return;
	}
}

void printFloat(float value)
{
	char outputString[50];
	sprintf(outputString, "%f", value);
	xil_printf(outputString);
	xil_printf("\n");
}

void printInt(int value)
{
	char outputString[50];
	sprintf(outputString, "%d", value);
	xil_printf(outputString);
	xil_printf("\n");
}

/* Semaphores */
//
//int AcquireSemaphore(void)
//{
//	bool check = true;
//	while (check)
//	{
//		while (s)
//			;					// wait for zero
//		Xil_ExceptionDisable(); // Disable interrupts during semaphore access
//		if (s == FALSE)			// check if value of s has changed during execution of last 2 lines
//		{
//			s = TRUE;	   // Activate semaphore
//			check = FALSE; // Leave loop
//		};
//		Xil_ExceptionEnable(); // Enable interrupts after semaphore has been accessed
//	}
//	return s;
//}
//
//int ReleaseSemaphore(void)
//{
//	s = 0;
//	return s;
//}

int main()
{
	initButtonInterrupts();
	setupUART();
	setupTimersAndRGBLed();

	uint16_t match_value = 0;
	uint8_t state = 0;
	volatile u32 *ptr_register = NULL;
	uint16_t rounds = 0;
	// Initializing PID controller and converter values
	float u0, u1, u2, Ki, Kp;
	uint8_t s = 0;
	u0 = getVoltageSetPoint(); //reference voltage - what we want
	u1 = 0;					   //actual voltage out of the controller
	u2 = 0;					   // process variable - voltage out of the converter
	// PID parameters
	// TODO protect them with semaphores
	Ki = 0.001;
	Kp = 0.01;

	while (rounds < 30000)
	{
		char input = uartReceive(); // polling UART receive buffer
		if (input)
		{
			uartSendString("UART console requesting control with command:\n");
			char c[50]; //size of the number
			sprintf(c, "%s", input);
			uartSendString(c);
			uartSendString("\n");
			set_leds(input); // if new data received call set_leds()
		}

		switch (state)
		{
		case 0:
			ptr_register = &TTC0_MATCH_0;
			break; // lets
		case 1:
			*ptr_register = match_value++;
			break;
		case 2:
			*ptr_register = match_value--;
			break;

		case 3:
			ptr_register = &TTC0_MATCH_1_COUNTER_2;
			break; // get
		case 4:
			*ptr_register = match_value++;
			break;
		case 5:
			*ptr_register = match_value--;
			break;

		case 6:
			ptr_register = &TTC0_MATCH_1_COUNTER_3;
			break; // the party
		case 7:
			*ptr_register = match_value++;
			break;
		case 8:
			*ptr_register = match_value--;
			break;

		case 9:
			TTC0_MATCH_0 = TTC0_MATCH_1_COUNTER_2 = TTC0_MATCH_1_COUNTER_3 = match_value++;
			break; // started
		case 10:
			TTC0_MATCH_0 = TTC0_MATCH_1_COUNTER_2 = TTC0_MATCH_1_COUNTER_3 = match_value--;
			break;
		}

		if (match_value == 0)
		{
			state == 10 ? state = 0 : state++; // change state
			// Send reference voltage and current voltage to controller
			u1 = PI(u0, u2, Ki, Kp); // input reference voltage u0, current voltage u2, Ki and Kp to PI controller
			u2 = convert(u1);		 // convert the input from PI controller to output voltage u2
			char c[50];				 //size of the number
			sprintf(c, "%f", u2);
			//			xil_printf(c);
			//			xil_printf("\n");
			rounds = rounds + 1;
		}
	}

	cleanup_platform();
	return 0;
}
