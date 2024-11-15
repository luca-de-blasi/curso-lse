#include "clock_config.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "board.h"
#include "FreeRTOS.h"
#include "fsl_debug_console.h"
#include "task.h"
#include "semphr.h"
#include "fsl_sctimer.h"
#include "fsl_swm.h"
#include "fsl_gpio.h"
#include "fsl_common.h"
#include "fsl_i2c.h"
#include "fsl_adc.h"
#include "queue.h"
#include "semphr.h"
#include "fsl_power.h"
#include <stdbool.h>
#include <stdio.h>

#define COM_1 8
#define COM_2 9
#define SEG_A 10
#define SEG_B 11
#define SEG_C 6
#define SEG_D 14
#define SEG_E 0
#define SEG_F 13
#define SEG_G 15
#define BUTTON_1 16
#define BUTTON_2 25
#define BUTTON_USER 4
#define PWM_FREQUENCY 1000
#define LIGHT_SENSOR_ADDR 0x5c
#define ADC_CHANNEL 8

uint32_t pwm_event;

// Queue definitions
QueueHandle_t queue_target;
QueueHandle_t queue_light_level;
QueueHandle_t queue_pwm_duty;
QueueHandle_t queue_display_mode;

// Semaphore definition
SemaphoreHandle_t count_semaphore;

// Function prototypes
void task_light_reading(void *params);
void task_display_control(void *params);
void task_adjust_target(void *params);
void task_pwm_control(void *params);
void task_logging(void *params);
void task_button_monitor(void *params);

void init_system();
void init_display();
void init_buttons();
void init_i2c();
void init_adc();
void init_pwm();
void display_show_number(uint8_t number);
void display_turn_off(void);
void display_select_digit(uint8_t com);
void turn_off_segments(void);
void activate_segment(uint8_t segment);

int main(void) {
    BOARD_BootClockFRO30M();

    init_system();

    count_semaphore = xSemaphoreCreateCounting(100, 25);

    // Create tasks
    xTaskCreate(task_button_monitor, "ButtonMonitor", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_display_control, "DisplayControl", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_light_reading, "LightSensor", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_adjust_target, "TargetAdjust", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_pwm_control, "PWMControl", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_logging, "Logging", 3 * configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    PRINTF("Time (ms) | Light | Target | PWM\n");

    vTaskStartScheduler();
}

void task_button_monitor(void *params) {
    bool display_target;

    while (true) {
        if (!GPIO_PinRead(GPIO, 0, BUTTON_USER)) {
            display_target = !display_target;
        }

        vTaskDelay(100);

        xQueueOverwrite(queue_display_mode, &display_target);
    }
}

void task_display_control(void *params) {
    bool display_target;
    uint32_t target_value;
    uint8_t light_level;
    uint8_t display_value;

    while (true) {
        xQueuePeek(queue_target, &target_value, 100);
        xQueuePeek(queue_light_level, &light_level, 100);
        xQueuePeek(queue_display_mode, &display_target, 100);

        display_value = display_target ? target_value : light_level;

        display_turn_off();
        display_show_number(display_value / 10);
        display_select_digit(COM_1);
        vTaskDelay(5);

        display_turn_off();
        display_show_number(display_value % 10);
        display_select_digit(COM_2);
        vTaskDelay(5);
    }
}

void task_light_reading(void *params) {
    while (true) {
        if (I2C_MasterStart(I2C1, LIGHT_SENSOR_ADDR, kI2C_Read) == kStatus_Success) {
            uint8_t data[2] = {0};
            I2C_MasterReadBlocking(I2C1, data, 2, kI2C_TransferDefaultFlag);
            I2C_MasterStop(I2C1);

            float lux = ((data[0] << 8) + data[1]) / 1.2;
            uint8_t lux_percentage = lux * 100 / 20000;

            xQueueOverwrite(queue_light_level, &lux_percentage);
        }
        vTaskDelay(50);
    }
}

void task_adjust_target(void *params) {
    while (true) {
        if (!GPIO_PinRead(GPIO, 0, BUTTON_1)) {
            xSemaphoreGive(count_semaphore);
        } else if (!GPIO_PinRead(GPIO, 0, BUTTON_2)) {
            xSemaphoreTake(count_semaphore, 10);
        }

        vTaskDelay(85);

        uint32_t target_value = uxSemaphoreGetCount(count_semaphore);
        xQueueOverwrite(queue_target, &target_value);
        vTaskDelay(50);
    }
}

void task_logging(void *params) {
    uint32_t target_value = 25;
    uint8_t light_level;
    uint8_t pwm_duty;

    while (true) {
        xQueuePeek(queue_target, &target_value, 100);
        xQueuePeek(queue_light_level, &light_level, 100);
        xQueuePeek(queue_pwm_duty, &pwm_duty, 100);

        uint32_t time = xTaskGetTickCount();

        PRINTF("%8d | %3d%% | %3d%% | %3d%%\n", time, light_level, target_value, pwm_duty);
        vTaskDelay(1000);
    }
}

void task_pwm_control(void *params) {
    adc_result_info_t adc_info;

    while (true) {
        ADC_DoSoftwareTriggerConvSeqA(ADC0);
        while (!ADC_GetChannelConversionResult(ADC0, ADC_CHANNEL, &adc_info));
        uint8_t pwm_duty = adc_info.result * 100 / 4095;

        SCTIMER_UpdatePwmDutycycle(SCT0, kSCTIMER_Out_4, pwm_duty, pwm_event);
        xQueueOverwrite(queue_pwm_duty, &pwm_duty);
        vTaskDelay(50);
    }
}

void init_system() {
    init_i2c();
    init_pwm();
    init_adc();
    init_display();
    init_buttons();
}

void init_i2c() {
    BOARD_BootClockFRO30M();

    CLOCK_Select(kI2C1_Clk_From_MainClk);
    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SDA, kSWM_PortPin_P0_27);
    SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SCL, kSWM_PortPin_P0_26);
    CLOCK_DisableClock(kCLOCK_Swm);

    i2c_master_config_t i2c_config;
    I2C_MasterGetDefaultConfig(&i2c_config);
    i2c_config.baudRate_Bps = 400000;
    I2C_MasterInit(I2C1, &i2c_config, SystemCoreClock);

    if (I2C_MasterStart(I2C1, LIGHT_SENSOR_ADDR, kI2C_Write) == kStatus_Success) {
        uint8_t cmd = 0x01;
        I2C_MasterWriteBlocking(I2C1, &cmd, 1, kI2C_TransferDefaultFlag);
        I2C_MasterStop(I2C1);
    }

    queue_light_level = xQueueCreate(1, sizeof(uint8_t));
}

