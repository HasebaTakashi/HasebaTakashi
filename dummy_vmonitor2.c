// dummy_vmonitor2.c
// A dummy implementation of the vmonitor2 library for testing purposes.

#include <stdio.h>

int VM2_Open() { return 0; }
int VM2_Close() { return 0; }
int VM2_Reset(unsigned int dwDelaySec) { return 0; }
int VM2_Restart(unsigned int dwDelaySec) { return 0; }
int VM2_Initialize() { return 0; }
int VM2_StartSampling() { return 0; }
int VM2_StopSampling() { return 0; }
int VM2_CheckBuffer(int* pExist) { *pExist = 1; return 0; }
int VM2_GetData(short* pData1, short* pData2, short* pData3, short* pData4,
                short* pData5, short* pData6, short* pData7, short* pData8,
                short* pData9, short* pData10, short* pData11, short* pData12,
                short* pData13, short* pData14, short* pData15, short* pData16,
                int* plPulse) { return 0; }
int VM2_GetStatus(unsigned int* pdwStatus) { *pdwStatus = 0; return 0; }
int VM2_GetDetailErrorCode(unsigned int* pdwCode) { *pdwCode = 0; return 0; }
int VM2_GetGain(int ch, unsigned int* pdwGain) { *pdwGain = 1; return 0; }
int VM2_SetGain(int ch, unsigned int dwGain) { return 0; }
int VM2_GetInputSelect(int ch, unsigned int* pdwSelect) { *pdwSelect = 0; return 0; }
int VM2_SetInputSelect(int ch, unsigned int dwSelect) { return 0; }
int VM2_GetPulseVoltage(unsigned int* pdwVol) { *pdwVol = 100; return 0; }
int VM2_SetPulseVoltage(unsigned int dwVol) { return 0; }
int VM2_GetTerminalAd(int ch, unsigned int* pdwAd) { *pdwAd = 40000; return 0; }
int VM2_GetDigitalIn(int ch, unsigned int* pdwDigitalIn) { *pdwDigitalIn = 0; return 0; }
int VM2_GetDigitalOut(int ch, unsigned int* pdwDigitalOut) { *pdwDigitalOut = 0; return 0; }
int VM2_SetDigitalOut(int ch, unsigned int dwDigitalOut) { return 0; }
int VM2_GetVersion(unsigned int* pdwVersion) { *pdwVersion = 0x010000; return 0; }
int VM2_GetIdleReboot(unsigned int* pdwReboot, unsigned int* pdwPeriod) { *pdwReboot = 0; *pdwPeriod = 0; return 0; }
int VM2_SetIdleReboot(unsigned int dwReboot, unsigned int dwPeriod) { return 0; }
