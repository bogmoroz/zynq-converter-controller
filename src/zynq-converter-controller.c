
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

// int ProcessEvent();

float Ki = 0.001;
float Kp = 0.01;
float voltageSetPoint = 50.0;
float converterOutputVoltate = 0.0;

// 0 for unlocked, 1 for locked with buttons, 2 for locked with uart
int semaphoreState = 0;
int semaphoreLockedPeriod = 0;

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

void setVoltageSetPoint(float n)
{
	voltageSetPoint = n;
}

float getVoltageSetPoint(void)
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

const char StateChangeTable[NUMBER_OF_STATES][NUMBER_OF_EVENTS] =
	// event  GO_TO_NEXT_STATE    GO_TO_NEXT_K
	{
		IDLING_STATE, CONFIGURATION_STATE_KP,	   // CONFIGURATION_STATE_KI
		IDLING_STATE, CONFIGURATION_STATE_KI,	   // CONFIGURATION_STATE_KP
		MODULATING_STATE, IDLING_STATE,			   // IDLING_STATE
		CONFIGURATION_STATE_KI, MODULATING_STATE}; // MODULATING_STATE

static int CurrentState = 0;

void setCurrentState(int n)
{
	CurrentState = n;
	// printCurrentState();
	printSystemState();
	switch (CurrentState)
	{
	case CONFIGURATION_STATE_KI:
		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD0);
		return;
	case CONFIGURATION_STATE_KP:
		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD1);
		return;
	case IDLING_STATE:
		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD2);
		return;
	case MODULATING_STATE:
		XGpio_DiscreteWrite(&LEDS, LEDS_channel, LD3);
		return;
	default:
		return;
	}
}

int getCurrentState()
{
	return CurrentState;
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
		setCurrentState(StateChangeTable[CurrentState][Event]);
	}

	return CurrentState; // we simply return current state if we receive event out of range
}

//void printCurrentState()
//{
//	if (CurrentState == 0)
//	{
//		xil_printf("Switching to state: %s\n", "CONFIGURATION_STATE_KI");
//	}
//
//	if (CurrentState == 1)
//	{
//		xil_printf("Switching to state: %s\n", "CONFIGURATION_STATE_KP");
//	}
//
//	if (CurrentState == 2)
//	{
//		xil_printf("Switching to state: %s\n", "IDLING_STATE");
//	}
//
//	if (CurrentState == 3)
//	{
//		xil_printf("Switching to state: %s\n", "MODULATING_STATE");
//	}
//}

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
	switch (CurrentState)
	{
	case CONFIGURATION_STATE_KP:
		// increment Kp
		if (command == 1)
		{
			setKp(getKp() + 0.01);
		}
		else if (command == 0)
		{
			setKp(getKp() - 0.01);
		}
		char outputStringKp[50];
		sprintf(outputStringKp, "%f", getKp());
		xil_printf(outputStringKp);
		xil_printf("\n");
		return;
	case CONFIGURATION_STATE_KI:
		// increment Ki
		if (command == 1)
		{
			setKi(getKi() + 0.001);
		}
		else if (command == 0)
		{
			setKi(getKi() - 0.001);
		}
		char outputStringKi[50];
		sprintf(outputStringKi, "%f", getKi());
		xil_printf(outputStringKi);
		xil_printf("\n");
		return;
	case MODULATING_STATE:
		// increment voltage set point
		if (command == 1)
		{
			setVoltageSetPoint(getVoltageSetPoint() + 1);
		}
		else if (command == 0)
		{
			setVoltageSetPoint(getVoltageSetPoint() - 1);
		}

		char outputStringVoltage[50];
		sprintf(outputStringVoltage, "%f", getVoltageSetPoint());
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
// 1 for buttons, 2 for UART
int acquireSemaphore(int codeOfRequestSource)
{
	bool check = true;
	while (check)
	{
		int semaphoreState = getSemaphoreState();

		Xil_ExceptionDisable();	 // Disable interrupts during semaphore access
		if (semaphoreState == 0) // check if value of s has changed during execution of last 2 lines
		{
			setSemaphoreState(codeOfRequestSource); // Activate semaphore
			// Record timer value when sempahore was locked, used to release the semaphore after a timeout
			setSemaphoreLockedPeriod(0);
		}
		else if (semaphoreState != codeOfRequestSource)
		{
			if (semaphoreState == 1) {
				uartSendString("Trying to use UART but semaphore locked by buttons, please wait a few seconds...\n");
			} else {
				uartSendString("Trying to use buttons but semaphore locked by UART, please wait a few seconds...\n");
			}

		} else if (semaphoreState == codeOfRequestSource) {
			setSemaphoreLockedPeriod(0);
		}

		check = false;		   // Leave loop
		Xil_ExceptionEnable(); // Enable interrupts after semaphore has been accessed
	}
	return semaphoreState;
}

void releaseSemaphore(void)
{
	setSemaphoreLockedPeriod(0);
	setSemaphoreState(0);
}

float PI(float y_ref, float y_act, float Ki, float Kp)
{
	static float u1_old = 0;
	float error_new, u1_new;
	float u1_max = 1.5;
	error_new = y_ref - y_act;
	u1_new = u1_old + Ki * error_new;
	if (abs(u1_new) > u1_max)
	{ //
		u1_new = u1_old;
	}
	u1_old = u1_new;
	return u1_new + Kp * error_new;
}

/*
 * converter.c
 *
 *  Created on: Jan 27, 2022
 *      Authors: Anssi & Bogdan
 */

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

	return states[5];
};


