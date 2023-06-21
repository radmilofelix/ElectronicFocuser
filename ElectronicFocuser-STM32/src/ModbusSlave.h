#ifndef MODBUSSLAVE___H
#define  MODBUSSLAVE___H

#include "ModbusRtu.h"


class ModbusSlave
{
    public:
    ModbusSlave(uint8_t slaveAddr, Stream& serialAddr, uint8_t enablePin);
    ~ModbusSlave();
    void begin(long baud);
    void poll(uint8_t dataSize);
    uint16_t dataBuffer[32];
    Modbus mbslave;
};

#endif
