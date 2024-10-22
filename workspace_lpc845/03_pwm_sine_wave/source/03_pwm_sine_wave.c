#include "board.h"
#include "fsl_sctimer.h"
#include "fsl_swm.h"
#include "pin_mux.h"
#include "fsl_gpio.h"
#include "fsl_common.h"
#include "fsl_swm.h"

#define PWM 10000

int main(void) {
	BOARD_InitDebugConsole();

    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_SCT_OUT4, kSWM_PortPin_P1_6);
    CLOCK_DisableClock(kCLOCK_Swm);

    uint32_t sctimer_clock = CLOCK_GetFreq(kCLOCK_Fro);
    sctimer_config_t sctimer_config;
    SCTIMER_GetDefaultConfig(&sctimer_config);
    SCTIMER_Init(SCT0, &sctimer_config);

    sctimer_pwm_signal_param_t pwm_config = {
		.output = kSCTIMER_Out_4,
		.level = kSCTIMER_HighTrue,
		.dutyCyclePercent = 50
    };

    uint32_t CDT;
    SCTIMER_SetupPwm(
		SCT0, &pwm_config, kSCTIMER_CenterAlignedPwm, PWM, sctimer_clock, &CDT
	);
    SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_U);

    while (true);

    return 0;
}
