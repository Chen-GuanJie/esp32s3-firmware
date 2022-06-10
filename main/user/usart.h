#ifndef __USART_H
#define __USART_H
#include "driver/uart.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
typedef void (*on_usart_msg_cb)(void *ctx, const void *msg, size_t len);
#include <array>
class usart
{
private:
    uart_config_t uart_config;
    uint16_t BufferSize = 1024 ;
    static QueueHandle_t uart0_queue;
    uint8_t UartNum;
    struct subscription
    {
        bool un_used = true;
        on_usart_msg_cb callback; // void (*callback)(void *ctx,const void* msg,size_t len);
        void *ctx;
    };
    std::array<subscription, 4> _subscriptions;
    // void (*callback)(T *ctx,const void* msg,size_t len);
public:
    void runTask();
    void install(int port_txd, int port_rxd);
    void start_server(const char *const pcName = "uart_task", const uint32_t usStackDepth = 2048, UBaseType_t uxPriority = 12);
    void send(const void *src, size_t size);
    bool subscribe(on_usart_msg_cb callback, void *ctx);
    usart(int uartnum, int baudrate);
    ~usart();
};

#endif
