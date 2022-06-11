#ifndef MAIN_TCPCLIENT_H_
#define MAIN_TCPCLIENT_H_

#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#define TAG "TCP" //打印的tag
/**
 * STA模式配置信息
 */
#define GATEWAY_SSID "Galaxy S21 5G"       //路由器账号
#define GATEWAY_PAS "hmvv7890"             //路由器密码
#define TCP_SERVER_ADRESS "192.168.192.22" //作为client，要连接TCP服务器地址
#define TCP_PORT 8888                      //统一的端口号，包括TCP客户端或者服务端

// FreeRTOS event group to signal when we are connected to wifi
#define WIFI_CONNECTED_BIT BIT0            //事件组标志。
extern EventGroupHandle_t tcp_event_group; // Tcp事件组
extern bool g_rxtx_need_restart;           //断线重连
//配置ESP32为STA
#ifdef __cplusplus //如果定义了宏__cplusplus就执行#ifdef 到 #endif之间的语句
extern "C"
{
#endif
    void wifi_init_sta();
    //创建一个TCP Client连接
    esp_err_t create_tcp_client();
    //数据接收任务，由FreeRTOS创建任务
    void recv_data(void *pvParameters);
    //关闭所有的socket
    void close_socket();
#ifdef __cplusplus
}
#endif
#endif /* MAIN_TCPCLIENT_H_ */