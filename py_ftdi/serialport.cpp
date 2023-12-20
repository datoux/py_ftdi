/**
 * Copyright (C) 2014 Daniel Turecek
 *
 * @file      serialport.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2014-03-23
 *
 */
#include "serialport.h"
#include <ctime>

#ifndef WIN32
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/time.h>
#endif

#define SP_INVALID_HANDLE 0xFFFFFFFF
#define COMREAD_TIMEOUT 2
#define COMBYTES_COUNT  100
#ifdef WIN32
#include "windows.h"
#define SLEEPTIME_0                     (1)
#else
#define SLEEPTIME_0                     0.000001    // sleep time for threads loops data
#endif
#define USLEEP_TIME                     1000        // sleep time for usleep function (in us)

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

typedef unsigned char byte;
typedef unsigned short u16;
typedef unsigned int u32;



static inline double getPrecTime() {
#ifdef WIN32
    static double initPrecTime = 0;
    static LARGE_INTEGER perfFrequency = {{0,0}};
    LARGE_INTEGER currentCount;
    if (perfFrequency.QuadPart == 0)
        QueryPerformanceFrequency(&perfFrequency);
    QueryPerformanceCounter(&currentCount);
    double precTime =  currentCount.QuadPart/(double)perfFrequency.QuadPart;
    if (initPrecTime == 0) {
        time_t initTime = 0;
        time(&initTime);
        SYSTEMTIME sysTime;
        GetSystemTime(&sysTime);
        initPrecTime = (double) initTime + sysTime.wMilliseconds * .001 - precTime;
    }
    return initPrecTime + precTime;
#else
    timeval timeNow;
    gettimeofday(&timeNow, 0);
    return (double) timeNow.tv_sec + ((double)timeNow.tv_usec)/1000000.0;
#endif
}



SerialPort::SerialPort()
    : mHandle(SP_INVALID_HANDLE)
    , mOpened(false)
{

}

SerialPort::~SerialPort()
{
    if (mOpened)
        close();
}

