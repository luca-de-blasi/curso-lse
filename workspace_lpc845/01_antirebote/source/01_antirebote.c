#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "fsl_debug_console.h"

// Etiqueta para el LED azul
#define LED_RED	2
#define USER_BUTTON 4

/*
 * @brief   Application entry point.
*/
int main(void) {
	// Inicializacion
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    // Estructura de configuracion para salida
    gpio_pin_config_t config = { kGPIO_DigitalOutput, 1 };
    // Habilito el clock del GPIO 1
    GPIO_PortInit(GPIO, 1);
    // Configuro el pin 0 del GPIO 1 como salida
    GPIO_PinInit(GPIO, 1, LED_RED, &config);

    gpio_pin_config_t in_config = { kGPIO_DigitalInput};
    GPIO_PortInit(GPIO, 0);
    GPIO_PinInit(GPIO, 0, USER_BUTTON, &in_config);

    while(1) {
    	if (!GPIO_PinRead(GPIO, 0, USER_BUTTON)){
    		GPIO_PinWrite(GPIO, 1, LED_RED, 0);
    	}
    	else {
    		GPIO_PinWrite(GPIO, 1, LED_RED, 1);
    	}
    	// Demora
    	for(uint32_t i = 0; i < 100000; i++);
    }
    return 0;
}
