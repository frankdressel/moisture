/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <nvs_flash.h>

#include "wifi.c"
#include "mqtt.c"
#include "events.h"
#include "measurement.c"

static void deep_sleep_task(void* args) {
    while(1) {
        esp_sleep_enable_timer_wakeup(CONFIG_SLEEP_PERIODE * 1000);
        vTaskDelay(CONFIG_MEASUREMENT_PERIODE/portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Entering deep sleep");
        esp_deep_sleep_start();
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_event_loop_args_t moisture_measurement_eventloop_args = {
        .queue_size = 5,
        .task_name = "loop_task", // task will be created
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY
    };

    ESP_LOGI(TAG, "Starting event loop");
    ESP_ERROR_CHECK(esp_event_loop_create(&moisture_measurement_eventloop_args, &moisture_measurement_eventloop));

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    mqtt_init();

    measurement_init();

    xTaskCreate(deep_sleep_task, "deep_sleep_task", 1024, NULL, uxTaskPriorityGet(NULL), NULL);
}
