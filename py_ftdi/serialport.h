/**
 * Copyright (C) 2014 Daniel Turecek
 *
 * @file      serialport.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2014-03-23
 *
 */
#ifndef SERIALPORT_H
#define SERIALPORT_H
#include <string>

class SerialPort
{
public:
    SerialPort();
    virtual ~SerialPort();

    int open(const char* serialPortID, bool defaultInit=false);
    int close();
    int setSerialParameters(unsigned baud, unsigned char byteSize, unsigned char parity, unsigned char stopBits);
    int send(unsigned char* data, size_t size);
    int receive(unsigned char* buffer, size_t bufferSize, size_t toReceiveCount, double timeout);
    std::string receiveLine(double timeout=2);

private:
    unsigned int mHandle;
    bool mOpened;
    std::string mExtraData;

};

#endif /* end of include guard: SERIALPORT_H */


