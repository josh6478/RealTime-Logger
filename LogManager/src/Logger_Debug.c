#include "Logger_Defs.h"
#include "Logger_Manager.h"
#include "Dbg_Print.h"
#include "Logger_Debug.h"
#include "Logger_Api.h"

extern LoggerManager_t gLoggerManager;

void LoggerDebug_PrintRAMstatus(void)
{
#define FLASH_MNG   gLoggerManager.FlashMng

    extern char *LoggerStateStr[];
    
    Printf("\n\nLogger statistics:\n_______________\n\n");
    
    Printf("TxCounter: %d\nRxCnt: %d\nBusyCnt: %d\nLowMemCnt: %d\nRunOverCnt: %d\n\n", 
           gLoggerManager.RxBuffer.TxCounter, gLoggerManager.RxBuffer.RxCounter, 
           gLoggerManager.RxBuffer.BusyCnt, gLoggerManager.RxBuffer.LowMemoryCnt, 
           gLoggerManager.RxBuffer.RunOverCnt);
    
    Printf("\n\nPointer Status:\n____________\n\npHead: 0x%p\n",  gLoggerManager.RxBuffer.pHead);
    
    Printf("pTail: 0x%p:\npRead: 0x%p\npWrite: 0x%p\n\n",
          gLoggerManager.RxBuffer.pTail, gLoggerManager.RxBuffer.pRead, gLoggerManager.RxBuffer.pWrite);
    
    Printf("\n\nFlash Status:\n_____________\n\n");
    
    Printf("CurrSector: 0x%x\nWriteAddr: 0x%x\nReadAddr: 0x%x\n", FLASH_MNG.CurrSectorAddr, FLASH_MNG.WriteAddr, FLASH_MNG.ReadAddr);
    Printf("UsedSize: %d\nFlags: 0x%x\n CmdStatus: %d\n\n", LOGGER_DEF_FLASH_USAGE_SIZE_BYTES(FLASH_MNG.ReadAddr, FLASH_MNG.WriteAddr, LOGGER_DEF_GEN_CONFIG.FlashSize),
           FLASH_MNG.Flags, FLASH_MNG.CmdStatus);
    
    Printf("Logger State: %s\n", LoggerStateStr[gLoggerManager.State]);
}

void LoggerDebug_PrintConfig(void)
{
#define GEN_CONFIG      gLoggerManager.Config.General
#define FLASH_CONFIG    gLoggerManager.Config.Flash
#define SOCKET_CONFIG   gLoggerManager.Config.Socket
#define FTP_CONFIG      gLoggerManager.Config.Ftp    
    
    extern char *LoggerLevelTableStr[];
    extern char *LoggerDestTypeStr[];
    extern char *LoggerModeStr[];
    extern char *LoggerSocketTypeStr[];
    
    Printf("\n\nLogger Gen Config:\n_______________\n\n");
    
    Printf("IsEnabled: %d\nIsPrintout: %d\nDestType: %s\n",
           GEN_CONFIG.IsEnabled, GEN_CONFIG.IsPrintoutEnabled, LoggerDestTypeStr[GEN_CONFIG.DestType]); 
    
    Printf("Mode: %s\nLevel: %s\nMask: 0x%x\n", LoggerModeStr[GEN_CONFIG.Mode], LoggerLevelTableStr[GEN_CONFIG.LogLevel], GEN_CONFIG.ModuleMask);
    
    Printf("TotalSize: %d KB\nSectorNum: %d\nSectorSize: %d KB\nSecEraseThresh: %d KB\n\n", 
        GEN_CONFIG.FlashSize, LOGGER_DEF_FLASH_NUM_OF_DEBUG_SECTORS(GEN_CONFIG.FlashSize), 
        LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB, FLASH_CONFIG_SECTOR_SIZE_THERSHOLD_KB);                
    
    Printf("\nResolution: %d MS\n", GEN_CONFIG.ResolutionMS);
    
    Printf("\n\nLogger Socket Config:\n_________________\n\n");
        
    Printf("IP: %s\n", SOCKET_CONFIG.DomainNameIp);
        
    Printf("RPort: %d\nLPort: %d\nType: %s\n", SOCKET_CONFIG.RemotePort, SOCKET_CONFIG.LocalPort, LoggerSocketTypeStr[SOCKET_CONFIG.SocketType]);
        
    Printf("UserName: %s\nPassword: %s\n", FTP_CONFIG.Username, FTP_CONFIG.Password);
            
    Printf("Path: %s\n\n", FTP_CONFIG.Path);
}


