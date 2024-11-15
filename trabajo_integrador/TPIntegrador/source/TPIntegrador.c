#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "fsl_debug_console.h"
#include "app_task.h"

// Colas
xQueueHandle queue_display_variable1;
xQueueHandle queue_display_variable2;
xQueueHandle queue_adc;
xQueueHandle queue_lux;
xQueueHandle queue_pwm;
xQueueHandle queue_time;

// Handler para la tarea de display
TaskHandle_t handle_display;

// Declaraciones de las funciones de las tareas
void task_init(void *params);
void task_adc_read(void *params);
void task_time(void *params);
void task_bh1750(void *params);
void task_print(void *params);
void task_btn(void *params);
void task_display_write(void *params);
void task_pwm(void *params);

/**
 * @brief Programa principal
 */
int main(void) {
    // Configuración inicial del sistema
    BOARD_BootClockFRO30M();
    BOARD_InitDebugConsole();

    // Inicialización de GPIO
    gpio_pin_config_t config = { kGPIO_DigitalOutput, 0 };
    GPIO_PortInit(GPIO, 0);
    GPIO_PinInit(GPIO, 0, 28, &config);
    GPIO_PinWrite(GPIO, 0, 28, 0);

    // Creación de tareas
    xTaskCreate(task_init, "Init", tskINIT_STACK, NULL, tskINIT_PRIORITY, NULL);
    xTaskCreate(task_time, "Time", tskTIME_STACK, NULL, tskTIME_PRIORITY, NULL);
    xTaskCreate(task_print, "Print", tskPRINT_STACK, NULL, tskPRINT_PRIORITY, NULL);
    xTaskCreate(task_bh1750, "BH1750", tskBH1750_STACK, NULL, tskBH1750_PRIORITY, NULL);
    xTaskCreate(task_btn, "Button", tskBTN_STACK, NULL, tskBTN_PRIORITY, NULL);
    xTaskCreate(task_display_write, "Write", tskDISPLAY_WRITE_STACK, NULL, tskDISPLAY_WRITE_PRIORITY, &handle_display);
    xTaskCreate(task_adc_read, "ADC", tskADC_READ_STACK, NULL, tskADC_READ_PRIORITY, NULL);
    xTaskCreate(task_pwm, "PWM", tskPWM_STACK, NULL, tskPWM_PRIORITY, NULL);

    // Inicia el planificador
    vTaskStartScheduler();

    while (1);  // Nunca debería llegar aquí
    return 0;
}

/**
 * @brief Inicializa periféricos y colas
 */
void task_init(void *params) {
    // Inicialización de periféricos
    wrapper_gpio_init(0);
    wrapper_adc_init();
    wrapper_display_init();
    wrapper_btn_init();
    wrapper_pwm_init();

    // Inicialización de colas
    queue_adc = xQueueCreate(1, sizeof(adc_data_t));
    queue_lux = xQueueCreate(1, sizeof(uint8_t));
    queue_pwm = xQueueCreate(1, sizeof(uint8_t));
    queue_time = xQueueCreate(1, sizeof(uint32_t));
    queue_display_variable1 = xQueueCreate(1, sizeof(display_variable_t));
    queue_display_variable2 = xQueueCreate(1, sizeof(display_variable_t));

    // Elimina la tarea para liberar recursos
    vTaskDelete(NULL);
}

// Implementación de las tareas

void task_adc_read(void *params) {
    while (1) {
        ADC_DoSoftwareTriggerConvSeqA(ADC0);
        vTaskDelay(250);
    }
}

void task_time(void *params) {
    uint32_t time = 0;
    while (1) {
        vTaskDelay(1);
        time++;
        xQueueOverwrite(queue_time, &time);
    }
}

