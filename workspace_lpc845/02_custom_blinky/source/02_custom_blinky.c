#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"
#include "fsl_swm.h"
#include "fsl_power.h"
#include "fsl_adc.h"

#define POTE 0
#define LED_RED 2

uint16_t tiempo = 0;

int main(void) {
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    gpio_pin_config_t out_config = {kGPIO_DigitalOutput, 1};
    GPIO_PortInit(GPIO, 1);
    GPIO_PinInit(GPIO, 1, LED_RED, &out_config);
    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN0, true);
    CLOCK_DisableClock(kCLOCK_Swm);
    CLOCK_Select(kADC_Clk_From_Fro);
    CLOCK_SetClkDivider(kCLOCK_DivAdcClk, 1);
    POWER_DisablePD(kPDRUNCFG_PD_ADC0);
   	uint32_t frequency = CLOCK_GetFreq(kCLOCK_Fro) / CLOCK_GetClkDivider(kCLOCK_DivAdcClk);
   	ADC_DoSelfCalibration(ADC0, frequency);
   	adc_config_t adc_config;
   	ADC_GetDefaultConfig(&adc_config);
   	ADC_Init(ADC0, &adc_config);
   	adc_conv_seq_config_t adc_sequence = {
   		.channelMask = 1 << POTE, .triggerMask = 0, .triggerPolarity = kADC_TriggerPolarityPositiveEdge, .enableSyncBypass = false, .interruptMode = kADC_InterruptForEachConversion
   	};

   	ADC_SetConvSeqAConfig(ADC0, &adc_sequence);
   	ADC_EnableConvSeqA(ADC0, true);

    SysTick_Config(SystemCoreClock / 1000);

    while (true){
    	adc_result_info_t adc_info;
    	ADC_DoSoftwareTriggerConvSeqA(ADC0);
    	while (!ADC_GetChannelConversionResult(ADC0, POTE, &adc_info));
    	tiempo = adc_info.result * 1900 / 4095 + 100;
    }
    return 0;
}
void SysTick_Handler(void) {
	static uint16_t i = 0;
	i++;
	if(i == tiempo) {
		i = 0;
		GPIO_PinWrite(GPIO, 1, LED_RED, !GPIO_PinRead(GPIO, 1, LED_RED));
	} else if (tiempo < i) {
		i = 0;
	}
}
