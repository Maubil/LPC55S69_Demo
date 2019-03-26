
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "LPC55S69_cm33_core0.h"
#include "fsl_debug_console.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define IOCON_PIO_FUNC0	0	/* weird fix for LED macros */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile uint32_t g_systickCounter;

/*******************************************************************************
 * Code
 ******************************************************************************/
void SysTick_Handler(void)
{
    if (g_systickCounter != 0U)
    {
        g_systickCounter--;
    }
}

void SysTick_DelayTicks(uint32_t n)
{
    g_systickCounter = n;
    while(g_systickCounter != 0U) {}
}

/*
 * @brief   Application entry point.
 */
int main(void)
{
  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();

    LED_BLUE_INIT(0);
    LED_RED_INIT(0);
    LED_GREEN_INIT(0);


    printf("Hello World\n");

    /* Set systick reload value to generate 1ms interrupt */
   if(SysTick_Config(SystemCoreClock / 1000U)) while(1) {}

//    PUF_Type bla = {};
//    PUF_Init(&bla, 20, SystemCoreClock);

    while(true)
    {
        LED_BLUE_ON();
    	SysTick_DelayTicks(1000U);
        LED_BLUE_OFF();
        LED_RED_ON();
        SysTick_DelayTicks(1000U);
        LED_RED_OFF();
        LED_GREEN_ON();
        SysTick_DelayTicks(1000U);
        LED_GREEN_OFF();
    }
    return 0;
}