void task_bh1750(void *params) {
    CLOCK_Select(kI2C1_Clk_From_MainClk);
    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SDA, kSWM_PortPin_P0_27);
    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SCL, kSWM_PortPin_P0_26);
    CLOCK_DisableClock(kCLOCK_Swm);

    i2c_master_config_t config;
    I2C_MasterGetDefaultConfig(&config);
    config.baudRate_Bps = 400000;
    I2C_MasterInit(I2C1, &config, SystemCoreClock);

    if (I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Write) == kStatus_Success) {
        uint8_t cmd = 0x01;
        I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
        I2C_MasterStop(I2C1);
    }
    if (I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Write) == kStatus_Success) {
        uint8_t cmd = 0x10;
        I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
        I2C_MasterStop(I2C1);
    }

    while (1) {
        if (I2C_MasterStart(I2C1, BH1750_ADDR, kI2C_Read) == kStatus_Success) {
            uint8_t res[2] = {0};
            I2C_MasterReadBlocking(I2C1, res, 2, kI2C_TransferDefaultFlag);
            I2C_MasterStop(I2C1);
            float lux = ((res[0] << 8) + res[1]) / 1.2;
            uint8_t percentLux = (lux * 100.00) / 30000.00;
            xQueueOverwrite(queue_lux, &percentLux);
        }
    }
}

void task_print(void *params) {
    uint32_t time = 0;
    uint8_t lux = 0;
    display_variable_t val = 50;
    uint8_t pwm = 0;
    PRINTF("Tiempo transcurrido | Luz | SP | PWM\n");
    while (1) {
        xQueueReceive(queue_time, &time, portMAX_DELAY);
        xQueueReceive(queue_lux, &lux, portMAX_DELAY);
        xQueuePeek(queue_display_variable2, &val, portMAX_DELAY);
        xQueuePeek(queue_pwm, &pwm, portMAX_DELAY);

        PRINTF("  %d    %d    %d    %d\n", time, lux, val, pwm);
        vTaskDelay(1000);
    }
}

void task_btn(void *params) {
    display_variable_t val = 50;
    display_variable_t change = 0;
    uint8_t lux = 0;

    while (1) {
        xQueueReceive(queue_lux, &lux, portMAX_DELAY);

        if (!wrapper_btn_get(USR_BTN)) {
            vTaskDelay(20);
            change = (change == 1) ? 0 : 1;
        }
        if (!wrapper_btn_get(S1_BTN)) {
            vTaskDelay(20);
            if (!wrapper_btn_get(S1_BTN) && val < 75) {
                val++;
            }
        }
        if (!wrapper_btn_get(S2_BTN)) {
            vTaskDelay(20);
            if (!wrapper_btn_get(S2_BTN) && val > 25) {
                val--;
            }
        }

        xQueueOverwrite(queue_display_variable1, &change);
        xQueueOverwrite(queue_display_variable2, &val);
        vTaskDelay(200);
    }
}

void task_display_write(void *params) {
    display_variable_t val = 50;
    display_variable_t change = 0;
    uint8_t lux = 0;
    uint8_t show = 0;

    while (1) {
        xQueuePeek(queue_display_variable1, &change, portMAX_DELAY);
        xQueuePeek(queue_display_variable2, &val, portMAX_DELAY);
        xQueuePeek(queue_lux, &lux, portMAX_DELAY);

        show = (change == 0) ? val : lux;

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

void task_pwm(void *params) {
    adc_data_t data = {0};
    uint8_t pwm = 0;

    while (1) {
        xQueueReceive(queue_adc, &data, portMAX_DELAY);
        pwm = (uint8_t)((data.ref_raw * 100) / 4095);
        wrapper_pwm_update(pwm);
        xQueueOverwrite(queue_pwm, &pwm);
    }
}

void ADC0_SEQA_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (kADC_ConvSeqAInterruptFlag == (kADC_ConvSeqAInterruptFlag & ADC_GetStatusFlags(ADC0))) {
        ADC_ClearStatusFlags(ADC0, kADC_ConvSeqAInterruptFlag);

        adc_result_info_t temp_info, ref_info;
        ADC_GetChannelConversionResult(ADC0, REF_POT_CH, &ref_info);
        ADC_GetChannelConversionResult(ADC0, LM35_CH, &temp_info);

        adc_data_t data = {
            .temp_raw = (uint16_t)temp_info.result,
            .ref_raw = (uint16_t)ref_info.result
        };

        xQueueOverwriteFromISR(queue_adc, &data, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
