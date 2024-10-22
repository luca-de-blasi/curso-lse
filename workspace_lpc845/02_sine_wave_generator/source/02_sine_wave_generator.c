#include "board.h"
#include "pin_mux.h"
#include "fsl_dac.h"
#include "fsl_swm.h"
#include "fsl_power.h"
#include "fsl_iocon.h"
#include <math.h>

#define PI 3.1416

int frec_s;

void SysTick_Handler(void);

int main(void) {
	frec_s = 1000;

	BOARD_BootClockFRO30M();
	CLOCK_EnableClock(kCLOCK_Swm);
	SWM_SetFixedPinSelect(SWM0, kSWM_DAC_OUT0, true);
	CLOCK_DisableClock(kCLOCK_Swm);

	CLOCK_EnableClock(kCLOCK_Iocon);
	IOCON_PinMuxSet(IOCON, 0, IOCON_PIO_DACMODE_MASK);
	CLOCK_DisableClock(kCLOCK_Iocon);

	POWER_DisablePD(kPDRUNCFG_PD_DAC0);

	dac_config_t dac_config;
	DAC_GetDefaultConfig(&dac_config);
	DAC_Init(DAC0, &dac_config);
	DAC_SetBufferValue(DAC0, 0);

	SysTick_Config(SystemCoreClock / 1000000);

    while (1);
    return 0;
}

void SysTick_Handler(void) {
	static uint16_t tiempo = 0;
	static uint16_t DAC = 0;

	DAC = (sin(2 * PI * frec_s * tiempo * pow(10, -6)) + 1) / 2 * 1024;

	DAC_SetBufferValue(DAC0, DAC);

	tiempo++;

	if (DAC == 1000){
		DAC = 0;
	}
}
