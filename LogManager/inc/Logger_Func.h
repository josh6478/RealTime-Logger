
#ifndef __LOGGER_FUNC_H
#define __LOGGER_FUNC_H

#define IN              /* function input paramter */
#define OUT             /* function output paramter */
#define INOUT           /* function input and output paramter */

void Logger_Init(IN LoggerConfig_t *pLoggerConfig, IN uint8 Attr);
void Logger_UnInit(void);
void Logger_SetLogLevel(uint8 LogLevel);
void Logger_SetModuleMask(uint32 ModuleMask);
void Logger_SwitchOnOff(BOOL IsOn, BOOL IsSaveDB);
void Logger_SetMode(uint8 Mode);
void Logger_SetPrintoutEnDis(BOOL IsEnabled);
void Logger_SetSocketConfig(LoggerSocketConfig_t *pSocketConfig);
void Logger_printf(uint8 level, char *fmt, ...);
BOOL Logger_SetConfig(LoggerConfig_t *pLoggerConfig);
void Logger_SetDefaultSocketConfig(void);
void Logger_SetTxResolution(uint8 Resolution);
void Logger_HandleSocketReply(uint16 cmd, uint32 status, uint16 SessionType);
void Logger_HandleTimerExpr(uint16 TimerType);
void Logger_SendConfigToSrv(IN uint8 ConfigType);
void Logger_HandleServerCmd(void* pDataIn, uint8 Len, uint8 CmdType);
BOOL Logger_SendLogFileToServer(LoggerFtpConfig_t *pLoggerFtpConfig, LoggerSocketConfig_t *pLoggerSocketConfig);
void Logger_HandleFtpPutReply(uint8 Status, uint8 Desc);
BOOL Logger_SetRAMGeneralConfig(LoggerGenConfig_t *pGeneralConfig);
void Logger_FlushRxBuffer(void);
BOOL Logger_GetModeOnOff(void);
void Logger_SetTaskState(uint8 State);
void Logger_PrintStartMessage();
BOOL Logger_ValidateGenConfig(LoggerGenConfig_t *pLoggerGenConfig);
BOOL Logger_ValidateTcpConfig(LoggerSocketConfig_t *pLoggerSocketConfig);
BOOL Logger_ValidateFtpConfig(LoggerFtpConfig_t *pLoggerFtpConfig);
BOOL Logger_SendDebugLog(uint16 SizeKB, uint8 DestType);
void Logger_SetDestinationType(uint8 DestType);

#endif //__LOGGER_FUNC_H
