#include "board.h"
#include "FreeRTOS.h"
#include "task.h"

#include "app_tasks.h"

/**
 * @brief Programa principal
 */
int main(void) {
	// Clock del sistema a 30 MHz
	BOARD_BootClockFRO30M();
	BOARD_InitDebugConsole();
	    gpio_pin_config_t config = { kGPIO_DigitalOutput, 0 };
	    // Habilito el clock del GPIO 1
	    GPIO_PortInit(GPIO, 0);
	    // Configuro el pin 0 del GPIO 1 como salida
	    GPIO_PinInit(GPIO, 0, 28, &config);
	    GPIO_PinWrite(GPIO, 0, 28, 0);
	// Creacion de tareas

	xTaskCreate(
		task_init,
		"Init",
		tskINIT_STACK,
		NULL,
		tskINIT_PRIORITY,
		NULL
	);

	xTaskCreate(
		task_time,
		"Time",
		tskTIME_STACK,
		NULL,
		tskTIME_PRIORITY,
		NULL
	);

	xTaskCreate(
		task_print,
		"PRINT",
		tskPRINT_STACK,
		NULL,
		tskPRINT_PRIORITY,
		NULL
	);

	xTaskCreate(
		task_bh1750,
		"BH1750",
		tskBH1750_STACK,
		NULL,
		tskBH1750_PRIORITY,
		NULL
	);

	xTaskCreate(
		task_btn,
		"Button",
		tskBTN_STACK,
		NULL,
		tskBTN_PRIORITY,
		NULL
	);

	xTaskCreate(
		task_display_write,
		"Write",
		tskDISPLAY_WRITE_STACK,
		NULL,
		tskDISPLAY_WRITE_PRIORITY,
		&handle_display
	);

	xTaskCreate(
		task_adc_read,
		"ADC",
		tskADC_READ_STACK,
		NULL,
		tskADC_READ_PRIORITY,
		NULL
	);

	xTaskCreate(
		task_pwm,
		"PWM",
		tskPWM_STACK,
		NULL,
		tskPWM_PRIORITY,
		NULL
	);

	vTaskStartScheduler();

    while(1);
    return 0;
}
