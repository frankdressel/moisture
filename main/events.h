#pragma once

#include "esp_event_base.h"

ESP_EVENT_DECLARE_BASE(MOISTURE_MEASUREMENT_EVENTS);
ESP_EVENT_DEFINE_BASE(MOISTURE_MEASUREMENT_EVENTS);
enum {
    MOISTURE_MEASUREMENT_EVENT
};

static esp_event_loop_handle_t moisture_measurement_eventloop;
