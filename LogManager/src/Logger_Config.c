
#include "Logger_Defs.h"
#include "Logger_Manager.h"

extern LoggerManager_t gLoggerManager;

void Logger_SetDefaultSocketConfig(void)
{
    /* copy default server domain name */
    OSAL_StrCopy(gLoggerManager.Config.Socket.DomainNameIp, LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_DNS, LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_DNS_LEN);
    
    /* set default domain name length (include null terminator) */
    gLoggerManager.Config.Socket.DomainNameLen = LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_DNS_LEN;
    
    /* set remote port */
    gLoggerManager.Config.Socket.RemotePort = LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_PORT;
    
    /* set local port */
    gLoggerManager.Config.Socket.LocalPort = LOGGER_CONFIG_DEFAULT_LOCAL_PORT;
    
    /* set socket type */
    gLoggerManager.Config.Socket.SocketType = LOGGER_SOCKET_TYPE_UDP;
    
}

static BOOL Logger_SetGeneralDefaultConfig(void)
{   
    
    /* set printout default config */
    gLoggerManager.Config.General.IsPrintoutEnabled = LOGGER_CONFIG_DEFAULT_IS_PRINTOUT_ENABLED;
    
    /* set logger mode default config */
    gLoggerManager.Config.General.Mode = LOGGER_CONFIG_DEFAULT_MODE_TYPE;
    
    /* set logger default level */
    gLoggerManager.Config.General.LogLevel = LOGGER_CONFIG_DEFAULT_LOG_LEVEL;
    
    /* set logger default mask */
    gLoggerManager.Config.General.ModuleMask = LOGGER_CONFIG_DEFAULT_LOG_MASK;
    
    /* set logger default destination type */
    gLoggerManager.Config.General.DestType = LOGGER_CONFIG_DEFAULT_DESTINATION_TYPE;
    
    /* set logger default flash size */
    gLoggerManager.Config.General.FlashSize = LOGGER_CONFIG_DEFAULT_FILE_SIZE_KB;
    
    /* set logger default resoluton */
    gLoggerManager.Config.General.ResolutionMS = LOGGER_CONFIG_DEFAULT_RESOLUTION_MS;
    
    /* set logger default attributes*/
    gLoggerManager.Config.General.Attributes = LOGGER_CONFIG_DEFAULT_ATTRIBUTES;
    
    gLoggerManager.Config.General.IsEnabled =  LOGGER_CONFIG_DEFAULT_FEATURE_IS_ENABLED;
    
    return LOGGER_CONFIG_DEFAULT_FEATURE_IS_ENABLED;
}

#if 0
static void Logger_SetFlashConfig(LoggerFLASHConfig_t *pFlashConfig)
{
    if(pFlashConfig == NULL)
    {
        /* configure default FLASH size */
        gLoggerManager.Config.Flash.FlashSegmentSizeKB = LOGGER_CONFIG_DEFAULT_FLASH_SEGMENT_SIZE_KB;
    }
    else
    {
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_FLASH_CONFIG.FlashSegmentSizeKB, 
                              pFlashConfig->FlashSegmentSizeKB, 0xFFFF, LOGGER_CONFIG_DEFAULT_FLASH_SEGMENT_SIZE_KB);
    }
}
#endif

static void Logger_SetSocketDefaultConfig(void)
{
    LOGGER_DEF_SOCKET_CONFIG.DomainNameLen = LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_DNS_LEN;
    
    LOGGER_DEF_SOCKET_CONFIG.RemotePort = LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_PORT;
    
    LOGGER_DEF_SOCKET_CONFIG.LocalPort = LOGGER_CONFIG_DEFAULT_LOCAL_PORT;
    
    LOGGER_DEF_SOCKET_CONFIG.SocketType = LOGGER_CONFIG_DEFAULT_SOCKET_TYPE;
    
    OSAL_StrCopy(LOGGER_DEF_SOCKET_CONFIG.DomainNameIp, LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_DNS, 
                         LOGGER_CONFIG_SOCKET_MAX_IP_LEN);    
}

