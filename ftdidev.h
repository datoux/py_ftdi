/**
 * Copyright (C) 2023 Daniel Turecek
 *
 * @file      ftdidev.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2023-02-05
 *
 */
#ifndef FTDIDEV_H
#define FTDIDEV_H
#include <string>
#include <vector>
#include <map>
typedef void (*FtdiOnDataType)(char* data, unsigned size, bool tx, void* userpar);
typedef void* FtdiHandle;

struct FtdiDevInfo
{
    FtdiDevInfo(std::string _name, std::string _serial, unsigned _vidpid)
        : name(_name), serial(_serial), vidpid(_vidpid) {}
    std::string name;
    std::string serial;
    unsigned vidpid;
};


class FtdiDev
{
public:
    enum FtdiBitMode {BIT_ASYNC, BIT_SYNC};

public:
    static int addVidPid(unsigned vid, unsigned pid);
    static int addVidPid(unsigned vidpid);
    static int listDevicesByName(const char* filters[], size_t size, std::vector<FtdiDevInfo> &devices, bool ignoreB = true, bool getSerial = false);
    static int listDevicesByNameFast(const char* filters[], size_t size, std::vector<FtdiDevInfo> &devices, bool ignoreB = true, bool getSerial = false);
    static int listDevicesByVidpid(unsigned long vidpids[], size_t size, std::vector<FtdiDevInfo> &devices, bool ignoreB = true);

public:
    FtdiDev(std::string nameOrSerial, bool isSerial=false);
    virtual ~FtdiDev();

public:
    int openDevice(bool flowControl = true, unsigned vidpid = 0);
    int closeDevice();
    bool isConnected();
    int setBitMode(FtdiBitMode mode);
    int setBitMode(unsigned char mode1, unsigned char mode2);
    int setLatencyTimer(unsigned time);
    int setBaudRate(int baudrate);
    int cyclePort();
    int inQueue();
    int clearBuffers();
    int send(char* buffer, size_t size, double timeout = 2);
    int receive(char* buffer, size_t buffSize, size_t toReceive, double timeout = 2, bool fixedTimeout=false);
    int receiveAll(char* buffer, size_t size, unsigned maxAttemps, double timeout = -1);
    int receiveAllUntilPattern(char* buffer, size_t size, char* pattern, size_t patSize, double timeout = 2);
    int skipAllUntilPattern(char* pattern, size_t patSize, double timeout = 2);
    int getLine(std::string &line, char separ='\n', double timeout = 2);
    bool lineAvailable(char separ = '\n') { size_t pos = mExtraData.find(separ); return pos != std::string::npos; }
    int rename(const char* name);
    std::string readName();
    const char* getLastError() { return mLastError.c_str();}
    void setOnDataFunc(FtdiOnDataType func, void* userData) { mOnDataFunc = func; mOnDataUserData = userData; }
    FtdiHandle handle() const { return mHandle; }
    std::string description();
    int setDescription(std::string newName);
    void enableLogFile(const char* logFileName) { mLogFile = logFileName; }
    void setNameOrSerial(const char* nameOrSerial) { mNameOrSerial = nameOrSerial; }

private:
    void logBuff(char* buffer, size_t size, bool rx);
    inline bool findPattern(char* buffer, size_t buffSize, char* pattern, size_t patternSize) {
        if (buffSize < patternSize) return false;
        for (size_t i = buffSize - patternSize; i < buffSize; i++)
            if (buffer[i] != pattern[i - buffSize + patternSize])
                return false;
        return true;
    }

private:
    FtdiHandle mHandle;
    std::string mNameOrSerial;
    bool mIsSerial;
    bool mFlowControl;
    bool mIsSerialPort;
    std::string mLastError;
    std::string mExtraData;
    std::string mLogFile;
    static std::vector<unsigned> mVidPids;
    static std::map<std::string, unsigned> mNameToVidPid;
    FtdiOnDataType mOnDataFunc;
    void* mOnDataUserData;
};



#endif /* end of include guard: FTDIDEV_H */


