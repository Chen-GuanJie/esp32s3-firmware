/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
//#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
// #define EXAMPLE_ESP_WIFI_SSID "esp32"
// #define EXAMPLE_ESP_WIFI_PASS "1234512345"
// #define EXAMPLE_ESP_WIFI_CHANNEL 1
// #define EXAMPLE_MAX_STA_CONN 1

static const char *TAG = "wifi softAP";

// static void wifi_event_handler(void *arg, esp_event_base_t event_base,
//                                int32_t event_id, void *event_data)
// {
//     if (event_id == WIFI_EVENT_AP_STACONNECTED)
//     {
//         wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
//         ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
//                  MAC2STR(event->mac), event->aid);
//     }
//     else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
//     {
//         wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
//         ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
//                  MAC2STR(event->mac), event->aid);
//     }
// }

// void wifi_init_softap(void)
// {
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     esp_netif_create_default_wifi_ap();

//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//     ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
//                                                         ESP_EVENT_ANY_ID,
//                                                         &wifi_event_handler,
//                                                         NULL,
//                                                         NULL));

//     wifi_config_t wifi_config = {
//         .ap = {
//             .ssid = {'e', 's', 'p', '3', '2', '\0'},
//             .password = {'1', '2', '3', '4', '5', '1', '2', '3', '4', '5', '\0'},
//             .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
//             .channel = EXAMPLE_ESP_WIFI_CHANNEL,
//             .authmode = WIFI_AUTH_WPA_WPA2_PSK,
//             .max_connection = EXAMPLE_MAX_STA_CONN,
//         },
//     };

//     if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
//     {
//         wifi_config.ap.authmode = WIFI_AUTH_OPEN;
//     }

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_start());

//     ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
//              EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
// }

#include "user/Server.h"
#include "user/TcpClient.h"

/*
 * 任务：建立TCP连接并从TCP接收数据
 * @param[in]   void  		       :无
 * @retval      void                :无
 */
static void tcp_connect(void *pvParameters)
{
    while (1)
    {
        g_rxtx_need_restart = false;
        //等待WIFI连接信号量(即等待ESP32作为STA连接到热点的信号)，死等
        xEventGroupWaitBits(tcp_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
        ESP_LOGI(TAG, "start tcp connected");
        TaskHandle_t tx_rx_task = NULL; //任务句柄
        //延时3S准备建立clien
        vTaskDelay(3000 / portTICK_RATE_MS);
        ESP_LOGI(TAG, "create tcp Client");
        //建立client
        int socket_ret = create_tcp_client();

        if (socket_ret == ESP_FAIL)
        {
            //建立失败
            ESP_LOGI(TAG, "create tcp socket error,stop...");
            continue;
        }
        else
        {
            //建立成功
            ESP_LOGI(TAG, "create tcp socket succeed...");
            //建立tcp接收数据任务
            if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task)) // tx_rx_task 任务句柄
            {
                //建立失败
                ESP_LOGI(TAG, "Recv task create fail!");
            }
            else
            {
                //建立成功
                ESP_LOGI(TAG, "Recv task create succeed!");
            }
        }
        while (1)
        { //每3秒钟检查一次是否需要重新连接
            vTaskDelay(3000 / portTICK_RATE_MS);
            if (g_rxtx_need_restart)
            {
                vTaskDelay(3000 / portTICK_RATE_MS);
                ESP_LOGI(TAG, "reStart create tcp client...");
                //建立client
                int socket_ret = create_tcp_client();

                if (socket_ret == ESP_FAIL)
                {
                    ESP_LOGE(TAG, "reStart create tcp socket error,stop...");
                    continue;
                }
                else
                {
                    ESP_LOGI(TAG, "reStart create tcp socket succeed...");
                    //重新建立完成，清除标记
                    g_rxtx_need_restart = false;
                    //建立tcp接收数据任务
                    if (pdPASS != xTaskCreate(&recv_data, "recv_data", 4096, NULL, 4, &tx_rx_task))
                    {
                        ESP_LOGE(TAG, "reStart Recv task create fail!");
                    }
                    else
                    {
                        ESP_LOGI(TAG, "reStart Recv task create succeed!");
                    }
                }
            }
        }
    }

    vTaskDelete(NULL); //删除任务本身
}

extern "C" void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_init_sta();
    xTaskCreate(&tcp_connect, "tcp_connect", 4096, NULL, 5, NULL); //新建一个TCP连接任务

    Server *s = new Server();
    s->startTask();

    while (1)
    {
        vTaskDelay(50);
    }
}
