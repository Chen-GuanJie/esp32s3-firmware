/*
 * TcpClient.c
 *
 *  Created on: 2021年12月23日
 *      Author: wangy
 */
#include "TcpClient.h"

EventGroupHandle_t tcp_event_group; // wifi建立成功信号量
bool g_rxtx_need_restart = false;   //异常后，重新连接标记
QueueHandle_t tcpMsgQueue;

/*
 * wifi 事件
 * @param[in]   void  		       :无
 * @retval      void                :无
 */
static esp_err_t event_handler(void *ctx, system_event_t *event)
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
        ESP_LOGI(TAG, "got ip:%s\n",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

/*
 * WIFI作为STA的初始化
 * @param[in]   void  		       :无
 * @retval      void                :无
 */
void wifi_init_sta()
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

static struct sockaddr_in server_addr; // server地址
static int connect_socket = 0;         //连接socket

/**
 * 创建一个TcpClient连接
 */
esp_err_t create_tcp_client()
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

/*
 * 接收数据任务
 * @param[in]   void  		       :无
 * @retval      void                :无
 */
void recv_data(void *pvParameters)
{
    int len = 0;         //长度
    char databuff[1024]; //缓存
    //tcpMsgQueue = xQueueCreate(5, sizeof(uint8_t) * 10);

    while (1)
    {
        //清空缓存
        memset(databuff, 0x00, sizeof(databuff));
        //读取接收数据
        len = recv(connect_socket, databuff, sizeof(databuff), 0);
        g_rxtx_need_restart = false;
        if (len > 0)
        {
            //打印接收到的数组
            ESP_LOGI(TAG, "recvData: %s", databuff);
            //接收数据回发
            xQueueSend(tcpMsgQueue, (void *)databuff, (portTickType)portMAX_DELAY);
            send(connect_socket, databuff, strlen(databuff), 0);
        }
        else
        {
            //打印错误信息
            ESP_LOGI(TAG, "recvData error");
            g_rxtx_need_restart = true; //连接异常，至断线重连标志为1，以便tcp_connect连接任务进行重连
            break;
        }
    }
    close_socket();
    g_rxtx_need_restart = true;
    vTaskDelete(NULL);
}

/*
 * 关闭socket
 * @param[in]   socket  		       :socket编号
 * @retval      void                :无
 */
void close_socket()
{
    close(connect_socket); //关闭客户端
}