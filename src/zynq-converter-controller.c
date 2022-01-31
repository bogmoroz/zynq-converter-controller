/*
 * zynq-converter-controller.c
 *
 *  Created on: Jan 7, 2022
 *  Authors: Bogdan Moroz &  Anssi Ronkainen
 */
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
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define ENTER_CRITICAL Xil_ExceptionDisable() // Disable Interrupts
#define EXIT_CRITICAL Xil_ExceptionEnable()	  // Enable Interrupts

#define BUTTONS_channel 2 // Defining buttons for interrupts
#define BUTTONS_AXI_ID XPAR_AXI_GPIO_SW_BTN_DEVICE_ID

#define SWITCHES_channel 1 // Defining switches for interrupts, not used
#define SWITCHES_AXI_ID XPAR_AXI_GPIO_SW_BTN_DEVICE_ID

#define LEDS_channel 1 // Defining LEDs
#define LEDS_AXI_ID XPAR_AXI_GPIO_LED_DEVICE_ID

#define INTC_DEVICE_ID XPAR_PS7_SCUGIC_0_DEVICE_ID
#define INT_PushButtons 61 // Button interrupts

#define LD0 0x1 // LED 1
#define LD1 0x2 // LED 2
#define LD2 0x4 // LED 3
#define LD3 0x8 // LED 4

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

int ProcessEvent();

float Ki = 0.001;			  // initialize Ki with value 0.001
float Kp = 0.01;			  // initialize Kp with value 0.01
float voltageSetPoint = 50.0; // initialize reference voltage with value 50
float converterOutputVoltate = 0.0;

// Semaphore: 0 for unlocked, 1 for locked with buttons, 2 for locked with uart
int semaphoreState = 0;
int semaphoreLockedPeriod = 0; // initialize semaphore timeout variable

void setSemaphoreState(int n)
{
	semaphoreState = n;
}

float getSemaphoreState(void)
{
	return semaphoreState;
}

void setSemaphoreLockedPeriod(int n)
{
	semaphoreLockedPeriod = n;
}

float getSemaphoreLockedPeriod(void)
{
	return semaphoreLockedPeriod;
}

void setKi(float n) // Set Ki value
{
	Ki = n;
}

float getKi(void) // Get Ki value
{
	return Ki;
}

void setKp(float n) // Set Kp value
{
	Kp = n;
}

float getKp(void) // Get Kp value
{
	return Kp;
}

void setVoltageSetPoint(float n) // Set Reference Voltage value
{
	voltageSetPoint = n;
}

float getVoltageSetPoint(void) // Get Reference Voltage value
{
	return voltageSetPoint;
}

void setConverterOutputVoltate(float n)
{
	converterOutputVoltate = n;
}

float getConverterOutputVoltate(void)
{
	return converterOutputVoltate;
}
/* Setup State change table for buttons, 
first button switches between idling state, modulating state and configuration state Ki
second button switches between Ki and Kp in configuration state, otherwise it switches to modulating state*/

const char StateChangeTable[NUMBER_OF_STATES][NUMBER_OF_EVENTS] =
	// event  GO_TO_NEXT_STATE    GO_TO_NEXT_K
	{
		IDLING_STATE, CONFIGURATION_STATE_KP,	   // CONFIGURATION_STATE_KI
		IDLING_STATE, CONFIGURATION_STATE_KI,	   // CONFIGURATION_STATE_KP
		MODULATING_STATE, IDLING_STATE,			   // IDLING_STATE
		CONFIGURATION_STATE_KI, MODULATING_STATE}; // MODULATING_STATE

static int CurrentState = 0; // initialize state to 0 = configuration state Ki

