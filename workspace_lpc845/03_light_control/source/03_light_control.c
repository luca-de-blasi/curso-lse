#include "board.h"
#include "fsl_sctimer.h"
#include "fsl_swm.h"
#include "pin_mux.h"
#include "fsl_gpio.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_i2c.h"

#define PWM 1000
#define BH1750	0x5c

uint32_t ciclo;
uint32_t CDT;

void SysTick_Handler(void);

int main(void) {
	BOARD_BootClockFRO30M();

	CLOCK_Select(kI2C1_Clk_From_MainClk);
	CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SDA, kSWM_PortPin_P0_27);
    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SCL, kSWM_PortPin_P0_26);
    CLOCK_DisableClock(kCLOCK_Swm);

    i2c_master_config_t config;
    I2C_MasterGetDefaultConfig(&config);
    config.baudRate_Bps = 400000;
    I2C_MasterInit(I2C1, &config, SystemCoreClock);

	if (I2C_MasterStart(I2C1, BH1750, kI2C_Write) == kStatus_Success) {
		uint8_t cmd = 0x01;
		I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
		I2C_MasterStop(I2C1);
	}
	if (I2C_MasterStart(I2C1, BH1750, kI2C_Write) == kStatus_Success) {
		uint8_t cmd = 0x10;
		I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
		I2C_MasterStop(I2C1);
	}

    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_SCT_OUT4, kSWM_PortPin_P0_29);
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

    SCTIMER_SetupPwm(
		SCT0,
		&pwm_config,
		kSCTIMER_CenterAlignedPwm,
		PWM,
		sctimer_clock,
		&CDT
	);

    SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_U);
    SysTick_Config(SystemCoreClock / 100000);

	while (true) {
		if (I2C_MasterStart(I2C1, BH1750, kI2C_Read) == kStatus_Success) {
			uint8_t res[2] = {0};
			I2C_MasterReadBlocking(I2C1, res, 2, kI2C_TransferDefaultFlag);
			I2C_MasterStop(I2C1);
			float lux = ((res[0] << 8) + res[1]) / 1.2;
			PRINTF("LUX : %d \r\n",(uint16_t) lux);
			uint32_t duty = (float) lux / 65535 * 100000;
			PRINTF("DUTY_CYCLE: %d\n", duty);

			if (duty <= 100 && duty >= 0){
				ciclo = duty;
			}
		}
    }
    return 0;
}

void SysTick_Handler(void) {
	static uint16_t i = 0;
	i++;

	if(i == 100) {
		i = 0;
		SCTIMER_UpdatePwmDutycycle(SCT0, kSCTIMER_Out_4, ciclo, CDT);
	}
}