static void Logger_SetRAMSocketConfig(LoggerSocketConfig_t *pSocketConfig)
{
    /* sanity check */
    if(pSocketConfig == NULL)
    {
        Logger_SetSocketDefaultConfig();
    }
    else
    {
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_SOCKET_CONFIG.DomainNameLen, pSocketConfig->DomainNameLen, 0xFF, LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_DNS_LEN);
        
        if(pSocketConfig->DomainNameIp[0] == 0xFF)
        {
            OSAL_StrCopy(LOGGER_DEF_SOCKET_CONFIG.DomainNameIp, LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_DNS, 
                         LOGGER_CONFIG_SOCKET_MAX_IP_LEN);
        }
        else
        {
            OSAL_StrCopy(LOGGER_DEF_SOCKET_CONFIG.DomainNameIp, pSocketConfig->DomainNameIp, LOGGER_CONFIG_SOCKET_MAX_IP_LEN);
        }
        
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_SOCKET_CONFIG.RemotePort, pSocketConfig->RemotePort, 0xFFFF, LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_PORT);
        
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_SOCKET_CONFIG.LocalPort, pSocketConfig->LocalPort, 0xFFFF, LOGGER_CONFIG_DEFAULT_LOCAL_PORT);
        
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_SOCKET_CONFIG.SocketType, pSocketConfig->SocketType, 0xFF, LOGGER_CONFIG_DEFAULT_SOCKET_TYPE);
        
    }
}

static void Logger_SetFtpDefaultConfig(void)
{
    OSAL_StrCopy(LOGGER_DEF_FTP_CONFIG.Username, LOGGER_CONFIG_DEFAULT_FTP_USERNAME, LOGGER_CONFIG_FTP_MAX_USER_NAME_LEN);
    
    OSAL_StrCopy(LOGGER_DEF_FTP_CONFIG.Password, LOGGER_CONFIG_DEFAULT_FTP_PASSWORD, LOGGER_CONFIG_FTP_MAX_PASSWORD_LEN);
    
    OSAL_StrCopy(LOGGER_DEF_FTP_CONFIG.Path, LOGGER_CONFIG_DEFAULT_FTP_PATH, LOGGER_CONFIG_FTP_MAX_PATH_LEN);
    
    LOGGER_DEF_FTP_CONFIG.FileSize = LOGGER_CONFIG_DEFAULT_FILE_SIZE_KB;
    
    LOGGER_DEF_FTP_CONFIG.Attributes = LOGGER_CONFIG_DEFAULT_ATTRIBUTES;
}


static void Logger_SetRAMFtpConfig(LoggerFtpConfig_t *pLoggerFtpConfig)
{
    if(pLoggerFtpConfig == NULL || pLoggerFtpConfig->Username[0] == 0xFF ||
       pLoggerFtpConfig->Password[0] == 0xFF || pLoggerFtpConfig->Path[0] == 0xFF)
    {
        Logger_SetFtpDefaultConfig();
    }
    else
    {
        OSAL_StrCopy(LOGGER_DEF_FTP_CONFIG.Username, pLoggerFtpConfig->Username, LOGGER_CONFIG_FTP_MAX_USER_NAME_LEN);
        
        OSAL_StrCopy(LOGGER_DEF_FTP_CONFIG.Password, pLoggerFtpConfig->Password, LOGGER_CONFIG_FTP_MAX_PASSWORD_LEN);
        
        OSAL_StrCopy(LOGGER_DEF_FTP_CONFIG.Path, pLoggerFtpConfig->Path, LOGGER_CONFIG_FTP_MAX_PATH_LEN);

    }
}

