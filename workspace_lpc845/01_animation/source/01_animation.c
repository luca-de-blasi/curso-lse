#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"
#include <stdbool.h>

#define USER_BUTTON 4
#define SEGMENT_B 10
#define SEGMENT_A 11
#define SEGMENT_F 13
#define SEGMENT_C 6
#define SEGMENT_E 0
#define SEGMENT_D 14
#define DIGIT_A1 8
#define DIGIT_A2 9
/*
 * @brief   Application entry point.
*/
int main(void) {

	BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    gpio_pin_config_t out_config = { kGPIO_DigitalOutput, 1};

    GPIO_PortInit(GPIO, 0);

    GPIO_PinInit(GPIO, 0, SEGMENT_B, &out_config);
    GPIO_PinInit(GPIO, 0, SEGMENT_A, &out_config);
    GPIO_PinInit(GPIO, 0, SEGMENT_F, &out_config);
    GPIO_PinInit(GPIO, 0, SEGMENT_C, &out_config);
    GPIO_PinInit(GPIO, 0, SEGMENT_E, &out_config);
    GPIO_PinInit(GPIO, 0, SEGMENT_D, &out_config);
    GPIO_PinInit(GPIO, 0, DIGIT_A1, &out_config);
    GPIO_PinInit(GPIO, 0, DIGIT_A2, &out_config);

    gpio_pin_config_t in_config = { kGPIO_DigitalInput};

    GPIO_PinInit(GPIO, 0, USER_BUTTON, &in_config);

    while(1) {
    	while (!GPIO_PinRead(GPIO, 0, USER_BUTTON)){
    			GPIO_PinWrite(GPIO, 0, DIGIT_A1, false);
    			GPIO_PinWrite(GPIO, 0, DIGIT_A2, false);
    			GPIO_PinWrite(GPIO, 0, SEGMENT_A, false);
    			for (uint32_t i = 0; i < 200000; i++);
    			GPIO_PinWrite(GPIO, 0, SEGMENT_A, true);
    			GPIO_PinWrite(GPIO, 0, SEGMENT_C, false);
    			for (uint32_t i = 0; i < 200000; i++);
    			GPIO_PinWrite(GPIO, 0, SEGMENT_C, true);
    			GPIO_PinWrite(GPIO, 0, SEGMENT_D, false);
    			for (uint32_t i = 0; i < 200000; i++);
    			GPIO_PinWrite(GPIO, 0, SEGMENT_D, true);
    			GPIO_PinWrite(GPIO, 0, SEGMENT_E, false);
    			for (uint32_t i = 0; i < 200000; i++);
    			GPIO_PinWrite(GPIO, 0, SEGMENT_E, true);
    			GPIO_PinWrite(GPIO, 0, SEGMENT_F, false);
    			for (uint32_t i = 0; i < 200000; i++);
    			GPIO_PinWrite(GPIO, 0, SEGMENT_F, true);
    			GPIO_PinWrite(GPIO, 0, SEGMENT_B, false);
    			for (uint32_t i = 0; i < 200000; i++);
    			GPIO_PinWrite(GPIO, 0, SEGMENT_B, true);
    	}
    		GPIO_PinWrite(GPIO, 0, DIGIT_A1, true);
    		GPIO_PinWrite(GPIO, 0, DIGIT_A2, true);
    		GPIO_PinWrite(GPIO, 0, SEGMENT_B, true);
    		GPIO_PinWrite(GPIO, 0, SEGMENT_A, true);
    		GPIO_PinWrite(GPIO, 0, SEGMENT_F, true);
    		GPIO_PinWrite(GPIO, 0, SEGMENT_C, true);
    		GPIO_PinWrite(GPIO, 0, SEGMENT_E, true);
    		GPIO_PinWrite(GPIO, 0, SEGMENT_D, true);

    	for (uint32_t i = 0; i < 200000; i++);
    }
    return 0;
}