void init_adc() {
    BOARD_InitDebugConsole();

    CLOCK_EnableClock(kCLOCK_Swm);
    SWM_SetFixedPinSelect(SWM0, kSWM_ADC_CHN8, true);
    CLOCK_DisableClock(kCLOCK_Swm);

    CLOCK_Select(kADC_Clk_From_Fro);
    CLOCK_SetClkDivider(kCLOCK_DivAdcClk, 1);

    POWER_DisablePD(kPDRUNCFG_PD_ADC0);

    uint32_t freq = CLOCK_GetFreq(kCLOCK_Fro) / CLOCK_GetClkDivider(kCLOCK_DivAdcClk);
    ADC_DoSelfCalibration(ADC0, freq);

    adc_config_t adc_cfg;
    ADC_GetDefaultConfig(&adc_cfg);
    ADC_Init(ADC0, &adc_cfg);

    adc_conv_seq_config_t adc_seq = { .channelMask = 1 << ADC_CHANNEL, .triggerMask = 0, .triggerPolarity = kADC_TriggerPolarityPositiveEdge, .enableSyncBypass = false, .interruptMode = kADC_InterruptForEachConversion };
    ADC_SetConvSeqAConfig(ADC0, &adc_seq);
    ADC_EnableConvSeqA(ADC0, true);
}

void init_display(void) {
    GPIO_PortInit(GPIO, 0);
    gpio_pin_config_t out_config = {kGPIO_DigitalOutput, true};
    uint32_t pins[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G, COM_1, COM_2};
    for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        GPIO_PinInit(GPIO, 0, pins[i], &out_config);
    }

    queue_display_mode = xQueueCreate(1, sizeof(bool));
}

void display_show_number(uint8_t number) {
    static const bool segments[10][7] = {
        {1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 0, 0, 0, 0}, {1, 1, 0, 1, 1, 0, 1}, {1, 1, 1, 1, 0, 0, 1}, {0, 1, 1, 0, 0, 1, 1},
        {1, 0, 1, 1, 0, 1, 1}, {1, 0, 1, 1, 1, 1, 1}, {1, 1, 1, 0, 0, 0, 0}, {1, 1, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 0, 1, 1}
    };
    for (int i = 0; i < 7; i++) {
        if (segments[number][i]) {
            activate_segment(SEG_A + i);
        }
    }
}

void display_turn_off(void) {
    turn_off_segments();
    GPIO_PinWrite(GPIO, 0, COM_1, 0);
    GPIO_PinWrite(GPIO, 0, COM_2, 0);
}

void turn_off_segments(void) {
    uint32_t pins[] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G};
    for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        GPIO_PinWrite(GPIO, 0, pins[i], 0);
    }
}

void display_select_digit(uint8_t com) {
    GPIO_PinWrite(GPIO, 0, com, 1);
}

void activate_segment(uint8_t segment) {
    GPIO_PinWrite(GPIO, 0, segment, 1);
}

void init_buttons() {
    GPIO_PortInit(GPIO, 0);
    gpio_pin_config_t button_config = {kGPIO_DigitalInput, 0};
    GPIO_PinInit(GPIO, 0, BUTTON_USER, &button_config);
    GPIO_PinInit(GPIO, 0, BUTTON_1, &button_config);
    GPIO_PinInit(GPIO, 0, BUTTON_2, &button_config);
}

void init_pwm() {
    CLOCK_AttachClk(kFRO_DIV4_to_SCT_CLK);

    sctimer_config_t pwm_cfg;
    SCTIMER_GetDefaultConfig(&pwm_cfg);
    SCTIMER_Init(SCT0, &pwm_cfg);

    sctimer_pwm_signal_param_t pwm_signal = { .output = kSCTIMER_Out_4, .level = kSCTIMER_HighTrue, .dutyCyclePercent = 0 };
    SCTIMER_SetupPwm(SCT0, &pwm_signal, kSCTIMER_CenterAlignedPwm, PWM_FREQUENCY, CLOCK_GetFreq(kCLOCK_Fro), &pwm_event);

    queue_pwm_duty = xQueueCreate(1, sizeof(uint8_t));
    queue_target = xQueueCreate(1, sizeof(uint32_t));
}
