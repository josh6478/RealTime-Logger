#ifndef _LOGGER_UTILS_H_
#define _LOGGER_UTILS_H_

#ifdef ENABLE_LOG_PRINT
#define LOG_PRINT(LEVEL, ...)   Logger_printf(LEVEL, __VA_ARGS__)
#define LOG_ENTRY()             Logger_printf(LEVEL_FLOW, "%s >>\n", __FUNCTION__)
#define LOG_LINE()              Logger_printf(LEVEL_FLOW, "%s %d", __FUNCTION__, __LINE__)
#define LOG_EXIT()              Logger_printf(LEVEL_FLOW, "%s %d <<\n", __FUNCTION__, __LINE__)
#else
#define LOG_PRINT(LEVEL, ...)
#endif

/* debug prints levels the sevirity is from low to high */
typedef enum {
    LEVEL_NONE,                     /* all prints are carried out, always, and from all tassk */
    LEVEL_CRITICAL,                  /* critical prints are carried out, or upper sevirity */ 
    LEVEL_ERROR,                    /* errors prints are carried out, or upper sevirity */ 
    LEVEL_WARNING,                  /* warning prints are carried out, or upper sevirity */ 
    LEVEL_INFO,                     /* info prints are carried out, or upper sevirity */ 
    LEVEL_DETAILS,                  /* details prints are carried out, or upper sevirity */
    LEVEL_FLOW,                     /* flow prints are carried out, or upper sevirity */
    LEVEL_MAX_NUM                   /* maximum number of level prints */
}LoggerLogLevel_e;

void Logger_printf(uint8 level, char *fmt, ...);
void Logger_SendPacketFromInt(void);
BOOL Logger_IsLoopBack(uint8 TaskId);
uint16 Logger_GetMaxBuffSize(void);
BOOL Logger_IsReady(void);
BOOL Logger_IsInit(void);
BOOL Logger_IsTestMode(void);
void Logger_SetTestMode(void);
BOOL Logger_UploadLogFile(uint16 SizeKB);
extern "C" void HardFaultHandler(uint32 *pHardFaultArgs);

#endif // _LOGGER_UTILS_H_
