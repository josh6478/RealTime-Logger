/* define logger statuses */

/* logger error status definitions */
#define LOGGER_STATUS_ERROR     -1      /* logger general error */

/* logger statuses */
#define LOGGER_STATUS_OK         0      /* logger command executed successfuly */
#define LOGGER_STATUS_BUSY       1      /* logger command could not be executed for now */

typedef LOGGER_STATUS   int8
/* logger flash manager handle */
typedef struct {
    uint8 *pWrite;                /* pointer to flash write location */
    uint8 *pRead;                 /* pointer to flash read location */    
}LoggerFLASHPtr_t;

typedef struct {
    LoggerFLASHPtr_t    Pointers;
    LoggerFLASHConfig_t *pFlashConfig;  /* pointer to flash configuration */    
}LoggerFLASHManager_t;

/* logger CB data structure */
typedef struct {
    (LOGGER_STATUS *)OpenSocketCb(LoggerSocketConfig_t *pSocketConfig);               /* open TCP\UDP socket callback */
    (LOGGER_STATUS *)CloseSocketCb(void);                                             /* close TCP\UDP socket callback */
    (LOGGER_STATUS *)SendSocketDataCb(uint8 *pBuff, uint16 Len);                      /* send TCP\UDP socket callback */
    (LOGGER_STATUS *)SendUartDataCb(uint8 *pBuff, uint16 Len);                        /* send UART data callback */
    (LOGGER_STATUS *)SendFlashDataCb(uint8 *Address, uint8 *pBuff, uint16 Len);       /* send FLASH data callback */
    (LOGGER_STATUS *)SendFtpDataCb(uint8 *Address, uint8 SizeKB, char FileName[]);    /* send FTP data callback */
    (LOGGER_STATUS *)GetTimeAndDateCb(LoggerTimeAndDate_t *pTimeAndDate);             /* get time and date callback */
    (LOGGER_STATUS *)SaveLoggerNVRCb(uint8 *Address, void *pBuff, uint8 SizeBytes);   /* save internal logger DB in NVR callback */
    (LOGGER_STATUS *)ReadLoggerNVRCb(uint8 *Address, void *pBuff, uint8 SizeBytes);   /* read internal logger DB in NVR callback */                           
}LoggerCB_t

typedef struct {
    char *pHead;                       /* a pointer to double buffer head */
    char *pTail;                       /* a pointer to double buffer tail */
    char *pRead;                       /* a pointer to read from double buffer */
    char *pWrite;                      /* a pointer to write to double buffer */
    OSAL_MUTEX_HANDLE pMutex;          /* a pointer to mutex to lock on write operations */
    uint32 Counter;                    /* the total number of RX debug messages */
}RXBuffer_t

/* logger manager handle */
typedef struct {
    RXBuffer_t RxBuffer;               /* Logger RX double buffer manager */
    LoggerCB_t *pCbList;               /* Logger callback list */
    uint32 TxCounter;                  /* the total number of TX packets */
    BOOL IsEnabled;                    /* is logger feature enabled */
    LoggerConfig_t Config;              /* logger user configuration */
}LoggerManager_t;
