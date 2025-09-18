
#ifndef __LOGGER_API_H
#define __LOGGER_API_H

/**
 * <pre>
 * void Logger_InitApi(LoggerConfig_t *pLoggerConfig, uint8 Attr)
 * </pre>
 *  
 * This api initializes the logger manager task. It gets a configuration data structure and configures the task accordingly.
 * If no input configuration is passed, the logger will start with the default hard codded configuration (see file Logger_Config.h)
 *
 * @param   pLoggerConfig  [in]    Optional. a pointer to logger configuration data structure
 * @param   Attr           [in]    The api possible attributes:
 *                                 LOGGER_ATTR_NONE - No special attribute
 *                                 LOOGER_ATTR_ERASE_FLASH_ON_RESET - currently not implemented (for future use)
 *                                 LOGGER_ATTR_ERASE_FLASH_NOW - erase flash on init
 *                                 LOGGER_ATTR_ERASE_FLASH_ON_SENDING - erase flash after sending log file. currently not implemented (for future use)
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_InitApi(LoggerConfig_t *pLoggerConfig, uint8 Attr);
/**
 * <pre>
 * void Logger_UnitApi(void)
 * </pre>
 *  
 * This api uninitializes the logger manager task. Currently not implemented (for future use)
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_UnitApi(void);
/**
 * <pre>
 * void Logger_SetLogLevelApi(uint8 LogLevel)
 * </pre>
 *  
 * This api sets the logger log level in runtime
 *
 * @param   LogLevel           [in]    The desired log level (see LoggerLogLevel_e)
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetLogLevelApi(uint8 LogLevel);
/**
 * <pre>
 * void Logger_SetModuleMaskApi(uint32 ModuleMask)
 * </pre>
 *  
 * This api sets the logger log level in runtime
 *
 * @param   LogLevel           [in]    The desired log level (see LoggerLogLevel_e)
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetModuleMaskApi(uint32 ModuleMask);
/**
 * <pre>
 * void Logger_SetModeApi(uint8 Mode)
 * </pre>
 *  
 * This api set logger mode in run time.
 *
 * @param   Mode  [in]    the logger mode to set. currently the possible modes are:
 *                        LOGGER_MODE_TYPE_PUSH - each log message is sent directly over a socket. (currently not supported)
 *                        LOGGER_MODE_TYPE_PULL - each log message is writen to the log file in the panel and sent by command over FTP    
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetModeApi(uint8 Mode);

/**
 * <pre>
 * void Logger_SetDestinationTypeApi(uint8 DestType)
 * </pre>
 *  
 * This api set logger destination type in run time.
 *
 * @param   Mode  [in]    the logger mode to set. currently the possible modes are:
 *                        LOGGER_DEST_TYPE_NONE - no destination type
 *                        LOGGER_DEST_TYPE_FLASH - Logs are sent to flash
 *                        LOGGER_DEST_TYPE_SOCKET - Logs are sent to socket
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

void Logger_SetDestinationTypeApi(uint8 DestType);

/**
 * <pre>
 * void Logger_SetRemoteDebugServerApi(LoggerSocketConfig_t *pSocket)
 * </pre>
 *  
 * This api set logger remote debug server in run time.
 *
 * @param   pSocket  [in]       a pointer to Logger socket configuration.
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetRemoteDebugServerApi(LoggerSocketConfig_t *pSocket);
/**
 * <pre>
 * void Logger_EnDisPrintoutApi(uint8 IsEnabled)
 * </pre>
 *  
 * This api open\close the RS232 log printout in runtime.
 *
 * @param   IsEnabled  [in]       1 - the printout is enabled 0 for disabled.
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_EnDisPrintoutApi(uint8 IsEnabled);
/**
 * <pre>
 * void Logger_SwitchOnOffApi(uint8 IsOn)
 * </pre>
 *  
 * This api enables\disables the logger feature in run time.
 *
 * @param   IsOn  [in]       1 for enabling the feature 0 for disabling.
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SwitchOnOffApi(uint8 IsOn);
/**
 * <pre>
 * void Logger_SetFlashSegmentSizeApi(uint16 FlashSizeKB)
 * </pre>
 *  
 * This api set the logger file size (flash) in run time 
 *
 * @param   FlashSizeKB  [in]       The flash log file size in KB.
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetFlashSegmentSizeApi(uint16 FlashSizeKB);
/**
 * <pre>
 * void Logger_DumpDebugLogApi(uint16 FlashSizeKB)
 * </pre>
 *  
 * This api dump the last FlashSizeKB size of log file to the RS232 (for debug)
 *
 * @param   FlashSizeKB  [in]       The last file size in KB to dump.
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_DumpDebugLogApi(uint16 FlashSizeKB);
/**
 * <pre>
 * void Logger_UploadLogFileApi(LoggerFtpConfig_t *pFtpConfig,  LoggerSocketConfig_t *pLoggerSocketConfig)
 * </pre>
 *  
 * This api upload a log file to the server. 
 *
 * @param   pFtpConfig           [in]       a pointer to Logger ftp server to upload the file to.
 * @param   pLoggerSocketConfig  [in]       a pointer to Logger socket releated to the FTP.
 * NOTE: in case both paramters are NULL the logger will use the default hard coded FTP server configured (see Logger_Config.h) 
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_UploadLogFileApi(LoggerFtpConfig_t *pFtpConfig,  LoggerSocketConfig_t *pLoggerSocketConfig);
/**
 * <pre>
 * void Logger_SendConfigToSrvApi(uint8 ConfigType)
 * </pre>
 *  
 * This api sends the current logger configuration to the server
 *
 * @param   ConfigType  [in]       The configuration type to send. Currently there are the following types:
 *                                 e_LOGGER_CONF_TYPE_GENERAL - general configuration type
 *                                 e_LOGGER_CONF_TYPE_SOCKET - socket configuration type
 *                                 e_LOGGER_CONF_TYPE_FTP - ftp server configuration type
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SendConfigToSrvApi(uint8 EventType);
/**
 * <pre>
 * void Logger_SetConfigFromSrvApi(uint8 ConfigType, void* pConfig)
 * </pre>
 *  
 * This api sets a logger configuration type in run time. the changes will also take affect after reset. 
 *
 * @param   ConfigType  [in]       The configuration type to send. Currently there are the following types:
 *                                 e_LOGGER_CONF_TYPE_GENERAL - general configuration type
 *                                 e_LOGGER_CONF_TYPE_SOCKET - socket configuration type
 *                                 e_LOGGER_CONF_TYPE_FTP - ftp server configuration type
 *
 * @param   pConfig  [in]          The configuration buffer to set (according to the ConfigType parameter)
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetConfigFromSrvApi(uint8 configType, void* pConfig);
#endif //__LOGGER_API_H