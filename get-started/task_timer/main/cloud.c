#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "cloud.h"

#define MQTT_BROKER   "v11e4189.ala.dedicated.aliyun.emqxcloud.cn"
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
    ESP_LOGI(TAG, "Connecting to EMQX Cloud at %s:1883...", MQTT_BROKER);

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
             "\"AM2020_TVOC\":%u,"
             "\"AM2020_NO2\":%u,"
             "\"AM2020_HCHO\":%u,"
             "\"AM2020_PM1_0\":%u,"
             "\"AM2020_PM2_5\":%u,"
             "\"AM2020_PM10\":%u,"
             "\"AM2020_Temperature\":%.1f,"
             "\"AM2020_Humidity\":%.1f"
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

void cloud_publish_sen68_measurement(const sen68_data_t *data)
{
    if (mqtt_client == NULL) {
        return;
    }

    char payload[512];
    snprintf(payload, sizeof(payload),
             "{"
             "\"SEN68_PM1_0\":%.1f,"
             "\"SEN68_PM2_5\":%.1f,"
             "\"SEN68_PM4_0\":%.1f,"
             "\"SEN68_PM10\":%.1f,"
             "\"SEN68_Temperature\":%.1f,"
             "\"SEN68_Humidity\":%.1f,"
             "\"SEN68_TVOC\":%.1f,"
             "\"SEN68_NOx_Index\":%.1f,"
             "\"SEN68_HCHO\":%.1f"
             "}",
             data->pm1_0,
             data->pm2_5,
             data->pm4_0,
             data->pm10,
             data->temperature,
             data->humidity,
             data->tvoc_ppb,
             data->nox_index,
             data->hcho);

    esp_mqtt_client_publish(mqtt_client, "sensor/sen68", payload, 0, 1, 1);
    ESP_LOGI(TAG, "[SEN68] %s", payload);
}