#include "app_tasks.h"
#include "fsl_debug_console.h"

// Cola para selecion de valor para el display
xQueueHandle queue_display_variable1;
xQueueHandle queue_display_variable2;
// Cola para datos del ADC
xQueueHandle queue_adc;
// Cola para datos de temperatura
xQueueHandle queue_lux;

xQueueHandle queue_pwm;

xQueueHandle queue_time;
// Handler para la tarea de display write
TaskHandle_t handle_display;


/**
 * @brief Inicializa todos los perifericos y colas
 */
void task_init(void *params) {
	// Inicializacion de GPIO
	wrapper_gpio_init(0);
	// Configuro el ADC
	wrapper_adc_init();
	// Configuro el display
	wrapper_display_init();
	// Configuro botones
	wrapper_btn_init();
	// Inicializo el PWM
	wrapper_pwm_init();
	// Inicializo I2C y Bh1750
	//wrapper_i2c_init();
	//wrapper_bh1750_init();

	// Inicializo colas
	queue_adc = xQueueCreate(1, sizeof(adc_data_t));
	queue_lux = xQueueCreate(1, sizeof(uint8_t));
	queue_pwm = xQueueCreate(1, sizeof(uint8_t));
	queue_time = xQueueCreate(1, sizeof(uint32_t));
	queue_display_variable1 = xQueueCreate(1, sizeof(display_variable_t));
	queue_display_variable2 = xQueueCreate(1, sizeof(display_variable_t));
	// Elimino tarea para liberar recursos
	vTaskDelete(NULL);
}

void task_adc_read(void *params) {

	while(1) {
		// Inicio una conversion
		ADC_DoSoftwareTriggerConvSeqA(ADC0);
		// Bloqueo la tarea por 250 ms
		vTaskDelay(250);
	}
}

void task_time(void *params){
	uint32_t time = 0;
	while(1){
		vTaskDelay(1);
		time++;
		xQueueOverwrite(queue_time, &time);
	}
}

void task_bh1750(void *params) {
	// Inicializo el clock del I2C1
		CLOCK_Select(kI2C1_Clk_From_MainClk);
	    // Asigno las funciones de I2C1 a los pines 26 y 27
		CLOCK_EnableClock(kCLOCK_Swm);
	    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SDA, kSWM_PortPin_P0_27);
	    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SCL, kSWM_PortPin_P0_26);
	    CLOCK_DisableClock(kCLOCK_Swm);

	    // Configuracion de master para el I2C con 400 KHz de clock
	    i2c_master_config_t config;
	    I2C_MasterGetDefaultConfig(&config);
	    config.baudRate_Bps = 400000;
	    // Usa el clock del sistema de base para generar el de la comunicacion
	    I2C_MasterInit(I2C1, &config, SystemCoreClock);

		if(I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Write) == kStatus_Success) {
			// Comando de power on
			uint8_t cmd = 0x01;
			I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
			I2C_MasterStop(I2C1);
		}
		if(I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Write) == kStatus_Success) {
			// Comando de medicion continua a 1 lux de resolucion
			uint8_t cmd = 0x10;
			I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
			I2C_MasterStop(I2C1);
		}

		while(1) {
			// Lectura del sensor
			if(I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Read) == kStatus_Success) {
				// Resultado
				uint8_t res[2] = {0};
				I2C_MasterReadBlocking(I2C1, res, 2, kI2C_TransferDefaultFlag);
				I2C_MasterStop(I2C1);
				// Devuelvo el resultado
				float lux = ((res[0] << 8) + res[1]) / 1.2;
				uint8_t percentLux = (lux * 100.00) / 30000.00;
				xQueueOverwrite(queue_lux, &percentLux);
			}
	    }
}

void task_print(void *params){
	uint32_t time = 0;
	uint8_t lux = 0;
	display_variable_t val = 50;
	uint8_t pwm = 0;
	PRINTF("Tiempo transcurrido | Luz | SP | PWM\n");
	while(1) {

		xQueueReceive(queue_time, &time, portMAX_DELAY);
		xQueueReceive(queue_lux, &lux, portMAX_DELAY);
		xQueuePeek(queue_display_variable2, &val, portMAX_DELAY);
		xQueuePeek(queue_pwm, &pwm, portMAX_DELAY);

		PRINTF("  %d    %d    %d    %d\n",
				time,
				lux,
				val,
				pwm
				);
		vTaskDelay(1000);

	}
}