void setCurrentState(int n) // Set current state (UART)
{
	CurrentState = n;	 // Set current state from the input received from UART
	printCurrentState(); // print to which state we are transitioning
	printSystemState();	 //  print current system values and state
	switch (CurrentState)
	{
	case CONFIGURATION_STATE_KI:
		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD0); // light up led 1 if we are in config Ki state
		return;
	case CONFIGURATION_STATE_KP:
		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD1); // light up led 2 if we are in config Kp state
		return;
	case IDLING_STATE:
		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD2); // light up led 3 if we are in idling state
		return;
	case MODULATING_STATE:
		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD3); // light up led 4 if we are in modulating state
		return;
	default:
		return;
	}
}

int getCurrentState() // Current state
{
	return CurrentState; // Return current state
}

int ProcessEvent(int Event) // Process state changes made with buttons
{

	xil_printf("Processing event with number: %d\n", Event); // Button pressed

	if (Event <= NUMBER_OF_EVENTS)
	{
		setCurrentState(StateChangeTable[CurrentState][Event]); // State change according to the state change table
	}

	return CurrentState; // we simply return current state if we receive event out of range
}

void printCurrentState()
{
	if (CurrentState == 0)
	{
		xil_printf("Switching to state: %s\n", "CONFIGURATION_STATE_KI"); // Inform user we are changing to CONFIGURATION_STATE_KI
	}

	if (CurrentState == 1)
	{
		xil_printf("Switching to state: %s\n", "CONFIGURATION_STATE_KP"); // Inform user we are changing to CONFIGURATION_STATE_KP
	}

	if (CurrentState == 2)
	{
		xil_printf("Switching to state: %s\n", "IDLING_STATE"); // Inform user we are changing to IDLING_STATE
	}

	if (CurrentState == 3)
	{
		xil_printf("Switching to state: %s\n", "MODULATING_STATE"); // Inform user we are changing to MODULATING_STATE
	}
}

/**
 * Function that increments or decrements a given value based on the current state of the system.
 * In CONFIGURATION_STATE_KP, the function increments or decrements Kp.
 * In CONFIGURATION_STATE_KI, the function increments or decrements Ki.
 * In MODULATING_STATE, the function increments or decrements the voltage set point.
 *
 * @parameter command - 1 means "increment", 0 means "decrement"
 */
void processIncrementDecrementRequest(int command)
{
	switch (CurrentState) // Check which state we are in and change parameter values with buttons
	{
	case CONFIGURATION_STATE_KP:

		if (command == 1) // increment Kp
		{
			setKp(getKp() + 0.01);
		}
		else if (command == 0) // decrement Kp
		{
			setKp(getKp() - 0.01);
		}

		printSystemState();
		return;
	case CONFIGURATION_STATE_KI:

		if (command == 1) // increment Ki
		{
			setKi(getKi() + 0.001);
		}
		else if (command == 0) // decrement Ki
		{
			setKi(getKi() - 0.001);
		}

		printSystemState();
		return;
	case MODULATING_STATE:

		if (command == 1) // increment voltage set point
		{
			setVoltageSetPoint(getVoltageSetPoint() + 1);
		}
		else if (command == 0) // decrement voltage set point
		{
			setVoltageSetPoint(getVoltageSetPoint() - 1);
		}

		printSystemState();
		return;
	default:
		return;
	}
}

/* Semaphores */
/**
 * Function that increments or decrements a given value based on the current state of the system.
 * In CONFIGURATION_STATE_KP, the function increments or decrements Kp.
 * In CONFIGURATION_STATE_KI, the function increments or decrements Ki.
 * In MODULATING_STATE, the function increments or decrements the voltage set point.
 *
 * @parameter codeOfRequestSource - 1 means "buttons", 0 means "UART"
 * @returns semaphoreState
 */