void LoggerDebug_SendCmd(uint8 Cmd, void *pBuff, uint8 BufLen, uint16 UserTag, uint8 DestTaskId)
{
    uint8 TaskId = OSAL_GetSelfTaskId();
    OSAL_Msg *pMessage;
    
    /* build message */
    pMessage               = OSAL_MsgNew();
    pMessage->cmd          = Cmd;
    pMessage->src          = TaskId;
    pMessage->origSrc      = TaskId;
    pMessage->dst          = DestTaskId;
    pMessage->reply        = FALSE;
    pMessage->status       = 0 ;
    pMessage->pDataIn      = pBuff;
    pMessage->DataInLen    = BufLen;
    pMessage-> CntrlBits   = OSAL_MSG_FREE_BY_TARGET;
    pMessage-> pDataOut    = NULL ;
    pMessage-> DataOutLen  = 0 ;
    pMessage-> userTag     = UserTag;
  
    /* send message */
    OSAL_SendMessageToTask(pMessage);


}
#ifdef LOGGER_MANAGER_TEST
static const char * const gLogDebugMessages[] = {
    "LOG DEBUG MSG NUM"
};
#define GET_RAND_LEVEL()   LEVEL_FLOW

OSAL_TASK LogDebugTask1( void * pTaskId )
{
    /* Logger manager task ID */
    uint8 TaskId = OSAL_TASK_EXTRACT_TASK_ID( pTaskId );
    
    uint32 MsgNum = 0;
    
    /* the received message pointer definiton */
    OSAL_Msg * pRxMsg = NULL;
    
    /* This flag indicates wheather logger manager task should be ended */
    BOOL IsTaskEnd = FALSE;
    
    /* ******************** main while loop for receiving a message ***************** */
    while( !IsTaskEnd)
    {
        /* wait for input osal message */
        pRxMsg = OSAL_WaitOnMessage( TaskId );
        
        /* check command type */
         switch(pRxMsg->cmd) {
             
            case e_LOGGER_CMD_DEBUG_START: 
                OSAL_StartTimer(0, pRxMsg->userTag, TRUE);                
            break;
            case e_LOGGER_CMD_DEBUG_STOP:
                OSAL_StopTimer(0);
            break;
            case OSAL_TIMER_MNG_TIMER_EXPIRE_CMD:
                LOG_PRINT(GET_RAND_LEVEL(), "DbgTask 1 %s %d\r\n", gLogDebugMessages[0], ++MsgNum);
            break;
             
         }//end case
         
        ///Free Data In buffer if requested by source task. 
        if( (pRxMsg->CntrlBits & OSAL_MSG_FREE_BY_TARGET) && pRxMsg->pDataIn != NULL)
            OSAL_HeapFreeBuff( (uint32 **)&(pRxMsg->pDataIn) );
        
        OSAL_MsgFree(pRxMsg);
         
    }//while ( !IsTaskEnd)
        
}

OSAL_TASK LogDebugTask2( void * pTaskId )
{
    /* Logger manager task ID */
    uint8 TaskId = OSAL_TASK_EXTRACT_TASK_ID( pTaskId );
    
    uint32 MsgNum = 0;
    
    /* the received message pointer definiton */
    OSAL_Msg * pRxMsg = NULL;
    
    /* This flag indicates wheather logger manager task should be ended */
    BOOL IsTaskEnd = FALSE;
    
    /* ******************** main while loop for receiving a message ***************** */
    while( !IsTaskEnd)
    {
        /* wait for input osal message */
        pRxMsg = OSAL_WaitOnMessage( TaskId );
        
        /* check command type */
         switch(pRxMsg->cmd) {
            
            case e_LOGGER_CMD_DEBUG_START: 
                OSAL_StartTimer(0, pRxMsg->userTag, TRUE);                
            break;
            case e_LOGGER_CMD_DEBUG_STOP:
                OSAL_StopTimer(0);
            break;
            case OSAL_TIMER_MNG_TIMER_EXPIRE_CMD:
                LOG_PRINT(GET_RAND_LEVEL(), "DbgTask 2 %s %d\r\n", gLogDebugMessages[0], ++MsgNum);
            break;
             
         }//end case
         
        ///Free Data In buffer if requested by source task. 
        if( (pRxMsg->CntrlBits & OSAL_MSG_FREE_BY_TARGET) && pRxMsg->pDataIn != NULL)
            OSAL_HeapFreeBuff( (uint32 **)&(pRxMsg->pDataIn) );
        
        OSAL_MsgFree(pRxMsg);
         
    }//while ( !IsTaskEnd)
        
}

