// /* Converter Model END*/

// int main(void)
// {
//     /* PWM input */
//     // Setup timer
//     TTC0_CLK_CNTRL = (0 << XTTCPS_CLK_CNTRL_PS_VAL_SHIFT) | XTTCPS_CLK_CNTRL_PS_EN_MASK;
//     TTC0_CNT_CNTRL = XTTCPS_CNT_CNTRL_RST_MASK | XTTCPS_CNT_CNTRL_DIS_MASK | XTTCPS_CNT_CNTRL_MATCH_MASK | XTTCPS_CNT_CNTRL_POL_WAVE_MASK;
//     TTC0_MATCH_0 = 0;
//     TTC0_CNT_CNTRL &= ~XTTCPS_CNT_CNTRL_DIS_MASK;
//     //
//     uint16_t match_value = 0;
//     uint8_t state = 0;
//     volatile u32 *ptr_register = NULL;
//     uint16_t rounds = 0;
//     // Initializing PID controller and converter values
//     float u0, u1, u2, Ki, Kp;
//     u0 = 50; //reference voltage - what we want
//     u1 = 0;  //actual voltage out of the controller
//     u2 = 0;  // process variable - voltage out of the converter
//     // PID parameters
//     // TODO protect them with semaphores
//     Ki = 0.001;
//     Kp = 0.01;

//     /* PWM input END */

//     while (rounds < 30000)
//     {

//         if (match_value == 0)
//         {
//             switch (state)
//             {
//             case 0:
//                 ptr_register = &TTC0_MATCH_0;
//                 break;
//             case 1:
//                 *ptr_register = match_value++;
//                 break;
//             case 2:
//                 *ptr_register = match_value--;
//                 break;
//             }

//             state == 2 ? state = 0 : state++; // change state
//             // Send reference voltage and current voltage to controller
//             u1 = PI(u0, u2, Ki, Kp);
//             u2 = convert(u1);
//             char c[50]; //size of the number
//             sprintf(c, "%f", u2);
//             xil_printf(c);
//             xil_printf("\n");
//         }

//         rounds = rounds + 1;
//     }
// }