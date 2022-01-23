///*
// * Exercise - Interrupts
// */
//
//#include <xscugic.h> // Generic interrupt controller (GIC) driver
//                     //#include "xil_exception.h"    // ARM Cortex A53,A9,R5 specific exception related APIs
//#include <xttcps.h>  // 16-bit timer counter driver - Triple Timer Counter (TTC)
//                     //#include "xil_printf.h"
//#include <sleep.h>
//#include <xuartps_hw.h>
//#include <zynq_registers.h>
//
///*
// * The following constants map to the XPAR parameters created in the
// * xparameters.h file. They are only defined here such that a user can easily
// * change all the needed parameters in one place.
// */
//
//// Device ID number for PS TTC0 (XPAR_XTTCPS_0_DEVICE_ID / XPAR_PS7_TTC_0_DEVICE_ID)
////#define TTC_TICK_DEVICE_ID    0U
//// Interrupt (IRQ) ID# number for TTC0 counter 0 from UG585 "TRM: - ch. 8.5 TTC" (XPAR_XTTCPS_0_INTR / XPS_TTC0_0_INT_ID)
//#define TTC_TICK_INTR_ID 42U
//// Interrupt (IRQ) ID# number for UART0 (XPAR_XUARTPS_1_INTR / XPS_UART1_INT_ID)
//#define UARTPS_1_INTR 82U
//
//// Device ID number for PS SCU GIC (XPAR_SCUGIC_SINGLE_DEVICE_ID)
//#define INTC_DEVICE_ID 0U
//
//void SetupUART();
//void SetupUARTInterrupt(XScuGic *InterruptControllerInstancePtr);
//static void UartIRQHandler(void);
//char uart_receive();
//
//// Set up routines for timer counters and interrupts
//void SetupTicker(XScuGic *InterruptControllerInstancePtr);
//void SetupTimer();
//void SetupInterruptSystem(XScuGic *InterruptControllerInstancePtr);
//static void TickHandler(void);
//
///*
// * This function sets up the interrupt system such that interrupts can occur.
// */
//void SetupInterruptSystem(XScuGic *InterruptControllerInstancePtr)
//{
//    xil_printf("SetupInterruptSystem() called!!!\r\n");
//
//    //XScuGic_Config structure contains configuration information for the device
//    XScuGic_Config InterruptControllerConfigInstance;                                  // Initialize structure of type XScuGic_Config
//    XScuGic_Config *InterruptControllerConfigPtr = &InterruptControllerConfigInstance; // These could be also declared global
//
//    // Initialize the interrupt controller driver (XScuGic_Config structure)
//    InterruptControllerConfigPtr->DeviceId = XPAR_PS7_SCUGIC_0_DEVICE_ID;
//    InterruptControllerConfigPtr->CpuBaseAddress = XPAR_PS7_SCUGIC_0_BASEADDR;
//    InterruptControllerConfigPtr->DistBaseAddress = XPAR_PS7_SCUGIC_0_DIST_BASEADDR;
//    InterruptControllerConfigPtr->HandlerTable[INTC_DEVICE_ID].CallBackRef = NULL;
//    InterruptControllerConfigPtr->HandlerTable[INTC_DEVICE_ID].Handler = NULL;
//
//    /**
//   * CfgInitialize a specific interrupt controller instance/driver. The
//   * initialization entails:
//   *
//   * - Initialize fields of the XScuGic structure
//   * - Initial vector table with stub function calls
//   * - All interrupt sources are disabled
//   */
//    XScuGic_CfgInitialize(InterruptControllerInstancePtr, InterruptControllerConfigPtr, InterruptControllerConfigPtr->CpuBaseAddress);
//    // Connect the interrupt controller interrupt handler to the hardware interrupt handling logic in the ARM processor.
//    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, InterruptControllerInstancePtr);
//    // Enable interrupts in the ARM
//    Xil_ExceptionEnable();
//}
//
///**
// * This function sets up a timer counter device,
// *  - initialize device
// *  - set options
// *  - set interval and prescaler value
// *  (similar initialization as in Exercise 5)
// */
//void SetupTimer()
//{
//    xil_printf("SetupTimer() called!!!\r\n");
//
//    TTC0_CNT_CNTRL |= XTTCPS_CNT_CNTRL_DIS_MASK; //Counter Control Register: "Disable the counter"
//    // Reset the count control register to it's default value.
//    TTC0_CNT_CNTRL = XTTCPS_CNT_CNTRL_RESET_VALUE; //Counter Control Register:" Reset value"
//
//    // Reset the rest of the registers to the default values.
//    TTC0_CLK_CNTRL = 0;
//    TTC0_INTERVAL_VAL = 0;
//    TTC0_MATCH_1 = 0;
//    TTC0_MATCH_2_COUNTER_2 = 0;
//    TTC0_MATCH_3_COUNTER_2 = 0;
//    TTC0_IER = 0;
//
//    // Reset the counter value
//    TTC0_CNT_CNTRL |= XTTCPS_CNT_CNTRL_RST_MASK; // Counter Control Register: "Reset counter"
//
//    // Set the options
//    TTC0_CNT_CNTRL = XTTCPS_CNT_CNTRL_DIS_MASK | XTTCPS_CNT_CNTRL_INT_MASK; //Counter Control Register: "Disable the counter" | "Interval mode"
//
//    // Set the interval and prescale. Base clock is 111MHz
//    // Prescale value (N): if prescale is enabled, the  count rate is divided by 2^(N+1)
//    // 1 / (111MHz) * 9000 * 2^(11+1) = 0.3321... [seconds]
//    TTC0_INTERVAL_VAL = 9000;
//    TTC0_CLK_CNTRL &= ~(XTTCPS_CLK_CNTRL_PS_VAL_MASK | XTTCPS_CLK_CNTRL_PS_EN_MASK);       // Clock Control register - clear: "Prescale value" & "Prescale enable"
//    TTC0_CLK_CNTRL |= (11 << XTTCPS_CLK_CNTRL_PS_VAL_SHIFT) | XTTCPS_CLK_CNTRL_PS_EN_MASK; // Clock Control register - set: "Prescale value" & "Prescale enable"
//}
//
///*
//* This function initializes the UART - serial connection to PC (from Exercise 4)
//*/
//void SetupUART()
//{
//    xil_printf("SetupUART() called!!!\r\n");
//
//    uint32_t r = 0; // Temporary value variable
//
//    r = UART_CTRL;
//    r &= ~(XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN); // Clear Tx & Rx Enable
//    r |= XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS;  // Tx & Rx Disable
//    UART_CTRL = r;
//
//    UART_MODE = 0;
//    UART_MODE &= ~XUARTPS_MR_CLKSEL;        // Clear "Input clock selection" - 0: clock source is uart_ref_clk
//    UART_MODE |= XUARTPS_MR_CHARLEN_8_BIT;  // Set "8 bits data"
//    UART_MODE |= XUARTPS_MR_PARITY_NONE;    // Set "No parity mode"
//    UART_MODE |= XUARTPS_MR_STOPMODE_1_BIT; // Set "1 stop bit"
//    UART_MODE |= XUARTPS_MR_CHMODE_NORM;    // Set "Normal mode"
//
//    // baud_rate = sel_clk / (CD * (BDIV + 1) (ref: UG585 - TRM - Ch. 19 UART)
//    UART_BAUD_DIV = 6;   // ("BDIV")
//    UART_BAUD_GEN = 124; // ("CD")
//    // Baud Rate = 100Mhz / (124 * (6 + 1)) = 115200 bps
//
//    UART_CTRL |= (XUARTPS_CR_TXRST | XUARTPS_CR_RXRST); // TX & RX logic reset
//
//    r = UART_CTRL;
//    r |= XUARTPS_CR_RX_EN | XUARTPS_CR_TX_EN;      // Set TX & RX enabled
//    r &= ~(XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS); // Clear TX & RX disabled
//    UART_CTRL = r;
//}
//
//void SetupUARTInterrupt(XScuGic *InterruptControllerInstancePtr)
//{
//    xil_printf("SetupUARTInterrupt() called!!!\r\n");
//
//    Xil_ExceptionDisable();
//
//    uint32_t r = 0; // Temporary value variable
//
//    r = UART_CTRL;
//    r &= ~(XUARTPS_CR_TX_EN | XUARTPS_CR_RX_EN); // Clear Tx & Rx Enable
//    r |= XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS;  // Tx & Rx Disable
//    UART_CTRL = r;
//
//    // Connect to the interrupt controller (pointer to function)
//    InterruptControllerInstancePtr->Config->HandlerTable[UARTPS_1_INTR].Handler = (Xil_InterruptHandler)UartIRQHandler;
//
//    // ICDISER1:  Interrupt Controller Distributor (ICD) - Interrupt Set-enable Register (ISER) 2
//    ICDISER1 = 1 << (UARTPS_1_INTR % 32); // Modulo operator (% 2^n) stripping off all but the n LSB bits (removes offset 32)
//
//    UART_IER = 0 | XUARTPS_IXR_RXOVR;
//    UART_IMR = 0 | XUARTPS_IXR_RXOVR;
//    UART_IDR = XUARTPS_IXR_MASK & ~(XUARTPS_IXR_RXOVR);
//
//    //UART_IER = XUARTPS_IXR_MASK;
//    //UART_IMR = XUARTPS_IXR_MASK;
//    //UART_IDR = 0;
//
//    UART_RXWM = XUARTPS_RXWM_MASK & 1U;
//
//    UART_CTRL |= (XUARTPS_CR_TXRST | XUARTPS_CR_RXRST); // TX & RX logic reset
//
//    r = UART_CTRL;
//    r |= XUARTPS_CR_RX_EN | XUARTPS_CR_TX_EN;      // Set TX & RX enabled
//    r &= ~(XUARTPS_CR_RX_DIS | XUARTPS_CR_TX_DIS); // Clear TX & RX disabled
//    UART_CTRL = r;
//
//    UART_ISR = XUARTPS_IXR_MASK;
//    Xil_ExceptionEnable();
//}
//
///*
// * This function sets up Generic Interrupt Controller (GIC) and Triple Timer Counter (TTC) for interrupts
// */
//void SetupTicker(XScuGic *InterruptControllerInstancePtr)
//{
//    xil_printf("SetupTicker() called!!!\r\n");
//    // Connect to the interrupt controller (pointer to function)
//    InterruptControllerInstancePtr->Config->HandlerTable[TTC_TICK_INTR_ID].Handler = (Xil_InterruptHandler)TickHandler;
//    // Enable the interrupt for the Timer counter
//    // ICDISER1:  Interrupt Controller Distributor (ICD) - Interrupt Set-enable Register (ISER) 1
//    ICDISER1 = 1 << (TTC_TICK_INTR_ID % 32); // Modulo operator (% 2^n) stripping off all but the n LSB bits (removes offset 32)
//    // Enable the interrupts for the tick timer/counter. We only care about the interval timeout.
//    // TCC0_IER: Triple Timer Counter (TCC) 0 - Interrupt Enable register (IER)
//    TTC0_IER |= XTTCPS_IXR_INTERVAL_MASK; // XTTCPS_IXR_INTERVAL_MASK: Interval Interrupt mask
//    // Start the tick timer/counter
//    TTC0_CNT_CNTRL &= ~XTTCPS_CNT_CNTRL_DIS_MASK; // Clear operation using XTTCPS_CNT_CNTRL_DIS_MASK
//}
//
///*
// * This function is the timer interrupt handler/
// * the function that's run on interrupt.
// */
//static void TickHandler(void)
//{
//    // Read the interrupt status to clear the interrupt.
//    // TTC0_ISR: Triple Timer Counter (TTC) 0 - Interrupt Status Register (ISR)
//    TTC0_ISR; // Cleared on read
//    xil_printf("TickHandler() called!!!\r\n");
//    AXI_LED_DATA ^= 0b0011; // Toggle (XOR operator (^)) two LEDs
//}
//
//static void UartIRQHandler(void)
//{
//    //Xil_ExceptionDisable();
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
//    //Xil_ExceptionEnable();
//}
//
//// Check if UART receive FIFO is not empty and return the new data
//char uart_receive()
//{
//    xil_printf("uart_receive() called!!!\r\n");
//    if ((UART_STATUS & XUARTPS_SR_RXEMPTY) == XUARTPS_SR_RXEMPTY)
//        return 0;
//    return UART_FIFO;
//}