BOOL Logger_SetRAMGeneralConfig(LoggerGenConfig_t *pGeneralConfig)
{
    
    /* sanity check */
    if(pGeneralConfig == NULL)
    {
        Logger_SetGeneralDefaultConfig();
    }
    else
    {
                
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_GEN_CONFIG.DestType, pGeneralConfig->DestType, 0xFF, LOGGER_CONFIG_DEFAULT_DESTINATION_TYPE);        
        
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_GEN_CONFIG.IsPrintoutEnabled, pGeneralConfig->IsPrintoutEnabled, 0xFF, LOGGER_CONFIG_DEFAULT_IS_PRINTOUT_ENABLED);            
        
        /* module mask input value 0xFF is a valid input value */
        LOGGER_DEF_GEN_CONFIG.ModuleMask = pGeneralConfig->ModuleMask;
        
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_GEN_CONFIG.LogLevel, pGeneralConfig->LogLevel, 0xFF, LOGGER_CONFIG_DEFAULT_LOG_LEVEL);            
        
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_GEN_CONFIG.Mode, pGeneralConfig->Mode, 0xFF, LOGGER_CONFIG_DEFAULT_MODE_TYPE);
        
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_GEN_CONFIG.FlashSize, pGeneralConfig->FlashSize, 0xFFFF, LOGGER_CONFIG_DEFAULT_FILE_SIZE_KB);
        
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_GEN_CONFIG.Attributes, pGeneralConfig->Attributes, 0xFF, LOGGER_CONFIG_DEFAULT_ATTRIBUTES);
        
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_GEN_CONFIG.ResolutionMS, pGeneralConfig->ResolutionMS, 0xFFFF, LOGGER_CONFIG_DEFAULT_RESOLUTION_MS);
        
        if(pGeneralConfig->Mode == LOGGER_MODE_TYPE_PUSH)
        {
            LOGGER_DEF_GEN_CONFIG.DestType = LOGGER_DEST_TYPE_SOCKET;
        }
        else if(pGeneralConfig->Mode == LOGGER_MODE_TYPE_PULL)
        {
            LOGGER_DEF_GEN_CONFIG.DestType = LOGGER_DEST_TYPE_FLASH;            
        }
        else
        {
            LOGGER_DEF_GEN_CONFIG.DestType = LOGGER_DEST_TYPE_NONE;                        
        }
        /* the last action would be to enable\disable the logger */
        LOGGER_DEF_SET_CONFIG(LOGGER_DEF_GEN_CONFIG.IsEnabled, pGeneralConfig->IsEnabled, 0xFF, LOGGER_CONFIG_DEFAULT_FEATURE_IS_ENABLED);        
    }
    return LOGGER_DEF_GEN_CONFIG.IsEnabled;
}

BOOL Logger_SetConfig(LoggerConfig_t *pLoggerConfig)
{
    BOOL IsOn;
    /* sanity check */
    if(pLoggerConfig == NULL)
    {
        if(gLoggerManager.pCbList->ReadConfigCb != NULL)            
        {
            LoggerFtpConfig_t *pDBFtpConfig = (LoggerFtpConfig_t*)OSAL_HeapAllocBuffer(OSAL_GetSelfTaskId(), sizeof(LoggerFtpConfig_t));
            LoggerSocketConfig_t *pDBSocketConfig = (LoggerSocketConfig_t*)OSAL_HeapAllocBuffer(OSAL_GetSelfTaskId(), sizeof(LoggerSocketConfig_t));
            LoggerGenConfig_t *pDBGeneralConfig = (LoggerGenConfig_t*)OSAL_HeapAllocBuffer(OSAL_GetSelfTaskId(), sizeof(LoggerGenConfig_t));
            
            if(!pDBFtpConfig||!pDBSocketConfig||!pDBGeneralConfig)
            {
                OSAL_ASSERT(FALSE);
                return FALSE;
            }
            if(gLoggerManager.pCbList->ReadConfigCb(e_LOGGER_CONF_TYPE_GENERAL, pDBGeneralConfig) == LOGGER_STATUS_OK)
            {
                IsOn = Logger_SetRAMGeneralConfig(pDBGeneralConfig);                                 
            }
            else
            {
                IsOn = Logger_SetGeneralDefaultConfig();
            }
            if(gLoggerManager.pCbList->ReadConfigCb(e_LOGGER_CONF_TYPE_SOCKET, pDBSocketConfig) == LOGGER_STATUS_OK)
            {
                Logger_SetRAMSocketConfig(pDBSocketConfig);                
            }
            else
            {
                Logger_SetDefaultSocketConfig();                
            }
            if(gLoggerManager.pCbList->ReadConfigCb(e_LOGGER_CONF_TYPE_FTP, pDBFtpConfig) == LOGGER_STATUS_OK)
            {
                Logger_SetRAMFtpConfig(pDBFtpConfig);                
            }
            else
            {
                Logger_SetFtpDefaultConfig();                
            }
            OSAL_HeapFreeBuff((uint32**)&pDBFtpConfig);
            OSAL_HeapFreeBuff((uint32**)&pDBSocketConfig);
            OSAL_HeapFreeBuff((uint32**)&pDBGeneralConfig);
        }
        else
        {
            IsOn = Logger_SetGeneralDefaultConfig();
                    
            Logger_SetDefaultSocketConfig();

            Logger_SetFtpDefaultConfig();
        }
        
    }
    else
    {
        IsOn = Logger_SetRAMGeneralConfig(&pLoggerConfig->General);
                    
        Logger_SetRAMSocketConfig(&pLoggerConfig->Socket);
        
        Logger_SetRAMFtpConfig(&pLoggerConfig->Ftp);
    }
    return IsOn;
}
