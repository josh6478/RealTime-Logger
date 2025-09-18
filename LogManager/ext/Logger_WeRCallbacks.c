#include "Logger_Defs.h"
#include "test_api.h"
#include "Uart_Api.h"
#include "Flash_Api.h"
#include "EEPROM_API.h"
#include "clock.h"
#include "SSP_SPI_API.h"
#include "util.h"
#include "ComManager_API.h"
#include "I2C_API.h"
#include "FtpPutManager_API.h"
#include "EEPROM_Api.h"


static LOGGER_STATUS WeRLogger_SendUartDataCb(char *pStr, uint16 StrLen)
{
    uint16 Handle;
    
    if(!TEST_IsTestApplicationActive())
    {
        return LOGGER_STATUS_BUSY;
    }
#if 0    
    if((*pStr < 10) || (*pStr > 126 && *pStr != LOGGER_CONFIG_BUFFER_OVERRUN_MARK_CHAR && *pStr != 0xFF))
    {
        OSAL_ASSERT(0);
    }
#endif    
    /* send the message to uart task, I use the test application task ID,
    ofcourse the test moudle (program 90) needs to be already initialized at this stage */
    if(UART_SendMessage ( TASK_APP_TEST_MANAGER_ID, 
        TEST_UART_DEVICE, 
        UART_TX_ATTR_DONT_FREE_BUFFER | 
        UART_TX_ATTR_DONT_SEND_COMPLETION_MSG | 
        UART_TX_ATTR_MASK_ALL_ACK_MSG |
        UART_TX_ATTR_WRITE_FROM_INTERRUPT, 
        CURRENT_ACQUIRE_HANDLE, 
        (uint8 *)pStr, 
        StrLen, 
        &Handle) != OSAL_STATUS_OK)
    {
        return LOGGER_STATUS_UART_WRITE_ERROR;
    }
    
    return LOGGER_STATUS_OK;
}

/**
 * <pre>
 * static LOGGER_STATUS WeRLogger_SendFlashDataCb(uint32 Address, char *pStr, uint16* pStrLen)
 * </pre>
 *  
 * this function parse a customized printf style RX log message and write into into a cyclic buffer
 *
 * @param   Address      [in]       The input address in FLASH to write to
 * @param   pStr         [in]       a pointer to input string buffer
 * @param   ap           [inout]    a pointer string buffer length
 *
 * @return LOGGER_STATUS_OK on success or negative value otherwise
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static LOGGER_STATUS WeRLogger_SendFlashDataCb(uint32 Address, char *pStr, uint16* pStrLen)
{
#define FLASH_WRITE_CHUNK_SIZE  256    
    uint16 DataWriten = 0;
    uint16 DataToWrite = *pStrLen;
    uint8 PageOffsetAddress = Address&0xFF;
    
    LOGGER_STATUS Status = LOGGER_STATUS_OK;
    
    if( (PageOffsetAddress) &&  (PageOffsetAddress + DataToWrite > FLASH_WRITE_CHUNK_SIZE))
    {
        
        DataToWrite = FLASH_WRITE_CHUNK_SIZE - PageOffsetAddress;
        DataWriten = FLASH_WriteFromInt( SSP1, (uint8 *)pStr, DataToWrite, Address);
        
        if(DataWriten != DataToWrite)
        {
            Status = LOGGER_STATUS_BUSY;
            goto EXIT;
        }
        DataToWrite = (*pStrLen) - DataWriten;
    }
    
    DataWriten += FLASH_WriteFromInt( SSP1, (uint8 *)pStr + DataWriten, DataToWrite, Address);
    
    if(DataWriten != *pStrLen)
    {
        Status = LOGGER_STATUS_BUSY;
    }
EXIT:
    *pStrLen = DataWriten;        
    return Status;    
}

static LOGGER_STATUS WeRLogger_EraseFlashSectorCb(uint32 StartAddress, uint8 NumOfSectors, uint16 FlashSize)
{
    uint32 Ret;
    
    volatile uint32 Status;
    /* calculate end address */
    uint32 EndAddress = StartAddress + NumOfSectors*(LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB<<10);
    
    /* sanity check make sure we don't exit the allowed flash bounderies */
    if(StartAddress < LOGGER_CONFIG_FLASH_START_ADDRESS  || StartAddress >= EndAddress || NumOfSectors > LOGGER_DEF_FLASH_NUM_OF_DEBUG_SECTORS(FlashSize))
    {
        OSAL_ASSERT(FALSE);
        return LOGGER_STATUS_ERROR_PARAM;
    }
    for(uint8 i =0; i< NumOfSectors; i++)
    {
        /* TODO: need to fix the attributes - pass it as an argument to the cb function */
        Ret = FLASH_SectorErase(SSP1, 0, StartAddress, 0, (uint32*)&Status, NULL);
        StartAddress += (LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB<<10);
        /* poll shortly on FLASH status erase command, we should poll relative to the maximum timeout the erase sector takes */
        for(uint8 j = 0; Status != OSAL_STATUS_OK && j < 3; j++)
        {
            OSAL_SuspendTask(LOGGER_CONFIG_MAX_SECTOR_ERASE_TIMEOUT_MS/3);
        }
    }
        /* sanity check */
    if(Status != OSAL_STATUS_OK)
    {
        OSAL_ASSERT(FALSE);
        return LOGGER_STATUS_FLASH_ERASE_ERROR;
    }

    if(Ret == OSAL_STATUS_OK)
    {
        return LOGGER_STATUS_OK;
    }
    else if (Ret == FLASH_TRANSMISSION_QUEUE_FULL)
    {
        return LOGGER_STATUS_BUSY;
    }
    
    return LOGGER_STATUS_FLASH_ERASE_ERROR;
}