float atof(const char *s)
{
  float a = 0.0;
  int e = 0;
  int c;
  while ((c = *s++) != '\0' && isdigit(c)) {
    a = a*10.0 + (c - '0');
  }
  if (c == '.') {
    while ((c = *s++) != '\0' && isdigit(c)) {
      a = a*10.0 + (c - '0');
      e = e-1;
    }
  }
  if (c == 'e' || c == 'E') {
    int sign = 1;
    int i = 0;
    c = *s++;
    if (c == '+')
      c = *s++;
    else if (c == '-') {
      c = *s++;
      sign = -1;
    }
    while (isdigit(c)) {
      i = i*10 + (c - '0');
      c = *s++;
    }
    e += i*sign;
  }
  while (e > 0) {
    a *= 10.0;
    e--;
  }
  while (e < 0) {
    a *= 0.1;
    e++;
  }
  return a;
}

void printSystemState() {
	uartSendString("=================================\n");
	uartSendString("System is now in state: ");
	int systemState = getCurrentState();
	if (systemState == CONFIGURATION_STATE_KI) {
		uartSendString("CONFIGURATION_STATE_KI");
	} else if (systemState == CONFIGURATION_STATE_KP) {
		uartSendString("CONFIGURATION_STATE_KP");
	} else if (systemState == IDLING_STATE) {
		uartSendString("IDLING_STATE");
	} else if (systemState == MODULATING_STATE) {
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


	if (systemState == MODULATING_STATE) {
		sprintf(outputString, "Current converter output: %f volts.", getConverterOutputVoltate());
		uartSendString(outputString);
		uartSendString("\n");
	}


	int semaphoreState = getSemaphoreState();
	if (semaphoreState == 0) {
		uartSendString("Semaphore is free.\n");
	} else if  (semaphoreState == 1) {
		uartSendString("Buttons control the system, UART input blocked. \n");
	} else if (semaphoreState == 2) {
		uartSendString("UART controls the system, button input blocked for a period. \n");
	}

	uartSendString("=================================\n");
}

int main()
{
//	setCurrentState(CONFIGURATION_STATE_KI);


	initButtonInterrupts();
	setupUART();

	acquireSemaphore(1);
	releaseSemaphore();

	uint16_t match_value = 0;
	uint8_t state = 0;
	volatile u32 *ptr_register = NULL;
	uint16_t rounds = 0;
	// Initializing PID controller and converter values
	float u1, u2, Ki, Kp;
	u1 = 0; //actual voltage out of the controller
	u2 = 0; // process variable - voltage out of the converter

	while (rounds < 300000)
	{
		char input = '1';
		input = uartReceive();

		if (input)
		{

			int index = 0;

			char rx_buf[30];
			rx_buf[0] = input;

			while (input != '\r')
			{
				input = uartReceive();
				if (input)
				{
					++index;
					rx_buf[index] = input;
				}
			}

			if (isdigit(*rx_buf))
			{
				float resolvedNumber = atof(rx_buf);

				if (getCurrentState() == CONFIGURATION_STATE_KI)
				{
					int semaphoreState = acquireSemaphore(2);
					if (semaphoreState == 2)
					{
						setKi(resolvedNumber);
						char outputStringKi[50];
						sprintf(outputStringKi, "%f", getKi());
						xil_printf(outputStringKi);
						xil_printf("\n");
					}
				}
				else if (getCurrentState() == CONFIGURATION_STATE_KP)
				{
					int semaphoreState = acquireSemaphore(2);
					if (semaphoreState == 2)
					{
						setKp(resolvedNumber);
						char outputStringKp[50];
						sprintf(outputStringKp, "%f", getKp());
						xil_printf(outputStringKp);
						xil_printf("\n");
					}

				}
				else if (getCurrentState() == MODULATING_STATE)
				{

					int semaphoreState = acquireSemaphore(2);
					if (semaphoreState == 2)
					{
						setVoltageSetPoint(resolvedNumber);
						char outputStringVoltage[50];
						sprintf(outputStringVoltage, "%f", getVoltageSetPoint());
						xil_printf(outputStringVoltage);
						xil_printf("\n");
					}
				}
				printSystemState();
			}
			else
			{
				if (index > 1 && strncmp("CONFIGURATION_STATE_KI", rx_buf, index) == 0)
				{
					int semaphoreState = acquireSemaphore(2);
					if (semaphoreState == 2)
					{
						setCurrentState(CONFIGURATION_STATE_KI);
					}
				}
				else if (index > 1 && strncmp("CONFIGURATION_STATE_KP", rx_buf, index) == 0)
				{
					int semaphoreState = acquireSemaphore(2);
					if (semaphoreState == 2)
					{
						setCurrentState(CONFIGURATION_STATE_KP);
					}
				}
				else if (index > 1 && strncmp("IDLING_STATE", rx_buf, index) == 0)
				{
					int semaphoreState = acquireSemaphore(2);
					if (semaphoreState == 2)
					{
						setCurrentState(IDLING_STATE);
					}
				}
				else if (index > 1 && strncmp("MODULATING_STATE", rx_buf, index) == 0)
				{
					int semaphoreState = acquireSemaphore(2);
					if (semaphoreState == 2)
					{
						setCurrentState(MODULATING_STATE);
					}
				}
			}
		}

		switch (state)
		{
		case 0:
			ptr_register = &TTC0_MATCH_0;
			break;
		case 1:
			match_value++;
			break;
		case 2:
			match_value--;
			break;
		}

		if (match_value == 0)
		{
			if (CurrentState == MODULATING_STATE)
			{
				state == 2 ? state = 0 : state++; // change state
				// Send reference voltage and current voltage to controller
				u1 = PI(getVoltageSetPoint(), u2, getKi(), getKp()); // input reference voltage u0, current voltage u2, Ki and Kp to PI controller
				*ptr_register = u1 * 1000;
				u2 = convert(u1); // convert the input from PI controller to output voltage u2

				setConverterOutputVoltate(u2);

				char c[50];		  //size of the number

				if (rounds % 100 == 0)
				{
					printSystemState();
				}
			}

			rounds = rounds + 1;

			int currentSemaphoreState = getSemaphoreState();
			int currentSemaphoreLockedPeriod = getSemaphoreLockedPeriod();

			if (currentSemaphoreState != 0)
			{
				if (currentSemaphoreLockedPeriod > 900)
				{
					xil_printf("Current locked period: %d ", currentSemaphoreLockedPeriod);
					uartSendString("Timeout passed, releasing semaphore \n");
					releaseSemaphore();
					printSystemState(0);
				}
				else
				{
					setSemaphoreLockedPeriod(currentSemaphoreLockedPeriod + 1);
				}
			}
		}
	}

	cleanup_platform();
	return 0;
}
