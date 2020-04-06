#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "events.h"
#include "moisture_constants.h"

extern const uint8_t certificate_pem_start[] asm("_binary_certificate_pem_start");
extern const uint8_t certificate_pem_end[] asm("_binary_certificate_pem_end");

static esp_mqtt_client_handle_t client;

static void moisture_measurement_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    ESP_LOGI(TAG, "Hallo");
    char *data = (char*) event_data;

    char topic[256] = {0};
    sprintf(topic, "%s/%d", CONFIG_TOPIC, CONFIG_SENSORID);
    ESP_LOGI(TAG, "Topic %s", topic);
    esp_mqtt_client_publish(client, "moisture/1", data, 0, 1, 0);
    ESP_LOGI(TAG, "handling %s:%s, iteration %s", base, "TASK_ITERATION_EVENT", data);
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
    client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            char msg[1024] = {0};
            sprintf(msg, "Sensor activated: %d", CONFIG_SENSORID);
            msg_id = esp_mqtt_client_publish(client, CONFIG_TOPIC, msg, 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            ESP_ERROR_CHECK(esp_event_handler_register_with(moisture_measurement_eventloop, MOISTURE_MEASUREMENT_EVENTS, MOISTURE_MEASUREMENT_EVENT, moisture_measurement_handler, moisture_measurement_eventloop));

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_init(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
	.username = CONFIG_MQTT_USER,
	.password = CONFIG_MQTT_PASS,
        .cert_pem = (const char *)certificate_pem_start
    };
    ESP_LOGI(TAG, "Sending mqtt message with user %s, password %s to %s", CONFIG_MQTT_USER, CONFIG_MQTT_PASS, CONFIG_BROKER_URL);

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}
