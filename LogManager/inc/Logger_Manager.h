/* define logger statuses */
#ifndef __LOGGER_MANAGER_H
#define __LOGGER_MANAGER_H

typedef enum {
    e_LOGGER_CMD_INIT,                          /* logger init command */
    e_LOGGER_CMD_UNINIT,                        /* logger uninit command */
    e_LOGGER_CMD_DIS_EN,                        /* logger enable\disable feature command */
    e_LOGGER_CMD_SET_MODE,                      /* logger set mode command */
    e_LOGGER_CMD_SET_DEST_TYPE,                 /* logger set destination type command */
    e_LOGGER_CMD_PRINTOUT_DIS_EN,               /* logger enable\disable printout command */
    e_LOGGER_CMD_SET_DEBUG_SERVER,              /* logger set remote debug server details command */
    e_LOGGER_CMD_SET_DEBUG_LEVEL,               /* logger set debug level command */
    e_LOGGER_CMD_SET_MODULE_MASK,               /* logger change module mask command */
    e_LOGGER_CMD_SET_FLASH_SEGMENT_SIZE,        /* logger change the FLASH segment size for debug messages */
    e_LOGGER_CMD_UPLOAD_DEBUG_LOG,              /* logger upload debug file command */
    e_LOGGER_CMD_DUMP_DEBUG_LOG,                /* logger dump debug file command */
    e_LOGGER_CMD_SET_CONFIG,                    /* logger set configuration command */
    e_LOGGER_CMD_SEND_CONF_TO_SRV,              /* logger send current configuration command */
}LoggerCmd_e;

typedef enum {
    e_LOGGER_STATE_DOWN,                        /* logger feature is shut down */
    e_LOGGER_STATE_INITIALIZING,                /* logger initializing state */
    e_LOGGER_STATE_OPENING_SOCKET,              /* logger opening socket */
    e_LOGGER_STATE_SENDING_LOG_FILE,            /* logger is sending log file to remote host */
    e_LOGGER_STATE_READY,                       /* logger is ready */
}LoggerState_e;

typedef struct {
    uint8 IsNextSectorErased   :1;      /* does flash erase needed */
}LoggerFlashFlags_t;

/* logger flash manager handle */
typedef struct {
    uint32 CurrSectorAddr;              /* current Flash Sector address */
    uint32 WriteAddr;                   /* address to flash write location */
    uint32 ReadAddr;                    /* address to flash read location */
    LoggerFlashFlags_t Flags;           /* Flash flags */
    uint32  CmdStatus;                  /* Flash command status */    
}LoggerFLASHManager_t;

typedef struct {
    uint8 IsLowMemory          :1;      /* is Rx buffer has low memory */
    uint8 IsUrgent             :1;      /* is the pritout urgent */
}LoggerRxBuffFlags_t;

typedef struct {
    uint8 IsSocketOpened       :1;      /* is socket opened */
}LoggerFlags_t;

typedef struct {
    char *pHead;                       /* a pointer to double buffer head */
    char *pTail;                       /* a pointer to double buffer tail */
    char *pRead;                       /* a pointer to read from double buffer */
    char *pWrite;                      /* a pointer to write to double buffer */
    OSAL_MUTEX_HANDLE pMutex;          /* a pointer to mutex to lock on write operations */
    uint32 RxCounter;                  /* the total number of RX debug messages */
    uint32 LowMemoryCnt;               /* how many times the Rx buffer got low memory */
    uint32 TxCounter;                  /* the total number of TX packets */
    uint32 BusyCnt;                    /* counter for error\busy in the low level for debug */
    uint32 RunOverCnt;                 /* counter for buffer run over */ 
    LoggerRxBuffFlags_t Flags;         /* Rx buffer flags */
}RXBuffer_t;

/* logger manager handle */
typedef struct {
    RXBuffer_t RxBuffer;               /* Logger RX double buffer manager */
    LoggerCB_t *pCbList;               /* Logger callback list */
    LoggerConfig_t Config;             /* logger user configuration */
    LoggerFLASHManager_t FlashMng;     /* logger Flash read and write adresses */
    uint8 State;                       /* logger manager task state */
    LoggerFlags_t Flags;               /* logger flags */ 
}LoggerManager_t;

#endif //__LOGGER_MANAGER_H