static LOGGER_STATUS WeRLogger_OpenSocketCb(LoggerSocketConfig_t *pSocketConfig)
{
    /* TODO: complete this function */
    ComSocketIpPort_s SocketStruct;
    uint8 DnsLen = LOGGER_DEF_MIN(LOGGER_CONFIG_SOCKET_MAX_IP_LEN, COM_MANAGER_IP_MAX_LEN);
    CoM_SocketType_e SocketType = pSocketConfig->SocketType == LOGGER_SOCKET_TYPE_TCP? COM_MANAGER_SOCKET_TYPE_TCP:COM_MANAGER_SOCKET_TYPE_UDP;
    /* fill in the socket strcuture info */
    OSAL_StrCopy((char*)SocketStruct.serverIp, pSocketConfig->DomainNameIp, DnsLen);
    
    SocketStruct.serverIpLen = OSAL_StrLen((char*)SocketStruct.serverIp, DnsLen);
    
    SocketStruct.serverPortLen = OSAL_num2decstr(pSocketConfig->RemotePort, SocketStruct.serverPort, COM_MANAGER_PORT_LEN);
    
    
    if(CoM_OpenSocket_API(/* void* */               &SocketStruct, 
                       /*ComRxCallBackFuncType*/ NULL,
                       /*CoM_SessionType_e*/     COM_MANAGER_SESSION_TYPE_DEBUG,
                       /*CoM_SocketType_e*/      SocketType,
                       /*CoM_SocketDirection_e*/ COM_SOCKET_DIRECTION_TX_ONLY,
                       /*CoM_SocketRxPriority_e*/COM_RX_PRIORITY_1,
                       /*CoM_SocketFlags_e*/     COM_SOCKET_FLAGS_OPEN_USING_ETH | COM_SOCKET_FLAGS_AUTO_REOPEN,
                       /*Attributes*/            COM_ATTR_SEND_COMPLETION_MSG|COM_ATTR_ALLOW_SEND_NACK_MSG,
                       /*OpenHandle*/            0) != OSAL_STATUS_OK)
    {
        //OSAL_ASSERT(0);
        return LOGGER_STATUS_OPEN_SOCKET_FAILED;
    }
    return LOGGER_STATUS_OK;
}