//0 for unlocked 1 for buttons, 2 for UART
int acquireSemaphore(int codeOfRequestSource)
{
	bool check = true;
	while (check)
	{
		int semaphoreState = getSemaphoreState();

		Xil_ExceptionDisable(); // Disable interrupts during semaphore access
		if (semaphoreState == 0)
		{
			setSemaphoreState(codeOfRequestSource); // Activate semaphore, grant access to the requesting source
			xil_printf("Acquiring semaphore...\n");
			// Start timer value to release the semaphore after a timeout
			setSemaphoreLockedPeriod(0);
		}
		else if (semaphoreState != codeOfRequestSource) // If one source requests access when another source has control over the semaphore
		{
			// Tell user semaphore is locked
			if (semaphoreState == 1)
			{
				uartSendString("Trying to use UART but semaphore locked by buttons, please wait a few seconds...\n");
			}
			else
			{
				uartSendString("Trying to use buttons but semaphore locked by UART, please wait a few seconds...\n");
			}
		}
		else if (semaphoreState == codeOfRequestSource)
		{
			// Resource that is already using the semaphore is requesting access again, reset the semaphore release timeout
			setSemaphoreLockedPeriod(0);
		}

		check = false;		   // Leave loop
		Xil_ExceptionEnable(); // Enable interrupts after semaphore has been accessed
	}
	return semaphoreState;
}

void releaseSemaphore(void) // releasing semaphore
{
	setSemaphoreLockedPeriod(0);
	setSemaphoreState(0); // semaphore released
}

/**
 * PI controller
 */
float PI(float y_ref, float y_act, float Ki, float Kp) // PI controller
{
	static float u1_old = 0;		  // initialize old value of u1
	float error_new, u1_new;		  // initialize new value of u1 and error
	float u1_max = 1.5;				  // maximum difference between new and old value for saturation
	error_new = y_ref - y_act;		  // error calculation
	u1_new = u1_old + Ki * error_new; // calculate new value of u1
	if (abs(u1_new) > u1_max)		  // check for saturation
	{
		u1_new = u1_old;
	}
	u1_old = u1_new;
	return u1_new + Kp * error_new; // return new controller output value
}

/* Converter model */
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

	return states[5]; // Return u3 as output
};

/*
 * atof is the only function we need from stdlib.h, so instead of importing stdlib we
 * are defining atof here to optimize the program
 *
 * @parameter s - string that we want to convert to float
 * @returns float value parsed from the string parameter
 */
float atof(const char *s)
{
	float a = 0.0;
	int e = 0;
	int c;
	while ((c = *s++) != '\0' && isdigit(c))
	{
		a = a * 10.0 + (c - '0');
	}
	if (c == '.')
	{
		while ((c = *s++) != '\0' && isdigit(c))
		{
			a = a * 10.0 + (c - '0');
			e = e - 1;
		}
	}
	if (c == 'e' || c == 'E')
	{
		int sign = 1;
		int i = 0;
		c = *s++;
		if (c == '+')
			c = *s++;
		else if (c == '-')
		{
			c = *s++;
			sign = -1;
		}
		while (isdigit(c))
		{
			i = i * 10 + (c - '0');
			c = *s++;
		}
		e += i * sign;
	}
	while (e > 0)
	{
		a *= 10.0;
		e--;
	}
	while (e < 0)
	{
		a *= 0.1;
		e++;
	}
	return a;
}

/*
 * print the current status of the system
 */
