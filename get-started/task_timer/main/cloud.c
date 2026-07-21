#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "cloud.h"

#define MQTT_BROKER   "8.153.89.110"
#define MQTT_USERNAME "lemon9527"
#define MQTT_PASSWORD "lemonliu121510"

static const char *TAG = "cloud";
static esp_mqtt_client_handle_t mqtt_client = NULL;

static void cloud_mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                      int32_t event_id, void *event_data)
{
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected to EMQX Cloud");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected, will auto-reconnect");
        break;
    default:
        break;
    }
}

void cloud_init(void)
{
    ESP_LOGI(TAG, "Connecting to EMQX at %s:1883...", MQTT_BROKER);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://" MQTT_BROKER,
        .credentials = {
            .client_id = "ESP32_AM2020_SEN68",
            .username = MQTT_USERNAME,
            .authentication.password = MQTT_PASSWORD,
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                    cloud_mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void cloud_publish_am2020_measurement(const am2020_data_t *data)
{
    if (mqtt_client == NULL) {
        return;
    }

    char payload[512];
    snprintf(payload, sizeof(payload),
             "{"
             "\"TVOC\":%u,"
             "\"NO2\":%u,"
             "\"HCHO\":%u,"
             "\"PM1_0\":%u,"
             "\"PM2_5\":%u,"
             "\"PM10\":%u,"
             "\"Temperature\":%.1f,"
             "\"Humidity\":%.1f"
             "}",
             data->tvoc,
             data->no2,
             data->hcho,
             data->pm1_0,
             data->pm2_5,
             data->pm10,
             data->temperature,
             data->humidity);

    esp_mqtt_client_publish(mqtt_client, "sensor/am2020", payload, 0, 1, 1);
    ESP_LOGI(TAG, "[AM2020] %s", payload);
}

void cloud_publish_sen6x_measurement(const sen6x_data_t *data, sen6x_type_t type)
{
    if (mqtt_client == NULL) {
        return;
    }

    char payload[512];
    snprintf(payload, sizeof(payload),
             "{"
             "\"PM1_0\":%.1f,"
             "\"PM2_5\":%.1f,"
             "\"PM4_0\":%.1f,"
             "\"PM10\":%.1f,"
             "\"Temperature\":%.1f,"
             "\"Humidity\":%.1f,"
             "\"TVOC\":%.1f,"
             "\"NOx_Index\":%.1f,"
             "\"HCHO\":%.1f,"
             "\"CO2\":%.1f"
             "}",
             data->pm1_0,
             data->pm2_5,
             data->pm4_0,
             data->pm10,
             data->temperature,
             data->humidity,
             data->tvoc_ppb,
             data->nox_index,
             data->hcho,
             data->co2);

    esp_mqtt_client_publish(mqtt_client, "sensor/sen6x", payload, 0, 1, 1);
    ESP_LOGI(TAG, "[%s] %s", (type == SEN6X_TYPE_66) ? "SEN66" : "SEN68", payload);
}

void cloud_publish_mcuc_measurement(const mcuc_data_t *data)
{
    if (mqtt_client == NULL) {
        return;
    }

    char payload[512];
    snprintf(payload, sizeof(payload),
             "{"
             "\"PM1_0\":%u,"
             "\"PM2_5\":%u,"
             "\"PM10\":%u,"
             "\"Temperature\":%u,"
             "\"Humidity\":%u,"
             "\"TVOC\":%u,"
             "\"CO2\":%u,"
             "\"Light\":%u,"
             "\"Pressure\":%u"
             "}",
             data->pms_in_pm1_0,
             data->pms_in_pm2_5,
             data->pms_in_pm10,
             data->temperature,
             data->humidity,
             data->tvoc_count,
             data->co2_count,
             data->light_state,
             data->pressure_count);

    esp_mqtt_client_publish(mqtt_client, "sensor/mcuc", payload, 0, 1, 1);
    ESP_LOGI(TAG, "[MCUC] %s", payload);
}

void cloud_publish_dummy(const char *payload)
{
    if (mqtt_client == NULL) {
        return;
    }
    esp_mqtt_client_publish(mqtt_client, "sensor/dummy", payload, 0, 1, 1);
    ESP_LOGI(TAG, "[DUMMY] %s", payload);
}