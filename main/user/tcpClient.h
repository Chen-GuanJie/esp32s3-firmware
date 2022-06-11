#ifndef _TCPCLIENT_H
#define _TCPCLIENT_H

#include "esp_wifi.h"
#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define GATEWAY_SSID "Galaxy S21 5G"       //路由器账号
#define GATEWAY_PAS "hmvv7890"             //路由器密码
#define TCP_SERVER_ADRESS "192.168.192.22" //作为client，要连接TCP服务器地址
#define TCP_PORT 8888                      //统一的端口号，包括TCP客户端或者服务端
#define WIFI_CONNECTED_BIT BIT0            //事件组标志。

class TcpClient
{
private:
    EventGroupHandle_t *tcp_event_group; // wifi建立成功信号量
    bool g_rxtx_need_restart = false;    //异常后，重新连接标记
    static esp_err_t event_handler(void *ctx, system_event_t *event);
    struct sockaddr_in *server_addr; // server地址
    int connect_socket = 0;          //连接socket

    /* data */
public:
    void wifi_init_sta();
    esp_err_t create_tcp_client();
    void recv_data(void *pvParameters);
    void close_socket();
    static void tcp_connect(void *pvParameters);

    TcpClient(/* args */);
    ~TcpClient();
};

TcpClient::TcpClient(/* args */)
{
}

TcpClient::~TcpClient()
{
}
void TcpClient::wifi_init_sta()
{
    tcp_event_group = xEventGroupCreate();
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = GATEWAY_SSID,     // STA账号
            .password = GATEWAY_PAS}, // STA密码
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s \n",
             GATEWAY_SSID, GATEWAY_PAS);
}
esp_err_t TcpClient::create_tcp_client()
{
    ESP_LOGI(TAG, "will connect gateway ssid : %s port:%d",
             TCP_SERVER_ADRESS, TCP_PORT);
    //新建socket对象
    connect_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (connect_socket < 0)
    {
        ESP_LOGI(TAG, "Failed connect gateway ssid : %s port:%d",
                 TCP_SERVER_ADRESS, TCP_PORT);
        //新建失败后，关闭新建的socket，等待下次新建
        close(connect_socket);
        return ESP_FAIL;
    }
    //配置连接服务器信息
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_PORT);
    server_addr.sin_addr.s_addr = inet_addr(TCP_SERVER_ADRESS);
    ESP_LOGI(TAG, "connectting server...");
    //连接服务器
    if (connect(connect_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        ESP_LOGE(TAG, "connect failed!");
        //连接失败后，关闭之前新建的socket，等待下次新建
        close(connect_socket);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "connect success!");
    return ESP_OK;
}

esp_err_t TcpClient::event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START: // STA模式-开始连接
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED: // STA模式-断线
        esp_wifi_connect();
        xEventGroupClearBits(tcp_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:                             // STA模式-连接成功
        xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT); //时间组发出连接成功
        break;
    case SYSTEM_EVENT_STA_GOT_IP: // STA模式-获取IP
        ESP_LOGI(TAG, "got ip:%s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

/*
 * 接收数据任务
 * @param[in]   void  		       :无
 * @retval      void                :无
 */
void TcpClient::recv_data(void *pvParameters)
{
    int len = 0;         //长度
    char databuff[1024]; //缓存
    while (1)
    {
        //清空缓存
        memset(databuff, 0x00, sizeof(databuff));
        //读取接收数据
        len = recv(connect_socket, databuff, sizeof(databuff), 0);
        bool g_rxtx_need_restart = false;
        if (len > 0)
        {
            //打印接收到的数组
            ESP_LOGI(TAG, "recvData: %s", databuff);
            //接收数据回发
            send(connect_socket, databuff, strlen(databuff), 0);
        }
        else
        {
            //打印错误信息
            ESP_LOGI(TAG, "recvData error");
            bool g_rxtx_need_restart = true; //连接异常，至断线重连标志为1，以便tcp_connect连接任务进行重连
            break;
        }
    }
    close_socket();
    bool g_rxtx_need_restart = true;
    vTaskDelete(NULL);
}

/*
 * 关闭socket
 * @param[in]   socket  		       :socket编号
 * @retval      void                :无
 */
void TcpClient::close_socket()
{
    close(connect_socket); //关闭客户端
}

/*
 * 任务：建立TCP连接并从TCP接收数据
 * @param[in]   void  		       :无
 * @retval      void                :无
 */
static void TcpClient::tcp_connect(void *pvParameters)
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

#endif