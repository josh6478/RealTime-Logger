/*--------------------------------------------------------------------------------------
 * Author: Joshua Levi
 * Date: 14/01/2020
 * Description: initial implementetion
 * -------------------------------------------------------------------------------------

#include "Logger_Defs.h"
#include "Logger_Func.h"
#include "Logger_Manager.h"
#include "DebugTask.h"

                /* ========================================== *
                 *     P U B L I C     F U N C T I O N S      *
                 * ========================================== */

OSAL_TASK LogManagerTask( void * pTaskId )
{
    /* Logger manager task ID */
    uint8 TaskId = OSAL_TASK_EXTRACT_TASK_ID( pTaskId );
    
    /* the received message pointer definiton */
    OSAL_Msg * pRxMsg = NULL;
    
    /* This flag indicates wheather logger manager task should be ended */
    BOOL IsTaskEnd = FALSE;
    
    /* ******************** main while loop for receiving a message ***************** */
    while( !IsTaskEnd)
    {
        /* wait for input osal message */
        pRxMsg = OSAL_WaitOnMessage( TaskId );

        Dbg_SetTestPoint( TaskId , pRxMsg->cmd , 1 ,0 );     
        
        /* handle completions */
        if(pRxMsg->reply)
        {
            /* sanity */
            OSAL_ASSERT(pRxMsg->dst == TASK_APP_COM_MANAGER_ID || pRxMsg->dst == TASK_APP_FTP_PUT_MANAGER_ID);
            if(pRxMsg->dst == TASK_APP_COM_MANAGER_ID)
            {
                Logger_HandleSocketReply(pRxMsg->cmd, pRxMsg->status, pRxMsg->userTag);
            }
            else if(TASK_APP_FTP_PUT_MANAGER_ID)
            {
                Logger_HandleFtpPutReply(pRxMsg->status, 0);
            }
        }
        else
        {
            /* check command type */
            switch(pRxMsg->cmd) {
                
                /* handle init command */
            case e_LOGGER_CMD_INIT:
                Logger_Init((LoggerConfig_t *)pRxMsg->pDataIn, pRxMsg->userTag);
                break;
                
                /* handle feature enable\disable command */
            case e_LOGGER_CMD_DIS_EN:
                Logger_SwitchOnOff(pRxMsg->userTag, TRUE);
                break; 
                
                /* handle set mode command */
            case e_LOGGER_CMD_SET_MODE:
                Logger_SetMode(pRxMsg->userTag);
                break;
                
            case e_LOGGER_CMD_SET_DEST_TYPE:
                Logger_SetDestinationType(pRxMsg->userTag);
                break;
                
                /* handle enable\disable printout command */ 
            case e_LOGGER_CMD_PRINTOUT_DIS_EN:
                Logger_SetPrintoutEnDis(pRxMsg->userTag);
                break;
                
                /* handle set remote debug server command */
            case e_LOGGER_CMD_SET_DEBUG_SERVER:
                Logger_SetSocketConfig((LoggerSocketConfig_t *)pRxMsg->pDataIn);
                break;
                
                /* handle set debug level command */
            case e_LOGGER_CMD_SET_DEBUG_LEVEL:
                Logger_SetLogLevel(pRxMsg->userTag);
                break;
                
                /* handle set module mask command */
            case e_LOGGER_CMD_SET_MODULE_MASK:
                Logger_SetModuleMask(pRxMsg->status);
                break;
                
                /* handle pull log buffer command */
            case e_LOGGER_CMD_UPLOAD_DEBUG_LOG:
                /* TODO: call FTP task to send the debug log message*/
                Logger_SendLogFileToServer((LoggerFtpConfig_t *)pRxMsg->pDataIn, (LoggerSocketConfig_t *)pRxMsg->pDataOut);
                if(pRxMsg->pDataOut)
                {
                    OSAL_HeapFreeBuff((uint32**)&pRxMsg->pDataOut);
                }
                break;
                
                /* handle dump log buffer command */
            case e_LOGGER_CMD_DUMP_DEBUG_LOG:
                Logger_SendDebugLog(pRxMsg->userTag, LOGGER_DEST_TYPE_RS232);
                break;
                
            case OSAL_TIMER_MNG_TIMER_EXPIRE_CMD:
                Logger_HandleTimerExpr(pRxMsg->userTag);
                break;
                
            case e_LOGGER_CMD_SET_CONFIG:
                Logger_HandleServerCmd(pRxMsg->pDataIn, pRxMsg->DataInLen, pRxMsg->userTag);
                break;
                
            case e_LOGGER_CMD_SEND_CONF_TO_SRV:
                Logger_SendConfigToSrv(pRxMsg->userTag);
                break;
                
                /* unknown command error */
            default:
                OSAL_ASSERT(0); 
            }
        }
        ///Free Data In buffer if requested by source task. 
        if( (pRxMsg->CntrlBits & OSAL_MSG_FREE_BY_TARGET) && pRxMsg->pDataIn != NULL)
            OSAL_HeapFreeBuff( (uint32 **)&(pRxMsg->pDataIn) );
        
        OSAL_MsgFree(pRxMsg);
    
        /* added killing logger task after every command has been handled, to release its 512 bytes buffer */
        IsTaskEnd = TRUE;   
        Dbg_SetTestPoint( TaskId , pRxMsg->cmd , 100 ,0 );
    }//while ( !IsTaskEnd)
    /* kill task */
    OSAL_SuspendTask(500);
    OSAL_TaskDelete( TaskId );
}

OSAL_ERROR_HANDLER LogManagerTaskErrorHandler( uint32 ErrorCode )
{
}
