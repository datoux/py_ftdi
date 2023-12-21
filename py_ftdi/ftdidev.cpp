/**
 * Copyright (C) 2023 Daniel Turecek
 *
 * @file      ftdidev.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2023-02-05
 *
 */
#define _CRT_SECURE_NO_WARNINGS
#include "ftdidev.h"
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <cstring>

// #ifndef FITPIX_LIBFTDI
// #define FITPIX_LIBFTDI
// #endif

#ifdef _MSC_VER
#define NOMINMAX
#endif
#ifdef WIN32
    #include "windows.h"
    #include "winnt.h"
#else
    #include <unistd.h>
    #include <pthread.h>
    #include <sys/time.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#ifdef __APPLE__
#define __useconds_t useconds_t
#endif

inline void sleepThreadF(double seconds);
double getPreciseTime();

std::vector<unsigned> FtdiDev::mVidPids;
std::map<std::string, unsigned> FtdiDev::mNameToVidPid;

//########################################################################################################################
//                                              LIB FTD2XX
//########################################################################################################################
#ifndef FITPIX_LIBFTDI
#include "ftd2xx.h"
#ifndef WIN32
#include "WinTypes.h"
#endif

#define FT_STEP          65535
#define SLEEPTIME_WIN    1     // in ms
#define SLEEPTIME_UNIX   1000  // in us
#define READ_TIMEOUT     500   // maximal read timeout of FTDI
#define WRITE_TIMEOUT    500   // maximal write timeout of FTDI

#define FT_ERR_MSG_LEN  19
const char* FT_ERR_MSG[FT_ERR_MSG_LEN]={
  "","Invalid handle","Device not found","Device not opened","IO error",
  "Insuficient resources","Invalid parameter","Invalid baud rate",
  "Device not opened for erase","Device not opened for write",
  "Failed to write device","EEPROM read failed","EEPROM write failed",
  "EEPROM erase failed","EEPROM not present","EEPROM not programmed",
  "Invalid Args","Not supported","Other Error"
};

FtdiDev::FtdiDev(std::string nameOrSerial, bool isSerial)
    : mNameOrSerial(nameOrSerial)
    , mIsSerial(isSerial)
    , mFlowControl(false)
    , mIsSerialPort(false)
    , mLastError("")
    , mExtraData("")
    , mOnDataFunc(NULL)
    , mOnDataUserData(NULL)
{
}

FtdiDev::~FtdiDev()
{
}

inline bool isValidDevice(const char* desc, const char* filters[], size_t size, bool ignoreB)
{
    if (ignoreB && strlen(desc) > 1 && desc[strlen(desc) - 1] == 'B')
        return false;

    if (size == 0 || !filters)
        return true;

    for (unsigned j = 0; j < size; j++)
        if (strncmp(desc, filters[j], strlen(filters[j])) == 0)
            return true;
    return false;
}

int FtdiDev::listDevicesByNameFast(const char* filters[], size_t size, std::vector<FtdiDevInfo> &devices, bool ignoreB, bool getSerial)
{
    DWORD devCount = 0;
    FT_STATUS fts = FT_OK;
    char* buffers[51];
    for (unsigned i = 0; i < 50; i++){
        buffers[i] = new char[65];
        memset(buffers[i], 0, 65);
    }
    buffers[50] = NULL;

    // list normal devices
    fts = FT_ListDevices(buffers, &devCount, FT_LIST_ALL | FT_OPEN_BY_DESCRIPTION);
    //if (fts == FT_OK) { // commented out because of FTDI error
        for (unsigned i = 0; i < devCount; i++) {
            const char* desc = buffers[i];

            if (!isValidDevice(desc, filters, size, ignoreB))
                continue;

            if (getSerial){
                char serialbuff[64];
                size_t devIndex = i;
                FT_ListDevices((PVOID)devIndex, serialbuff, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);
                devices.push_back(FtdiDevInfo(desc, serialbuff, 0));
            }else
                devices.push_back(FtdiDevInfo(desc, "", 0));
        }
    //}

#ifndef WIN32
    // on Mac and Linux - list special VID PID devices:
    if (!mVidPids.empty()){
        for (unsigned vp = 0; vp < mVidPids.size(); vp++){
            DWORD oldVID = 0, oldPID = 0;
            unsigned vid = (mVidPids[vp] >> 16) & 0xFFFF;
            unsigned pid = mVidPids[vp] & 0xFFFF;
            FT_GetVIDPID(&oldVID, &oldPID);
            FT_SetVIDPID(vid, pid);

            fts = FT_ListDevices(buffers, &devCount, FT_LIST_ALL | FT_OPEN_BY_DESCRIPTION);
            if (fts != FT_OK) {
                for (unsigned i = 0; i < devCount; i++) {
                    const char* desc = buffers[i];

                    if (!isValidDevice(desc, filters, size, ignoreB))
                        continue;

                    FtdiDevInfo devInfo("", "", 0);
                    if (getSerial){
                        char serialbuff[64];
                        size_t devIndex = i;
                        FT_ListDevices((PVOID)devIndex, serialbuff, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);
                        devInfo = FtdiDevInfo(desc, serialbuff, 0);
                    }else
                        devInfo = FtdiDevInfo(desc, "", 0);

                    bool exists = false;
                    for (size_t devidx = 0; devidx < devices.size(); devidx++){
                        if (devices[i].name.compare(devInfo.name) == 0 && devices[i].serial.compare(devInfo.serial) == 0){
                            exists = true;
                            break;
                        }
                    }

                    if (!exists){
                        devices.push_back(devInfo);
                        mNameToVidPid[std::string(getSerial ? devInfo.serial : devInfo.name)] = mVidPids[vp];
                    }
                }
            }

            FT_SetVIDPID(oldVID, oldPID);
        }
    }
#endif

    for (unsigned i = 0; i < 50; i++)
        delete[] buffers[i];
    return fts;
}


