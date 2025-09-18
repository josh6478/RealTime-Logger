
                /* ========================================== *
                 *          D E F I N I T I O N S             *
                 * ========================================== */

                /* ========================================== *
                 *     P R I V A T E     F U N C T I O N S    *
                 * ========================================== */

#include "Logger_Defs.h"
#include "Logger_Manager.h"
#include "Logger_Func.h"
#include "HW_TIMER_API.h"
#include "ComManagerInclude.h"
#include "msme.h"
#include "Logger_Release.h"
#include "MessageManager_Api.h"
#include "Util.h"
#include "Dbg_Print.h"
/* TEMP this include should not be here */
#include "Logger_WeRFaultHandler.h"

/* this is the logger manager handle. contains the DB needed to manager the logger */
LoggerManager_t gLoggerManager;

static void Logger_SetState(uint8 State)
{
    gLoggerManager.State = State;
}

/**
 * <pre>
 * static void Logger_InitRxBuffer(RXBuffer_t *pRxBuffer)
 * </pre>
 *  
 * this function initilizes the logger RX buffer to its init values
 * @param   pRxBuffer           [out]    a pointer to the RX buffer data structure, for the function to initialize its values.
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

static void Logger_InitRxBuffer(OUT RXBuffer_t *pRxBuffer)
{
    static char gLoggerMessagesBuffer [LOGGER_CONFIG_DOUBLE_BUFFER_SIZE_BYTES];
    
    /* initialize RX and TX pointers */
    pRxBuffer->pHead = pRxBuffer->pRead = pRxBuffer->pWrite =  gLoggerMessagesBuffer;
        
    /* intialize tail pointer */
    pRxBuffer->pTail = pRxBuffer->pHead + LOGGER_CONFIG_DOUBLE_BUFFER_SIZE_BYTES;
    
    /* create mutex */
    OSAL_PortMutexCreate(&pRxBuffer->pMutex);
    
    /* init counters */
    pRxBuffer->RxCounter =  pRxBuffer->TxCounter = pRxBuffer->BusyCnt =  pRxBuffer->LowMemoryCnt = pRxBuffer->RunOverCnt = 0;
    
    /* init flags */
    pRxBuffer->Flags.IsLowMemory = pRxBuffer->Flags.IsUrgent = 0;
        
}

/**
 * <pre>
 * static void Logger_OpenSocket(LoggerSocketConfig_t *pSocketConfig)
 * </pre>
 *  
 * this function open logger socket. should be called once at init in PUSH mode
 * @param   pSocketConfig           [in]    a pointer to the socket configuration data structure.
 *                                          contains all the information needed to open the socket.
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static void Logger_OpenSocket(IN LoggerSocketConfig_t *pSocketConfig)
{    

    if(gLoggerManager.Flags.IsSocketOpened)
    {
        /* socket is already opened */
        return;
    }
    /* sanity */
    if(gLoggerManager.State < e_LOGGER_STATE_INITIALIZING || gLoggerManager.pCbList->OpenSocketCb == NULL)
    {
        OSAL_ASSERT(0);
        return;
    }
    
    if(gLoggerManager.pCbList->OpenSocketCb(pSocketConfig) == LOGGER_STATUS_OK)
    {
        gLoggerManager.State = e_LOGGER_STATE_OPENING_SOCKET;            
    }
    /* set a timeout to monitor the opening of the socket */
    OSAL_StartTimer(LOGGER_TIMER_OPEN_SOCKET_ID, LOGGER_CONFIG_OPEN_SOCKET_TIMEOUT_MS, FALSE);
    
}

/**
 * <pre>
 * static LOGGER_STATUS Logger_EraseFlashSectors(uint32 StartAddress, uint8 NumOfSectors)
 * </pre>
 *  
 * this function erases log file sectors.
 * @param   StartAddress           [in]    the address of the first sector to erase
 * @param   NumOfSectors           [in]    the number of sectors to erase
 *
 * @return LOGGER_STATUS_OK on sucess, or negative value otherwise
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static LOGGER_STATUS Logger_EraseFlashSectors(IN uint32 StartAddress, IN uint8 NumOfSectors)
{
    if(gLoggerManager.pCbList->EraseFlashSectorCb != NULL)
    {
        /* this variable is volatile as its a shared memory changed in another task */
        volatile uint32 StatusCmd;
        
        if(gLoggerManager.pCbList->EraseFlashSectorCb(StartAddress, NumOfSectors, gLoggerManager.Config.General.FlashSize) == LOGGER_STATUS_OK)
        {
            /* on success erase the bit which indicate a flash sector erase is needed */
            gLoggerManager.FlashMng.Flags.IsNextSectorErased = 0;
            
            /* return success */
            return LOGGER_STATUS_OK;
        }
    }
    /* no callback exists to perform the erase. return error */
    return LOGGER_STATUS_FLASH_ERASE_ERROR;
}

/**
 * <pre>
 * static BOOL Logger_ValidateNVRParameters(LoggerFLASHManager_t *pFlashMng, uint16 FlashSizeKb)
 * </pre>
 *  
 * this function validates that the logger NVR parameters are valid. 
 * @param   pFlashMng           [in]    a pointer to logger flash manager handle
 * @param   FlashSizeKb         [in]    the flash size in KB
 *
 * @return TRUE if the NVR parameters are valid, or FALSE otherwise
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

static BOOL Logger_ValidateNVRParameters(IN LoggerFLASHManager_t *pFlashMng, IN uint16 FlashSizeKb)
{
    uint32 FlashEndAddress = LOGGER_DEF_FLASH_END_ADDRESS(LOGGER_CONFIG_FLASH_START_ADDRESS, FlashSizeKb);
    
    if(pFlashMng->CurrSectorAddr < LOGGER_CONFIG_FLASH_START_ADDRESS ||
       pFlashMng->CurrSectorAddr > FlashEndAddress                   ||
       pFlashMng->WriteAddr < LOGGER_CONFIG_FLASH_START_ADDRESS      ||
       pFlashMng->WriteAddr > FlashEndAddress                        ||
       pFlashMng->ReadAddr < LOGGER_CONFIG_FLASH_START_ADDRESS       ||
       pFlashMng->ReadAddr > FlashEndAddress)
    {
        return FALSE;
    }
    return TRUE;
}

/**
 * <pre>
 * static BOOL Logger_FlashMngInitDefault(LoggerFLASHManager_t *pFlashMng, uint16 FlashSize)
 * </pre>
 *  
 * this function initializes the logger flash manager structure and erases the flash sectors.
 * @param   pFlashMng           [out]    a pointer to logger flash manager handle to initialize the default values
 * @param   FlashSizeKb         [in]    the flash size in KB
 *
 * @return TRUE if succeeded erasing flash sectors, or FALSE otherwised
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

static BOOL Logger_FlashMngInitDefault(OUT LoggerFLASHManager_t *pFlashMng, IN uint16 FlashSize)
{
    /* 1. set all values to 0 */
    pFlashMng->Flags.IsNextSectorErased = 0;
    
    pFlashMng->CmdStatus = 0;
    
    /* 2. we need to set all NVR paramters to default value */
    pFlashMng->CurrSectorAddr = pFlashMng->WriteAddr = pFlashMng->ReadAddr = LOGGER_CONFIG_FLASH_START_ADDRESS;
    
    /* 3. we need to erase all FLASH sectors. */
    if(Logger_EraseFlashSectors(LOGGER_CONFIG_FLASH_START_ADDRESS, LOGGER_DEF_FLASH_NUM_OF_DEBUG_SECTORS(FlashSize)) == LOGGER_STATUS_OK)
    {
        return TRUE;
    }
    
    return FALSE;
}

