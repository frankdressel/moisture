#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
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
bool initialised = {false};

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_ERROR_CHECK(esp_event_post_to(moisture_eventloop, MOISTURE_EVENTS, MOISTURE_STORAGE_AVAILABLE_EVENT, NULL, sizeof(NULL), portMAX_DELAY));

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);

            ESP_ERROR_CHECK(esp_event_post_to(moisture_eventloop, MOISTURE_EVENTS, MOISTURE_STORED_EVENT, NULL, sizeof(NULL), portMAX_DELAY));

            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");

            ESP_ERROR_CHECK(esp_event_post_to(moisture_eventloop, MOISTURE_EVENTS, MOISTURE_WIFI_AVAILABLE_EVENT, NULL, sizeof(NULL), portMAX_DELAY));

            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

static void store_data(char *data) {
    char *msg = calloc(sizeof(data) + 20 + sizeof(CONFIG_SENSORID), sizeof(char));
    sprintf(msg, "{\"data\": %s, \"id\": \"%s\"}", data, CONFIG_SENSORID);

    char *topic = calloc(sizeof(CONFIG_TOPIC) + 1 + sizeof(CONFIG_SENSORID), sizeof(char));
    sprintf(topic, "%s/%s", CONFIG_TOPIC, CONFIG_SENSORID);

    esp_mqtt_client_publish(client, topic, msg, 0, 1, 0);

    free(topic);
    free(msg);
}

static void mqtt_init(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
	.username = CONFIG_MQTT_USER,
	.password = CONFIG_MQTT_PASS,
        .cert_pem = (const char *)certificate_pem_start
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    initialised = true;
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

static void mqtt_stop() {
    if(initialised) {
        esp_mqtt_client_stop(client);
    }
}
