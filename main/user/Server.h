#ifndef __SERVER_H
#define __SERVER_H
#include "moto.h"
class Server 
{
private:
    QueueHandle_t Queue;
    gpios *ctrl;
    usart *u1;
    usart *u2;
    usart *u3;
    moto *m1;
    moto *m2;
    moto *m3;
    void callback(const uint8_t *msg, size_t len);

public:
    Server(/* args */);
    ~Server();
    void startTask(const char *const pcName = "server_task", const uint32_t usStackDepth = 1024, UBaseType_t uxPriority = 12);
    void runTask();
};

#endif