/**
 * <pre>
 * static BOOL Logger_FlashMngInitFromDB(LoggerFLASHManager_t *pFlashMng, uint16 FlashSize)
 * </pre>
 *  
 * this function initializes the logger flash manager structure and erase flash sectors.
 * @param   pFlashMng           [inout]    a pointer to logger flash manager handle to initialize input values
 * @param   FlashSizeKb         [in]       the flash size in KB
 *
 * @return TRUE if succeeded erasing flash sectors, or FALSE otherwised
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

static BOOL Logger_FlashMngInitFromDB(INOUT LoggerFLASHManager_t *pFlashMng, IN uint16 FlashSize)
{    
    /* we need to validate NVR parameters */
    if(Logger_ValidateNVRParameters(&gLoggerManager.FlashMng, FlashSize) == FALSE)
    {
        /* in any case of validation error reset to default value */
        return Logger_FlashMngInitDefault(pFlashMng, FlashSize);
    }
    
    /* we need to check if we have to delete only the next sector */
    if(gLoggerManager.FlashMng.Flags.IsNextSectorErased)
    {
        /* calculate the next sector to erase */
        uint32 NextSectorAddress = LOGGER_DEF_FLASH_NEXT_SECTOR_ADDRESS(FlashSize, gLoggerManager.FlashMng.CurrSectorAddr, LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB);
            
        if(Logger_EraseFlashSectors(NextSectorAddress, 1) == LOGGER_STATUS_OK)
        {
             /* if we have just erased the current read address, update it to the next sector */
            if(gLoggerManager.FlashMng.ReadAddr == NextSectorAddress)
            {
                /* we have just erased the read section, set the read section to the next section */
                gLoggerManager.FlashMng.ReadAddr = LOGGER_DEF_FLASH_NEXT_SECTOR_ADDRESS(FlashSize, gLoggerManager.FlashMng.ReadAddr, LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB);
                
                /* save the current sector address for next reset */
                if(gLoggerManager.pCbList->SaveNVRParamsCb(LOGGER_CONFIG_NVR_ADDRESS, &gLoggerManager.FlashMng, LOGGER_CONFIG_NVR_SIZE_BYTES) != LOGGER_STATUS_OK)
                {
                    return FALSE;
                }
                
            }
        
        }
        else
        {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * <pre>
 * static void Logger_InitFlash(uint16 FlashSize, BOOL IsFlashReset)
 * </pre>
 *  
 * this function initializes the logger flash and erases flash sector if needed
 * @param   FlashSize           [in]    The flash size in KB
 * @param   IsFlashReset        [in]    Is flash sector erases needed.
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static BOOL Logger_InitFlash(IN uint16 FlashSize, IN BOOL IsFlashReset)
{    
    /* read NVR paramaters */
    if(gLoggerManager.pCbList->ReadNVRParamsCb != NULL)
    {
        gLoggerManager.pCbList->ReadNVRParamsCb(LOGGER_CONFIG_NVR_ADDRESS, &gLoggerManager.FlashMng, LOGGER_CONFIG_NVR_SIZE_BYTES);
        
        /* check FLASH reset conditions */
        if(IsFlashReset || gLoggerManager.FlashMng.CurrSectorAddr == LOGGER_DEF_BLANK_NVR_PARAM_32||
                           gLoggerManager.FlashMng.WriteAddr == LOGGER_DEF_BLANK_NVR_PARAM_32 ||
                           gLoggerManager.FlashMng.ReadAddr == LOGGER_DEF_BLANK_NVR_PARAM_32)
        {
            /* if one of the logger NVR parameters are not valid, or if the user has specifically requested FLASH log erase */
            if(Logger_FlashMngInitDefault(&gLoggerManager.FlashMng, FlashSize) == FALSE)
            {
                OSAL_ASSERT(0);
                return FALSE;
            }
        }
        else
        {
            /* otherwise the logger has a valid log file, initilize the flash manager from NVR values and continue writing without erasing old log file */
            if(Logger_FlashMngInitFromDB(&gLoggerManager.FlashMng, FlashSize) == FALSE)
            {
                OSAL_ASSERT(0);
                return FALSE;
            }
        }
    }
    else
    {
        OSAL_ASSERT(0);
        return FALSE;
    }
    /* init has passed successfully, logger is now ready to work with flash */
    return TRUE;
    //Logger_SetState(e_LOGGER_STATE_READY);
}

/**
 * <pre>
 * static uint32 Logger_GetFlashReadStartAddress(uint32 StartAddr, uint32 EndAddr, uint32 WriteAddr, uint32 ReadAddr, uint32 ReadSizeBytes)
 * </pre>
 *  
 * this function returns the current flash log file start address to read from, according to the size desired to read.
 * @param   StartAddr      [in]    The logger flash start address (head pointer)
 * @param   EndAddr        [in]    The logger flash end address (tail pointer)
 * @param   WriteAddr      [in]    The logger flash current write address pointer
 * @param   ReadAddr       [in]    The logger flash current read address pointer
 * @param   ReadSizeBytes  [in]    The desired last log file read size chunk in bytes
 *
 * @return the calculated current start read address
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static uint32 Logger_GetFlashReadStartAddress(IN uint32 StartAddr, IN uint32 EndAddr, IN uint32 WriteAddr, IN uint32 ReadAddr, IN uint32 ReadSizeBytes)
{
    /* the address to start reading from */    
    uint32 NewReadAddr;
        
    /* simple case write address is bigger than read address */
    if(WriteAddr > ReadAddr)
    {
        /*  option 1: write add > Read add 
                                    FLASH           
                            |                       |       
                            |                       |       
                 start add->+-----------------------+       
                            | FFFFFFFFFFFFFFFFFFFFF |       
                 read add-> | --------------------- |
                            | valid log file section|
                            | \/\/\/\/\/\/\/\/\/\/\ |
                 write add->|-----------------------|
                            |     erased sector     |
                            |  FFFFFFFFFFFFFFFFFFFF |
                 end add--->+-----------------------+       */
        
        /* the valid log file section size in bytes */
        uint32 ValidLogFileSection = WriteAddr - ReadAddr;                                                                              
        
        if(ValidLogFileSection > ReadSizeBytes)
        {
            /* the desired read section is smaller than log file size */
            NewReadAddr = WriteAddr - ReadSizeBytes;
        }
        else
        {
            /* the desired read section is bigger than log file size */
            NewReadAddr = ReadAddr;            
        }
    }
    /* harder case read address is bigger than write address */
    else
    {
        /* option 2: read add > write add
                                    FLASH           
                            |                        |       
                            |                        |       
                 start add->+------------------------+       
                            | \/\/\/\/\/\/\/\/\/\/\/ |       
                            | valid log file section1|
                 write add->|------------------------|
                            | FFFFFFFFFFFFFFFFFFFFF  |
                 read add-> |------------------------|
                            | /\/\/\/\/\/\/\/\/\/\/\ |       
                            | valid log file section2|
                 end add--->+-----------------------+           */
        
        uint32 ValidLogFileSection1 = WriteAddr - StartAddr;
        uint32 ValidLogFileSection2 = EndAddr - ReadAddr;
        
        if(ReadSizeBytes <= ValidLogFileSection1)
        {
            /* the desired read section is smaller than log file size and the start address is in valid section 1*/
            NewReadAddr = WriteAddr - ReadSizeBytes;
        }
        else
        {
            /* calculate the desired read file size after substructing the ValidLogFileSection1 */
            ReadSizeBytes -= ValidLogFileSection1;
            
            /* option 1 the result is maller than ValidLogFileSection2 */
            if(ReadSizeBytes>=ValidLogFileSection2)
            {
            /* the desired read section is still smaller than log file size (but now the start address is in valid section2)*/
                NewReadAddr = EndAddr - ValidLogFileSection2;
            }
            else
            {
                /* the desired read section is bigger than log file size */
                NewReadAddr = ReadAddr;
            }            
        }
    }
        
    return NewReadAddr;
}

                /* ========================================== *
                 *     P U B L I C     F U N C T I O N S      *
                 * ========================================== */

/**
 * <pre>
 * void Logger_Init(IN LoggerConfig_t *pLoggerConfig, IN uint8 Attr)
 * </pre>
 *  
 * this function initializes the logger feature DB and handles all init 
 * flows such as opening socket to RDS or erasing FLASH 
 *
 * @param   pLoggerConfig      [in]    a pointer to logger configuration
 * @param   Attr               [in]    The logger run time attributes
 *
 * @return None
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

void Logger_Init(IN LoggerConfig_t *pLoggerConfig, IN uint8 Attr)
{
    extern LoggerCB_t gLoggerCbList;
    
    BOOL IsFlashInit = FALSE;    
    /* sanity check */
    if(gLoggerManager.State > e_LOGGER_STATE_INITIALIZING)
    {
        /* logger was already initialized */
        OSAL_ASSERT(FALSE);
        return;
    }
    
    /* initialize the logger TX flow HW timer and interrupt handler routine */
    HW_TIMER_Init( 3, 1000, Logger_SendPacketFromInt);
    
    /* set logger state */
    gLoggerManager.State = e_LOGGER_STATE_INITIALIZING;
    
    /* init flags */
    *(uint8*)&gLoggerManager.Flags = 0;
    
    /* initialize RX buffer */
    Logger_InitRxBuffer(&gLoggerManager.RxBuffer);
    
    /* init callback list */
    gLoggerManager.pCbList = &gLoggerCbList;
    
    /* int the feature configuration */
    Logger_SetConfig(pLoggerConfig);
    
    /* validate configuration is correct */
    if(Logger_ValidateGenConfig(&gLoggerManager.Config.General))
    {
        OSAL_ASSERT(0);
    }
    
    /* initialize the FLASH DB and erase log file if attributes erase flas is set */    
    IsFlashInit = Logger_InitFlash(gLoggerManager.Config.General.FlashSize, Attr & LOGGER_ATTR_ERASE_FLASH_NOW);
    
    /* if logger is using socket we need to open it now */
    if(gLoggerManager.Config.General.IsEnabled)
    {        
        
        switch(gLoggerManager.Config.General.DestType)
        {
            /* handle socket opening */
        case LOGGER_DEST_TYPE_SOCKET:
            Logger_OpenSocket(&LOGGER_DEF_SOCKET_CONFIG);
            break;
            
            /* handle initializing the flash DB */
        case LOGGER_DEST_TYPE_FLASH:
            if(IsFlashInit)
            {
                Logger_SetState(e_LOGGER_STATE_READY);
            }
            break;
            /* can be used if only local port is opened */
        case LOGGER_DEST_TYPE_NONE:
            Logger_SetState(e_LOGGER_STATE_READY);
            break;
            
            /* error should not get here */
        default:
            OSAL_ASSERT(0);
            break;
            
        }//switch
        
        /* check if default handler was reached before panel was reboot and handle crash dump */    
        if(IS_FAULT_HANDLER())
        {
            /* reset the fault handler magic number for next reset */
            RESET_FAULT_HANDLER();
            
            /* print crash dump */
            Logger_PrintFaultHandler();
        }
        
    }
    /* Set TX HW timer to normal resolution mode */
    Logger_SetTxResolution(LOGGER_RESOLUTION_TYPE_LOW);    
}


