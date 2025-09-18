
#include "Logger_Defs.h"
#include "Logger_Manager.h"


                /* ========================================== *
                 *     P R I V A T E     F U N C T I O N S    *
                 * ========================================== */

/**
 * <pre>
 * static void Logger_SendGeneralCmd(void *pInputBuff, uint16 BufLen, LoggerCmd_e Cmd, uint16 Param)
 * </pre>
 *  
 * This function sends general command to the logger task
 *
 * @param   pInputBuff     [in]    Optional. a pointer to input buffer for the task to process (in case the data contains only one parameter use Param below)
 * @param   BufLen         [in]    the input buffer length in bytes
 * @param   Cmd            [in]    the logger command enum
 * @param   Param          [in]    additional input parameter
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

static void Logger_SendGeneralCmd(void *pInputBuff, uint16 BufLen, LoggerCmd_e Cmd, uint16 Param)
{
    uint8 TaskId = OSAL_GetSelfTaskId();
    OSAL_Msg *pMessage;
        
    /* build message */
    pMessage               = OSAL_MsgNew();
    pMessage->cmd          = Cmd;
    pMessage->src          = TaskId;
    pMessage->origSrc      = TaskId;
    pMessage->dst          = TASK_APP_LOGGER_MANAGER_ID;
    pMessage->reply        = FALSE;
    pMessage->status       = 0;
    pMessage->pDataIn      = pInputBuff;
    pMessage->DataInLen    = BufLen;
    pMessage-> CntrlBits   = pInputBuff?OSAL_MSG_FREE_BY_TARGET:0;
    pMessage-> pDataOut    = NULL ;
    pMessage-> DataOutLen  = 0 ;
    pMessage-> userTag     = Param;
  
    /* send message */
    OSAL_SendMessageToTask(pMessage);

}

                /* ========================================== *
                 *     A P I     F U N C T I O N S            *
                 * ========================================== */

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
void Logger_InitApi(LoggerConfig_t *pLoggerConfig, uint8 Attr)
{
    uint8 TaskId = OSAL_GetSelfTaskId();
    OSAL_Msg *pMessage;
        
    /* build message */
    pMessage               = OSAL_MsgNew();
    pMessage->cmd          = e_LOGGER_CMD_INIT;
    pMessage->src          = TaskId;
    pMessage->origSrc      = TaskId;
    pMessage->dst          = TASK_APP_LOGGER_MANAGER_ID;
    pMessage->reply        = FALSE;
    pMessage->status       = 0 ;
    pMessage->pDataIn      = pLoggerConfig;
    pMessage->DataInLen    = sizeof(pLoggerConfig);
    pMessage-> CntrlBits   = OSAL_MSG_FREE_BY_TARGET;
    pMessage-> pDataOut    = NULL ;
    pMessage-> DataOutLen  = 0 ;
    pMessage-> userTag     = Attr;
  
    /* send message */
    OSAL_SendMessageToTask(pMessage);
    
}

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

void Logger_UnitApi(void)
{
    Logger_SendGeneralCmd(NULL, 0, e_LOGGER_CMD_UNINIT, 0);
}

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

void Logger_SetLogLevelApi(uint8 LogLevel)
{
    Logger_SendGeneralCmd(NULL, 0, e_LOGGER_CMD_SET_DEBUG_LEVEL, LogLevel);
}

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
void Logger_SetModuleMaskApi(uint32 ModuleMask)
{
    uint8 TaskId = OSAL_GetSelfTaskId();
    OSAL_Msg *pMessage;
    
    /* build message */
    pMessage               = OSAL_MsgNew();
    pMessage->cmd          = e_LOGGER_CMD_SET_MODULE_MASK;
    pMessage->src          = TaskId;
    pMessage->origSrc      = TaskId;
    pMessage->dst          = TASK_APP_LOGGER_MANAGER_ID;
    pMessage->reply        = FALSE;
    pMessage->status       = ModuleMask ;
    pMessage->pDataIn      = NULL;
    pMessage->DataInLen    = 0;
    pMessage-> CntrlBits   = 0;
    pMessage-> pDataOut    = NULL ;
    pMessage-> DataOutLen  = 0 ;
    pMessage-> userTag     = 0;
  
    /* send message */
    OSAL_SendMessageToTask(pMessage);
}

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

