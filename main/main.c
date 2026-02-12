#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define DEBUG_BUILD // comment out for PRODUCTION

#ifdef DEBUG_BUILD
#define TIME_SCALE 1000UL
#else
#define TIME_SCALE 1UL
#endif

// Define the GPIO pins
#define SIGNALA_GPIO GPIO_NUM_16
#define SIGNALB_GPIO GPIO_NUM_21

#define BUTTON_ENABLE GPIO_NUM_32
#define BUTTON_SELECT GPIO_NUM_33

// Application state
volatile bool ENABLE_STATE = 0;
volatile bool NORMAL_STATE = 0;

// Helper macros for signal control
#define ON(pin) gpio_set_level(pin, 1)
#define OFF(pin) gpio_set_level(pin, 0)
#define WAIT(us) esp_rom_delay_us(us)

// Delay parameters for interrupt
#define debounceDelay 500 // ms
volatile unsigned long lastPressTimeEnable = 0;
volatile unsigned long lastPressTimeSelect = 0;

// timing parameters
const unsigned long T_ON1 = 100UL;                 // 100 µs base
const unsigned long T_OFF = 400UL * TIME_SCALE;    // 400 µs
const unsigned int NUM = 5;                        // number of pulses
const unsigned long T_WAIT = 4500UL * TIME_SCALE;  // 4500 µs
const unsigned long T_SYNC_ON = 50UL * TIME_SCALE; // 50 µs

unsigned int getT_ON(unsigned int n)
{
    return (n > 1 ? T_ON1 + (n - 1) * 50 : T_ON1) * TIME_SCALE;
}

// Queue handle for GPIO events
static QueueHandle_t gpio_evt_queue = NULL;

bool afterDebounceDelay(unsigned long lastPressTime)
{
    return (xTaskGetTickCount() * portTICK_PERIOD_MS - lastPressTime) > debounceDelay;
}

// ISR handler (MUST be very short and fast)
static void IRAM_ATTR button_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Task that handles button events
static void button_task(void *arg)
{
    uint32_t io_num;

    while (1)
    {
        // Wait for interrupt event
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            printf("Button %lu pressed! Toggling LED...\n", io_num);
            if (io_num == BUTTON_ENABLE)
            {
                if (afterDebounceDelay(lastPressTimeEnable))
                {
                    lastPressTimeEnable = xTaskGetTickCount() * portTICK_PERIOD_MS;

                    ENABLE_STATE = !ENABLE_STATE;
                    gpio_set_level(SIGNALA_GPIO, ENABLE_STATE);
                }
            }
            else if (io_num == BUTTON_SELECT)
            {
                if (afterDebounceDelay(lastPressTimeSelect))
                {
                    lastPressTimeSelect = xTaskGetTickCount() * portTICK_PERIOD_MS;

                    NORMAL_STATE = !NORMAL_STATE;
                    gpio_set_level(SIGNALB_GPIO, NORMAL_STATE);
                }
            }
        }
    }
}

static void signal_task(void *arg)
{
    while (1)
    {
        if (ENABLE_STATE)
        {
            if (NORMAL_STATE)
            {
                // normal form
                OFF(SIGNALA_GPIO);
                ON(SIGNALB_GPIO);
                WAIT(T_SYNC_ON);

                for (int i = 1; i <= NUM; i++)
                {
                    ON(SIGNALA_GPIO);
                    OFF(SIGNALB_GPIO);
                    WAIT(getT_ON(i));

                    OFF(SIGNALA_GPIO);
                    WAIT(T_OFF);
                }

                WAIT(T_WAIT);
            }
            else
            {
                // alternative form
                OFF(SIGNALA_GPIO);
                ON(SIGNALB_GPIO);
                WAIT(T_SYNC_ON);

                for (int i = NUM; i >= 1; i--)
                {
                    ON(SIGNALA_GPIO);
                    OFF(SIGNALB_GPIO);
                    WAIT(getT_ON(i));

                    OFF(SIGNALA_GPIO);
                    WAIT(T_OFF);
                }

                WAIT(T_WAIT);
            }
        }

        // vTaskDelay(1);
    }
}

void app_main(void)
{
    // --- LED Configuration ---
    gpio_config_t signalA_conf = {};
    signalA_conf.intr_type = GPIO_INTR_DISABLE;
    signalA_conf.mode = GPIO_MODE_OUTPUT;
    signalA_conf.pin_bit_mask = (1ULL << SIGNALA_GPIO);
    gpio_config(&signalA_conf);

    gpio_config_t signalB_conf = {};
    signalB_conf.intr_type = GPIO_INTR_DISABLE;
    signalB_conf.mode = GPIO_MODE_OUTPUT;
    signalB_conf.pin_bit_mask = (1ULL << SIGNALB_GPIO);
    gpio_config(&signalB_conf);

    // --- Button Configuration ---
    gpio_config_t btn_enable_conf = {};
    btn_enable_conf.intr_type = GPIO_INTR_POSEDGE; // Interrupt on rising edge
    btn_enable_conf.mode = GPIO_MODE_INPUT;
    btn_enable_conf.pin_bit_mask = (1ULL << BUTTON_ENABLE);
    btn_enable_conf.pull_up_en = 0;
    btn_enable_conf.pull_down_en = 0; // Disable pull-down if using active-high button
    gpio_config(&btn_enable_conf);

    gpio_config_t btn_select_conf = {};
    btn_select_conf.intr_type = GPIO_INTR_POSEDGE; // Interrupt on rising edge
    btn_select_conf.mode = GPIO_MODE_INPUT;
    btn_select_conf.pin_bit_mask = (1ULL << BUTTON_SELECT);
    btn_select_conf.pull_up_en = 0;
    btn_select_conf.pull_down_en = 0; // Disable pull-down if using active-high button
    gpio_config(&btn_select_conf);

    // Create queue
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Create task to handle button
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);

    // Install GPIO ISR service
    gpio_install_isr_service(0);

    // Attach ISR handler
    gpio_isr_handler_add(BUTTON_ENABLE, button_isr_handler, (void *)BUTTON_ENABLE);
    gpio_isr_handler_add(BUTTON_SELECT, button_isr_handler, (void *)BUTTON_SELECT);

    printf("Generating LED signals...\n");

    xTaskCreate(signal_task, "signal_task", 4096, NULL, 5, NULL);
}
