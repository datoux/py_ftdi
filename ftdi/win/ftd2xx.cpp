/**
 * Copyright (C) 2018 Daniel Turecek
 *
 * @file      ftdiloader.cpp
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2018-05-27
 *
 */
#include "windows.h"
#include "ftd2xx.h"


typedef FT_STATUS (WINAPI*FT_Open_Func)( int deviceNumber, FT_HANDLE *pHandle);
typedef FT_STATUS (WINAPI*FT_OpenEx_Func)( PVOID pArg1, DWORD Flags, FT_HANDLE *pHandle);
typedef FT_STATUS (WINAPI*FT_ListDevices_Func)( PVOID pArg1, PVOID pArg2, DWORD Flags);
typedef FT_STATUS (WINAPI*FT_Close_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_Read_Func)( FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD dwBytesToRead, LPDWORD lpBytesReturned);
typedef FT_STATUS (WINAPI*FT_Write_Func)( FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD dwBytesToWrite, LPDWORD lpBytesWritten);
typedef FT_STATUS (WINAPI*FT_IoCtl_Func)( FT_HANDLE ftHandle, DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
typedef FT_STATUS (WINAPI*FT_SetBaudRate_Func)( FT_HANDLE ftHandle, ULONG BaudRate);
typedef FT_STATUS (WINAPI*FT_SetDivisor_Func)( FT_HANDLE ftHandle, USHORT Divisor);
typedef FT_STATUS (WINAPI*FT_SetDataCharacteristics_Func)( FT_HANDLE ftHandle, UCHAR WordLength, UCHAR StopBits, UCHAR Parity);
typedef FT_STATUS (WINAPI*FT_SetFlowControl_Func)( FT_HANDLE ftHandle, USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar);
typedef FT_STATUS (WINAPI*FT_ResetDevice_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_SetDtr_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_ClrDtr_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_SetRts_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_ClrRts_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_GetModemStatus_Func)( FT_HANDLE ftHandle, ULONG *pModemStatus);
typedef FT_STATUS (WINAPI*FT_SetChars_Func)( FT_HANDLE ftHandle, UCHAR EventChar, UCHAR EventCharEnabled, UCHAR ErrorChar, UCHAR ErrorCharEnabled);
typedef FT_STATUS (WINAPI*FT_Purge_Func)( FT_HANDLE ftHandle, ULONG Mask);
typedef FT_STATUS (WINAPI*FT_SetTimeouts_Func)( FT_HANDLE ftHandle, ULONG ReadTimeout, ULONG WriteTimeout);
typedef FT_STATUS (WINAPI*FT_GetQueueStatus_Func)( FT_HANDLE ftHandle, DWORD *dwRxBytes);
typedef FT_STATUS (WINAPI*FT_SetEventNotification_Func)( FT_HANDLE ftHandle, DWORD Mask, PVOID Param);
typedef FT_STATUS (WINAPI*FT_GetStatus_Func)( FT_HANDLE ftHandle, DWORD *dwRxBytes, DWORD *dwTxBytes, DWORD *dwEventDWord);
typedef FT_STATUS (WINAPI*FT_SetBreakOn_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_SetBreakOff_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_SetWaitMask_Func)( FT_HANDLE ftHandle, DWORD Mask);
typedef FT_STATUS (WINAPI*FT_WaitOnMask_Func)( FT_HANDLE ftHandle, DWORD *Mask);
typedef FT_STATUS (WINAPI*FT_GetEventStatus_Func)( FT_HANDLE ftHandle, DWORD *dwEventDWord);
typedef FT_STATUS (WINAPI*FT_ReadEE_Func)( FT_HANDLE ftHandle, DWORD dwWordOffset, LPWORD lpwValue);
typedef FT_STATUS (WINAPI*FT_WriteEE_Func)( FT_HANDLE ftHandle, DWORD dwWordOffset, WORD wValue);
typedef FT_STATUS (WINAPI*FT_EraseEE_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_EE_Program_Func)( FT_HANDLE ftHandle, PFT_PROGRAM_DATA pData);
typedef FT_STATUS (WINAPI*FT_EE_ProgramEx_Func)( FT_HANDLE ftHandle, PFT_PROGRAM_DATA pData, char *Manufacturer, char *ManufacturerId, char *Description, char *SerialNumber);
typedef FT_STATUS (WINAPI*FT_EE_Read_Func)( FT_HANDLE ftHandle, PFT_PROGRAM_DATA pData);
typedef FT_STATUS (WINAPI*FT_EE_ReadEx_Func)( FT_HANDLE ftHandle, PFT_PROGRAM_DATA pData, char *Manufacturer, char *ManufacturerId, char *Description, char *SerialNumber);
typedef FT_STATUS (WINAPI*FT_EE_UASize_Func)( FT_HANDLE ftHandle, LPDWORD lpdwSize);
typedef FT_STATUS (WINAPI*FT_EE_UAWrite_Func)( FT_HANDLE ftHandle, PUCHAR pucData, DWORD dwDataLen);
typedef FT_STATUS (WINAPI*FT_EE_UARead_Func)( FT_HANDLE ftHandle, PUCHAR pucData, DWORD dwDataLen, LPDWORD lpdwBytesRead);
typedef FT_STATUS (WINAPI*FT_SetLatencyTimer_Func)( FT_HANDLE ftHandle, UCHAR ucLatency);
typedef FT_STATUS (WINAPI*FT_GetLatencyTimer_Func)( FT_HANDLE ftHandle, PUCHAR pucLatency);
typedef FT_STATUS (WINAPI*FT_SetBitMode_Func)( FT_HANDLE ftHandle, UCHAR ucMask, UCHAR ucEnable);
typedef FT_STATUS (WINAPI*FT_GetBitMode_Func)( FT_HANDLE ftHandle, PUCHAR pucMode);
typedef FT_STATUS (WINAPI*FT_SetUSBParameters_Func)( FT_HANDLE ftHandle, ULONG ulInTransferSize, ULONG ulOutTransferSize);
typedef FT_STATUS (WINAPI*FT_SetDeadmanTimeout_Func)( FT_HANDLE ftHandle, ULONG ulDeadmanTimeout);
typedef FT_STATUS (WINAPI*FT_GetDeviceInfo_Func)( FT_HANDLE ftHandle, FT_DEVICE *lpftDevice, LPDWORD lpdwID, PCHAR SerialNumber, PCHAR Description, LPVOID Dummy);
typedef FT_STATUS (WINAPI*FT_StopInTask_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_RestartInTask_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_SetResetPipeRetryCount_Func)( FT_HANDLE ftHandle, DWORD dwCount);
typedef FT_STATUS (WINAPI*FT_ResetPort_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_CyclePort_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_GetDriverVersion_Func)( FT_HANDLE ftHandle, LPDWORD lpdwVersion);
typedef FT_STATUS (WINAPI*FT_GetLibraryVersion_Func)( LPDWORD lpdwVersion);
typedef FT_STATUS (WINAPI*FT_Rescan_Func)( void);
typedef FT_STATUS (WINAPI*FT_Reload_Func)( WORD wVid, WORD wPid);
typedef FT_STATUS (WINAPI*FT_GetComPortNumber_Func)( FT_HANDLE ftHandle, LPLONG  lpdwComPortNumber);
typedef FT_STATUS (WINAPI*FT_W32_ClearCommBreak_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_W32_ClearCommError_Func)( FT_HANDLE ftHandle, LPDWORD lpdwErrors, LPFTCOMSTAT lpftComstat);
typedef FT_STATUS (WINAPI*FT_W32_EscapeCommFunction_Func)( FT_HANDLE ftHandle, DWORD dwFunc);
typedef FT_STATUS (WINAPI*FT_W32_GetCommModemStatus_Func)( FT_HANDLE ftHandle, LPDWORD lpdwModemStatus);
typedef FT_STATUS (WINAPI*FT_W32_GetCommState_Func)( FT_HANDLE ftHandle, LPFTDCB lpftDcb);
typedef FT_STATUS (WINAPI*FT_W32_GetCommTimeouts_Func)( FT_HANDLE ftHandle, FTTIMEOUTS *pTimeouts);
typedef FT_STATUS (WINAPI*FT_W32_PurgeComm_Func)( FT_HANDLE ftHandle, DWORD dwMask);
typedef FT_STATUS (WINAPI*FT_W32_SetCommBreak_Func)( FT_HANDLE ftHandle);
typedef FT_STATUS (WINAPI*FT_W32_SetCommMask_Func)( FT_HANDLE ftHandle, ULONG ulEventMask);
typedef FT_STATUS (WINAPI*FT_W32_GetCommMask_Func)( FT_HANDLE ftHandle, LPDWORD lpdwEventMask);
typedef FT_STATUS (WINAPI*FT_W32_SetCommState_Func)( FT_HANDLE ftHandle, LPFTDCB lpftDcb);
typedef FT_STATUS (WINAPI*FT_W32_SetCommTimeouts_Func)( FT_HANDLE ftHandle, FTTIMEOUTS *pTimeouts);
typedef FT_STATUS (WINAPI*FT_W32_SetupComm_Func)( FT_HANDLE ftHandle, DWORD dwReadBufferSize, DWORD dwWriteBufferSize);
typedef FT_STATUS (WINAPI*FT_W32_WaitCommEvent_Func)( FT_HANDLE ftHandle, PULONG pulEvent, LPOVERLAPPED lpOverlapped);
typedef FT_STATUS (WINAPI*FT_EE_ReadConfig_Func)( FT_HANDLE ftHandle, UCHAR ucAddress, PUCHAR pucValue);
typedef FT_STATUS (WINAPI*FT_EE_WriteConfig_Func)( FT_HANDLE ftHandle, UCHAR ucAddress, UCHAR ucValue);
typedef FT_STATUS (WINAPI*FT_EE_ReadECC_Func)( FT_HANDLE ftHandle, UCHAR ucOption, LPWORD lpwValue);
typedef FT_STATUS (WINAPI*FT_GetQueueStatusEx_Func)( FT_HANDLE ftHandle, DWORD *dwRxBytes);
typedef FT_STATUS (WINAPI*FT_GetQueueStatusEx_Func)( FT_HANDLE ftHandle, DWORD *dwRxBytes);

typedef FT_STATUS (WINAPI*FT_CreateDeviceInfoList_Func)(LPDWORD lpdwNumDevs);
typedef FT_STATUS (WINAPI*FT_GetDeviceInfoList_Func)(FT_DEVICE_LIST_INFO_NODE *pDest, LPDWORD lpdwNumDevs);
typedef FT_STATUS (WINAPI*FT_GetDeviceInfoDetail_Func)( DWORD dwIndex, LPDWORD lpdwFlags, LPDWORD lpdwType, LPDWORD lpdwID, LPDWORD lpdwLocId, LPVOID lpSerialNumber, LPVOID lpDescription, FT_HANDLE *pftHandle);




class FT2XXLoader
{
public:
    FT2XXLoader() : mDll(0) { mDll = LoadLibraryA("ftd2xx.dll"); loadFuncs(); }
    virtual ~FT2XXLoader() { FreeLibrary(mDll);  }

    void loadFuncs()
    {
        mFT_Open = (FT_Open_Func)GetProcAddress(mDll, "FT_Open");
        mFT_OpenEx = (FT_OpenEx_Func)GetProcAddress(mDll, "FT_OpenEx");
        mFT_ListDevices = (FT_ListDevices_Func)GetProcAddress(mDll, "FT_ListDevices");
        mFT_Close = (FT_Close_Func)GetProcAddress(mDll, "FT_Close");
        mFT_Read = (FT_Read_Func)GetProcAddress(mDll, "FT_Read");
        mFT_Write = (FT_Write_Func)GetProcAddress(mDll, "FT_Write");
        mFT_IoCtl = (FT_IoCtl_Func)GetProcAddress(mDll, "FT_IoCtl");
        mFT_SetBaudRate = (FT_SetBaudRate_Func)GetProcAddress(mDll, "FT_SetBaudRate");
        mFT_SetDivisor = (FT_SetDivisor_Func)GetProcAddress(mDll, "FT_SetDivisor");
        mFT_SetDataCharacteristics = (FT_SetDataCharacteristics_Func)GetProcAddress(mDll, "FT_SetDataCharacteristics");
        mFT_SetFlowControl = (FT_SetFlowControl_Func)GetProcAddress(mDll, "FT_SetFlowControl");
        mFT_ResetDevice = (FT_ResetDevice_Func)GetProcAddress(mDll, "FT_ResetDevice");
        mFT_SetDtr = (FT_SetDtr_Func)GetProcAddress(mDll, "FT_SetDtr");
        mFT_ClrDtr = (FT_ClrDtr_Func)GetProcAddress(mDll, "FT_ClrDtr");
        mFT_SetRts = (FT_SetRts_Func)GetProcAddress(mDll, "FT_SetRts");
        mFT_ClrRts = (FT_ClrRts_Func)GetProcAddress(mDll, "FT_ClrRts");
        mFT_GetModemStatus = (FT_GetModemStatus_Func)GetProcAddress(mDll, "FT_GetModemStatus");
        mFT_SetChars = (FT_SetChars_Func)GetProcAddress(mDll, "FT_SetChars");
        mFT_Purge = (FT_Purge_Func)GetProcAddress(mDll, "FT_Purge");
        mFT_SetTimeouts = (FT_SetTimeouts_Func)GetProcAddress(mDll, "FT_SetTimeouts");
        mFT_GetQueueStatus = (FT_GetQueueStatus_Func)GetProcAddress(mDll, "FT_GetQueueStatus");
        mFT_SetEventNotification = (FT_SetEventNotification_Func)GetProcAddress(mDll, "FT_SetEventNotification");
        mFT_GetStatus = (FT_GetStatus_Func)GetProcAddress(mDll, "FT_GetStatus");
        mFT_SetBreakOn = (FT_SetBreakOn_Func)GetProcAddress(mDll, "FT_SetBreakOn");
        mFT_SetBreakOff = (FT_SetBreakOff_Func)GetProcAddress(mDll, "FT_SetBreakOff");
        mFT_SetWaitMask = (FT_SetWaitMask_Func)GetProcAddress(mDll, "FT_SetWaitMask");
        mFT_WaitOnMask = (FT_WaitOnMask_Func)GetProcAddress(mDll, "FT_WaitOnMask");
        mFT_GetEventStatus = (FT_GetEventStatus_Func)GetProcAddress(mDll, "FT_GetEventStatus");
        mFT_ReadEE = (FT_ReadEE_Func)GetProcAddress(mDll, "FT_ReadEE");
        mFT_WriteEE = (FT_WriteEE_Func)GetProcAddress(mDll, "FT_WriteEE");
        mFT_EraseEE = (FT_EraseEE_Func)GetProcAddress(mDll, "FT_EraseEE");
        mFT_EE_Program = (FT_EE_Program_Func)GetProcAddress(mDll, "FT_EE_Program");
        mFT_EE_ProgramEx = (FT_EE_ProgramEx_Func)GetProcAddress(mDll, "FT_EE_ProgramEx");
        mFT_EE_Read = (FT_EE_Read_Func)GetProcAddress(mDll, "FT_EE_Read");
        mFT_EE_ReadEx = (FT_EE_ReadEx_Func)GetProcAddress(mDll, "FT_EE_ReadEx");
        mFT_EE_UASize = (FT_EE_UASize_Func)GetProcAddress(mDll, "FT_EE_UASize");
        mFT_EE_UAWrite = (FT_EE_UAWrite_Func)GetProcAddress(mDll, "FT_EE_UAWrite");
        mFT_EE_UARead = (FT_EE_UARead_Func)GetProcAddress(mDll, "FT_EE_UARead");
        mFT_SetLatencyTimer = (FT_SetLatencyTimer_Func)GetProcAddress(mDll, "FT_SetLatencyTimer");
        mFT_GetLatencyTimer = (FT_GetLatencyTimer_Func)GetProcAddress(mDll, "FT_GetLatencyTimer");
        mFT_SetBitMode = (FT_SetBitMode_Func)GetProcAddress(mDll, "FT_SetBitMode");
        mFT_GetBitMode = (FT_GetBitMode_Func)GetProcAddress(mDll, "FT_GetBitMode");
        mFT_SetUSBParameters = (FT_SetUSBParameters_Func)GetProcAddress(mDll, "FT_SetUSBParameters");
        mFT_SetDeadmanTimeout = (FT_SetDeadmanTimeout_Func)GetProcAddress(mDll, "FT_SetDeadmanTimeout");
        mFT_GetDeviceInfo = (FT_GetDeviceInfo_Func)GetProcAddress(mDll, "FT_GetDeviceInfo");
        mFT_StopInTask = (FT_StopInTask_Func)GetProcAddress(mDll, "FT_StopInTask");
        mFT_RestartInTask = (FT_RestartInTask_Func)GetProcAddress(mDll, "FT_RestartInTask");
        mFT_SetResetPipeRetryCount = (FT_SetResetPipeRetryCount_Func)GetProcAddress(mDll, "FT_SetResetPipeRetryCount");
        mFT_ResetPort = (FT_ResetPort_Func)GetProcAddress(mDll, "FT_ResetPort");
        mFT_CyclePort = (FT_CyclePort_Func)GetProcAddress(mDll, "FT_CyclePort");
        mFT_GetDriverVersion = (FT_GetDriverVersion_Func)GetProcAddress(mDll, "FT_GetDriverVersion");
        mFT_GetLibraryVersion = (FT_GetLibraryVersion_Func)GetProcAddress(mDll, "FT_GetLibraryVersion");
        mFT_Rescan = (FT_Rescan_Func)GetProcAddress(mDll, "FT_Rescan");
        mFT_Reload = (FT_Reload_Func)GetProcAddress(mDll, "FT_Reload");
        mFT_GetComPortNumber = (FT_GetComPortNumber_Func)GetProcAddress(mDll, "FT_GetComPortNumber");
        mFT_W32_ClearCommBreak = (FT_W32_ClearCommBreak_Func)GetProcAddress(mDll, "FT_W32_ClearCommBreak");
        mFT_W32_ClearCommError = (FT_W32_ClearCommError_Func)GetProcAddress(mDll, "FT_W32_ClearCommError");
        mFT_W32_EscapeCommFunction = (FT_W32_EscapeCommFunction_Func)GetProcAddress(mDll, "FT_W32_EscapeCommFunction");
        mFT_W32_GetCommModemStatus = (FT_W32_GetCommModemStatus_Func)GetProcAddress(mDll, "FT_W32_GetCommModemStatus");
        mFT_W32_GetCommState = (FT_W32_GetCommState_Func)GetProcAddress(mDll, "FT_W32_GetCommState");
        mFT_W32_GetCommTimeouts = (FT_W32_GetCommTimeouts_Func)GetProcAddress(mDll, "FT_W32_GetCommTimeouts"); 
        mFT_W32_PurgeComm = (FT_W32_PurgeComm_Func)GetProcAddress(mDll, "FT_W32_PurgeComm");
        mFT_W32_SetCommBreak = (FT_W32_SetCommBreak_Func)GetProcAddress(mDll, "FT_W32_SetCommBreak");
        mFT_W32_SetCommMask = (FT_W32_SetCommMask_Func)GetProcAddress(mDll, "FT_W32_SetCommMask");
        mFT_W32_GetCommMask = (FT_W32_GetCommMask_Func)GetProcAddress(mDll, "FT_W32_GetCommMask");
        mFT_W32_SetCommState = (FT_W32_SetCommState_Func)GetProcAddress(mDll, "FT_W32_SetCommState");
        mFT_W32_SetCommTimeouts = (FT_W32_SetCommTimeouts_Func)GetProcAddress(mDll, "FT_W32_SetCommTimeouts");
        mFT_W32_SetupComm = (FT_W32_SetupComm_Func)GetProcAddress(mDll, "FT_W32_SetupComm");
        mFT_W32_WaitCommEvent = (FT_W32_WaitCommEvent_Func)GetProcAddress(mDll, "FT_W32_WaitCommEvent");
        mFT_EE_ReadConfig = (FT_EE_ReadConfig_Func)GetProcAddress(mDll, "FT_EE_ReadConfig");
        mFT_EE_WriteConfig = (FT_EE_WriteConfig_Func)GetProcAddress(mDll, "FT_EE_WriteConfig");
        mFT_EE_ReadECC = (FT_EE_ReadECC_Func)GetProcAddress(mDll, "FT_EE_ReadECC");
        mFT_GetQueueStatusEx = (FT_GetQueueStatusEx_Func)GetProcAddress(mDll, "FT_GetQueueStatusEx");

        mFT_CreateDeviceInfoList = (FT_CreateDeviceInfoList_Func)GetProcAddress(mDll, "FT_CreateDeviceInfoList");
        mFT_GetDeviceInfoList = (FT_GetDeviceInfoList_Func)GetProcAddress(mDll, "FT_GetDeviceInfoList");
        mFT_GetDeviceInfoDetail = (FT_GetDeviceInfoDetail_Func)GetProcAddress(mDll, "FT_GetDeviceInfoDetail");
    }

public:
    HINSTANCE mDll;

    FT_Open_Func mFT_Open;
    FT_OpenEx_Func mFT_OpenEx;
    FT_ListDevices_Func mFT_ListDevices;
    FT_Close_Func mFT_Close;
    FT_Read_Func mFT_Read;
    FT_Write_Func mFT_Write;
    FT_IoCtl_Func mFT_IoCtl;
    FT_SetBaudRate_Func mFT_SetBaudRate;
    FT_SetDivisor_Func mFT_SetDivisor;
    FT_SetDataCharacteristics_Func mFT_SetDataCharacteristics;
    FT_SetFlowControl_Func mFT_SetFlowControl;
    FT_ResetDevice_Func mFT_ResetDevice;
    FT_SetDtr_Func mFT_SetDtr;
    FT_ClrDtr_Func mFT_ClrDtr;
    FT_SetRts_Func mFT_SetRts;
    FT_ClrRts_Func mFT_ClrRts;
    FT_GetModemStatus_Func mFT_GetModemStatus;
    FT_SetChars_Func mFT_SetChars;
    FT_Purge_Func mFT_Purge;
    FT_SetTimeouts_Func mFT_SetTimeouts;
    FT_GetQueueStatus_Func mFT_GetQueueStatus;
    FT_SetEventNotification_Func mFT_SetEventNotification;
    FT_GetStatus_Func mFT_GetStatus;
    FT_SetBreakOn_Func mFT_SetBreakOn;
    FT_SetBreakOff_Func mFT_SetBreakOff;
    FT_SetWaitMask_Func mFT_SetWaitMask;
    FT_WaitOnMask_Func mFT_WaitOnMask;
    FT_GetEventStatus_Func mFT_GetEventStatus;
    FT_ReadEE_Func mFT_ReadEE;
    FT_WriteEE_Func mFT_WriteEE;
    FT_EraseEE_Func mFT_EraseEE;
    FT_EE_Program_Func mFT_EE_Program;
    FT_EE_ProgramEx_Func mFT_EE_ProgramEx;
    FT_EE_Read_Func mFT_EE_Read;
    FT_EE_ReadEx_Func mFT_EE_ReadEx;
    FT_EE_UASize_Func mFT_EE_UASize;
    FT_EE_UAWrite_Func mFT_EE_UAWrite;
    FT_EE_UARead_Func mFT_EE_UARead;
    FT_SetLatencyTimer_Func mFT_SetLatencyTimer;
    FT_GetLatencyTimer_Func mFT_GetLatencyTimer;
    FT_SetBitMode_Func mFT_SetBitMode;
    FT_GetBitMode_Func mFT_GetBitMode;
    FT_SetUSBParameters_Func mFT_SetUSBParameters;
    FT_SetDeadmanTimeout_Func mFT_SetDeadmanTimeout;
    FT_GetDeviceInfo_Func mFT_GetDeviceInfo;
    FT_StopInTask_Func mFT_StopInTask;
    FT_RestartInTask_Func mFT_RestartInTask;
    FT_SetResetPipeRetryCount_Func mFT_SetResetPipeRetryCount;
    FT_ResetPort_Func mFT_ResetPort;
    FT_CyclePort_Func mFT_CyclePort;
    FT_GetDriverVersion_Func mFT_GetDriverVersion;
    FT_GetLibraryVersion_Func mFT_GetLibraryVersion;
    FT_Rescan_Func mFT_Rescan;
    FT_Reload_Func mFT_Reload;
    FT_GetComPortNumber_Func mFT_GetComPortNumber;
    FT_W32_ClearCommBreak_Func mFT_W32_ClearCommBreak;
    FT_W32_ClearCommError_Func mFT_W32_ClearCommError;
    FT_W32_EscapeCommFunction_Func mFT_W32_EscapeCommFunction;
    FT_W32_GetCommModemStatus_Func mFT_W32_GetCommModemStatus;
    FT_W32_GetCommState_Func mFT_W32_GetCommState;
    FT_W32_GetCommTimeouts_Func mFT_W32_GetCommTimeouts; 
    FT_W32_PurgeComm_Func mFT_W32_PurgeComm;
    FT_W32_SetCommBreak_Func mFT_W32_SetCommBreak;
    FT_W32_SetCommMask_Func mFT_W32_SetCommMask;
    FT_W32_GetCommMask_Func mFT_W32_GetCommMask;
    FT_W32_SetCommState_Func mFT_W32_SetCommState;
    FT_W32_SetCommTimeouts_Func mFT_W32_SetCommTimeouts;
    FT_W32_SetupComm_Func mFT_W32_SetupComm;
    FT_W32_WaitCommEvent_Func mFT_W32_WaitCommEvent;
    FT_EE_ReadConfig_Func mFT_EE_ReadConfig;
    FT_EE_WriteConfig_Func mFT_EE_WriteConfig;
    FT_EE_ReadECC_Func mFT_EE_ReadECC;
    FT_GetQueueStatusEx_Func mFT_GetQueueStatusEx;
    FT_CreateDeviceInfoList_Func mFT_CreateDeviceInfoList;
    FT_GetDeviceInfoList_Func mFT_GetDeviceInfoList;
    FT_GetDeviceInfoDetail_Func mFT_GetDeviceInfoDetail;
};

FT2XXLoader gFT2XX;



extern "C" {

 FT_STATUS WINAPI FT_Open( int deviceNumber, FT_HANDLE *pHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_Open( deviceNumber, pHandle);
}

 FT_STATUS WINAPI FT_OpenEx( PVOID pArg1, DWORD Flags, FT_HANDLE *pHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_OpenEx( pArg1, Flags, pHandle);
}

 FT_STATUS WINAPI FT_ListDevices( PVOID pArg1, PVOID pArg2, DWORD Flags) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_ListDevices( pArg1, pArg2, Flags);
}

 FT_STATUS WINAPI FT_Close( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_Close( ftHandle);
}

 FT_STATUS WINAPI FT_Read( FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD dwBytesToRead, LPDWORD lpBytesReturned) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_Read( ftHandle, lpBuffer, dwBytesToRead, lpBytesReturned);
}

 FT_STATUS WINAPI FT_Write( FT_HANDLE ftHandle, LPVOID lpBuffer, DWORD dwBytesToWrite, LPDWORD lpBytesWritten) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_Write( ftHandle, lpBuffer, dwBytesToWrite, lpBytesWritten);
}

 FT_STATUS WINAPI FT_IoCtl(FT_HANDLE ftHandle, DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
{
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_IoCtl( ftHandle, dwIoControlCode, lpInBuf, nInBufSize, lpOutBuf, nOutBufSize, lpBytesReturned, lpOverlapped);
}

 FT_STATUS WINAPI FT_SetBaudRate( FT_HANDLE ftHandle, ULONG BaudRate) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetBaudRate( ftHandle, BaudRate);
}

 FT_STATUS WINAPI FT_SetDivisor( FT_HANDLE ftHandle, USHORT Divisor) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetDivisor( ftHandle, Divisor);
}

 FT_STATUS WINAPI FT_SetDataCharacteristics( FT_HANDLE ftHandle, UCHAR WordLength, UCHAR StopBits, UCHAR Parity) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetDataCharacteristics( ftHandle, WordLength, StopBits, Parity);
}

 FT_STATUS WINAPI FT_SetFlowControl( FT_HANDLE ftHandle, USHORT FlowControl, UCHAR XonChar, UCHAR XoffChar) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetFlowControl( ftHandle, FlowControl, XonChar, XoffChar);
}

 FT_STATUS WINAPI FT_ResetDevice( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_ResetDevice( ftHandle);
}

 FT_STATUS WINAPI FT_SetDtr( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetDtr( ftHandle);
}

 FT_STATUS WINAPI FT_ClrDtr( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_ClrDtr( ftHandle);
}

 FT_STATUS WINAPI FT_SetRts( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetRts( ftHandle);
}

 FT_STATUS WINAPI FT_ClrRts( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_ClrRts( ftHandle);
}

 FT_STATUS WINAPI FT_GetModemStatus( FT_HANDLE ftHandle, ULONG *pModemStatus) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_GetModemStatus( ftHandle, pModemStatus);
}

 FT_STATUS WINAPI FT_SetChars( FT_HANDLE ftHandle, UCHAR EventChar, UCHAR EventCharEnabled, UCHAR ErrorChar, UCHAR ErrorCharEnabled) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetChars( ftHandle, EventChar, EventCharEnabled, ErrorChar, ErrorCharEnabled);
}

 FT_STATUS WINAPI FT_Purge( FT_HANDLE ftHandle, ULONG Mask) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_Purge( ftHandle, Mask);
}

 FT_STATUS WINAPI FT_SetTimeouts( FT_HANDLE ftHandle, ULONG ReadTimeout, ULONG WriteTimeout) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetTimeouts( ftHandle, ReadTimeout, WriteTimeout);
}

 FT_STATUS WINAPI FT_GetQueueStatus( FT_HANDLE ftHandle, DWORD *dwRxBytes) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_GetQueueStatus( ftHandle,dwRxBytes);
}

 FT_STATUS WINAPI FT_SetEventNotification( FT_HANDLE ftHandle, DWORD Mask, PVOID Param) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetEventNotification( ftHandle, Mask, Param);
}

 FT_STATUS WINAPI FT_GetStatus( FT_HANDLE ftHandle, DWORD *dwRxBytes, DWORD *dwTxBytes, DWORD *dwEventDWord) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_GetStatus( ftHandle,dwRxBytes,dwTxBytes,dwEventDWord);
}

 FT_STATUS WINAPI FT_SetBreakOn( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetBreakOn( ftHandle);
}

 FT_STATUS WINAPI FT_SetBreakOff( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetBreakOff( ftHandle);
}

 FT_STATUS WINAPI FT_SetWaitMask( FT_HANDLE ftHandle, DWORD Mask) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetWaitMask( ftHandle, Mask);
}

 FT_STATUS WINAPI FT_WaitOnMask( FT_HANDLE ftHandle, DWORD *Mask) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_WaitOnMask( ftHandle,Mask);
}

 FT_STATUS WINAPI FT_GetEventStatus( FT_HANDLE ftHandle, DWORD *dwEventDWord) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_GetEventStatus( ftHandle,dwEventDWord);
}

 FT_STATUS WINAPI FT_ReadEE( FT_HANDLE ftHandle, DWORD dwWordOffset, LPWORD lpwValue) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_ReadEE( ftHandle, dwWordOffset, lpwValue);
}

 FT_STATUS WINAPI FT_WriteEE( FT_HANDLE ftHandle, DWORD dwWordOffset, WORD wValue) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_WriteEE( ftHandle, dwWordOffset, wValue);
}

 FT_STATUS WINAPI FT_EraseEE( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_EraseEE( ftHandle);
}

 FT_STATUS WINAPI FT_EE_Program( FT_HANDLE ftHandle, PFT_PROGRAM_DATA pData) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_EE_Program( ftHandle,  pData);
}

 FT_STATUS WINAPI FT_EE_ProgramEx( FT_HANDLE ftHandle, PFT_PROGRAM_DATA pData, char *Manufacturer, char *ManufacturerId, char *Description, char *SerialNumber) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_EE_ProgramEx( ftHandle,  pData,Manufacturer,ManufacturerId,Description,SerialNumber);
}

 FT_STATUS WINAPI FT_EE_Read( FT_HANDLE ftHandle, PFT_PROGRAM_DATA pData) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_EE_Read( ftHandle, pData);
}

 FT_STATUS WINAPI FT_EE_ReadEx( FT_HANDLE ftHandle, PFT_PROGRAM_DATA pData, char *Manufacturer, char *ManufacturerId, char *Description, char *SerialNumber) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_EE_ReadEx( ftHandle, pData,Manufacturer,ManufacturerId,Description,SerialNumber);
}

 FT_STATUS WINAPI FT_EE_UASize( FT_HANDLE ftHandle, LPDWORD lpdwSize) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_EE_UASize( ftHandle, lpdwSize);
}

 FT_STATUS WINAPI FT_EE_UAWrite( FT_HANDLE ftHandle, PUCHAR pucData, DWORD dwDataLen) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_EE_UAWrite( ftHandle, pucData, dwDataLen);
}

 FT_STATUS WINAPI FT_EE_UARead( FT_HANDLE ftHandle, PUCHAR pucData, DWORD dwDataLen, LPDWORD lpdwBytesRead) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_EE_UARead( ftHandle, pucData, dwDataLen, lpdwBytesRead);
}

 FT_STATUS WINAPI FT_SetLatencyTimer( FT_HANDLE ftHandle, UCHAR ucLatency) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetLatencyTimer( ftHandle, ucLatency);
}

 FT_STATUS WINAPI FT_GetLatencyTimer( FT_HANDLE ftHandle, PUCHAR pucLatency) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_GetLatencyTimer( ftHandle, pucLatency);
}

 FT_STATUS WINAPI FT_SetBitMode( FT_HANDLE ftHandle, UCHAR ucMask, UCHAR ucEnable) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetBitMode( ftHandle, ucMask, ucEnable);
}

 FT_STATUS WINAPI FT_GetBitMode( FT_HANDLE ftHandle, PUCHAR pucMode) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_GetBitMode( ftHandle, pucMode);
}

 FT_STATUS WINAPI FT_SetUSBParameters( FT_HANDLE ftHandle, ULONG ulInTransferSize, ULONG ulOutTransferSize) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetUSBParameters( ftHandle, ulInTransferSize, ulOutTransferSize);
}

 FT_STATUS WINAPI FT_SetDeadmanTimeout( FT_HANDLE ftHandle, ULONG ulDeadmanTimeout) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetDeadmanTimeout( ftHandle, ulDeadmanTimeout);
}

 FT_STATUS WINAPI FT_GetDeviceInfo( FT_HANDLE ftHandle, FT_DEVICE *lpftDevice, LPDWORD lpdwID, PCHAR SerialNumber, PCHAR Description, LPVOID Dummy) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_GetDeviceInfo( ftHandle, lpftDevice, lpdwID, SerialNumber, Description, Dummy);
}

 FT_STATUS WINAPI FT_StopInTask( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_StopInTask( ftHandle);
}

 FT_STATUS WINAPI FT_RestartInTask( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_RestartInTask( ftHandle);
}

 FT_STATUS WINAPI FT_SetResetPipeRetryCount( FT_HANDLE ftHandle, DWORD dwCount) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_SetResetPipeRetryCount( ftHandle, dwCount);
}

 FT_STATUS WINAPI FT_ResetPort( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_ResetPort( ftHandle);
}

 FT_STATUS WINAPI FT_CyclePort( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_CyclePort( ftHandle);
}

 FT_STATUS WINAPI FT_GetDriverVersion( FT_HANDLE ftHandle, LPDWORD lpdwVersion) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_GetDriverVersion( ftHandle, lpdwVersion);
}

 FT_STATUS WINAPI FT_GetLibraryVersion( LPDWORD lpdwVersion) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_GetLibraryVersion( lpdwVersion);
}

 FT_STATUS WINAPI FT_Rescan() {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_Rescan();
}

 FT_STATUS WINAPI FT_Reload( WORD wVid, WORD wPid) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_Reload( wVid, wPid);
}

 FT_STATUS WINAPI FT_GetComPortNumber( FT_HANDLE ftHandle, LPLONG  lpdwComPortNumber) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_GetComPortNumber( ftHandle,  lpdwComPortNumber);
}

 BOOL WINAPI FT_W32_ClearCommBreak( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_ClearCommBreak( ftHandle);
}

 BOOL WINAPI FT_W32_ClearCommError( FT_HANDLE ftHandle, LPDWORD lpdwErrors, LPFTCOMSTAT lpftComstat) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_ClearCommError( ftHandle, lpdwErrors,  lpftComstat);
}

 BOOL WINAPI FT_W32_EscapeCommFunction( FT_HANDLE ftHandle, DWORD dwFunc) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_EscapeCommFunction( ftHandle, dwFunc);
}

 BOOL WINAPI FT_W32_GetCommModemStatus( FT_HANDLE ftHandle, LPDWORD lpdwModemStatus) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_GetCommModemStatus( ftHandle, lpdwModemStatus);
}

 BOOL WINAPI FT_W32_GetCommState( FT_HANDLE ftHandle, LPFTDCB lpftDcb) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_GetCommState( ftHandle, lpftDcb);
}

 BOOL WINAPI FT_W32_GetCommTimeouts( FT_HANDLE ftHandle, FTTIMEOUTS *pTimeouts) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_GetCommTimeouts( ftHandle,pTimeouts);
}
 
 BOOL WINAPI FT_W32_PurgeComm( FT_HANDLE ftHandle, DWORD dwMask) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_PurgeComm( ftHandle, dwMask);
}

 BOOL WINAPI FT_W32_SetCommBreak( FT_HANDLE ftHandle) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_SetCommBreak( ftHandle);
}

 BOOL WINAPI FT_W32_SetCommMask( FT_HANDLE ftHandle, ULONG ulEventMask) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_SetCommMask( ftHandle, ulEventMask);
}

 BOOL WINAPI FT_W32_GetCommMask( FT_HANDLE ftHandle, LPDWORD lpdwEventMask) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_GetCommMask( ftHandle, lpdwEventMask);
}

 BOOL WINAPI FT_W32_SetCommState( FT_HANDLE ftHandle, LPFTDCB lpftDcb) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_SetCommState( ftHandle, lpftDcb);
}

 BOOL WINAPI FT_W32_SetCommTimeouts( FT_HANDLE ftHandle, FTTIMEOUTS *pTimeouts) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_SetCommTimeouts( ftHandle,pTimeouts);
}

 BOOL WINAPI FT_W32_SetupComm( FT_HANDLE ftHandle, DWORD dwReadBufferSize, DWORD dwWriteBufferSize) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_SetupComm( ftHandle, dwReadBufferSize, dwWriteBufferSize);
}

 BOOL WINAPI FT_W32_WaitCommEvent( FT_HANDLE ftHandle, PULONG pulEvent, LPOVERLAPPED lpOverlapped) {
     if (!gFT2XX.mDll)
         return FALSE;
    return gFT2XX.mFT_W32_WaitCommEvent( ftHandle, pulEvent, lpOverlapped);
}

 FT_STATUS WINAPI FT_EE_ReadConfig( FT_HANDLE ftHandle, UCHAR ucAddress, PUCHAR pucValue) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_EE_ReadConfig( ftHandle, ucAddress, pucValue);
}

 FT_STATUS WINAPI FT_EE_WriteConfig( FT_HANDLE ftHandle, UCHAR ucAddress, UCHAR ucValue) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_EE_WriteConfig( ftHandle, ucAddress, ucValue);
}

 FT_STATUS WINAPI FT_EE_ReadECC( FT_HANDLE ftHandle, UCHAR ucOption, LPWORD lpwValue) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_EE_ReadECC( ftHandle, ucOption, lpwValue);
}

 FT_STATUS WINAPI FT_GetQueueStatusEx( FT_HANDLE ftHandle, DWORD *dwRxBytes) {
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_GetQueueStatusEx( ftHandle,dwRxBytes);
}