void task_btn(void *params) {


	display_variable_t val = 50;
	display_variable_t change = 0;
	uint8_t lux = 0;

	while(1) {
		xQueueReceive(queue_lux, &lux, portMAX_DELAY);
		// Veo que boton se presiono
		if(!wrapper_btn_get(USR_BTN)) {
			// Antirebote
			vTaskDelay(20);
			if(change == 1){
				// USR Button para cambio
				change = 0;
			}
			else{
				change = 1;
			}
		}
		if(!wrapper_btn_get(S1_BTN)) {
			// Antirebote
			vTaskDelay(20);
			if(!wrapper_btn_get(S1_BTN) && val < 75) {
				// ISP Button para la referencia
				val++;
			}
		}
		if(!wrapper_btn_get(S2_BTN)) {
			// Antirebote
			vTaskDelay(20);
			if(!wrapper_btn_get(S2_BTN) && val > 25) {
				// ISP Button para la referencia
				val--;
			}
		}

		// Mando datos en la cola
		xQueueOverwrite(queue_display_variable1, &change);
		xQueueOverwrite(queue_display_variable2, &val);
		vTaskDelay(200);
	}
}

/**
 * @brief Escribe valores en el display
 */
void task_display_write(void *params) {
	// Variable a mostrar
	display_variable_t val = 50;
	display_variable_t change = 0;
	uint8_t lux = 0;
	uint8_t show = 0;

	while(1) {
		// Veo que variable hay que mostrar
		xQueuePeek(queue_display_variable1, &change, portMAX_DELAY);
		xQueuePeek(queue_display_variable2, &val, portMAX_DELAY);
		xQueuePeek(queue_lux, &lux, portMAX_DELAY);
		if(change == 0){
			show = val;
		}
		else{
			show = lux;
		}

		// Muestro el numero
		wrapper_display_off();
		wrapper_display_write((uint8_t)(show / 10));
		wrapper_display_on(COM_1);
		vTaskDelay(10);
		wrapper_display_off();
		wrapper_display_write((uint8_t)(show % 10));
		wrapper_display_on(COM_2);
		vTaskDelay(10);
	}
}

/**
 * @brief Actualiza el duty del PWM
 */
void task_pwm(void *params) {
	// Valores de ADC
	adc_data_t data = {0};
	uint8_t pwm = 0;

	while(1) {
		// Bloqueo hasta que haya algo que leer
		xQueueReceive(queue_adc, &data, portMAX_DELAY);
		pwm = (uint8_t)((data.ref_raw * 100) / 4095);
		// Actualizo el duty
		wrapper_pwm_update(pwm);
		xQueueOverwrite(queue_pwm, &pwm);
	}
}

void ADC0_SEQA_IRQHandler(void) {
	// Variable de cambio de contexto
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	// Verifico que se haya terminado la conversion correctamente
	if(kADC_ConvSeqAInterruptFlag == (kADC_ConvSeqAInterruptFlag & ADC_GetStatusFlags(ADC0))) {
		// Limpio flag de interrupcio
		ADC_ClearStatusFlags(ADC0, kADC_ConvSeqAInterruptFlag);
		// Resultado de conversion
		adc_result_info_t temp_info, ref_info;
		// Leo el valor del ADC
		ADC_GetChannelConversionResult(ADC0, REF_POT_CH, &ref_info);
		// Leo el valor del ADC
		ADC_GetChannelConversionResult(ADC0, LM35_CH, &temp_info);
		// Estructura de datos para mandar
		adc_data_t data = {
			.temp_raw = (uint16_t)temp_info.result,
			.ref_raw = (uint16_t)ref_info.result
		};
		// Mando por la cola los datos
		xQueueOverwriteFromISR(queue_adc, &data, &xHigherPriorityTaskWoken);
		// Veo si hace falta un cambio de contexto
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}
