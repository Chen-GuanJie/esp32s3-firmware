#include "moto.h"
#define GET_LOW_BYTE(A) ((uint8_t)(A))
#define GET_HIGH_BYTE(A) ((uint8_t)((A) >> 8))
#define BYTE_TO_HW(A, B) ((((uint16_t)(A)) << 8) | (uint8_t)(B))
#include "esp_log.h"

moto::moto(usart *u, gpios *c) //(uint8_t uartnum, int baudrate) : usart(uartnum, baudrate)
{
  uart = u;
  ctrl = c;
}
moto::~moto()
{
}

void moto::startTask(const char *const Name, const uint32_t usStackDepth, UBaseType_t uxPriority)
{
  // 开始任务
  queue = xQueueCreate(5, sizeof(queuedata));
  auto runtask = [](void *pvParameters)
  {
    moto *Pointer = (moto *)pvParameters;
    Pointer->runTask();
  };
  xTaskCreate(runtask, Name, usStackDepth, this, uxPriority, NULL);
}
void moto::enablesend()
{
  switch (uart->get_num())
  {
  case 0:
    ESP_LOGI("moto", "000");
    // ctrl->set(4, 0);
    // ctrl->set(2, 1);
    break;
  case 1:
    ESP_LOGI("moto", "111");
    // ctrl->set(21, 0);
    // ctrl->set(37, 1);
    break;
  case 2:
    ESP_LOGI("moto", "222");
    ctrl->set(34, 0);
    ctrl->set(18, 1);
    break;
  default:
    break;
  }
}
void moto::runTask()
{
  // 主任务
  queuedata *pv = (queuedata *)malloc(sizeof(queuedata));
  uint8_t txbuffer[BUFFER_NUM];
  uint8_t i = 0;
  for (;;)
  {
    if (xQueueReceive(queue, pv, (portTickType)portMAX_DELAY))
    {
      // if (uart->get_num() == 2)
      // {
        for (i = 0; i < pv->len; i++)
        {
          LobotSerialServoMove(txbuffer, pv->md[i].id, pv->md[i].angle, pv->md[i].time);
          // ESP_LOGI("moto", "%s", txbuffer);
          enablesend();
          uart->send(txbuffer, 10);
        }
      // }
    }
    // ESP_LOGI("moto", "line:%d,Task = %d", __LINE__,uxTaskGetStackHighWaterMark(NULL));
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
  free(pv);
}

uint8_t moto::LobotCheckSum(uint8_t buf[])
{
  uint8_t i;
  uint16_t temp = 0;
  for (i = 2; i < buf[3] + 2; i++)
  {
    temp += buf[i];
  }
  temp = ~temp;
  i = (uint8_t)temp;
  return i;
}
void moto::sendQueue(queuedata *qd)
{
  if (qd == nullptr)
  {
    return;
  }
  xQueueSend(queue, qd, 20 / portTICK_RATE_MS);
}
uint8_t *moto::LobotSerialServoSetID(uint8_t oldID, uint8_t newID)
{
  uint8_t buf[7];
  buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
  buf[2] = oldID;
  buf[3] = 4;
  buf[4] = LOBOT_SERVO_ID_WRITE;
  buf[5] = newID;
  buf[6] = LobotCheckSum(buf);
  uint8_t *p = &buf[0];
  return p;
}
void moto::LobotSerialServoMove(uint8_t *buf, uint8_t id, int16_t position, uint16_t time)
{
  if (position < 0)
    position = 0;
  if (position > 1000)
    position = 1000;
  buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
  buf[2] = id;
  buf[3] = 7;
  buf[4] = LOBOT_SERVO_MOVE_TIME_WRITE;
  buf[5] = GET_LOW_BYTE(position);
  buf[6] = GET_HIGH_BYTE(position);
  buf[7] = GET_LOW_BYTE(time);
  buf[8] = GET_HIGH_BYTE(time);
  buf[9] = LobotCheckSum(buf);
}
uint8_t *moto::LobotSerialServoUnload(uint8_t id)
{
  uint8_t buf[7];
  buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
  buf[2] = id;
  buf[3] = 4;
  buf[4] = LOBOT_SERVO_LOAD_OR_UNLOAD_WRITE;
  buf[5] = 0;
  buf[6] = LobotCheckSum(buf);
  uint8_t *p = &buf[0];
  return p;
}
uint8_t *moto::LobotSerialServoLoad(uint8_t id)
{
  uint8_t buf[7];
  buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
  buf[2] = id;
  buf[3] = 4;
  buf[4] = LOBOT_SERVO_LOAD_OR_UNLOAD_WRITE;
  buf[5] = 1;
  buf[6] = LobotCheckSum(buf);
  uint8_t *p = &buf[0];
  return p;
}
void moto::LobotSerialServoReadPosition(uint8_t *buf, uint8_t id)
{
  buf[0] = buf[1] = LOBOT_SERVO_FRAME_HEADER;
  buf[2] = id;
  buf[3] = 3;
  buf[4] = LOBOT_SERVO_POS_READ;
  buf[5] = LobotCheckSum(buf);
}
int moto::LobotSerialMsgHandle(void)
{
  uint8_t cmd;
  int ret;

  if (LobotCheckSum(LobotRxBuf) != LobotRxBuf[LobotRxBuf[3] + 2])
  {
    return -2049;
  }

  cmd = LobotRxBuf[4];
  switch (cmd)
  {
  case LOBOT_SERVO_POS_READ:
    ret = (int)BYTE_TO_HW(LobotRxBuf[6], LobotRxBuf[5]);
    return ret;
  default:
    break;
  }
  return 0;
}