int SerialPort::open(const char* serialPortID, bool defaultInit)
{
#ifdef WIN32
    mHandle = (size_t)CreateFileA(serialPortID, GENERIC_READ | GENERIC_WRITE, 0, 0,
                               OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL
                               |FILE_FLAG_NO_BUFFERING, NULL);   // handle to file with attributes to copy
    if ((HANDLE)mHandle != INVALID_HANDLE_VALUE && defaultInit)
        setSerialParameters(9600, 8, 0, 1);
    if ((HANDLE)mHandle == INVALID_HANDLE_VALUE)
        return -1;
    return 0;
#else
    int fd;
    fd = ::open(serialPortID, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
        return -1;
    mHandle = (size_t)fd;
    if (defaultInit)
        setSerialParameters(9600, 8, 0, 1);
    fcntl(fd, F_SETFL, FNDELAY);
    return 0;
#endif
}

int SerialPort::close()
{
#ifdef WIN32
    return (mHandle != SP_INVALID_HANDLE) ? (size_t)CloseHandle((HANDLE)mHandle) : -1;
#else
    return ::close(mHandle);
#endif
}
static int rateToConstant(int baudrate) {
#ifdef __linux__
#define B(x) case x: return B##x
    switch(baudrate) {
        B(50);     B(75);     B(110);    B(134);    B(150);
        B(200);    B(300);    B(600);    B(1200);   B(1800);
        B(2400);   B(4800);   B(9600);   B(19200);  B(38400);
        B(57600);  B(115200); B(230400); B(460800); B(500000);
        B(576000); B(921600); B(1000000);B(1152000);B(1500000);
    default: return 0;
    }
#undef B
#else
    return baudrate;
#endif
}

int SerialPort::setSerialParameters(unsigned baud, byte byteSize, byte parity, byte stopBits)
{
#ifdef WIN32
    // the time-out parameters for a communications device
    COMMTIMEOUTS tout;
    tout.ReadIntervalTimeout = MAXDWORD;
    tout.ReadTotalTimeoutMultiplier = MAXDWORD;
    tout.ReadTotalTimeoutConstant = 50;
    tout.WriteTotalTimeoutMultiplier = MAXDWORD;
    tout.WriteTotalTimeoutConstant = 50;
    if (!SetCommTimeouts((HANDLE)mHandle, &tout))
        return -1;

    // defines the control setting for a serial communications device
    DCB dcb;
    SecureZeroMemory(&dcb, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState((HANDLE)mHandle, &dcb))
        return -2;
    dcb.BaudRate = baud;
    dcb.ByteSize = byteSize;
    dcb.Parity   = parity;
    dcb.StopBits = stopBits - 1;
    return SetCommState((HANDLE)mHandle, &dcb) ? 0 : -3;
#else
    struct termios options;
    int rc = 0;
    if ((rc = tcgetattr(mHandle, &options)))
        fprintf(stderr, "Cannot get attr: %d, %s\n", mHandle, strerror(errno));
    if ((rc = cfsetispeed(&options, rateToConstant(baud))))
        fprintf(stderr, "Cannot set speed in: %d, %s\n", mHandle, strerror(errno));
    if ((rc = cfsetospeed(&options, rateToConstant(baud))))
        fprintf(stderr, "Cannot set speed out: %d, %s\n", mHandle, strerror(errno));
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_iflag &= ~(ICRNL);
    options.c_oflag &= ~(ONLCR);
    options.c_oflag &= ~(OPOST);
    options.c_lflag &= ~(ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE);
    options.c_cc[VMIN] = 0;
    if (stopBits == 2) options.c_cflag |= CSTOPB;
    if (parity == 1)   options.c_cflag |= PARODD | PARENB;
    if (parity == 2)   options.c_cflag |= PARENB;
    switch (byteSize) {
         case 8: default: options.c_cflag |= CS8; break;
         case 7: options.c_cflag |= CS7; break;
         case 6: options.c_cflag |= CS6; break;
         case 5: options.c_cflag |= CS5; break;
    }
    if ((rc = tcsetattr(mHandle, TCSANOW, &options)))
        fprintf(stderr, "Cannot set options : %d, %s\n", mHandle, strerror(errno));
    return 0;
#endif
}

int SerialPort::send(byte* data, size_t size)
{
#ifdef WIN32
    if (mHandle == SP_INVALID_HANDLE)
        return -1;
    if (size == 0)
        size = strlen((char*)data);
    if (size == 0)
        return -2;
    DWORD sendBytes = 0;
    WriteFile((HANDLE)mHandle, data, size, &sendBytes, NULL);
    return sendBytes;
#else
    int n = (int)write(mHandle, data, (ssize_t)size);
    return n;
#endif
}

int SerialPort::receive(byte* buffer, size_t bufferSize, size_t toReceiveCount, double timeout)
{
    if (timeout < 0) timeout = COMREAD_TIMEOUT;
    if (toReceiveCount > bufferSize) toReceiveCount = bufferSize;

    double startTime = getPrecTime();
    size_t allBytesReceived = 0, bytesToTake = COMBYTES_COUNT;
#ifdef WIN32
    double sleepTime = SLEEPTIME_0;
    DWORD bytesReceived = 0;
#else
    int bytesReceived = 0;
    int received = 0;
#endif

    while (allBytesReceived < toReceiveCount) {
        // timeout occured
        if (getPrecTime() - startTime > timeout)
            return -1;

        u32 getbytes = (u32)min(bytesToTake, (toReceiveCount - allBytesReceived));

        #ifdef WIN32
            ReadFile((HANDLE)mHandle, buffer + allBytesReceived, getbytes, &bytesReceived, NULL);
        #else
            received = (int)read(mHandle, buffer + allBytesReceived, getbytes);
            if (received > 0)
                bytesReceived = received;
        #endif

        allBytesReceived += (size_t)bytesReceived;

        #ifdef WIN32
            if (sleepTime > 16)
                sleepThread(sleepTime/1000);
        #else
            usleep(USLEEP_TIME);
        #endif
    } //while

    if ((u32)allBytesReceived < bufferSize)
        buffer[allBytesReceived] = 0; // Zero terminating if there is space in buffer
    return (int)allBytesReceived;
}

std::string SerialPort::receiveLine(double timeout)
{
    double startTime = getPrecTime();
    size_t bytesToTake = COMBYTES_COUNT;
    if (timeout < 0)
        timeout = COMREAD_TIMEOUT;

#ifdef WIN32
    double sleepTime = SLEEPTIME_0;
    DWORD bytesReceived = 0;
#else
    int bytesReceived = 0;
    int received = 0;
#endif

    char buffer[2049];
    size_t size = 2048;

    while (1) {
        size_t pos = mExtraData.find_first_of('\n');
        if (pos != std::string::npos){
            std::string line = mExtraData.substr(0, pos);
            mExtraData.erase(0, pos + 1);
            return line;
        }

        // timeout occured
        if (getPrecTime() - startTime > timeout)
            return "";

        u32 getbytes = (u32)min(bytesToTake, size);
        memset(buffer, 0, size+1);

        #ifdef WIN32
            ReadFile((HANDLE)mHandle, buffer, getbytes, &bytesReceived, NULL);
        #else
            received = (int)read(mHandle, buffer, getbytes);
            if (received > 0)
                bytesReceived = received;
        #endif

        if (bytesReceived > 0)
            mExtraData += buffer;

        #ifdef WIN32
            if (sleepTime > 16)
                sleepThread(sleepTime/1000);
        #else
            usleep(USLEEP_TIME);
        #endif
    } //while

    return "";
}