int FtdiDev::listDevicesByName(const char* filters[], size_t size, std::vector<FtdiDevInfo> &devices, bool ignoreB, bool getSerial)
{
    (void)getSerial;
    DWORD devCount = 0;
    FT_STATUS fts = FT_OK;

    // first list normal devices:
    if ((fts = FT_CreateDeviceInfoList(&devCount)) != FT_OK)
        return fts;

    FT_DEVICE_LIST_INFO_NODE* devInfos = new FT_DEVICE_LIST_INFO_NODE[devCount];
    if ((fts = FT_GetDeviceInfoList(devInfos, &devCount)) != FT_OK){
        delete[] devInfos;
        return fts;
    }

    for (unsigned i = 0; i < devCount; i++) {
        const char* desc = devInfos[i].Description;
        if (!isValidDevice(desc, filters, size, ignoreB))
            continue;
        devices.push_back(FtdiDevInfo(desc, devInfos[i].SerialNumber, devInfos->ID));

    }
    delete[] devInfos;


    // on Mac and Linux - list devices with special VID and PID
    #ifndef WIN32
    if (!mVidPids.empty()){
        for (unsigned vp = 0; vp < mVidPids.size(); vp++){
            DWORD oldVID = 0, oldPID = 0;
            unsigned vid = (mVidPids[vp] >> 16) & 0xFFFF;
            unsigned pid = mVidPids[vp] & 0xFFFF;
            FT_GetVIDPID(&oldVID, &oldPID);
            FT_SetVIDPID(vid, pid);

            if ((fts = FT_CreateDeviceInfoList(&devCount)) != FT_OK)
                return fts;

            FT_DEVICE_LIST_INFO_NODE* devInfos = new FT_DEVICE_LIST_INFO_NODE[devCount];
            if ((fts = FT_GetDeviceInfoList(devInfos, &devCount)) != FT_OK){
                delete[] devInfos;
                return fts;
            }

            for (unsigned i = 0; i < devCount; i++) {
                const char* desc = devInfos[i].Description;

                if (!isValidDevice(desc, filters, size, ignoreB))
                    continue;

                bool exists = false;
                for (size_t devidx = 0; devidx < devices.size(); devidx++){
                    if (devices[i].name.compare(desc) == 0 && devices[i].serial.compare(devInfos[i].SerialNumber) == 0){
                        exists = true;
                        break;
                    }
                }

                if (!exists){
                    devices.push_back(FtdiDevInfo(desc, getSerial ? devInfos[i].SerialNumber : "", devInfos->ID));
                    mNameToVidPid[std::string(desc)] = mVidPids[vp];
                }

            }

            delete[] devInfos;
            FT_SetVIDPID(oldVID, oldPID);
        }
    }
    #endif

    return 0;
}

int FtdiDev::addVidPid(unsigned vid, unsigned pid)
{
#ifndef WIN32
    mVidPids.push_back(((vid << 16) & 0xFFFF0000) | (pid & 0xFFFF));
    return 0;
#else
    (void)vid;
    (void)pid;
    return 0;
#endif
}

int FtdiDev::addVidPid(unsigned vidpid)
{
#ifndef WIN32
    for (size_t i = 0; i < mVidPids.size(); i++){
        if (mVidPids[i] == vidpid)
            return 0;
    }
    mVidPids.push_back(vidpid);
#endif
    return 0;
}


int FtdiDev::listDevicesByVidpid(unsigned long vidpids[], size_t size, std::vector<FtdiDevInfo> &devices, bool ignoreB)
{
    // add vid pids to table (needed for Mac and Linux)
#ifndef WIN32
    for (unsigned i = 0; i < size; i++)
        FT_SetVIDPID((vidpids[i] >> 16) & 0xFFFF, vidpids[i] & 0xFFFF);
#endif

    DWORD devCount = 0;
    FT_STATUS fts = FT_OK;
    if ((fts = FT_CreateDeviceInfoList(&devCount)) != FT_OK)
        return fts;

    FT_DEVICE_LIST_INFO_NODE* devInfos = new FT_DEVICE_LIST_INFO_NODE[devCount];
    if ((fts = FT_GetDeviceInfoList(devInfos, &devCount)) != FT_OK){
        delete[] devInfos;
        return fts;
    }

    for (unsigned i = 0; i < devCount; i++) {
        const char* desc = devInfos[i].Description;
        if (ignoreB && strlen(desc) > 1 && desc[strlen(desc) - 1] == 'B')  // ignore B parts of the devices
            continue;
        for (unsigned j = 0; j < size; j++) {
            if (devInfos[i].ID == vidpids[j]){
                devices.push_back(FtdiDevInfo(desc, devInfos[i].SerialNumber, devInfos[i].ID));
                break;
            }
        }
    }
    delete[] devInfos;
    return 0;
}


int FtdiDev::openDevice(bool flowControl, unsigned vidpid, unsigned intf)
{
    (void)vidpid;
#ifndef WIN32
    if (mNameToVidPid.find(mNameOrSerial) != mNameToVidPid.end()){
        unsigned vidpid = mNameToVidPid[mNameOrSerial];
        unsigned vid = (vidpid >> 16) & 0xFFFF;
        unsigned pid = vidpid & 0xFFFF;
        FT_SetVIDPID(vid, pid);
    }
#endif
    std::string nameOrSerial = mNameOrSerial;

    if (nameOrSerial.size() > 0 && intf > 0 && intf < 5) {
        char lastChar = nameOrSerial[nameOrSerial.size() - 1];
        char endChars[] = {' ', 'A', 'B', 'C', 'D'};
        if (lastChar != endChars[intf]) {
            nameOrSerial += " ";
            nameOrSerial += endChars[intf];
        }
    }

    mFlowControl = flowControl;
    FT_STATUS fts = FT_OpenEx(const_cast<char*>(nameOrSerial.c_str()),
                              mIsSerial ? FT_OPEN_BY_SERIAL_NUMBER : FT_OPEN_BY_DESCRIPTION,
                              (FT_HANDLE*)&mHandle);
    if (fts != FT_OK){
        mHandle = NULL;
        mLastError = FT_ERR_MSG[fts];
        return fts;
    }
    FT_Purge((FT_HANDLE)mHandle, FT_PURGE_RX | FT_PURGE_TX);
    FT_SetTimeouts((FT_HANDLE)mHandle, READ_TIMEOUT, WRITE_TIMEOUT);
    FT_SetLatencyTimer((FT_HANDLE)mHandle, 2);
    if (flowControl)
        FT_SetFlowControl((FT_HANDLE)mHandle, FT_FLOW_RTS_CTS, 0x0, 0x0);
    // sets the size of usb packets. When used in virtual machines
    // sometimes it is better not to set this.
    FT_SetUSBParameters((FT_HANDLE)mHandle, 0x10000, 0x10000);
    return 0;
}

