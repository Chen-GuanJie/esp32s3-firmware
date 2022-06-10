#ifndef __MOTO_H
#define __MOTO_H
#include "usart.h"
#include "gpios.h"
#define MOTO_NUM 3
#define BUFFER_NUM 20
struct queuedata
{
    struct motodata
    {
        uint8_t id;
        int16_t angle;
        uint16_t time;
    } * md;
    uint8_t len;
};

class moto // : private usart
{
private:
    usart *uart;
    gpios *ctrl;
    uint8_t LobotRxBuf[16];
    QueueHandle_t queue;
    uint8_t LobotCheckSum(uint8_t buf[]);
    uint8_t *LobotSerialServoSetID(uint8_t oldID, uint8_t newID);
    void LobotSerialServoMove(uint8_t *buf, uint8_t id, int16_t position, uint16_t time);
    uint8_t *LobotSerialServoUnload(uint8_t id);
    uint8_t *LobotSerialServoLoad(uint8_t id);
    void LobotSerialServoReadPosition(uint8_t *buf, uint8_t id);
    int LobotSerialMsgHandle(void);

public:
    moto(usart *uart, gpios *ctrl);
    moto(uint8_t uartnum, int baudrate);
    void runTask();
    void sendQueue(queuedata* qd);
    void startTask(const char *const Name = "moto_task", const uint32_t usStackDepth = 2048, UBaseType_t uxPriority = 12);
    ~moto();
};
#define LOBOT_SERVO_FRAME_HEADER 0x55
#define LOBOT_SERVO_MOVE_TIME_WRITE 1
#define LOBOT_SERVO_MOVE_TIME_READ 2
#define LOBOT_SERVO_MOVE_TIME_WAIT_WRITE 7
#define LOBOT_SERVO_MOVE_TIME_WAIT_READ 8
#define LOBOT_SERVO_MOVE_START 11
#define LOBOT_SERVO_MOVE_STOP 12
#define LOBOT_SERVO_ID_WRITE 13
#define LOBOT_SERVO_ID_READ 14
#define LOBOT_SERVO_ANGLE_OFFSET_ADJUST 17
#define LOBOT_SERVO_ANGLE_OFFSET_WRITE 18
#define LOBOT_SERVO_ANGLE_OFFSET_READ 19
#define LOBOT_SERVO_ANGLE_LIMIT_WRITE 20
#define LOBOT_SERVO_ANGLE_LIMIT_READ 21
#define LOBOT_SERVO_VIN_LIMIT_WRITE 22
#define LOBOT_SERVO_VIN_LIMIT_READ 23
#define LOBOT_SERVO_TEMP_MAX_LIMIT_WRITE 24
#define LOBOT_SERVO_TEMP_MAX_LIMIT_READ 25
#define LOBOT_SERVO_TEMP_READ 26
#define LOBOT_SERVO_VIN_READ 27
#define LOBOT_SERVO_POS_READ 28
#define LOBOT_SERVO_OR_MOTOR_MODE_WRITE 29
#define LOBOT_SERVO_OR_MOTOR_MODE_READ 30
#define LOBOT_SERVO_LOAD_OR_UNLOAD_WRITE 31
#define LOBOT_SERVO_LOAD_OR_UNLOAD_READ 32
#define LOBOT_SERVO_LED_CTRL_WRITE 33
#define LOBOT_SERVO_LED_CTRL_READ 34
#define LOBOT_SERVO_LED_ERROR_WRITE 35
#define LOBOT_SERVO_LED_ERROR_READ 36

#endif
