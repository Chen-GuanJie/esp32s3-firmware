#include "Server.h"
#include "driver/gpio.h"

Server::Server(/* args */)
{
  uint8_t pins[6] = {2, 4, 18, 34, 21, 37};
  ctrl = new gpios(pins, 6);
  u1 = new usart(0, 115200);
  u2 = new usart(1, 115200);
  u3 = new usart(2, 115200);
  u1->install(UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  u2->install(GPIO_NUM_38, GPIO_NUM_36);
  u3->install(GPIO_NUM_33, GPIO_NUM_35);

  u1->subscribe([](void *ctx, const void *msg, size_t len)
                { ((Server *)ctx)->callback((uint8_t *)msg, len); },
                this);
  u2->subscribe([](void *ctx, const void *msg, size_t len)
                { ((Server *)ctx)->callback((uint8_t *)msg, len); },
                this);
  u2->subscribe([](void *ctx, const void *msg, size_t len)
                { ((Server *)ctx)->callback((uint8_t *)msg, len); },
                this);

  m1 = new moto(u1, ctrl);
  m2 = new moto(u2, ctrl);
  m3 = new moto(u3, ctrl);
}

void Server::startTask(const char *const pcName, const uint32_t usStackDepth, UBaseType_t uxPriority)
{
  // 开始任务
  auto runtask = [](void *pvParameters)
  {
    Server *Pointer = (Server *)pvParameters;
    Pointer->runTask();
  };
  u1->start_server();
  u2->start_server();
  u3->start_server();
  m1->startTask();
  m2->startTask();
  m3->startTask();
  xTaskCreate(runtask, pcName, usStackDepth, this, uxPriority, NULL);
}
void Server::runTask()
{
  // 主任务
  Queue = xQueueCreate(5, sizeof(uint8_t) * 10);
  void *pv = (void *)malloc(sizeof(uint8_t) * 10);
  queuedata *qd = (queuedata *)malloc(sizeof(queuedata));
  qd->len = 1;
  qd->md = (queuedata::motodata *)malloc(sizeof(queuedata::motodata));
  qd->md->id = 1;
  qd->md->angle = 789;
  qd->md->time = 987;

  for (;;)
  {
    // if (xQueueReceive(Queue, pv, (portTickType)portMAX_DELAY))
    // {
    // }
    m1->sendQueue(qd);
    m2->sendQueue(qd);
    m3->sendQueue(qd);
    vTaskDelay(500);
  }
  free(pv);
}
Server::~Server()
{
}
void Server::callback(const uint8_t *msg, size_t len)
{
  xQueueSend(Queue, (void *)msg, (portTickType)portMAX_DELAY);
}
