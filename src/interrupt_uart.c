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
////// Set up routines for timer counters and interrupts
////void SetupTicker(XScuGic *InterruptControllerInstancePtr);
////void SetupTimer();
////void SetupInterruptSystem(XScuGic *InterruptControllerInstancePtr);
////static void TickHandler(void);
//
///*
//// * This function sets up the interrupt system such that interrupts can occur.
//// */
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
////// Check if UART receive FIFO is not empty and return the new data
////char uart_receive()
////{
////    xil_printf("uart_receive() called!!!\r\n");
////    if ((UART_STATUS & XUARTPS_SR_RXEMPTY) == XUARTPS_SR_RXEMPTY)
////        return 0;
////    return UART_FIFO;
////}
