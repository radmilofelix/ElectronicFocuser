#include "modbusSlave.h"

ModbusSlave::ModbusSlave(uint8_t slaveAddr, Stream& serialAddr, uint8_t enablePin) : mbslave(slaveAddr, serialAddr, enablePin)
{
	for(int i=0; i<32; i++)
		dataBuffer[i]=0;
}

ModbusSlave::~ModbusSlave()
{
}
void ModbusSlave::begin(long baud)
{
    Serial1.begin(19200);
    mbslave.start();
}

void ModbusSlave::poll(uint8_t dataSize)
{
    mbslave.poll(dataBuffer, dataSize);
}