FT_STATUS WINAPI FT_CreateDeviceInfoList(LPDWORD lpdwNumDevs)
{
     if (!gFT2XX.mDll){
         if (lpdwNumDevs)
             *lpdwNumDevs = 0;
         return FT_OTHER_ERROR;
     }
    return gFT2XX.mFT_CreateDeviceInfoList(lpdwNumDevs);
}

FT_STATUS WINAPI FT_GetDeviceInfoList(FT_DEVICE_LIST_INFO_NODE *pDest, LPDWORD lpdwNumDevs)
{
     if (!gFT2XX.mDll){
         if (pDest)
         return FT_OTHER_ERROR;
     }
    return gFT2XX.mFT_GetDeviceInfoList(pDest, lpdwNumDevs);
}

FT_STATUS WINAPI FT_GetDeviceInfoDetail( DWORD dwIndex, LPDWORD lpdwFlags, LPDWORD lpdwType, LPDWORD lpdwID, LPDWORD lpdwLocId, LPVOID lpSerialNumber, LPVOID lpDescription, FT_HANDLE *pftHandle)
{
     if (!gFT2XX.mDll)
         return FT_OTHER_ERROR;
    return gFT2XX.mFT_GetDeviceInfoDetail(dwIndex, lpdwFlags, lpdwType, lpdwID, lpdwLocId, lpSerialNumber, lpDescription, pftHandle);
}

}