/**
 * <pre>
 * void Logger_UnInit(void)
 * </pre>
 *  
 * this function uninit the logger manager handle.
 *
 * @return None
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_UnInit(void)
{
    /* uninit head, tail, read and write pointers */
    gLoggerManager.RxBuffer.pWrite = gLoggerManager.RxBuffer.pRead = gLoggerManager.RxBuffer.pHead = gLoggerManager.RxBuffer.pTail = NULL;
    
    /* destroy mutex */
    if(gLoggerManager.RxBuffer.pMutex != NULL)
    {
        OSAL_PortMutexDelete(gLoggerManager.RxBuffer.pMutex);
    }    
}

/**
 * <pre>
 * void Logger_SetLogLevel(uint8 LogLevel)
 * </pre>
 *  
 * this function sets the RAM logger log level configuration.
 *
 * @param   LogLevel      [in]    the input log level to set.
 *
 * @return None
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetLogLevel(uint8 LogLevel)
{
    gLoggerManager.Config.General.LogLevel = LogLevel;
}

/**
 * <pre>
 * void Logger_SetModuleMask(uint32 ModuleMask)
 * </pre>
 *  
 * this function sets the RAM logger module mask configuration.
 *
 * @param   ModuleMask      [in]    the input module mask to set.
 *
 * @return None
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetModuleMask(uint32 ModuleMask)
{
    gLoggerManager.Config.General.ModuleMask = ModuleMask;
}

/**
 * <pre>
 * void Logger_SwitchOnOff(BOOL IsOn, BOOL IsSaveDB)
 * </pre>
 *  
 * this function switches the logger feature ON and OFF in runtime.
 *
 * @param   IsOn          [in]    Set the feautre ON or OFF.
 * @param   IsSaveDB      [in]    Does the logger feature has new configuration to save in DB.
 *
 * @return None
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SwitchOnOff(BOOL IsOn, BOOL IsSaveDB)
{
    /* set the logger manager RAM DB */
    gLoggerManager.Config.General.IsEnabled = IsOn;
    
    if(!IsOn)
    {
        /* in case we turn the logger OFF we need to stop HW timer */
        HW_TIMER_Stop(3);
    }
    else
    {
        /* turning ON the logger */
        if(gLoggerManager.Config.General.DestType == LOGGER_DEST_TYPE_FLASH)
        {
            /* reset the read and write pointers in FLASH to start from scratch - old data is not valid */
            gLoggerManager.FlashMng.ReadAddr = gLoggerManager.FlashMng.WriteAddr;
            /* save in eeprom the new write address for next reset */
            if(gLoggerManager.pCbList->SaveNVRParamsCb != NULL)
            {
                /* save the pointers to NVR to be save for next reboot */
                gLoggerManager.pCbList->SaveNVRParamsCb(LOGGER_CONFIG_NVR_ADDRESS, &gLoggerManager.FlashMng, LOGGER_CONFIG_NVR_SIZE_BYTES);
            }            
        }
        /* print start message */
        Logger_PrintStartMessage();
        
        /* set TX HW timer to normal resolution */ 
        Logger_SetTxResolution(LOGGER_RESOLUTION_TYPE_LOW);
    }
    if(IsSaveDB && gLoggerManager.pCbList->WriteConfigCb != NULL)
    {
        /* if there is new configuration save it now */
        gLoggerManager.pCbList->WriteConfigCb(e_LOGGER_CONF_TYPE_GENERAL, &gLoggerManager.Config.General);
    }
        
}