static LOGGER_STATUS WeRLogger_ReadNVRParamsCb(uint32 Address, void *pBuff, uint8 SizeBytes)
{
    
    if(EepromReadPoll (Address, (uint8*)pBuff, SizeBytes, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    return LOGGER_STATUS_OK;
}


static LOGGER_STATUS WeRLogger_GetDateAndTimeCb(LoggerDateAndTime_t *pDateAndTime)
{
    if(Clock_GetTimeAndDate((ClockTimeAndDate_t*)pDateAndTime) == TRUE)
    {
        return LOGGER_STATUS_OK;
    }
    return LOGGER_STATUS_CLOCK_ERROR;
}

static LOGGER_STATUS WeRLogger_GetPanelIdCb(char PanelIdStr[LOGGER_CONFIG_PANEL_ID_SIZE_BYTES])
{
    /* I know this is an ugly code, but I'm OK with it for now */
    static UINT8  PanelNo[4];
    uint8 StrIdx = 0;
    extern void RfSys_CopyPanelSerialNo ( UINT8 *pDest, int IsComplete );
    
    RfSys_CopyPanelSerialNo ( PanelNo, TRUE);
    
    for (uint8 i = 0; i < sizeof(PanelNo); i++ )
    {
        PanelIdStr[StrIdx++] = hex2ascii(PanelNo[i] >>  4);
        PanelIdStr[StrIdx++] = hex2ascii(PanelNo[i] & 0xF);
    }
    
    return LOGGER_STATUS_OK;
}

static LOGGER_STATUS WeRLogger_ReadFlashDataCb(uint32 Address, char *pStr, uint16 StrLen)
{
    volatile uint32 Status = (uint32)-1;
    
    if(FLASH_Read(SSP1, (uint8*)pStr, StrLen, 0, Address, 0, (uint32*)&Status, NULL) != OSAL_STATUS_OK)
    {
        OSAL_ASSERT(0);
    }
    
    OSAL_SuspendTask(10);
    
    for(uint8 i=0; Status != OSAL_STATUS_OK && i<100; i++)
    {
        OSAL_SuspendTask(50);        
    }
    if(Status == OSAL_STATUS_OK)
    {
        return LOGGER_STATUS_OK;
    }
    return LOGGER_STATUS_FLASH_READ_ERROR;
}

static LOGGER_STATUS WeRLogger_SaveNVRParamsCb(uint32 Address, void *pBuff, uint8 SizeBytes)
{
    /* sanity check */
    if(Address < LOGGER_CONFIG_NVR_ADDRESS || Address + SizeBytes > LOGGER_CONFIG_NVR_ADDRESS + LOGGER_CONFIG_NVR_SIZE_BYTES)
    {
        OSAL_ASSERT(0);
        return LOGGER_STATUS_ERROR_PARAM;
    }
    
    if(EEPROM_WriteFromInt((uint8 *)pBuff, SizeBytes, Address) == FALSE)
    {
        return LOGGER_STATUS_EEPROM_WRITE_ERROR;
    }
    return LOGGER_STATUS_OK;
}

static LOGGER_STATUS WeRLogger_SendSocketDataCb(uint8 *pBuff, uint16 Len, uint8 Mode)
{
    uint16 RetVal;
    
    if(CoM_IsSessionActive_API( COM_MANAGER_SESSION_TYPE_DEBUG ))
    {
            RetVal = CoM_SendMsgDataFromInt_API(pBuff, Len, 0, COM_ATTR_DONT_FREE_BUFFER|COM_ATTR_DONT_SEND_COMPLETION_MSG|COM_ATTR_DONT_ALLOW_SEND_NACK_MSG|COM_ATTR_DONT_FREE_BUFFER_ON_FAILURE,
                        COM_MANAGER_SESSION_TYPE_DEBUG, COM_SEND_DATA_LOW_PRIORITY, 0);
            
        if(RetVal != COM_MANAGER_FUNC_ERR_OK)
        {
            if(RetVal == COM_MANAGER_FUNC_ERR_MODEM_BUSY)
            {
                return LOGGER_STATUS_BUSY;
            }
            else
            {                
                return LOGGER_STATUS_SOCKET_SEND_ERROR;
            }
        }
    }
    else
    {
        return LOGGER_STATUS_SOCKET_SEND_ERROR;        
    }
    return LOGGER_STATUS_OK;
}

static LOGGER_STATUS WeRLogger_SendFtpFileCb(LoggerSocketConfig_t *pSocketConfig, LoggerFtpConfig_t *pFtpConfig, uint32 Address, 
                                             int FileSize, char FileName[], uint8 FileNameLen)
{

    uint8 TaskId = OSAL_GetSelfTaskId();
    ComFtpData_s *pFtpStruct = (ComFtpData_s *)OSAL_HeapAllocBuffer(20, sizeof(ComFtpData_s));
    ComSocketIpPort_s *pSocketStruct = (ComSocketIpPort_s *)OSAL_HeapAllocBuffer(TaskId, sizeof(ComSocketIpPort_s));
    /* default size for ',' in size */
    uint8 UserNamePassSize;
    uint8 UserNameSize;
    uint8 PasswordSize;
    
    char *pItr;
    
    if(pSocketStruct == NULL || pFtpStruct == NULL)
    {
        OSAL_ASSERT(FALSE);
        return LOGGER_STATUS_SEND_LOG_FILE_FAILED;
        
    }
    
    pSocketStruct->serverIpLen = OSAL_StrLen(pSocketConfig->DomainNameIp, LOGGER_CONFIG_SOCKET_MAX_IP_LEN);
    OSAL_StrCopy((char*)pSocketStruct->serverIp, pSocketConfig->DomainNameIp, pSocketStruct->serverIpLen);
    //pSocketStruct->serverPortLen = Num2Str(pSocketConfig->RemotePort, pSocketStruct->serverPort, LOGGER_CONFIG_SOCKET_MAX_IP_LEN);
    pSocketStruct->serverPortLen = 0;
    pSocketStruct->SourcePortLen = 0;
    
    pFtpStruct->FtpSocketData = pSocketStruct;
    UserNameSize = OSAL_StrLen(pFtpConfig->Username, LOGGER_CONFIG_FTP_MAX_USER_NAME_LEN);
    PasswordSize = OSAL_StrLen(pFtpConfig->Password, LOGGER_CONFIG_FTP_MAX_PASSWORD_LEN);
    UserNamePassSize = UserNameSize + PasswordSize + 1; /* +1 for ',' */
    pFtpStruct->pUserPassword = (uint8*)OSAL_HeapAllocBuffer(TaskId, UserNamePassSize);
    if(pFtpStruct->pUserPassword == NULL)
    {
        OSAL_ASSERT(FALSE);
        OSAL_HeapFreeBuff((uint32**)&pFtpStruct);
        OSAL_HeapFreeBuff((uint32**)&pSocketStruct);
        return LOGGER_STATUS_SEND_LOG_FILE_FAILED;
    }
    pItr = OSAL_StrCopy((char*)pFtpStruct->pUserPassword, pFtpConfig->Username, UserNameSize);
    *pItr++ = ',';
    OSAL_StrCopy(pItr, pFtpConfig->Password, PasswordSize);
    pFtpStruct->pFilePath = (uint8*)OSAL_HeapAllocBuffer(TaskId, LOGGER_CONFIG_FTP_MAX_PATH_LEN + FileNameLen);
    if(pFtpStruct->pFilePath == NULL)
    {
        OSAL_ASSERT(FALSE);
        OSAL_HeapFreeBuff((uint32**)&pFtpStruct->pFilePath);
        OSAL_HeapFreeBuff((uint32**)&pFtpStruct);
        OSAL_HeapFreeBuff((uint32**)&pSocketStruct);
        return LOGGER_STATUS_SEND_LOG_FILE_FAILED;
    }
    pItr = OSAL_StrCopy((char*)pFtpStruct->pFilePath, pFtpConfig->Path, LOGGER_CONFIG_FTP_MAX_PATH_LEN) -1; /* -1 to avoid the null terminator */
    *pItr++ = '\\';
    pItr = OSAL_StrCopy(pItr, FileName, FileNameLen);
    pFtpStruct->FilePathLen = (uint8*)pItr - pFtpStruct->pFilePath;
    
    pFtpStruct->FileSize = FileSize;
    pFtpStruct->userPasswordLen = UserNamePassSize;
    pFtpStruct->FtpDataAddress = Address;
    
    if(FtpPutMng_SendDataFromFlash_API(COM_MANAGER_SESSION_TYPE_DEBUG, Address, FileSize, pFtpStruct, 
                                    COM_MANAGER_SOCKET_TYPE_FTP, Logger_UploadLogFile, 
                                    FPM_ATTR_FREE_BUFFER|FPM_ATTR_SEND_COMPLETION_MSG|FPM_ATTR_ALLOW_SEND_NACK_MSG, NULL) != FPM_MANAGER_STATUS_OK)
    {
        OSAL_HeapFreeBuff((uint32**)&pFtpStruct->pFilePath);
        OSAL_HeapFreeBuff((uint32**)&pSocketStruct);
        OSAL_HeapFreeBuff((uint32**)&pFtpStruct->pFilePath);
        OSAL_HeapFreeBuff((uint32**)&pFtpStruct);
        return LOGGER_STATUS_SEND_LOG_FILE_FAILED;
    }
    return LOGGER_STATUS_OK;
}

static LOGGER_STATUS LoggerReadGeneralConfig(LoggerGenConfig_t *pConfig)
{
    /* DB configuration */    
    if(EepromReadPoll (EEPROM_LOG_IS_ENABLED, (uint8*)&pConfig->IsEnabled, 1, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    /* for now we want the enable parameter to reset to the hardcoded configuration on every reset, so we dont read this specific parameter from EEPROM */
    if(EepromReadPoll (EEPROM_LOG_IS_PRINTOUT_ENABLED, (uint8*)&pConfig->IsPrintoutEnabled, 1, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromReadPoll (EEPROM_LOG_DEBUG_MODE, (uint8*)&pConfig->Mode, 1, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromReadPoll (EEPROM_LOG_DEBUG_LEVEL, (uint8*)&pConfig->LogLevel, 1, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromReadPoll (EEPROM_LOG_DEBUG_MASK, (uint8*)&pConfig->ModuleMask, 4, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromReadPoll (EEPROM_LOG_DEBUG_FLASH_SEGMENT_SIZE, (uint8*)&pConfig->FlashSize, 2, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    /* hard coded configuration */
    pConfig->Attributes = LOGGER_CONFIG_DEFAULT_ATTRIBUTES;
    
    if(pConfig->Mode == LOGGER_MODE_TYPE_PULL)
    {
//        pConfig->IsPrintoutEnabled = FALSE;
        
        pConfig->DestType = LOGGER_DEST_TYPE_FLASH;
    }
    else if(pConfig->Mode == LOGGER_MODE_TYPE_PUSH)
    {
        pConfig->DestType = LOGGER_DEST_TYPE_SOCKET;
    }
    else
    {
        pConfig->DestType = LOGGER_DEST_TYPE_NONE;
    }
    pConfig->ResolutionMS = LOGGER_CONFIG_DEFAULT_RESOLUTION_MS;
        
    return LOGGER_STATUS_OK;
}

static LOGGER_STATUS LoggerReadSocketConfig(LoggerSocketConfig_t *pConfig)
{
    /* DB configuration */    
    if(EepromReadPoll (EEPROM_LOG_SOCKET_REMOTE_PORT, (uint8*)&pConfig->RemotePort, 2, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromReadPoll (EEPROM_LOG_SOCKET_REMOTE_PORT, (uint8*)&pConfig->LocalPort, 2, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromReadPoll (EEPROM_LOG_SOCKET_TYPE, (uint8*)&pConfig->SocketType, 1, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromReadPoll (EEPROM_LOG_SOCKET_DOMAIN, (uint8*)&pConfig->DomainNameIp, 64, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromReadPoll (EEPROM_LOG_SOCKET_DOMAIN_LEN, (uint8*)&pConfig->DomainNameLen, 2, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    return LOGGER_STATUS_OK;
}

static LOGGER_STATUS LoggerReadFTPConfig(LoggerFtpConfig_t *pConfig)
{
    
    /* DB configuration */    
    if(EepromReadPoll (EEPROM_LOG_FTP_USERNAME, (uint8*)&pConfig->Username, 11, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromReadPoll (EEPROM_LOG_FTP_PASSWORD, (uint8*)&pConfig->Password, 11, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromReadPoll (EEPROM_LOG_FTP_PATH, (uint8*)&pConfig->Path, 64, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    /* hard coded configuration */
    pConfig->Attributes = 0;
    
    pConfig->FileSize = LOGGER_DEF_GET_ALL_FLASH_SIZE_KB;
    
    return LOGGER_STATUS_OK;
}

static LOGGER_STATUS WerLogger_ReadConfigCb(LoggerConfigType_e ConfigType, void *pConfig)
{
    switch(ConfigType)
    {
        case e_LOGGER_CONF_TYPE_GENERAL:
        {
            return LoggerReadGeneralConfig((LoggerGenConfig_t *)pConfig);
        }
                
        case e_LOGGER_CONF_TYPE_SOCKET:
        {
            return LoggerReadSocketConfig((LoggerSocketConfig_t*)pConfig);
        }        
        
        case e_LOGGER_CONF_TYPE_FTP:
        {
            return LoggerReadFTPConfig((LoggerFtpConfig_t*)pConfig);
        }        
        
        default:
            OSAL_ASSERT(0);
            return LOGGER_STATUS_INVALID_PARAM;
        
    }
    
}

static LOGGER_STATUS LoggerWriteGeneralConfig(LoggerGenConfig_t *pConfig)
{
    /* DB configuration */    
    if(EepromWritePoll (EEPROM_LOG_IS_ENABLED, (uint8*)&pConfig->IsEnabled, 1, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromWritePoll (EEPROM_LOG_IS_PRINTOUT_ENABLED, (uint8*)&pConfig->IsPrintoutEnabled, 1, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromWritePoll (EEPROM_LOG_DEBUG_MODE, (uint8*)&pConfig->Mode, 1, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromWritePoll (EEPROM_LOG_DEBUG_LEVEL, (uint8*)&pConfig->LogLevel, 1, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromWritePoll (EEPROM_LOG_DEBUG_MASK, (uint8*)&pConfig->ModuleMask, 4, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromWritePoll (EEPROM_LOG_DEBUG_FLASH_SEGMENT_SIZE, (uint8*)&pConfig->FlashSize, 2, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    return LOGGER_STATUS_OK;
}

static LOGGER_STATUS LoggerWriteSocketConfig(LoggerSocketConfig_t *pConfig)
{
    /* DB configuration */    
    if(EepromWritePoll (EEPROM_LOG_SOCKET_REMOTE_PORT, (uint8*)&pConfig->RemotePort, 2, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromWritePoll (EEPROM_LOG_SOCKET_REMOTE_PORT, (uint8*)&pConfig->LocalPort, 2, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromWritePoll (EEPROM_LOG_SOCKET_TYPE, (uint8*)&pConfig->SocketType, 1, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromWritePoll (EEPROM_LOG_SOCKET_DOMAIN, (uint8*)&pConfig->DomainNameIp, 64, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromWritePoll (EEPROM_LOG_SOCKET_DOMAIN_LEN, (uint8*)&pConfig->DomainNameLen, 2, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    return LOGGER_STATUS_OK;
}

static LOGGER_STATUS LoggerWriteFTPConfig(LoggerFtpConfig_t *pConfig)
{
    
    /* DB configuration */    
    if(EepromWritePoll (EEPROM_LOG_FTP_USERNAME, (uint8*)&pConfig->Username, 11, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromWritePoll (EEPROM_LOG_FTP_PASSWORD, (uint8*)&pConfig->Password, 11, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    if(EepromWritePoll (EEPROM_LOG_FTP_PATH, (uint8*)&pConfig->Path, 64, 0, 10, 500)!=TRUE)
    {
        return LOGGER_STATUS_NVR_READ_ERROR;
    }
    return LOGGER_STATUS_OK;
}

static LOGGER_STATUS WerLogger_WriteConfigCb(LoggerConfigType_e ConfigType, void *pConfig)
{
    switch(ConfigType)
    {
        case e_LOGGER_CONF_TYPE_GENERAL:
        {
            return LoggerWriteGeneralConfig((LoggerGenConfig_t *)pConfig);
        }
                
        case e_LOGGER_CONF_TYPE_SOCKET:
        {
            return LoggerWriteSocketConfig((LoggerSocketConfig_t*)pConfig);
        }        
        
        case e_LOGGER_CONF_TYPE_FTP:
        {
            return LoggerWriteFTPConfig((LoggerFtpConfig_t*)pConfig);
        }        
        
        default:
            OSAL_ASSERT(0);
            return LOGGER_STATUS_INVALID_PARAM;
        
    }
}

/* below is the logger callback function. every platform needs to fill in its own implementation to every callback so the feature can work */

LoggerCB_t gLoggerCbList = {
    WeRLogger_OpenSocketCb,
    NULL,
    WeRLogger_SendSocketDataCb,
    WeRLogger_SendUartDataCb,
    WeRLogger_SendFlashDataCb,
    WeRLogger_ReadFlashDataCb,
    WeRLogger_EraseFlashSectorCb,
    WeRLogger_SendFtpFileCb,
    WeRLogger_GetDateAndTimeCb,
    WeRLogger_SaveNVRParamsCb,
    WeRLogger_ReadNVRParamsCb,
    WeRLogger_GetPanelIdCb,
    WerLogger_ReadConfigCb,
    WerLogger_WriteConfigCb,
};