OSAL_TASK LogDebugTask3( void * pTaskId )
{
    /* Logger manager task ID */
    uint8 TaskId = OSAL_TASK_EXTRACT_TASK_ID( pTaskId );
    
    uint32 MsgNum = 0;
    
    /* the received message pointer definiton */
    OSAL_Msg * pRxMsg = NULL;
    
    /* This flag indicates wheather logger manager task should be ended */
    BOOL IsTaskEnd = FALSE;
    
    /* ******************** main while loop for receiving a message ***************** */
    while( !IsTaskEnd)
    {
        /* wait for input osal message */
        pRxMsg = OSAL_WaitOnMessage( TaskId );
        
        /* check command type */
         switch(pRxMsg->cmd) {
            
            case e_LOGGER_CMD_DEBUG_START: 
                OSAL_StartTimer(0, pRxMsg->userTag, TRUE);                
            break;
            case e_LOGGER_CMD_DEBUG_STOP:
                OSAL_StopTimer(0);
            break;
            case OSAL_TIMER_MNG_TIMER_EXPIRE_CMD:
                LOG_PRINT(GET_RAND_LEVEL(), "DbgTask 3 %s %d\r\n", gLogDebugMessages[0], ++MsgNum);
            break;
             
         }//end case
         
        ///Free Data In buffer if requested by source task. 
        if( (pRxMsg->CntrlBits & OSAL_MSG_FREE_BY_TARGET) && pRxMsg->pDataIn != NULL)
            OSAL_HeapFreeBuff( (uint32 **)&(pRxMsg->pDataIn) );
        
        OSAL_MsgFree(pRxMsg);
         
    }//while ( !IsTaskEnd)
        
}

OSAL_ERROR_HANDLER LogDebugTaskErrorHandler( uint32 ErrorCode )
{
    return;
}

void LoggerDebug_TestStartStop(uint8 IsStart, uint16 Timeout, uint8 NumOfTasks)
{
    if(IsStart)
    {
        Logger_SwitchOnOffApi(TRUE);
        LoggerDebug_SendCmd(e_LOGGER_CMD_DEBUG_START, 0, 0, Timeout, TASK_APP_LD1_ID);
        
        if(NumOfTasks > 1)
        {
            LoggerDebug_SendCmd(e_LOGGER_CMD_DEBUG_START, 0, 0, Timeout, TASK_APP_LD2_ID);            
        }
        if(NumOfTasks > 2)
        {
            LoggerDebug_SendCmd(e_LOGGER_CMD_DEBUG_START, 0, 0, Timeout, TASK_APP_LD3_ID);
        }
        
    }
    else
    {
        LoggerDebug_SendCmd(e_LOGGER_CMD_DEBUG_STOP, 0, 0, 0, TASK_APP_LD1_ID);
        LoggerDebug_SendCmd(e_LOGGER_CMD_DEBUG_STOP, 0, 0, 0, TASK_APP_LD2_ID);
        LoggerDebug_SendCmd(e_LOGGER_CMD_DEBUG_STOP, 0, 0, 0, TASK_APP_LD3_ID);        
    }
}
#else
OSAL_TASK LogDebugTask1( void * pTaskId )
{
}
OSAL_TASK LogDebugTask2( void * pTaskId )
{
}
OSAL_TASK LogDebugTask3( void * pTaskId )
{
}
OSAL_ERROR_HANDLER LogDebugTaskErrorHandler( uint32 ErrorCode )
{
}
void LoggerDebug_TestStartStop(uint8 IsStart, uint16 Timeout, uint8 NumOfTasks)
{
}
#endif