/**
 * <pre>
 * BOOL Logger_GetModeOnOff(void)
 * </pre>
 *  
 * this function returns the ON\OFF feature mode 
 *
 * @param None
 *
 * @return None
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
BOOL Logger_GetModeOnOff(void)
{
    return gLoggerManager.Config.General.IsEnabled;
}

/**
 * <pre>
 * void Logger_SetTaskState(uint8 State)
 * </pre>
 *  
 * this function sets logger manager task state
 *
 * @param   State      [in]    Task state to set:
 *                             e_LOGGER_STATE_DOWN              - task state is down
 *                             e_LOGGER_STATE_INITIALIZING      - task is initializing
 *                             e_LOGGER_STATE_OPENING_SOCKET    - task is opening socket
 *                             e_LOGGER_STATE_SENDING_LOG_FILE  - task is sending log file
 *
 * @return None
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetTaskState(uint8 State)
{
    gLoggerManager.State = State;
}

/**
 * <pre>
 * void Logger_SetMode(uint8 Mode)
 * </pre>
 *  
 * this function sets logger manager mode to RAM DB. 
 *
 * @param   Mode      [in]    Logger mode to set.
 *
 * @return None
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetMode(uint8 Mode)
{
    gLoggerManager.Config.General.Mode = Mode;
}

/**
 * <pre>
 * void Logger_SetDestinationType(uint8 DestType)
 * </pre>
 *  
 * this function sets logger manager TX destination type configuration in RAM DB. 
 *
 * @param   DestType      [in]   destination type to set:
 *                               LOGGER_DEST_TYPE_NONE   -  no destination type
 *                               LOGGER_DEST_TYPE_FLASH  -  destination type flash
 *                               LOGGER_DEST_TYPE_SOCKET -  destination type socket
 *                               LOGGER_DEST_TYPE_RS232  -  destination type RS232
 *
 * @return None
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetDestinationType(uint8 DestType)
{
    gLoggerManager.Config.General.DestType = DestType;
}

/**
 * <pre>
 * void Logger_SetPrintoutEnDis(BOOL IsEnabled)
 * </pre>
 *  
 * this function sets logger manager printout enabled configuration in RAM DB
 *
 * @param   IsEnabled      [in]    is prinout enabled.
 *
 * @return None
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetPrintoutEnDis(BOOL IsEnabled)
{
    gLoggerManager.Config.General.IsPrintoutEnabled = IsEnabled;
}

void Logger_SetSocketConfig(LoggerSocketConfig_t *pSocketConfig)
{

    /* if no socket config - fill in the default configuration */
    if(pSocketConfig == NULL)
    {
        Logger_SetDefaultSocketConfig();
    }
    
    /* TODO: fill in the user configuration */
}