int FtdiDev::closeDevice()
{
    if (mHandle){
        FT_STATUS fts = FT_Close((FT_HANDLE)mHandle);
        mLastError = FT_ERR_MSG[fts];
        return fts;
    }
    mHandle = NULL;
    return 0;
}

bool FtdiDev::isConnected()
{
    DWORD rx, tx, event;
    if (!mHandle)
        return false;
    return FT_GetStatus((FT_HANDLE)mHandle, &rx, &tx, &event) == FT_OK;
}

#define ASYNC_MODE 0x01
#define SYNC_MODE  0x40
int FtdiDev::setBitMode(FtdiBitMode mode)
{
    FT_STATUS fts = FT_OK;
    if (mode == BIT_ASYNC)
         fts = FT_SetBitMode((FT_HANDLE)mHandle, 0xff, ASYNC_MODE);
    else
         fts = FT_SetBitMode((FT_HANDLE)mHandle, 0xff, SYNC_MODE);
    mLastError = FT_ERR_MSG[fts];
    return fts;
}

int FtdiDev::setBitMode(unsigned char mode1, unsigned char mode2)
{
    FT_STATUS fts = FT_OK;
    fts = FT_SetBitMode((FT_HANDLE)mHandle, mode1, mode2);
    mLastError = FT_ERR_MSG[fts];
    return fts;
}

int FtdiDev::setLatencyTimer(unsigned time)
{
    FT_STATUS fts = FT_OK;
    fts = FT_SetLatencyTimer((FT_HANDLE)mHandle, time);
    mLastError = FT_ERR_MSG[fts];
    return fts;
}