void Logger_SetModeApi(uint8 Mode)
{
    Logger_SendGeneralCmd(NULL, 0, e_LOGGER_CMD_SET_MODE, Mode);    
}

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

void Logger_SetDestinationTypeApi(uint8 DestType)
{
    Logger_SendGeneralCmd(NULL, 0, e_LOGGER_CMD_SET_DEST_TYPE, DestType);    
}


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
void Logger_SetRemoteDebugServerApi(LoggerSocketConfig_t *pSocket)
{
    uint8 TaskId = OSAL_GetSelfTaskId();
    OSAL_Msg *pMessage;
    
    LoggerSocketConfig_t *pSocketBuff = (LoggerSocketConfig_t *)OSAL_HeapAllocBuffer(OSAL_GetSelfTaskId(), sizeof(LoggerSocketConfig_t));
    
    if(pSocketBuff == NULL)
    {
        OSAL_ASSERT(0);
        return;
    }
    OSAL_MemCopy(pSocketBuff, pSocket, sizeof(LoggerSocketConfig_t));
    
    /* build message */
    pMessage               = OSAL_MsgNew();
    pMessage->cmd          = e_LOGGER_CMD_SET_DEBUG_SERVER;
    pMessage->src          = TaskId;
    pMessage->origSrc      = TaskId;
    pMessage->dst          = TASK_APP_LOGGER_MANAGER_ID;
    pMessage->reply        = FALSE;
    pMessage->status       = 0 ;
    pMessage->pDataIn      = pSocketBuff;
    pMessage->DataInLen    = sizeof(pSocketBuff);
    pMessage-> CntrlBits   = OSAL_MSG_FREE_BY_TARGET;
    pMessage-> pDataOut    = NULL ;
    pMessage-> DataOutLen  = 0;
    pMessage-> userTag     = 0;
  
    /* send message */
    OSAL_SendMessageToTask(pMessage);
    
}

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
void Logger_UploadLogFileApi(LoggerFtpConfig_t *pFtpConfig,  LoggerSocketConfig_t *pLoggerSocketConfig)
{
    uint8 TaskId = OSAL_GetSelfTaskId();
    OSAL_Msg *pMessage;
        
    LoggerFtpConfig_t *pFtpConfigBuff = (LoggerFtpConfig_t *)OSAL_HeapAllocBuffer(OSAL_GetSelfTaskId(), sizeof(LoggerFtpConfig_t));
    LoggerSocketConfig_t *pSocketConfigBuff = (LoggerSocketConfig_t *)OSAL_HeapAllocBuffer(OSAL_GetSelfTaskId(), sizeof(LoggerSocketConfig_t));
    if(pFtpConfigBuff == NULL || pSocketConfigBuff == NULL)
    {
        OSAL_ASSERT(0);
        return;
    }
    OSAL_MemCopy(pFtpConfigBuff, pFtpConfig, sizeof(LoggerFtpConfig_t));
    OSAL_MemCopy(pSocketConfigBuff, pLoggerSocketConfig, sizeof(LoggerSocketConfig_t));

    /* build message */
    pMessage               = OSAL_MsgNew();
    pMessage->cmd          = e_LOGGER_CMD_UPLOAD_DEBUG_LOG;
    pMessage->src          = TaskId;
    pMessage->origSrc      = TaskId;
    pMessage->dst          = TASK_APP_LOGGER_MANAGER_ID;
    pMessage->reply        = FALSE;
    pMessage->status       = 0 ;
    pMessage->pDataIn      = pFtpConfigBuff;
    pMessage->DataInLen    = sizeof(pFtpConfigBuff);
    pMessage-> CntrlBits   = OSAL_MSG_FREE_BY_TARGET;
    /* NOTE: this is not an output buffer. I use this pointer instead of allocating a new buffer to hold both in and output buffers */
    /* I release the buffer in the handling task */
    pMessage-> pDataOut    = pSocketConfigBuff ;
    pMessage-> DataOutLen  = sizeof(LoggerSocketConfig_t);
    pMessage-> userTag     = 0;
  
    /* send message */
    OSAL_SendMessageToTask(pMessage);
    
}

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
void Logger_EnDisPrintoutApi(uint8 IsEnabled)
{
    Logger_SendGeneralCmd(NULL, 0, e_LOGGER_CMD_PRINTOUT_DIS_EN, IsEnabled);
}

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
void Logger_SwitchOnOffApi(uint8 IsOn)
{
    Logger_SendGeneralCmd(NULL, 0, e_LOGGER_CMD_DIS_EN, IsOn);
}

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
void Logger_SetFlashSegmentSizeApi(uint16 FlashSizeKB)
{
    Logger_SendGeneralCmd(NULL, 0, e_LOGGER_CMD_SET_FLASH_SEGMENT_SIZE, FlashSizeKB);
}
#if 0
void Logger_PullDebugLogApi(uint16 FlashSizeKB, LoggerFtpConfig_t *pFtpConfig)
{
    uint8 TaskId = OSAL_GetSelfTaskId();
    OSAL_Msg *pMessage;
        
    /* build message */
    pMessage               = OSAL_MsgNew();
    pMessage->cmd          = e_LOGGER_CMD_PULL_DEBUG_LOG;
    pMessage->src          = TaskId;
    pMessage->origSrc      = TaskId;
    pMessage->dst          = TASK_APP_LOGGER_MANAGER_ID;
    pMessage->reply        = FALSE;
    pMessage->status       = 0 ;
    pMessage->pDataIn      = pFtpConfig;
    pMessage->DataInLen    = sizeof(pFtpConfig);
    pMessage-> CntrlBits   = OSAL_MSG_FREE_BY_TARGET;
    pMessage-> pDataOut    = NULL ;
    pMessage-> DataOutLen  = 0 ;
    pMessage-> userTag     = FlashSizeKB;
  
    /* send message */
    OSAL_SendMessageToTask(pMessage);

}
#endif

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
void Logger_DumpDebugLogApi(uint16 FlashSizeKB)
{
    Logger_SendGeneralCmd(NULL, 0, e_LOGGER_CMD_DUMP_DEBUG_LOG, FlashSizeKB);
}

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
void Logger_SendConfigToSrvApi(uint8 ConfigType)
{
   Logger_SendGeneralCmd(NULL, 0, e_LOGGER_CMD_SEND_CONF_TO_SRV, ConfigType); 
}

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
void Logger_SetConfigFromSrvApi(uint8 ConfigType, void* pConfig)
{
    void *pInputBuff;
    uint16 Len;
    switch((LoggerConfigType_e)ConfigType)
    {
        case e_LOGGER_CONF_TYPE_GENERAL:
            Len = sizeof(LoggerGenConfig_t);
            pInputBuff = OSAL_HeapAllocBuffer(OSAL_GetSelfTaskId(), Len);
            OSAL_MemCopy(pInputBuff, pConfig, Len);
        break;
        case e_LOGGER_CONF_TYPE_SOCKET:
            Len = sizeof(LoggerSocketConfig_t);
            pInputBuff = OSAL_HeapAllocBuffer(OSAL_GetSelfTaskId(), Len);
            OSAL_MemCopy(pInputBuff, pConfig, Len);
        break;
        case e_LOGGER_CONF_TYPE_FTP:
            Len =  sizeof(LoggerFtpConfig_t);
            pInputBuff = OSAL_HeapAllocBuffer(OSAL_GetSelfTaskId(), Len);
            OSAL_MemCopy(pInputBuff, pConfig, Len);
        break;
    }
    Logger_SendGeneralCmd(pInputBuff, Len, e_LOGGER_CMD_SET_CONFIG, ConfigType);
}