/**
 * <pre>
 * void Logger_SetTxResolution(uint8 Resolution)
 * </pre>
 *  
 * this function sets logger manager printout enabled configuration in RAM DB
 *
 * @param   IsEnabled      [in]    is prinout enabled.
 *
 * @return None
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetTxResolution(uint8 Resolution)
{
    if(LOGGER_RESOLUTION_TYPE_LOW == Resolution)
    {
        HW_TIMER_Start (3, LOGGER_CONFIG_LOW_RESOLUTION_MS, TRUE);    
        
    }
    else if(LOGGER_RESOLUTION_TYPE_HIGH)
    {
        HW_TIMER_Start (3, LOGGER_CONFIG_HIGH_RESOLUTION_MS, TRUE);
    }
    else
    {
        OSAL_ASSERT(0);
    }
}

void Logger_HandleSocketReply(uint16 Cmd, uint32 Status, uint16 SessionType)
{
    /*sanity*/
    if(SessionType != COM_MANAGER_SESSION_TYPE_DEBUG)
    {
        OSAL_ASSERT(0);
        return;
    }
    
    switch(Cmd)
    {
        case COM_MANAGER_OPEN_SOCKET:
           if(Status == COM_MANAGER_FUNC_ERR_OK)
           {
               Logger_SetState(e_LOGGER_STATE_READY);
               gLoggerManager.Flags.IsSocketOpened = 1;
               OSAL_StopTimer(LOGGER_TIMER_OPEN_SOCKET_ID);
           }
        break;
        
        case COM_MANAGER_CLOSE_SOCKET:            
              gLoggerManager.State = e_LOGGER_STATE_OPENING_SOCKET;            
        break;
        
        /* sanity */
        default:
            OSAL_ASSERT(0);
    }
}

void Logger_HandleTimerExpr(IN uint16 TimerType)
{
    switch(TimerType)
    {
        case LOGGER_TIMER_OPEN_SOCKET_ID:
            Logger_OpenSocket(&gLoggerManager.Config.Socket);
        break;
        
        default:
            OSAL_ASSERT(0);
    }
}

void Logger_SendConfigToSrv(IN uint8 ConfigType)
{
#define MSG_GEN_CONFIG          CodeArgs.LoggerCurrConfMsg.GenConfig
#define MSG_SOKCET_CONFIG       CodeArgs.LoggerCurrConfMsg.SocketConfig
#define MSG_FTP_CONFIG          CodeArgs.LoggerCurrConfMsg.FtpConfig
#define MSG_FTP_SOCKET_CONFIG   CodeArgs.LoggerCurrConfMsg.FtpConfig.SocketConf
    U_CODE_ARGUMENT CodeArgs;
    DEV_TYPE_t DevType;
    
    DevType.s = CONTROL_PANEL_TYPE;

    CodeArgs.LoggerCurrConfMsg.VerMajor = LOGGER_VERSION[0];
    CodeArgs.LoggerCurrConfMsg.VerMinor = LOGGER_VERSION[2];

    switch ((LoggerConfigType_e)ConfigType)
    {
        case e_LOGGER_CONF_TYPE_GENERAL:
            CodeArgs.LoggerCurrConfMsg.ConfType = 0;
            MSG_GEN_CONFIG.IsEnabled = LOGGER_DEF_GEN_CONFIG.IsEnabled;
            MSG_GEN_CONFIG.LogLevel = LOGGER_DEF_GEN_CONFIG.LogLevel;
            MSG_GEN_CONFIG.ModuleMask = LOGGER_DEF_GEN_CONFIG.ModuleMask;
            MSG_GEN_CONFIG.Mode = LOGGER_DEF_GEN_CONFIG.Mode;
            MSG_GEN_CONFIG.FlashSize = LOGGER_DEF_GEN_CONFIG.FlashSize;
            MSG_GEN_CONFIG.Attributes = LOGGER_DEF_GEN_CONFIG.Attributes;
        break;
            
        case e_LOGGER_CONF_TYPE_SOCKET:
            CodeArgs.LoggerCurrConfMsg.ConfType = 1;
            MSG_SOKCET_CONFIG.DomainLen = OSAL_StrLen(LOGGER_DEF_SOCKET_CONFIG.DomainNameIp, LOGGER_CONFIG_SOCKET_MAX_IP_LEN);
            MSG_SOKCET_CONFIG.pDomainName = LOGGER_DEF_SOCKET_CONFIG.DomainNameIp;
            MSG_SOKCET_CONFIG.RemotePort = LOGGER_DEF_SOCKET_CONFIG.RemotePort;
        break;
                    
        case e_LOGGER_CONF_TYPE_FTP:
            CodeArgs.LoggerCurrConfMsg.ConfType = 2;
            
            MSG_FTP_SOCKET_CONFIG.DomainLen = OSAL_StrLen(LOGGER_DEF_SOCKET_CONFIG.DomainNameIp, LOGGER_CONFIG_SOCKET_MAX_IP_LEN);
            MSG_FTP_SOCKET_CONFIG.pDomainName = LOGGER_DEF_SOCKET_CONFIG.DomainNameIp;
            MSG_FTP_SOCKET_CONFIG.RemotePort = LOGGER_DEF_SOCKET_CONFIG.RemotePort;
            MSG_FTP_CONFIG.UsernameLen = OSAL_StrLen(LOGGER_DEF_FTP_CONFIG.Username, LOGGER_CONFIG_FTP_MAX_USER_NAME_LEN);
            MSG_FTP_CONFIG.pUserName = LOGGER_DEF_FTP_CONFIG.Username;
            MSG_FTP_CONFIG.PasswordLen = OSAL_StrLen(LOGGER_DEF_FTP_CONFIG.Password, LOGGER_CONFIG_FTP_MAX_PASSWORD_LEN);
            MSG_FTP_CONFIG.pPassword = LOGGER_DEF_FTP_CONFIG.Password;
            MSG_FTP_CONFIG.PathnameLen = OSAL_StrLen(LOGGER_DEF_FTP_CONFIG.Path, LOGGER_CONFIG_FTP_MAX_PATH_LEN);
            MSG_FTP_CONFIG.pPathname = LOGGER_DEF_FTP_CONFIG.Path;
            MSG_FTP_CONFIG.FileSize = LOGGER_DEF_FTP_CONFIG.FileSize;
            MSG_FTP_CONFIG.Attributes = LOGGER_DEF_FTP_CONFIG.Attributes;
        break;
            
        default:
            LOG_PRINT(LEVEL_ERROR, "UNKNOWN CONFIG TYPE(%x)\n", ConfigType);
            OSAL_ASSERT(0);
            
    }
    OMM_SendMsg ( OSAL_GetSelfTaskId(), MSME_LOGGER_CONFIGURATION_EVENT_MSG, &CodeArgs, DevType, MSME_MSG_2_SPECIFIC_USER,
                 CONTROL_PANEL_TYPE_ID, OMM_DONT_SEND_COMPLETION_AFTER_SERVER_ACK);
    
}

