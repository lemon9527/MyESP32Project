#include <stdio.h>
#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_sntp.h"
#include "mqtt_client.h"
#include "mbedtls/md.h"
#include "cloud.h"
#include "am2020.h"

#define PRODUCT_KEY   "a1RROlKS48Q"
#define DEVICE_NAME   "esp32_am2020_01"
#define DEVICE_SECRET "b072b9feb2be71c0b2c68daa5d267ed2"

#define MQTT_BROKER   PRODUCT_KEY ".iot-as-mqtt.cn-shanghai.aliyuncs.com"

static const char *TAG = "cloud";
static esp_mqtt_client_handle_t mqtt_client = NULL;

static void cloud_sntp_init(void)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_init();

    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    while (timeinfo.tm_year < (2024 - 1900) && ++retry < 20) {
        ESP_LOGI(TAG, "Waiting for SNTP time sync...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    ESP_LOGI(TAG, "SNTP time synced");
}

static void cloud_hmac_sha1(const uint8_t *key, size_t key_len,
                             const uint8_t *data, size_t data_len,
                             uint8_t *output)
{
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
    mbedtls_md_hmac_starts(&ctx, key, key_len);
    mbedtls_md_hmac_update(&ctx, data, data_len);
    mbedtls_md_hmac_finish(&ctx, output);
    mbedtls_md_free(&ctx);
}

static void cloud_mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                      int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected to Alibaba Cloud IoT");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT data: topic=%.*s, data=%.*s",
                 event->topic_len, event->topic,
                 event->data_len, event->data);
        break;
    default:
        break;
    }
}

void cloud_init(void)
{
    cloud_sntp_init();

    time_t now;
    time(&now);
    char timestamp[16];
    snprintf(timestamp, sizeof(timestamp), "%lld", (long long)now * 1000);

    char client_id[256];
    snprintf(client_id, sizeof(client_id),
             "%s.%s|securemode=2,signmethod=hmacsha1,timestamp=%s|",
             PRODUCT_KEY, DEVICE_NAME, timestamp);

    char username[128];
    snprintf(username, sizeof(username), "%s&%s", DEVICE_NAME, PRODUCT_KEY);

    char short_client_id[64];
    snprintf(short_client_id, sizeof(short_client_id), "%s.%s", PRODUCT_KEY, DEVICE_NAME);

    char sign_content[512];
    snprintf(sign_content, sizeof(sign_content),
             "clientId%sdeviceName%sproductKey%stimestamp%s",
             short_client_id, DEVICE_NAME, PRODUCT_KEY, timestamp);

    uint8_t hmac[20];
    cloud_hmac_sha1((const uint8_t *)DEVICE_SECRET, strlen(DEVICE_SECRET),
                     (const uint8_t *)sign_content, strlen(sign_content),
                     hmac);

    char password[64];
    for (int i = 0; i < 20; i++) {
        snprintf(password + i * 2, 3, "%02X", hmac[i]);
    }

    ESP_LOGI(TAG, "Broker: %s", MQTT_BROKER);
    ESP_LOGI(TAG, "Client ID: %s", client_id);
    ESP_LOGI(TAG, "Username: %s", username);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://" MQTT_BROKER,
        .credentials = {
            .client_id = client_id,
            .username = username,
            .authentication.password = password,
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                    cloud_mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void cloud_publish_measurement(const am2020_data_t *data)
{
    if (mqtt_client == NULL) {
        return;
    }

    char topic[128];
    snprintf(topic, sizeof(topic),
             "/sys/%s/%s/thing/event/property/post",
             PRODUCT_KEY, DEVICE_NAME);

    time_t now;
    time(&now);
    long long ts = (long long)now * 1000;

    char payload[1024];
    snprintf(payload, sizeof(payload),
             "{"
             "\"id\":\"%lld\","
             "\"version\":\"1.0\","
             "\"sys\":{\"ack\":0},"
             "\"params\":{"
             "\"TVOC\":{\"value\":%u,\"time\":%lld},"
             "\"NO2\":{\"value\":%u,\"time\":%lld},"
             "\"HCHO\":{\"value\":%u,\"time\":%lld},"
             "\"PM1_0\":{\"value\":%u,\"time\":%lld},"
             "\"PM2_5\":{\"value\":%u,\"time\":%lld},"
             "\"PM10\":{\"value\":%u,\"time\":%lld},"
             "\"Temperature\":{\"value\":%.1f,\"time\":%lld},"
             "\"Humidity\":{\"value\":%.1f,\"time\":%lld}"
             "},"
             "\"method\":\"thing.event.property.post\""
             "}",
             ts,
             data->tvoc, ts,
             data->no2, ts,
             data->hcho, ts,
             data->pm1_0, ts,
             data->pm2_5, ts,
             data->pm10, ts,
             data->temperature, ts,
             data->humidity, ts);

    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
    ESP_LOGI(TAG, "Published to %s, msg_id=%d", topic, msg_id);
    ESP_LOGI(TAG, "Payload: %s", payload);
}