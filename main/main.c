/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sleep.h"

static const char *TAG = "time-example";

#define SLEEP_EVENT_DEEP_SLEEP         (1 << 0)
uint64_t sleeptime = (uint64_t)(1000000 * 60);

static EventGroupHandle_t sleep_event_group;
static TimerHandle_t sleep_timer_watchdog;

void start_deep_sleep();
void deep_sleep_trigger();
void sleep_init();
void watchdog_init();
void TimeTask(void *pvParameter);
static void TimerCallback_Watchdog();

void sleep_init(){
    sleep_event_group = xEventGroupCreate();
    watchdog_init();
    xTaskCreate(&TimeTask, "TimeTask", 4096, NULL, 1, NULL);
}

void watchdog_init(){
    sleep_timer_watchdog = xTimerCreate("sleep_timer_watchdog", 1000, pdFALSE, NULL, TimerCallback_Watchdog);
    if (!sleep_timer_watchdog) {
        ESP_LOGE(TAG, "sensors_prepare_timer: failed to create the sleep_timer_watchdog timer!");
        return;
    }

    // We need and have a timer, so make sure it uses the correct interval, then start it.
    ESP_LOGD(TAG, "sleep_prepare_timer: timer interval = %d s\n", 1);
    xTimerChangePeriod(sleep_timer_watchdog, pdMS_TO_TICKS(1000 * 10), pdMS_TO_TICKS(5000));

    if (pdFAIL == xTimerReset(sleep_timer_watchdog, pdMS_TO_TICKS(5000))) {
        ESP_LOGE(TAG, "sleep_prepare_timer: failed to start the sleep_timer_watchdog timer!");
        return;
    }
}

static void TimerCallback_Watchdog(){
    ESP_LOGE(TAG, "[WATCHDOG] time limit reached. Forcing sleep..");
    xEventGroupSetBits(sleep_event_group, SLEEP_EVENT_DEEP_SLEEP);
}

void deep_sleep_trigger(){
    xEventGroupSetBits(sleep_event_group, SLEEP_EVENT_DEEP_SLEEP);
}

void TimeTask(void *pvParameter){

    EventBits_t timerEvent;

    ESP_LOGI(TAG, "Timer task\r\n");
    timerEvent = xEventGroupWaitBits(sleep_event_group,SLEEP_EVENT_DEEP_SLEEP, pdTRUE, pdFALSE, portMAX_DELAY);

    if( (timerEvent & SLEEP_EVENT_DEEP_SLEEP) == SLEEP_EVENT_DEEP_SLEEP ){
        start_deep_sleep();
    }
    vTaskDelete( NULL );
}

void start_deep_sleep(){
    ESP_LOGI(TAG, "All operations complete. Going into deep sleep for %lld seconds",  sleeptime/1000000);
    esp_sleep_enable_timer_wakeup(sleeptime);
    esp_deep_sleep_start();
}

void app_main(void)
{
    sleep_init();

    uint16_t counter = 0;
    while(1){
        ESP_LOGW(TAG, "Running time: %d\n", counter++);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}