void Logger_HandleServerCmd(void* pDataIn, uint8 Len, uint8 CmdType)
{
#define MSG_LOGGER_STATUS CodeArgs.LoggerStatusMsg
#define OFF     FALSE    
    BOOL IsError;
    U_CODE_ARGUMENT CodeArgs;
    DEV_TYPE_t DevType;
        
    DevType.s = CONTROL_PANEL_TYPE;

    MSG_LOGGER_STATUS.VerMajor = LOGGER_VERSION[0];
    MSG_LOGGER_STATUS.VerMinor = LOGGER_VERSION[2];

    switch(CmdType)
    {
        case e_LOGGER_CONF_TYPE_GENERAL:
            IsError = Logger_ValidateGenConfig((LoggerGenConfig_t *)pDataIn);
            if(!IsError)
            {
                Logger_SwitchOnOff(OFF, FALSE);
                
                Logger_SetState(e_LOGGER_STATE_INITIALIZING);
                
                if(gLoggerManager.pCbList->WriteConfigCb != NULL)
                {    
                    gLoggerManager.pCbList->WriteConfigCb(e_LOGGER_CONF_TYPE_GENERAL,(LoggerGenConfig_t *)pDataIn);
                }
               
                Logger_InitFlash(LOGGER_DEF_GEN_CONFIG.FlashSize, ((LoggerGenConfig_t *)pDataIn)->Attributes & LOGGER_ATTR_ERASE_FLASH_NOW);
                
                BOOL IsOn = Logger_SetRAMGeneralConfig((LoggerGenConfig_t *)pDataIn);
                                 
                Logger_InitRxBuffer(&gLoggerManager.RxBuffer);
                
                if(gLoggerManager.Config.General.DestType == LOGGER_DEST_TYPE_SOCKET)
                {
                    Logger_OpenSocket(&LOGGER_DEF_SOCKET_CONFIG);                    
                }
                Logger_SetState(e_LOGGER_STATE_READY);
                
                Logger_SwitchOnOff(IsOn, FALSE);

            }
        break;
            
        case e_LOGGER_CONF_TYPE_SOCKET:
            IsError = Logger_ValidateTcpConfig((LoggerSocketConfig_t *)pDataIn);
        break;
        
        case e_LOGGER_CONF_TYPE_FTP:
            IsError = Logger_ValidateFtpConfig((LoggerFtpConfig_t *)pDataIn);
        break;
        
        default:
            OSAL_ASSERT(0);
    }
    MSG_LOGGER_STATUS.Command = CmdType;
    MSG_LOGGER_STATUS.Status = IsError;
    MSG_LOGGER_STATUS.Description = 0;

    OMM_SendMsg ( OSAL_GetSelfTaskId(), MSME_LOGGER_STATUS_EVENT_MSG, &CodeArgs, DevType, MSME_MSG_2_SPECIFIC_USER,
                 CONTROL_PANEL_TYPE_ID, OMM_DONT_SEND_COMPLETION_AFTER_SERVER_ACK);
    
}

void Logger_FlushRxBuffer(void)
{
    gLoggerManager.RxBuffer.Flags.IsUrgent = 1;
    Logger_SetTxResolution(LOGGER_RESOLUTION_TYPE_HIGH);
}

BOOL Logger_SendLogFileToServer(LoggerFtpConfig_t *pLoggerFtpConfig, LoggerSocketConfig_t *pLoggerSocketConfig)
{
    uint16 SizeKB;
    int ReadSizeBytes;
    uint8 FileNameLen;         
    char FileName[LOGGER_CONFIG_FTP_FILE_NAME_LEN];    
    LoggerDateAndTime_t DateAndTime = {0};
    uint16 FlashUsageKB;
    
    /* sanity - both paramters must be set or both can be not set */
    if((pLoggerFtpConfig == NULL && pLoggerSocketConfig != NULL) || (pLoggerSocketConfig == NULL && pLoggerFtpConfig != NULL))
    {
        return FALSE;
    }
    /* if both paramters are null we must take  them from default paramters */
    if(pLoggerFtpConfig == NULL && pLoggerSocketConfig  == NULL)
    {
        pLoggerFtpConfig = &LOGGER_DEF_FTP_CONFIG;
        pLoggerSocketConfig = &LOGGER_DEF_SOCKET_CONFIG;
    }
    SizeKB = pLoggerFtpConfig->FileSize;
    /* current FLASH usage of debug log */
    FlashUsageKB = LOGGER_DEF_FLASH_USAGE_SIZE_KB(gLoggerManager.FlashMng.ReadAddr, gLoggerManager.FlashMng.WriteAddr, LOGGER_DEF_GEN_CONFIG.FlashSize);

     ReadSizeBytes = (SizeKB == LOGGER_DEF_GET_ALL_FLASH_SIZE_KB || SizeKB >=FlashUsageKB)?
        LOGGER_DEF_FLASH_USAGE_SIZE_BYTES(gLoggerManager.FlashMng.ReadAddr, gLoggerManager.FlashMng.WriteAddr, LOGGER_DEF_GEN_CONFIG.FlashSize):SizeKB<<10;
    
    OSAL_ASSERT(ReadSizeBytes<=LOGGER_DEF_GEN_CONFIG.FlashSize<<10);
     
    /* singular case FLASH is empty */
    if(ReadSizeBytes == 0)
    {
        Logger_HandleFtpPutReply(OSAL_STATUS_OK, e_LOGGER_MSG_ERROR_DES_NO_FILE_TO_SEND);
        return FALSE;
    }
    
    if(gLoggerManager.pCbList->GetPanelIdCb != NULL)
    {
        gLoggerManager.pCbList->GetPanelIdCb(FileName);
        
        if(gLoggerManager.pCbList->GetDateAndTimeCb != NULL)
        {
            /* fill in the file name */
            gLoggerManager.pCbList->GetDateAndTimeCb(&DateAndTime);
            char *pItr = &FileName[LOGGER_CONFIG_PANEL_ID_SIZE_BYTES - 1];
            *pItr++ = '_';
            pItr+= num2decstr(DateAndTime.Day, (uint8*)pItr, 2);
            pItr+= num2decstr(DateAndTime.Month, (uint8*)pItr, 2);
            pItr+= num2decstr(DateAndTime.Year, (uint8*)pItr, 2);
            pItr+= num2decstr(DateAndTime.Hour, (uint8*)pItr, 2);
            pItr+= num2decstr(DateAndTime.Minute, (uint8*)pItr, 2);
            pItr = OSAL_StrCopy(pItr, ".txt", 5);
            FileNameLen = pItr - FileName;
        }
        else
        {
            return FALSE;
        }
    
    }
    else
    {
        return FALSE;
    }
    if(gLoggerManager.pCbList->SendFtpFileCb == NULL)
    {
        return FALSE;
    }
    /* stop the logger while sending the file */
    Logger_FlushRxBuffer();
    
    if(gLoggerManager.pCbList->SendFtpFileCb(pLoggerSocketConfig, pLoggerFtpConfig, 
                                             gLoggerManager.FlashMng.ReadAddr,ReadSizeBytes, 
                                             FileName, FileNameLen) != LOGGER_STATUS_OK)
    {
        return FALSE;
    }
    return TRUE;
}

