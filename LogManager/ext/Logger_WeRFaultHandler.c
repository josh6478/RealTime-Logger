#include "osal.h"
#include "Logger_Utils.h"
#include "Logger_WeRFaultHandler.h"
#include "DebugTask.h"
#include "RTC_API.h"
#include "Dbg_Print.h"

/**
 * <pre>
 * void HardFaultHandler(uint32 *pHardFaultArgs)
 *
 * </pre>
 *  
 * This function handles hard fault handler. it captures the current stack registers, date and time, task name, ID and current command executing.
 * The information is writen to a special location in the RAM that is not initialized on reset, thus allow it to be saved in none volatile memory
 * after reset.
 *
 * @param   pHardFaultArgs [in]    the current registers value.
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

extern "C" void HardFaultHandler(uint32 *pHardFaultArgs)
{   
    MAGIC_ADDRESS = MAGIC_NUM;
    R0_REG_ADDRESS = pHardFaultArgs[0];
    R1_REG_ADDRESS = pHardFaultArgs[1];
    R2_REG_ADDRESS = pHardFaultArgs[2];
    R3_REG_ADDRESS = pHardFaultArgs[3];
    LR_REG_ADDRESS = pHardFaultArgs[5];
    PC_REG_ADDRESS = pHardFaultArgs[6];
    OSAL_GetRunningTaskName(DBG_TASK_NAME_ADDRESS);
    Dbg_GetFaultTask((uint16*)&DBG_TASK_CMD_ADDRESS, (uint8*)&DBG_TASK_ID_ADDRESS);
    RTC_GetTimeAndDate((RTC_Time_t*)&TIME_AND_DATE_ADDRESS);
    while(1);    
}

/**
 * <pre>
 * void Logger_PrintFaultHandler(void)
 *
 * </pre>
 *  
 * This function prints (to FLASH, RS232 or socket) the latest crash dump availalble in memory, using the logger module.
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

void Logger_PrintFaultHandler(void)
{
    RTC_Time_t *pTimeAndDate = (RTC_Time_t *)&TIME_AND_DATE_ADDRESS;
    
    LOG_PRINT(LEVEL_ERROR, "\r\n#########################################\r\n");
    LOG_PRINT(LEVEL_ERROR,"\r\n\noopsi, SYSTEM CRASHED on %d/%d/%d %d:%d:%d\r\n\n", 
              pTimeAndDate->DayOfMonth, pTimeAndDate->Month, pTimeAndDate->Year, pTimeAndDate->Hour, 
              pTimeAndDate->Minute, pTimeAndDate->Second);
    
    LOG_PRINT(LEVEL_ERROR, "RO = 0x%x\r\n", R0_REG_ADDRESS);
    LOG_PRINT(LEVEL_ERROR, "R1 = 0x%x\r\n", R1_REG_ADDRESS);
    LOG_PRINT(LEVEL_ERROR, "R2 = 0x%x\r\n", R2_REG_ADDRESS);
    LOG_PRINT(LEVEL_ERROR, "R3 = 0x%x\r\n", R3_REG_ADDRESS);
    LOG_PRINT(LEVEL_ERROR, "LR = 0x%x\r\n", LR_REG_ADDRESS);
    LOG_PRINT(LEVEL_ERROR, "PC = 0x%x\r\n", PC_REG_ADDRESS);
    LOG_PRINT(LEVEL_ERROR, "TaskName = %s\r\n", DBG_TASK_NAME_ADDRESS);
    if(DBG_TASK_ID_ADDRESS != 0xFF)
    {
        LOG_PRINT(LEVEL_ERROR, "TaskID = %d\r\n", DBG_TASK_ID_ADDRESS);
        LOG_PRINT(LEVEL_ERROR, "Cmd = 0x%x\r\n", DBG_TASK_CMD_ADDRESS);
    }
    LOG_PRINT(LEVEL_ERROR, "\r\n#########################################\r\n");
}

/**
 * <pre>
 * void Logger_RS232PrintFaultHandler(void)
 *
 * </pre>
 *  
 * This function prints out the latest crash dump availalble in memory, using the logger module.
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

void Logger_RS232PrintFaultHandler(void)
{
    RTC_Time_t *pTimeAndDate = (RTC_Time_t *)&TIME_AND_DATE_ADDRESS;
    
    if(pTimeAndDate->Year<=2011)
    {
        Printf("No Crash Found\n");
    }
    else
    {
        Printf("\r\n#########################################\r\n");
        Printf("\r\n\nSYSTEM CRASHED on %d/%d/%d %d:%d:%d\r\n\n", 
                  pTimeAndDate->DayOfMonth, pTimeAndDate->Month, pTimeAndDate->Year, pTimeAndDate->Hour, 
                  pTimeAndDate->Minute, pTimeAndDate->Second);
        
        Printf("RO = 0x%x\r\n", R0_REG_ADDRESS);
        Printf("R1 = 0x%x\r\n", R1_REG_ADDRESS);
        Printf("R2 = 0x%x\r\n", R2_REG_ADDRESS);
        Printf("R3 = 0x%x\r\n", R3_REG_ADDRESS);
        Printf("LR = 0x%x\r\n", LR_REG_ADDRESS);
        Printf("PC = 0x%x\r\n", PC_REG_ADDRESS);
        Printf("TaskName = %s\r\n", DBG_TASK_NAME_ADDRESS);
        if(DBG_TASK_ID_ADDRESS != 0xFF)
        {
            Printf("TaskID = %d\r\n", DBG_TASK_ID_ADDRESS);
            Printf("Cmd = 0x%x\r\n", DBG_TASK_CMD_ADDRESS);
        }
        Printf("\r\n#########################################\r\n");
    }
}