void printSystemState()
{
	uartSendString("=================================\n");
	uartSendString("System is now in state: ");
	int systemState = getCurrentState();
	if (systemState == CONFIGURATION_STATE_KI)
	{
		uartSendString("CONFIGURATION_STATE_KI");
	}
	else if (systemState == CONFIGURATION_STATE_KP)
	{
		uartSendString("CONFIGURATION_STATE_KP");
	}
	else if (systemState == IDLING_STATE)
	{
		uartSendString("IDLING_STATE");
	}
	else if (systemState == MODULATING_STATE)
	{
		uartSendString("MODULATING_STATE");
	}
	uartSendString("\n");

	uartSendString("PI controller parameters are as follows:\n");
	char outputString[50];
	sprintf(outputString, "Ki: %f, ", getKi());
	uartSendString(outputString);
	sprintf(outputString, "Kp: %f, ", getKp());
	uartSendString(outputString);

	sprintf(outputString, "Voltage Set Point: %f volts.", getVoltageSetPoint());
	uartSendString(outputString);
	uartSendString("\n");

	if (systemState == MODULATING_STATE)
	{
		sprintf(outputString, "Current converter output: %f volts.", getConverterOutputVoltate());
		uartSendString(outputString);
		uartSendString("\n");
	}

	int semaphoreState = getSemaphoreState();
	if (semaphoreState == 0)
	{
		uartSendString("Semaphore is free.\n");
	}
	else if (semaphoreState == 1)
	{
		uartSendString("Buttons control the system, UART input blocked. \n");
	}
	else if (semaphoreState == 2)
	{
		uartSendString("UART controls the system, button input blocked for a period. \n");
	}

	uartSendString("=================================\n");
}