void Logger_HandleFtpPutReply(uint8 Status, uint8 Desc)
{
#define MSG_LOGGER_STATUS CodeArgs.LoggerStatusMsg
    
    U_CODE_ARGUMENT CodeArgs;
    DEV_TYPE_t DevType;
    
    DevType.s = CONTROL_PANEL_TYPE;

    MSG_LOGGER_STATUS.VerMajor = LOGGER_VERSION[0];
    MSG_LOGGER_STATUS.VerMinor = LOGGER_VERSION[2];

    MSG_LOGGER_STATUS.Command = e_LOGGER_CONF_TYPE_FTP;
        
    MSG_LOGGER_STATUS.Status = Status == OSAL_STATUS_OK?e_LOGGER_MSG_STATUS_OK:e_LOGGER_MSG_STATUS_FAIL;
    MSG_LOGGER_STATUS.Description = Desc;

    OMM_SendMsg ( OSAL_GetSelfTaskId(), MSME_LOGGER_STATUS_EVENT_MSG, &CodeArgs, DevType, MSME_MSG_2_SPECIFIC_USER,
                 CONTROL_PANEL_TYPE_ID, OMM_DONT_SEND_COMPLETION_AFTER_SERVER_ACK);
    Logger_SetTaskState(e_LOGGER_STATE_READY);
}

void Logger_PrintStartMessage(void)
{
        
    extern char *LoggerLevelTableStr[];
    
    char PanelId[LOGGER_CONFIG_PANEL_ID_SIZE_BYTES] = {0};
    /* sanity check */
    if(gLoggerManager.pCbList == NULL)
    {
        OSAL_ASSERT(0);
        return;
    }
           
    if(gLoggerManager.pCbList->GetPanelIdCb != NULL)
    {
        gLoggerManager.pCbList->GetPanelIdCb(PanelId);
    }
    else
    {
        OSAL_ASSERT(0);
        return;
    }
    
    LOG_PRINT(LEVEL_NONE, "\nVER: %s TIME:%T CP:%x LEVEL:%s MASK:0x%x\n\n", LOGGER_VERSION, PanelId, 
              LoggerLevelTableStr[LOGGER_DEF_GEN_CONFIG.LogLevel], LOGGER_DEF_GEN_CONFIG.ModuleMask);
    
}

BOOL Logger_ValidateGenConfig(LoggerGenConfig_t *pLoggerGenConfig)
{
    BOOL IsError = FALSE;
    /* check if isEnabled is in the proper range */
    if(pLoggerGenConfig->IsEnabled != 0 && pLoggerGenConfig->IsEnabled != 1)
    {
        pLoggerGenConfig->IsEnabled = LOGGER_CONFIG_DEFAULT_FEATURE_IS_ENABLED;
        IsError = TRUE;
    }
    /* check if log level is in the proper range */
    if(pLoggerGenConfig->LogLevel >= LEVEL_MAX_NUM)
    {
        pLoggerGenConfig->LogLevel = LOGGER_CONFIG_DEFAULT_LOG_LEVEL;
        IsError = TRUE;
    }
    
    /* validate mode */
    if(pLoggerGenConfig->Mode > LOGGER_MODE_MAX_VALUE)
    {
        pLoggerGenConfig->Mode = LOGGER_CONFIG_DEFAULT_MODE_TYPE;
        IsError = TRUE;
    }
    
    /* validate resolution */
    if(pLoggerGenConfig->ResolutionMS == 0 || pLoggerGenConfig->ResolutionMS == 0xFFFF)
    {
        pLoggerGenConfig->ResolutionMS = LOGGER_CONFIG_DEFAULT_RESOLUTION_MS;
    }
    
    /* validate the flash size does not out of allowed flash bounderies */
    if(pLoggerGenConfig->FlashSize < LOGGER_CONFIG_FLASH_MIN_SEGMENT_SIZE_KB)
    {
        if(pLoggerGenConfig->FlashSize  == 0)
        {
            pLoggerGenConfig->FlashSize = LOGGER_CONFIG_DEFAULT_FLASH_SIZE;
        }
        else
        {
            pLoggerGenConfig->FlashSize = LOGGER_CONFIG_FLASH_MIN_SEGMENT_SIZE_KB;
        }
    }
    else if(pLoggerGenConfig->FlashSize == 0xFFFF)
    {
        pLoggerGenConfig->FlashSize = LOGGER_CONFIG_DEFAULT_FLASH_SIZE;
    }
    else if(pLoggerGenConfig->FlashSize > LOGGER_CONFIG_FLASH_MAX_SEGMENT_SIZE_KB)
    {
        pLoggerGenConfig->FlashSize = LOGGER_CONFIG_FLASH_MAX_SEGMENT_SIZE_KB;
    }/* flash size is withing range - check alignement to sector size */
    else if(pLoggerGenConfig->FlashSize & (LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB-1))
    {
        IsError = TRUE;
        pLoggerGenConfig->FlashSize = LOGGER_CONFIG_FLASH_MAX_SEGMENT_SIZE_KB;        
    }
    
    LOG_PRINT(LEVEL_DETAILS, "Gen Conf: Enable(%d) Level(%d) Mask(%x) Mode(%d) FlashSize(%d) Resolution(%d) Attr(%x)\n", 
              pLoggerGenConfig->IsEnabled, pLoggerGenConfig->LogLevel, pLoggerGenConfig->ModuleMask, pLoggerGenConfig->Mode, 
              pLoggerGenConfig->FlashSize, pLoggerGenConfig->ResolutionMS, pLoggerGenConfig->Attributes);
    
    return IsError;

}

