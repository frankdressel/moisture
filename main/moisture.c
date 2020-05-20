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

#include "driver/rtc_io.h"

#include "wifi.c"
#include "mqtt.c"
#include "events.h"
#include "measurement.c"

static void moisture_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ESP_LOGI(TAG, "Got event: %d", event_id);
    if (event_id == MOISTURE_WIFI_AVAILABLE_EVENT) {
        mqtt_init();
    }
    if (event_id == MOISTURE_STORAGE_AVAILABLE_EVENT) {
        measurement_init();
    }
    if (event_id == MOISTURE_MEASUREMENT_EVENT) {
        char *data = (char*) event_data;
        store_data(data);
    }
    if (event_id == MOISTURE_STORED_EVENT || event_id == MOISTURE_ERROR_EVENT) {
        uint64_t sp = (uint64_t)CONFIG_SLEEP_PERIODE;
        esp_sleep_enable_timer_wakeup(sp * 1000000);
        ESP_LOGI(TAG, "Entering deep sleep");
        mqtt_stop();
        esp_wifi_stop();

        rtc_gpio_isolate(GPIO_NUM_12);

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

    esp_event_loop_args_t moisture_eventloop_args = {
        .queue_size = 5,
        .task_name = "loop_task", // task will be created
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY
    };

    ESP_LOGI(TAG, "Starting event loop");
    ESP_ERROR_CHECK(esp_event_loop_create(&moisture_eventloop_args, &moisture_eventloop));

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    ESP_ERROR_CHECK(esp_event_handler_register_with(moisture_eventloop, MOISTURE_EVENTS, ESP_EVENT_ANY_ID, &moisture_event_handler, NULL));
    wifi_init_sta();
}
