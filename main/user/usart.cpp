#include "usart.h"
//#include "esp_log.h"
#include <bits/stdc++.h>
static const char *TAsG = "uart_events";
#define PATTERN_CHR_NUM (3) /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

usart::usart(int uartnum, int baudrate)
{
    if ((uartnum < 3) && (uartnum >= 0))
    {
        this->UartNum = uartnum;
    }
    this->uart_config = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
}

usart::~usart()
{
}

void usart::install(int port_txd, int port_rxd)
{
    uart_driver_install(UartNum, this->BufferSize * 2, 0, 0, NULL, 0);
    uart_param_config(UartNum, &uart_config);
    uart_set_pin(UartNum, port_txd, port_rxd, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // uart_driver_install(UartNum, this->BufferSize * 2, this->BufferSize * 2, 20, &uart0_queue, 0);
    // uart_param_config(UartNum, &uart_config);
    // // Set UART log level
    // esp_log_level_set(TAG, ESP_LOG_INFO);
    // // Set UART pins (using UART0 default pins ie no changes.)
    // uart_set_pin(UartNum, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // // Set uart pattern detect function.
    // uart_enable_pattern_det_baud_intr(UartNum, '+', PATTERN_CHR_NUM, 9, 0, 0);
    // // Reset the pattern queue length to record at most 20 pattern positions.
    // uart_pattern_queue_reset(UartNum, 20);
}

void usart::send(const void *src, size_t size)
{
    // 串口发送
    uart_write_bytes(UartNum, (const char *)src, size);
}

void usart::start_server(const char *const pcName, const uint32_t usStackDepth, UBaseType_t uxPriority)
{
    // 开始串口接收任务
    auto runtask = [](void *pvParameters)
    {
        usart *Pointer = (usart *)pvParameters;
        Pointer->runTask();
        // (usart *)pvParameters->startTask();
    };
    xTaskCreate(runtask, pcName, usStackDepth, this, uxPriority, NULL);
}

void usart::runTask()
{
    // 串口接收任务
    uint8_t RD_BUF_SIZE = this->BufferSize;
    uint8_t *rx_buffer = (uint8_t *)malloc(RD_BUF_SIZE);
    for (;;)
    {
        int len = uart_read_bytes(UartNum, rx_buffer, RD_BUF_SIZE, 20 / portTICK_RATE_MS);
        if (len > 0)
        {
            for (auto it = _subscriptions.begin(); it != _subscriptions.end(); it++)
            {
                if (it->un_used)
                {
                    continue;
                }
                it->callback(it->ctx, (const char *)rx_buffer, len);
            }
        }
        // ESP_LOGI(TAsG, "line:%d,Task = %d", __LINE__,uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(50);
        // sub->callback(sub->ctx, rx_buffer, len);
        // uart_write_bytes(UartNum, (const char *)rx_buffer, len);
    }
    free(rx_buffer);
}

bool usart::subscribe(on_usart_msg_cb callback, void *ctx)
{
    // 订阅串口信息
    auto it = std::find_if(_subscriptions.begin(), _subscriptions.end(),
                           [](subscription &sub)
                           { return sub.un_used; });
    if (it == _subscriptions.end())
    {
        return false;
    }
    it->callback = callback;
    it->ctx = ctx;
    it->un_used = false;
    return true;
}