BOOL Logger_ValidateTcpConfig(LoggerSocketConfig_t *pLoggerSocketConfig)
{
    LOG_PRINT(LEVEL_DETAILS, "Socket Conf: Domain(%s) Len(%d) RPort(%d) LPort(%d) Type(%d)\n", 
              pLoggerSocketConfig->DomainNameIp, pLoggerSocketConfig->DomainNameLen, pLoggerSocketConfig->RemotePort, 
              pLoggerSocketConfig->LocalPort, pLoggerSocketConfig->SocketType);
    
    return FALSE;
}

BOOL Logger_ValidateFtpConfig(LoggerFtpConfig_t *pLoggerFtpConfig)
{
    LOG_PRINT(LEVEL_DETAILS, "Ftp Conf: Username(%s) Password(%s) Path(%s) FileSize(%d) Attr(%x)\n", 
              pLoggerFtpConfig->Username, pLoggerFtpConfig->Password, pLoggerFtpConfig->Path, pLoggerFtpConfig->FileSize, pLoggerFtpConfig->Attributes);
    return FALSE;    
}

BOOL Logger_SendDebugLog(uint16 SizeKB, uint8 DestType)
{
#define ON TRUE
#define OFF FALSE
#define READ_CHUNK_SIZE         256
    
    /* return value */
    BOOL RetVal = FALSE;
    
    /* Iterator to run over FLASH address */
    uint32 Iter;
        
    /* Flash end address for calculations */
    uint32 EndAddress = LOGGER_DEF_FLASH_END_ADDRESS(LOGGER_CONFIG_FLASH_START_ADDRESS, gLoggerManager.Config.General.FlashSize);
    
    /* current FLASH usage of debug log */
    uint16 FlashUsageKB = LOGGER_DEF_FLASH_USAGE_SIZE_KB(gLoggerManager.FlashMng.ReadAddr, gLoggerManager.FlashMng.WriteAddr, LOGGER_DEF_GEN_CONFIG.FlashSize);
    
    int ReadSizeBytes = (SizeKB == LOGGER_DEF_GET_ALL_FLASH_SIZE_KB || SizeKB >=FlashUsageKB)?
        LOGGER_DEF_FLASH_USAGE_SIZE_BYTES(gLoggerManager.FlashMng.ReadAddr, gLoggerManager.FlashMng.WriteAddr, LOGGER_DEF_GEN_CONFIG.FlashSize):SizeKB<<10;
    
     /* temp buff to read and dump flash log */
    char *pTempBuff;

    /* sanity check */
    if(ReadSizeBytes<0 || 
       ((DestType == LOGGER_DEST_TYPE_RS232 && gLoggerManager.pCbList->SendUartDataCb == NULL) || (DestType == LOGGER_DEST_TYPE_SOCKET && gLoggerManager.pCbList->SendSocketDataCb == NULL)))
    {
        OSAL_ASSERT(0);
        return FALSE;
    }
    
    /* singular case FLASH is empty */
    if(ReadSizeBytes == 0)
    {
        Printf("FLASH is empty\n");
        return FALSE;
    }
    /* sanity. check that task is ready */
    if(gLoggerManager.State != e_LOGGER_STATE_READY)
    {
        Printf("Task is not ready yet TaskState(%d)\n", gLoggerManager.State);
        return FALSE;
    }
    
    /* change task state */
    Logger_SetState(e_LOGGER_STATE_SENDING_LOG_FILE);
    
    /* allocate the working buffer to send log file */
    pTempBuff = (char*)OSAL_HeapAllocBuffer(OSAL_GetSelfTaskId(), READ_CHUNK_SIZE);
    
    if(pTempBuff == NULL)
    {
        OSAL_ASSERT(0);
        return FALSE;
    }
    
    /* get the flash start address */
    Iter = Logger_GetFlashReadStartAddress(LOGGER_CONFIG_FLASH_START_ADDRESS, EndAddress, gLoggerManager.FlashMng.WriteAddr, gLoggerManager.FlashMng.ReadAddr, ReadSizeBytes);
    /* assumptions:
       1. if dump command has arrived the UART is available 
       2. the logger task priority is lower than uart task 
       3. there is a task suspend between reading from flash to writing to uart */ 
    while(ReadSizeBytes > 0)
    {
        LOGGER_STATUS Status;
        
        /* read the minimum of 3 - READ_CHUNK_SIZE, Size left to read, and the size left to read at the bottom of the flash */ 
        uint16 FlashReadSize = LOGGER_DEF_MIN3VARS(READ_CHUNK_SIZE, ReadSizeBytes, EndAddress - Iter);
        /* 1. read chunks of READ_CHUNK_SIZE bytes from flash */
        if(gLoggerManager.pCbList->ReadFlashDataCb(Iter, pTempBuff, FlashReadSize) != LOGGER_STATUS_OK)
        {
            OSAL_ASSERT(0);
            goto EXIT;
        }
        
        if(DestType == LOGGER_DEST_TYPE_RS232)
        {
            Status = gLoggerManager.pCbList->SendUartDataCb(pTempBuff, FlashReadSize);
        }
        else
        {
            Status = gLoggerManager.pCbList->SendSocketDataCb((uint8*)pTempBuff, FlashReadSize, LOGGER_MODE_TYPE_PULL);
        }
        if(Status != LOGGER_STATUS_OK)
        {
            if(Status == LOGGER_STATUS_BUSY)
            {
                goto WAIT;
            }
            else
            {
                OSAL_ASSERT(0);
                goto EXIT;
            }
            
        }
        Iter+=FlashReadSize;        
        ReadSizeBytes -= FlashReadSize;
        
        if(Iter >= EndAddress)
        {
            /* make sure we dont read too much */
            OSAL_ASSERT(Iter<=EndAddress);
            
            /* wrap arround - we have reached the end of the flash continue from the begining */
            Iter = LOGGER_CONFIG_FLASH_START_ADDRESS;
        }
WAIT:        
        OSAL_SuspendTask(10);
    }
                
    RetVal = TRUE;
    
EXIT:
    /* change task state */
    Logger_SetState(e_LOGGER_STATE_READY);

    /* free working buffer */    
    OSAL_HeapFreeBuff((uint32**)&pTempBuff);
    
    /* return status */
    return RetVal;
}