int FtdiDev::setBaudRate(int baudrate)
{
    FT_SetBaudRate((FT_HANDLE)mHandle, baudrate);
    FT_SetDataCharacteristics((FT_HANDLE)mHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
    if (mFlowControl)
        FT_SetFlowControl((FT_HANDLE)mHandle, FT_FLOW_RTS_CTS, 0x0, 0x0);
    mIsSerialPort = true;
    return 0;
}

int FtdiDev::cyclePort()
{
#ifdef WIN32
    FT_STATUS fts =  FT_CyclePort((FT_HANDLE)mHandle);
    mLastError = FT_ERR_MSG[fts];
    return fts;
#else
    mLastError = "cyclePort not supported on Linux/Mac";
    return 0;
#endif
}

int FtdiDev::inQueue()
{
    FT_STATUS fts = FT_OK;
    DWORD bytes;
    fts = FT_GetQueueStatus((FT_HANDLE)mHandle, &bytes);
    if (fts != FT_OK)
        mLastError = FT_ERR_MSG[fts];
    return (fts != FT_OK) ? -(int)fts : (int)bytes;
}

int FtdiDev::clearBuffers()
{
    DWORD received = 1;
    char buffer[1000];
    while (received > 0){
        if (inQueue() > 0){
            if (FT_Read((FT_HANDLE)mHandle, buffer, 1000, &received) != FT_OK)
                return -1;
        }else
            break;
    }
    return 0;
}

void FtdiDev::logBuff(char* buffer, size_t size, bool rx)
{
    FILE* f = fopen(mLogFile.c_str(), "a");
    fprintf(f, rx ? "<" : ">");
    for (size_t i = 0; i < size; i++)
        fprintf(f, "%02X ", (unsigned char)buffer[i]);
    fprintf(f, "\n");
    fclose(f);
}

int FtdiDev::send(char* buffer, size_t size, double timeout)
{
    DWORD bytesSent;
    char* buff = buffer;
    size_t sent = 0;
    size_t bytesToSend = size;

    if (!mLogFile.empty())
        logBuff(buffer, size, false);

    double startTime = getPreciseTime();
    do {
        FT_STATUS fts = FT_Write((FT_HANDLE)mHandle, buff, static_cast<DWORD>(std::min((size_t)FT_STEP, bytesToSend)), &bytesSent);
        if (fts != FT_OK) {
            mLastError = FT_ERR_MSG[fts];
            return -(int)fts;
        }

        if ((unsigned)bytesSent > bytesToSend){
            mLastError = "Device disconnected";
            return -1;
        }

        sent += (unsigned)bytesSent;
        bytesToSend -= (unsigned)bytesSent;
        buff += bytesSent;
        if (getPreciseTime() - startTime > timeout && bytesSent == 0) {
            mLastError = "Timeout";
            return -1;
        }

        if (bytesSent > 0)
            startTime = getPreciseTime();

    } while (bytesToSend);

    if (mOnDataFunc)
        mOnDataFunc(buffer, static_cast<unsigned>(sent), true, mOnDataUserData);
    return static_cast<int>(sent);
}

int FtdiDev::receive(char* buffer, size_t buffSize, size_t toReceive, double timeout, bool fixedTimeout)
{
    if (toReceive > buffSize)
        toReceive = buffSize;
    if (toReceive == 0){
        int bytes = inQueue();
        if (bytes <= 0) return bytes;
        toReceive = (unsigned)bytes;
    }

    FT_STATUS fts;
    double endTime = getPreciseTime() + timeout;
    DWORD received = 0;
    int receivedTotal = 0, bytesToTake = FT_STEP;
    double sleepTime = SLEEPTIME_WIN;

    while (getPreciseTime() < endTime) {
        bytesToTake = inQueue();
        if (bytesToTake < 0){
            return bytesToTake;
        }

        if (bytesToTake == 0){
            if (mFlowControl && !mIsSerialPort){
                FT_SetDtr((FT_HANDLE)mHandle);
                FT_ClrRts((FT_HANDLE)mHandle);
            }
            received = 0;
            if (sleepTime*2 < timeout/2.0)
                sleepTime *= 2;
        #ifndef WIN32
            //usleep(SLEEPTIME_UNIX);
        #endif
            continue;
        }

        if ((fts = FT_Read((FT_HANDLE)mHandle, buffer + receivedTotal, static_cast<DWORD>(std::min((size_t)bytesToTake, (toReceive - receivedTotal))), &received))){
            mLastError = FT_ERR_MSG[fts];
            if (!mLogFile.empty())
                logBuff(buffer, receivedTotal, true);
            return -(int)fts;
        }

        if (mFlowControl && !mIsSerialPort){
            FT_SetDtr((FT_HANDLE)mHandle);
            FT_ClrRts((FT_HANDLE)mHandle);
        }

        receivedTotal += received;
        if (receivedTotal == (int)toReceive) // all received or timeout
            break;

        if (received > 0 && !fixedTimeout)
            endTime = getPreciseTime() + timeout;

        #ifdef WIN32
            if (sleepTime > 16)
                sleepThreadF(sleepTime/1000);
        #else
            //usleep(SLEEPTIME_UNIX);
        #endif
    }

    // terminate buff with \0 if space
    if ((unsigned)receivedTotal < buffSize)
        buffer[receivedTotal] = 0;

    if (mOnDataFunc)
        mOnDataFunc(buffer, receivedTotal, false, mOnDataUserData);

    if (receivedTotal < (int)toReceive)
        mLastError = "Timeout";

    if (!mLogFile.empty())
        logBuff(buffer, receivedTotal, true);
    return receivedTotal;
}


int FtdiDev::receiveAll(char* buffer, size_t size, unsigned maxAttemps, double timeout)
{
    (void)timeout;
    FT_STATUS fts;
    char* pbuff = buffer;
    DWORD received = 0;
    size_t receivedTotal = 0, attemps = 0;
    while (attemps < maxAttemps){
        attemps++;
        size_t toReceive = inQueue();
        if (toReceive == 0){
            //sleepThreadF(0.001);
            if (mFlowControl && !mIsSerialPort){
                FT_SetDtr((FT_HANDLE)mHandle);
                FT_ClrRts((FT_HANDLE)mHandle);
            }
            continue;
        }

        if ((fts = FT_Read((FT_HANDLE)mHandle, pbuff, static_cast<DWORD>(std::min(toReceive, size-receivedTotal)),  &received)) != FT_OK){
            mLastError = FT_ERR_MSG[fts];
            if (!mLogFile.empty())
                logBuff(buffer, receivedTotal, true);
            return -(int)fts;
        }

        if (mFlowControl && !mIsSerialPort){
            FT_SetDtr((FT_HANDLE)mHandle);
            FT_ClrRts((FT_HANDLE)mHandle);
        }
        pbuff += received;
        receivedTotal += received;
        if (receivedTotal > 0)
            break;
    }

    if (!mLogFile.empty())
        logBuff(buffer, receivedTotal, true);
    return static_cast<int>(receivedTotal);
}

int FtdiDev::receiveAllUntilPattern(char* buffer, size_t size, char* pattern, size_t patSize, double timeout)
{
    FT_STATUS fts;
    DWORD received = 0;
    size_t toReceive = 0;
    char* pbuff = buffer;
    size_t receivedTotal = 0;

    double endTime = getPreciseTime() + timeout;
    while (getPreciseTime() < endTime){
        toReceive = inQueue();
        if (toReceive == 0){
            if (mFlowControl && !mIsSerialPort){
                FT_SetDtr((FT_HANDLE)mHandle);
                FT_ClrRts((FT_HANDLE)mHandle);
            }
            continue;
        }
        if ((fts = FT_Read((FT_HANDLE)mHandle, pbuff, static_cast<DWORD>(std::min(toReceive, size-receivedTotal)),  &received)) != FT_OK){
            mLastError = FT_ERR_MSG[fts];
            if (!mLogFile.empty())
                logBuff(buffer, receivedTotal, true);
            return -(int)fts;
        }

        if (mFlowControl && !mIsSerialPort){
            FT_SetDtr((FT_HANDLE)mHandle);
            FT_ClrRts((FT_HANDLE)mHandle);
        }

        pbuff += received;
        receivedTotal += received;
        if (findPattern(buffer, receivedTotal, pattern, patSize))
            break;
    }

    if (mOnDataFunc)
        mOnDataFunc(buffer, static_cast<unsigned>(receivedTotal), false, mOnDataUserData);
    if (!mLogFile.empty())
        logBuff(buffer, receivedTotal, true);

    return static_cast<int>(receivedTotal);
}

int FtdiDev::skipAllUntilPattern(char* pattern, size_t patSize, double timeout)
{
    FT_STATUS fts;
    DWORD received = 0;
    unsigned toReceive = 0;
    char* buff = new char[4096];
    unsigned size = 4096;
    unsigned receivedTotal = 0;
    unsigned patIndex = 0;

    double endTime = getPreciseTime() + timeout;
    while (getPreciseTime() < endTime){
        toReceive = inQueue();
        if (toReceive == 0){
            if (mFlowControl && !mIsSerialPort){
                FT_SetDtr((FT_HANDLE)mHandle);
                FT_ClrRts((FT_HANDLE)mHandle);
            }
            continue;
        }

        if ((fts = FT_Read((FT_HANDLE)mHandle, buff, std::min(toReceive, size),  &received)) != FT_OK){
            mLastError = FT_ERR_MSG[fts];
            delete[] buff;
            return -(int)fts;
        }

        if (mFlowControl && !mIsSerialPort){
            FT_SetDtr((FT_HANDLE)mHandle);
            FT_ClrRts((FT_HANDLE)mHandle);
        }
        receivedTotal += received;

        unsigned idx = 0;
        while (idx < received){
            if (buff[idx] == pattern[patIndex]){
                if (patIndex == patSize-1){
                    delete[] buff;
                    return receivedTotal;
                }
                patIndex++;
            }else{
                if (patIndex > 0){
                    patIndex = 0;
                    continue;
                }
                patIndex = 0;
            }
            idx++;
        }

    }
    delete[] buff;
    mLastError = "Timeout";
    return receivedTotal == 0 ? -1 : -static_cast<int>(receivedTotal);
}

int FtdiDev::getLine(std::string &line, char separ, double timeout)
{
    FT_STATUS fts;
    DWORD received = 0;
    unsigned toReceive = 0;
    char buff[1024];

    // if there is line left in mExtraData, return it
    size_t pos = mExtraData.find(separ);
    if (pos != std::string::npos){
        line = pos > 0 ?  mExtraData.substr(0, pos) : "";
        mExtraData = mExtraData.substr(pos+1);
        return 0;
    }

    line = mExtraData;
    mExtraData.clear();
    double endTime = getPreciseTime() + timeout;
    while (getPreciseTime() < endTime){
        toReceive = inQueue();
        if (toReceive == 0){
            if (mFlowControl && !mIsSerialPort){
                FT_SetDtr((FT_HANDLE)mHandle);
                FT_ClrRts((FT_HANDLE)mHandle);
            }
            continue;
        }

        if ((fts = FT_Read((FT_HANDLE)mHandle, buff, toReceive<1023?toReceive:1023,  &received)) != FT_OK){
            mLastError = FT_ERR_MSG[fts];
            return -(int)fts;
        }
        buff[received] = '\0';

        if (mFlowControl && !mIsSerialPort){
            FT_SetDtr((FT_HANDLE)mHandle);
            FT_ClrRts((FT_HANDLE)mHandle);
        }

        for (unsigned i = 0; i < received; i++){
            if (buff[i] == separ){
                buff[i] = '\0';
                line += mExtraData + std::string(buff); // copy line to line
                if (i < received-1)
                    mExtraData = std::string(buff+i+1); // copy rest of the data to extra data
                return 0;
            }
        }

        // no new line found, copy everythin to line, and get new data
        line += std::string(buff);
    }
    return -1;
}


int FtdiDev::rename(const char* name)
{
    return FT_EE_UAWrite((FT_HANDLE)mHandle, (UCHAR*)name, static_cast<DWORD>(strlen(name)));
}

std::string FtdiDev::readName()
{
    DWORD bytesRead;
    char buff[64];
    memset(buff, 0, 64);
    FT_STATUS ftStatus = FT_EE_UARead((FT_HANDLE)mHandle, (UCHAR*)buff, 64, &bytesRead);
    return (ftStatus == FT_OK && bytesRead > 0 && strlen(buff) > 0) ? buff : "";
}

std::string FtdiDev::description()
{
    FT_PROGRAM_DATA ftData;
    char ManufacturerBuf[32];
    char ManufacturerIdBuf[16];
    char DescriptionBuf[64];
    char SerialNumberBuf[16];
    ftData.Signature1 = 0x00000000;
    ftData.Signature2 = 0xffffffff;
    ftData.Version = 0x00000005; // EEPROM structure with FT232H extensions
    ftData.Manufacturer = ManufacturerBuf;
    ftData.ManufacturerId = ManufacturerIdBuf;
    ftData.Description = DescriptionBuf;
    ftData.SerialNumber = SerialNumberBuf;

    FT_EE_Read((FT_HANDLE)mHandle, &ftData);
    return std::string(ftData.Description);
}

int FtdiDev::setDescription(std::string newName)
{
    FT_PROGRAM_DATA ftData;
    char ManufacturerBuf[32];
    char ManufacturerIdBuf[16];
    char DescriptionBuf[64];
    char SerialNumberBuf[16];
    ftData.Signature1 = 0x00000000;
    ftData.Signature2 = 0xffffffff;
    ftData.Version = 0x00000005; // EEPROM structure with FT232H extensions
    ftData.Manufacturer = ManufacturerBuf;
    ftData.ManufacturerId = ManufacturerIdBuf;
    ftData.Description = DescriptionBuf;
    ftData.SerialNumber = SerialNumberBuf;
    FT_STATUS ftStatus = FT_EE_Read((FT_HANDLE)mHandle, &ftData);
    strcpy(ftData.Description, newName.c_str());
    ftStatus = FT_EE_Program ((FT_HANDLE)mHandle, &ftData );
    return ftStatus;
}


#else
//########################################################################################################################
//                                              LIB FTDI
//########################################################################################################################

#include "ftdi.h"
#define FT_HANDLE struct ftdi_context

#define FT_STEP          4096
#define SLEEPTIME_WIN    1     // in ms
#define SLEEPTIME_UNIX   1000  // in us
#define READ_TIMEOUT     500   // maximal read timeout of FTDI
#define WRITE_TIMEOUT    500   // maximal write timeout of FTDI


FtdiDev::FtdiDev(std::string nameOrSerial, bool isSerial)
    : mHandle(NULL)
    , mNameOrSerial(nameOrSerial)
    , mIsSerial(isSerial)
    , mFlowControl(false)
    , mLastError("")
    , mExtraData("")
    , mOnDataFunc(NULL)
    , mOnDataUserData(NULL)
{
    mHandle = new struct ftdi_context;
    (void)mIsSerial;
    (void)mIsSerialPort;
}

FtdiDev::~FtdiDev()
{
    delete (struct ftdi_context*)mHandle;
}

int FtdiDev::listDevicesByName(const char* filters[], size_t size, std::vector<FtdiDevInfo> &devices, bool ignoreB, bool getSerial)
{
    int rc = 0;
    struct ftdi_context ftdic;
    struct ftdi_device_list *devlist, *curdev;
    char manufacturer[129], description[129], serial[129];
    memset(description, 0, 129);
    memset(manufacturer, 0, 129);
    memset(serial, 0, 129);

    if (ftdi_init(&ftdic) < 0)
        return -1;

    addVidPid(0x04036010);
    addVidPid(0x04036001);
    addVidPid(0x04036014);
    addVidPid(0x04036015);
    mNameToVidPid.clear();

    for (unsigned vp = 0; vp < mVidPids.size(); vp++) {
        unsigned vid = (mVidPids[vp] >> 16) & 0xFFFF;
        unsigned pid = mVidPids[vp] & 0xFFFF;

        if ((rc = ftdi_usb_find_all(&ftdic, &devlist, vid, pid)) < 0) {
            ftdi_deinit(&ftdic);
            return -2;
        }

        for (curdev = devlist; curdev != NULL; ) {
            if ((rc = ftdi_usb_get_strings(&ftdic, curdev->dev, manufacturer, 128, description, 128, serial, 128)) < 0) {
                ftdi_list_free(&devlist);
                ftdi_deinit(&ftdic);
                return -3;
            }

            if (ignoreB && strlen(description) > 1 && description[strlen(description) - 1] == 'B')
                continue;

            if (size == 0){
                devices.push_back(FtdiDevInfo(std::string(description), getSerial ? std::string(serial): "", mVidPids[vp]));
                mNameToVidPid[std::string(description)] = mVidPids[vp];
            }

            for (unsigned j = 0; j < size; j++) {
                if (strncmp(description, filters[j], strlen(filters[j])) == 0) {
                    devices.push_back(FtdiDevInfo(std::string(description), getSerial ? std::string(serial) : "", mVidPids[vp]));
                    mNameToVidPid[std::string(description)] = mVidPids[vp];
                    break;
                }
            }
            curdev = curdev->next;
        }
        ftdi_list_free(&devlist);
    }

    ftdi_deinit(&ftdic);
    return 0;
}

int FtdiDev::listDevicesByNameFast(const char* filters[], size_t size, std::vector<FtdiDevInfo> &devices, bool ignoreB, bool getSerial)
{
    return listDevicesByName(filters, size, devices, ignoreB, getSerial);
}

int FtdiDev::listDevicesByVidpid(unsigned long vidpids[], size_t size, std::vector<FtdiDevInfo> &devices, bool ignoreB)
{
    int rc = 0, devcount = 0;
    struct ftdi_context ftdic;
    struct ftdi_device_list *devlist, *curdev;
    char manufacturer[129], description[129], serial[129];
    memset(description, 0, 129);
    memset(manufacturer, 0, 129);
    memset(serial, 0, 129);

    if (ftdi_init(&ftdic) < 0)
        return -1;

    mNameToVidPid.clear();
    for (unsigned i = 0; i < size; i++) {
        unsigned vid = (vidpids[i] >> 16) & 0xFFFF;
        unsigned pid = vidpids[i] & 0xFFFF;

        if ((devcount = ftdi_usb_find_all(&ftdic, &devlist, vid, pid)) < 0){
            ftdi_deinit(&ftdic);
            return -2;
        }

        if (devcount == 0){
            ftdi_list_free(&devlist);
            continue;
        }

        for (curdev = devlist; curdev != NULL; ) {
            if ((rc = ftdi_usb_get_strings(&ftdic, curdev->dev, manufacturer, 128, description, 128, serial, 128)) < 0){
                ftdi_deinit(&ftdic);
                return -3;
            }
            if (ignoreB && strlen(description) > 1 && description[strlen(description) - 1] == 'B') 
                continue;
            devices.push_back(FtdiDevInfo(std::string(description), std::string(serial), (unsigned)vidpids[i]));
            mNameToVidPid[std::string(description)] = (unsigned)vidpids[i];
            curdev = curdev->next;
        }

        ftdi_list_free(&devlist);
    }

    ftdi_deinit(&ftdic);
    return 0;
}

int FtdiDev::openDevice(bool flowControl, unsigned vidpid, unsigned intf)
{
    mFlowControl = flowControl;
    if (ftdi_init((FT_HANDLE*)mHandle) < 0) {
        mLastError = "Cannot initialize ftdi.";
        return -1;
    }

    if (intf > 0)
        ftdi_set_interface((FT_HANDLE*)mHandle, (enum ftdi_interface)intf);

    unsigned vid = 0x403;
    unsigned pid = 0x6010;
    if (vidpid != 0){
        vid = (vidpid >> 16) & 0xFFFF;
        pid = vidpid & 0xFFFF;
    }
    if (mNameToVidPid.find(mNameOrSerial) != mNameToVidPid.end()){
        unsigned vidPid = mNameToVidPid[mNameOrSerial];
        vid = (vidPid >> 16) & 0xFFFF;
        pid = vidPid & 0xFFFF;
    }

    int rc = ftdi_usb_open_desc((FT_HANDLE*)mHandle, vid, pid, mNameOrSerial.c_str(), NULL);
    if (rc){
        mLastError = ftdi_get_error_string((FT_HANDLE*)mHandle);
        return -1;
    }

    ftdi_usb_reset((FT_HANDLE*)mHandle);
    ftdi_usb_purge_buffers((FT_HANDLE*)mHandle);

    ftdi_set_bitmode((FT_HANDLE*)mHandle, 0xFF, BITMODE_RESET);
    if (flowControl)
        ftdi_setflowctrl((FT_HANDLE*)mHandle, SIO_RTS_CTS_HS);
    else
        ftdi_setflowctrl((FT_HANDLE*)mHandle, SIO_DISABLE_FLOW_CTRL);

    ftdi_read_data_set_chunksize((FT_HANDLE*)mHandle, 0x10000);
    ftdi_write_data_set_chunksize((FT_HANDLE*)mHandle, 0x10000);
    ftdi_set_latency_timer((FT_HANDLE*)mHandle, 2);
    return 0;
}

int FtdiDev::closeDevice()
{
    if (((FT_HANDLE*)mHandle)->usb_dev == 0)
        return 0;
    int rc = ftdi_usb_close((FT_HANDLE*)mHandle);
    ftdi_deinit((FT_HANDLE*)mHandle);
    ((FT_HANDLE*)mHandle)->usb_dev = 0;
    return rc;
}

bool FtdiDev::isConnected()
{
    if (((FT_HANDLE*)mHandle)->usb_dev == 0)
        return false;
    unsigned char latency;
    return ftdi_get_latency_timer((FT_HANDLE*)mHandle, &latency) == 0;
}

#define ASYNC_MODE 0x01
#define SYNC_MODE  0x40
int FtdiDev::setBitMode(FtdiBitMode mode)
{
    ftdi_usb_reset((FT_HANDLE*)mHandle);
    ftdi_usb_purge_buffers((FT_HANDLE*)mHandle);

    ftdi_set_bitmode((FT_HANDLE*)mHandle, 0xFF, BITMODE_RESET);
    ftdi_set_bitmode((FT_HANDLE*)mHandle, 0xFF, mode == BIT_ASYNC ? ASYNC_MODE : SYNC_MODE);
    if (mFlowControl)
        ftdi_setflowctrl((FT_HANDLE*)mHandle, SIO_RTS_CTS_HS);
    else
        ftdi_setflowctrl((FT_HANDLE*)mHandle, SIO_DISABLE_FLOW_CTRL);
    ftdi_read_data_set_chunksize((FT_HANDLE*)mHandle, 0x10000);
    ftdi_write_data_set_chunksize((FT_HANDLE*)mHandle, 0x10000);
    ftdi_set_latency_timer((FT_HANDLE*)mHandle, 2);
    return 0;
}

int FtdiDev::setBitMode(unsigned char mode1, unsigned char mode2)
{
    ftdi_set_bitmode((FT_HANDLE*)mHandle, mode1, mode2);
    return 0;
}

int FtdiDev::setBaudRate(int baudrate)
{
    //ftdi_set_bitmode((FT_HANDLE*)mHandle, 1, BITMODE_RESET);
    ftdi_set_line_property((FT_HANDLE*)mHandle, BITS_8, STOP_BIT_1, NONE);
    ftdi_set_baudrate((FT_HANDLE*)mHandle, baudrate);
    return 0;
}

int FtdiDev::setLatencyTimer(unsigned time)
{
    ftdi_set_latency_timer((FT_HANDLE*)mHandle, time);
    return 0;
}

int FtdiDev::cyclePort()
{
    mLastError = "cyclePort not supported on Linux/Mac";
    return 0;
}

int FtdiDev::inQueue()
{
    mLastError = "Not supported in libFTDI";
    return -1;
}

int FtdiDev::clearBuffers()
{
    return ftdi_usb_purge_buffers((FT_HANDLE*)mHandle);
}

int FtdiDev::send(char* buffer, size_t size, double timeout)
{
    int bytesSent;
    char* buff = buffer;
    unsigned sent = 0;
    size_t bytesToSend = size;
    double startTime = getPreciseTime();

    if (!mLogFile.empty())
        logBuff(buffer, size, false);

    do {
        bytesSent = ftdi_write_data((FT_HANDLE*)mHandle, (unsigned char*)buff, (int)bytesToSend);
        if (bytesSent < 0) {
            mLastError = ftdi_get_error_string((FT_HANDLE*)mHandle);
            return bytesSent;
        }

        if ((unsigned)bytesSent > bytesToSend){
            mLastError = "Device disconnected";
            return -1;
        }

        sent += (unsigned)bytesSent;
        bytesToSend -= (unsigned)bytesSent;
        buff += bytesSent;
        if (getPreciseTime() - startTime > timeout && bytesSent == 0) {
            mLastError = "Timeout";
            return -1;
        }

        if (bytesSent > 0)
            startTime = getPreciseTime();

    } while (bytesToSend);

    if (mOnDataFunc)
        mOnDataFunc(buffer, sent, true, mOnDataUserData);
    return sent;
}

int FtdiDev::receive(char* buffer, size_t buffSize, size_t toReceive, double timeout, bool fixedTimeout)
{
    if (toReceive > buffSize)
        toReceive = buffSize;
    if (toReceive == 0){
        int bytes = inQueue();
        if (bytes <= 0) return bytes;
        toReceive = (unsigned)bytes;
    }

    double endTime = getPreciseTime() + timeout;
    int received = 0;
    int receivedTotal = 0;

    while (getPreciseTime() < endTime) {

        received = ftdi_read_data((FT_HANDLE*)mHandle, (unsigned char*)(buffer + receivedTotal),  (int)(toReceive - receivedTotal));
        if (received < 0){
            mLastError = ftdi_get_error_string((FT_HANDLE*)mHandle);
            if (!mLogFile.empty())
                logBuff(buffer, receivedTotal, true);
            return received;
        }

        receivedTotal += received;
        if (receivedTotal == (int)toReceive) // all received or timeout
            break;

        if (received > 0)
            endTime = getPreciseTime() + timeout;

        #ifdef WIN32
            if (sleepTime > 16)
                sleepThreadF(sleepTime/1000);
        #else
            //usleep(SLEEPTIME_UNIX);
        #endif
    }

    // terminate buff with \0 if space
    if ((unsigned)receivedTotal < buffSize && !fixedTimeout)
        buffer[receivedTotal] = 0;

    if (mOnDataFunc)
        mOnDataFunc(buffer, receivedTotal, false, mOnDataUserData);

    if (receivedTotal < (int)toReceive)
        mLastError = "Timeout";

    if (!mLogFile.empty())
        logBuff(buffer, receivedTotal, true);
    return receivedTotal;
}

int FtdiDev::receiveAll(char* buffer, size_t size, unsigned maxAttemps, double timeout)
{
    char* pbuff = buffer;
    int received = 0;
    unsigned receivedTotal = 0, attemps = 0;
    double startTime = getPreciseTime();
    while (attemps < maxAttemps){
        attemps++;
        received = ftdi_read_data((FT_HANDLE*)mHandle, (unsigned char*)pbuff, std::min((int)65536, (int)(size - receivedTotal)));
        if (received < 0){
            mLastError = ftdi_get_error_string((FT_HANDLE*)mHandle);
            return received;
        }

        pbuff += received;
        receivedTotal += received;

        if (timeout > 0 && getPreciseTime() - startTime > timeout)
            break;

        if (size - receivedTotal == 0)
            break;

        if (received == 0 && receivedTotal > 0)
            break;
    }
    return receivedTotal;
}

int FtdiDev::receiveAllUntilPattern(char* buffer, size_t size, char* pattern, size_t patSize, double timeout)
{
    int received = 0;
    char* pbuff = buffer;
    size_t receivedTotal = 0;
    double endTime = getPreciseTime() + timeout;

    while (getPreciseTime() < endTime){
        received = ftdi_read_data((FT_HANDLE*)mHandle, (unsigned char*)pbuff, (int)(size - receivedTotal));
        if (received < 0){
            mLastError = ftdi_get_error_string((FT_HANDLE*)mHandle);
            if (!mLogFile.empty())
                logBuff(buffer, receivedTotal, true);
            return received;
        }

        pbuff += received;
        receivedTotal += received;
        if (findPattern(buffer, receivedTotal, pattern, patSize))
            break;
    }

    if (mOnDataFunc)
        mOnDataFunc(buffer, static_cast<unsigned>(receivedTotal), false, mOnDataUserData);

    if (!mLogFile.empty())
        logBuff(buffer, receivedTotal, true);
    return static_cast<int>(receivedTotal);
}

int FtdiDev::skipAllUntilPattern(char* pattern, size_t patSize, double timeout)
{
    int received = 0;
    char* buff = new char[4096];
    unsigned size = 4096;
    unsigned receivedTotal = 0;
    unsigned patIndex = 0;

    double endTime = getPreciseTime() + timeout;
    while (getPreciseTime() < endTime){

        received = ftdi_read_data((FT_HANDLE*)mHandle, (unsigned char*)buff, size);
        if (received < 0){
            mLastError = ftdi_get_error_string((FT_HANDLE*)mHandle);
            delete[] buff;
            return received;
        }

        receivedTotal += received;
        int idx = 0;
        while (idx < received){
            if (buff[idx] == pattern[patIndex]){
                if (patIndex == patSize-1){
                    delete[] buff;
                    return receivedTotal;
                }
                patIndex++;
            }else{
                if (patIndex > 0){
                    patIndex = 0;
                    continue;
                }
                patIndex = 0;
            }
            idx++;
        }

    }
    delete[] buff;
    mLastError = "Timeout";
    return receivedTotal == 0 ? -1 : -static_cast<int>(receivedTotal);
}

int FtdiDev::getLine(std::string &line, char separ, double timeout)
{
    int received = 0;
    char buff[1024];

    // if there is line left in mExtraData, return it
    size_t pos = mExtraData.find(separ);
    if (pos != std::string::npos){
        line = pos > 0 ?  mExtraData.substr(0, pos) : "";
        mExtraData = mExtraData.substr(pos+1);
        return 0;
    }

    line = mExtraData;
    mExtraData.clear();
    double endTime = getPreciseTime() + timeout;
    while (getPreciseTime() < endTime){

        received = ftdi_read_data((FT_HANDLE*)mHandle, (unsigned char*)buff, 1023);
        if (received < 0){
            mLastError = ftdi_get_error_string((FT_HANDLE*)mHandle);
            return received;
        }
        buff[received] = '\0';

        for (int i = 0; i < received; i++){
            if (buff[i] == separ){
                buff[i] = '\0';
                line += mExtraData + std::string(buff); // copy line to line
                if (i < received-1)
                    mExtraData = std::string(buff+i+1); // copy rest of the data to extra data
                return 0;
            }
        }
        // no new line found, copy everythin to line, and get new data
        line += std::string(buff);
    }
    return -1;
}

int FtdiDev::addVidPid(unsigned vid, unsigned pid)
{
    addVidPid(((vid << 16) & 0xFFFF0000) | (pid & 0xFFFF));
    return 0;
}

int FtdiDev::addVidPid(unsigned vidpid)
{
    for (size_t i = 0; i < mVidPids.size(); i++){
        if (mVidPids[i] == vidpid)
            return 0;
    }
    mVidPids.push_back(vidpid);
    return 0;
}

int FtdiDev::rename(const char* name)
{
    (void)name;
    //return FT_EE_UAWrite((FT_HANDLE)mHandle, (UCHAR*)name, strlen(name));
    mLastError = "Not supported in libFTDI";
    return -1;
}

void FtdiDev::logBuff(char* buffer, size_t size, bool rx)
{
    FILE* f = fopen(mLogFile.c_str(), "a");
    fprintf(f, rx ? "<" : ">");
    for (size_t i = 0; i < size; i++)
        fprintf(f, "%02X ", (unsigned char)buffer[i]);
    fprintf(f, "\n");
    fclose(f);
}

std::string FtdiDev::readName()
{
    mLastError = "Not supported in libFTDI";
    return "";
}

#endif




//########################################################################################################################
//                                              UTILITIES
//########################################################################################################################

double getPreciseTime() {
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

inline void sleepThreadF(double seconds)
{
#ifdef WIN32
    DWORD milliseconds = (DWORD)(seconds * 1000);
    Sleep(milliseconds);
#else
    usleep((__useconds_t)(seconds * 1000000));
#endif
}



