
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "LPC55S69_cm33_core0.h"

#include "fsl_debug_console.h"
#include "fsl_puf.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define IOCON_PIO_FUNC0			0	/* weird fix for LED macros */
#define LOG_RET(x)				return_code_logger("MAIN", #x, __LINE__, x)
#define STRING_MAX				255
#define CORE_CLK_FREQ 			CLOCK_GetFreq(kCLOCK_CoreSysClk)
/* Worst-case time in ms to fully discharge PUF SRAM */
#define PUF_DISCHARGE_TIME 		400
#define PUF_INTRINSIC_KEY_SIZE 	16

/*******************************************************************************
 * Typedefs
 ******************************************************************************/
typedef struct {
	status_t status;
	const char *string;
} status_strings_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void return_code_logger(const char *filename, const char *funcname, uint32_t line, status_t status);
static void puf_test(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile uint32_t g_systickCounter;

static status_strings_t variable_strings[] = {
		{kStatus_Success, "Success"},
		{kStatus_Fail, "Failure"},
		{kStatus_ReadOnly, "ReadOnly"},
		{kStatus_OutOfRange, "OutOfRange"},
		{kStatus_InvalidArgument, "InvalidArgument"},
		{kStatus_Timeout, "Timeout"},
		{kStatus_NoTransferInProgress, "NoTransferInProgress"}};
static size_t variable_strings_len = sizeof(variable_strings)/sizeof(variable_strings[0]);

/* User key in little-endian format. */
/* 32 bytes key for ECB method: "Thispasswordisveryuncommonforher". */
static const uint8_t s_userKey256[] __attribute__((aligned)) = {
    0x72, 0x65, 0x68, 0x72, 0x6f, 0x66, 0x6e, 0x6f, 0x6d, 0x6d, 0x6f, 0x63, 0x6e, 0x75, 0x79, 0x72,
    0x65, 0x76, 0x73, 0x69, 0x64, 0x72, 0x6f, 0x77, 0x73, 0x73, 0x61, 0x70, 0x73, 0x69, 0x68, 0x54};

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

    /* Set systick reload value to generate 1ms interrupt */
	if(SysTick_Config(SystemCoreClock / 1000U)) while(1) {}

	puf_test();

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

static void return_code_logger(const char *filename, const char *funcname, uint32_t line, status_t status)
{
	if (status == kStatus_Success) return; /* remove this line if you want to print successful call */
	for (size_t i = 0; i < variable_strings_len; i++)
	{
		if (status == variable_strings[i].status)
		{
			printf("[%s] %s - line %u returned %s\n", filename, funcname, line, variable_strings[i].string);
			return;
		}
	}
	printf("[%s] %s - line %u returned unknown code %u\n", filename, funcname, line, status);
}

static void puf_test(void)
{
    uint8_t activationCode[PUF_ACTIVATION_CODE_SIZE];
    uint8_t keyCode0[PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(32)];
    uint8_t keyCode1[PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(PUF_INTRINSIC_KEY_SIZE)];
    uint8_t intrinsicKey[16] = {0};

	srand(0xbabadeda);
	PRINTF("PUF Peripheral Driver Example\r\n\r\n");

	/* Initialize PUF peripheral */
	LOG_RET(PUF_Init(PUF, PUF_DISCHARGE_TIME, CORE_CLK_FREQ));

	/* Perform enroll to get device specific PUF activation code */
	LOG_RET(PUF_Enroll(PUF, activationCode, sizeof(activationCode)));

	PUF_Deinit(PUF, PUF_DISCHARGE_TIME, CORE_CLK_FREQ);

	/* Reinitialize PUF after enroll */
	LOG_RET(PUF_Init(PUF, PUF_DISCHARGE_TIME, CORE_CLK_FREQ));

	/* Start PUF by loading generated activation code */
	LOG_RET(PUF_Start(PUF, activationCode, sizeof(activationCode)));

	/* Create keycode for user key with index 0 */
	/* Index 0 selects that the key shall be ouptut (by PUF_GetHwKey()) to a SoC specific private hardware bus. */
	LOG_RET(PUF_SetUserKey(PUF, kPUF_KeyIndex_00, s_userKey256, 32, keyCode0, sizeof(keyCode0)));

	/* Generate new intrinsic key with index 1 */
	LOG_RET(PUF_SetIntrinsicKey(PUF, kPUF_KeyIndex_01, PUF_INTRINSIC_KEY_SIZE, keyCode1, sizeof(keyCode1)));

	/* Reconstruct key from keyCode0 to HW bus for crypto module */
	LOG_RET(PUF_GetHwKey(PUF, keyCode0, sizeof(keyCode0), kPUF_KeySlot0, rand()));

	/* Reconstruct intrinsic key from keyCode1 generated by PUF_SetIntrinsicKey() */
	LOG_RET(PUF_GetKey(PUF, keyCode1, sizeof(keyCode1), intrinsicKey, sizeof(intrinsicKey)));

	PRINTF("Reconstructed intrinsic key = ");
	for (size_t i = 0; i < PUF_INTRINSIC_KEY_SIZE; i++)
	{
		PRINTF("%x ", intrinsicKey[i]);
	}

	PUF_Deinit(PUF, PUF_DISCHARGE_TIME, CORE_CLK_FREQ);
	PRINTF("\r\n\nExample end.\r\n");
}