int main()
{
	initButtonInterrupts(); // initialize button interrupts
	setupUART();			// setup uart
	setupRGBLed();			// Configure RGB led used to display controller output

	setCurrentState(CONFIGURATION_STATE_KI); // Put the system into its initial state

	uint16_t match_value = 0;		   // 16 bit match value for timers
	uint8_t state = 0;				   // state for match_value -- increment when state 1, decrement when state 2
	volatile u32 *ptr_register = NULL; // pointer to RBG LED Register
	uint16_t rounds = 0;			   // for timing purposes
	float u1, u2, Ki, Kp;			   // Initializing PID controller and converter values
	u1 = 0;							   //actual voltage out of the controller
	u2 = 0;							   // process variable - voltage out of the converter

	while (1)
	{
		char input = '1';
		input = uartReceive(); // Receive constantly from UART

		// The following if-else block handles input from UART (string and numeric)
		if (input) // if something is received from UART
		{
			int index = 0;

			char rx_buf[30];
			rx_buf[0] = input;

			while (input != '\r') // run one character at a time until we reach the end
			{
				input = uartReceive(); // check one character from UART input
				if (input)
				{
					++index;
					rx_buf[index] = input; // put the received character into rx_buffer
				}
			}

			if (isdigit(*rx_buf) && index >= 1) // check if the received character was a number
			{
				float resolvedNumber = atof(rx_buf); // change received number from string to float

				if (getCurrentState() == CONFIGURATION_STATE_KI) // if we are in CONFIGURATION_STATE_KI
				{
					int semaphoreState = acquireSemaphore(2); // check if the semaphore is reserved for UART
					if (semaphoreState == 2)
					{
						setKi(resolvedNumber); // set new Ki value with the value received from UART
						char outputStringKi[50];
						sprintf(outputStringKi, "%f", getKi()); // print new value of Ki
						xil_printf(outputStringKi);
						xil_printf("\n");
					}
				}
				else if (getCurrentState() == CONFIGURATION_STATE_KP)
				{
					int semaphoreState = acquireSemaphore(2); // check if the semaphore is reserved for UART
					if (semaphoreState == 2)
					{
						setKp(resolvedNumber); // set new Kp value with the value received from UART
						char outputStringKp[50];
						sprintf(outputStringKp, "%f", getKp()); // print new value of Kp
						xil_printf(outputStringKp);
						xil_printf("\n");
					}
				}
				else if (getCurrentState() == MODULATING_STATE)
				{

					int semaphoreState = acquireSemaphore(2); // check if the semaphore is reserved for UART
					if (semaphoreState == 2)
					{
						setVoltageSetPoint(resolvedNumber); // set new reference voltage value with the value received from UART
						char outputStringVoltage[50];
						sprintf(outputStringVoltage, "%f", getVoltageSetPoint()); // print new value of reference voltage
						xil_printf(outputStringVoltage);
						xil_printf("\n");
					}
				}
				printSystemState();
			}
			else
			{
				// if the received UART input is one the defined states, we change state accordingly

				// if the input string is equal to CONFIGURATION_STATE_KI
				/*
				 * rx_buf has a fixed length of 30. When we read an input string, we write the first several characters of rx_buf.
				 * The length of the read input is equal to "index". With strncmp we compare the "substring" of rx_buf of length equal to index
				 * to the provided system state string. This allows us to discard the autofilled end of
				 * the rx_buf and only compare the input prefix to the state string.
				 */
				if (index > 1 && strncmp("CONFIGURATION_STATE_KI", rx_buf, index) == 0)
				{
					int semaphoreState = acquireSemaphore(2); // check if the semaphore is reserved for UART
					if (semaphoreState == 2)
					{
						setCurrentState(CONFIGURATION_STATE_KI); // Switch state to CONFIGURATION_STATE_KI
					}
				}
				// if the input string is equal to CONFIGURATION_STATE_KP
				else if (index > 1 && strncmp("CONFIGURATION_STATE_KP", rx_buf, index) == 0)
				{
					int semaphoreState = acquireSemaphore(2); // check if the semaphore is reserved for UART
					if (semaphoreState == 2)
					{
						setCurrentState(CONFIGURATION_STATE_KP); // Switch state to CONFIGURATION_STATE_KP
					}
				}
				// if the input string is equal to IDLING_STATE
				else if (index > 1 && strncmp("IDLING_STATE", rx_buf, index) == 0)
				{
					int semaphoreState = acquireSemaphore(2); // check if the semaphore is reserved for UART
					if (semaphoreState == 2)
					{
						setCurrentState(IDLING_STATE); // Switch state to IDLING_STATE
					}
				}
				// if the input string is equal to MODULATING_STATE
				else if (index > 1 && strncmp("MODULATING_STATE", rx_buf, index) == 0)
				{
					int semaphoreState = acquireSemaphore(2); // check if the semaphore is reserved for UART
					if (semaphoreState == 2)
					{
						setCurrentState(MODULATING_STATE); // Switch state to MODULATING_STATE
					}
				}
			}
		}

		switch (state)
		{
		case 0:
			ptr_register = &TTC0_MATCH_0; // set ptr_register to point to TTC0_MATCH_0 register
			state = 1;
			break;
		case 1:
			match_value++; // increase match value by 1 every loop
			break;
		case 2:
			match_value--; // decrease match value by 1 every loop
			break;
		}

		if (match_value == 0) // match value reaches its maximum value every 65536 loops and then resets back to 0
		{
			if (CurrentState == MODULATING_STATE)
			{
				state == 2 ? state = 0 : state++; // change state
				// Send reference voltage and current voltage to controller
				u1 = PI(getVoltageSetPoint(), u2, getKi(), getKp()); // input reference voltage u0, current voltage u2, Ki and Kp to PI controller
				*ptr_register = u1 * 1000;
				u2 = convert(u1); // convert the input from PI controller to output voltage u2

				setConverterOutputVoltate(u2);

				char c[50]; //size of the number

				if (rounds % 100 == 0) // print value of the converter output voltage every time rounds is divisible by 100
				{
					printSystemState();
				}
			}

			rounds++; // increment value of rounds for timing purposes

			int currentSemaphoreState = getSemaphoreState();
			int currentSemaphoreLockedPeriod = getSemaphoreLockedPeriod();

			if (currentSemaphoreState != 0)
			{
				if (currentSemaphoreLockedPeriod > 900) // timeout timer for semaphore, 900 equals to ~8 seconds
				{
					uartSendString("Timeout passed, releasing semaphore \n");
					releaseSemaphore(); // release semaphore
					printSystemState();
				}
				else
				{
					setSemaphoreLockedPeriod(currentSemaphoreLockedPeriod + 1); // increment the timeout timer val
				}
			}
		}
	}

	cleanup_platform();
	return 0;
}
