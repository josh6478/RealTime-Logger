
/* Logger general definitions */
#ifndef __LOGER_DEFS_H
#define __LOGER_DEFS_H

#include "osal.h"
#include "Logger_Config.h"

/* log mask definitions */
#define LOGGER_MASK_ALL_TASKS                               0       /* all tasks are masked from logger  */
#define LOGGER_MASK_NO_TASK                                 0xFFFFFFFF    /* no task is masked from logger    */

/* socket type definitions  */
#define LOGGER_SOCKET_TYPE_TCP                              0
#define LOGGER_SOCKET_TYPE_UDP                              1
#define LOGGER_SOCKET_TYPE_FTP                              2

/* logger destination type */
#define LOGGER_DEST_TYPE_NONE                               0       /* no destination type (RS232 can be still enabled) */
#define LOGGER_DEST_TYPE_FLASH                              1       /* destination type is flash (log file in PULL mode) */
#define LOGGER_DEST_TYPE_SOCKET                             2       /* destination type is socket (in PUSH mode) */
#define LOGGER_DEST_TYPE_RS232                              3       /* destination type is RS232 */

/* logger mode types */
#define LOGGER_MODE_TYPE_PUSH                               0       /* in this mode the logger sends all logs to UDP socket */
#define LOGGER_MODE_TYPE_PULL                               1       /* in this mode the logger store all logs in FLASH */
#define LOGGER_MODE_TYPE_TEST                               2       /* in this mode the logger prints only production logs */
#define LOGGER_MODE_TYPE_NONE                               3       /* in this mode the logger doesnt push or pull (used for prints debug) */ 
#define LOGGER_MODE_MAX_VALUE                               LOGGER_MODE_TYPE_NONE

/* logger TX resolution */ 
#define LOGGER_RESOLUTION_TYPE_LOW                          0
#define LOGGER_RESOLUTION_TYPE_HIGH                         1

#define LOGGER_ATTR_NONE                                    0           /* Logger attribute - None */
#define LOOGER_ATTR_ERASE_FLASH_ON_RESET                    1           /* Logger attribute - Erase logger flash after reset (currently not implemented) */
#define LOGGER_ATTR_ERASE_FLASH_NOW                         (1<<1)      /* Logger attribute - Erase logger flash now */
#define LOGGER_ATTR_ERASE_FLASH_ON_SENDING                  (1<<2)      /* Logger attribute - Erase logger flash after sending log file (currenlty not implemented) */
#define LOGGER_ATTR_MAX_VALUE                               LOGGER_ATTR_ERASE_FLASH_ON_SENDING

/* logger timers definitions */
#define LOGGER_TIMER_OPEN_SOCKET_ID                         0    /* logger open socket timer expiration command */


/* Logger FLASH definitions */
/* I assume flash read pointer always points to a begining of the sector, and that sector size is always a power of 2 */                
#define LOGGER_DEF_FLASH_USAGE_SIZE_KB(P_READ, P_WRITE, SIZE_KB)                    P_WRITE>=P_READ?((P_WRITE-P_READ)>>10):(SIZE_KB - ((P_READ - P_WRITE)>>10))
#define LOGGER_DEF_FLASH_USAGE_SIZE_BYTES(P_READ, P_WRITE, SIZE_KB)                 P_WRITE>=P_READ?(P_WRITE-P_READ):((SIZE_KB<<10) - (P_READ - P_WRITE))
#define LOGGER_DEF_FLASH_SECTOR_USAGE_SIZE_KB(P_READ, P_WRITE)                      CALC_FLASH_USAGE_SIZE_BYTES(P_READ, P_WRITE)&(LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB-1)
#define LOGGER_DEF_FLASH_NUM_OF_DEBUG_SECTORS(FLASH_SIZE_KB)                        FLASH_SIZE_KB/LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB
#define LOGGER_DEF_FLASH_NEXT_SECTOR_ADDRESS(FLASH_DEBUG_SIZE_KB, \
    P_CURRENT_SECTOR, SECTOR_SIZE_KB)                                               (P_CURRENT_SECTOR - LOGGER_CONFIG_FLASH_START_ADDRESS)>>10 >= \
                                                                                    FLASH_DEBUG_SIZE_KB - SECTOR_SIZE_KB? LOGGER_CONFIG_FLASH_START_ADDRESS: (P_CURRENT_SECTOR + (SECTOR_SIZE_KB<<10))
#define LOGGER_DEF_FLASH_END_ADDRESS(FLASH_START_ADDRESS, FLASH_SIZE_KB)            (FLASH_START_ADDRESS +  (FLASH_SIZE_KB<<10))
//#define LOGGER_DEF_IS_ADDRESS_ALIGNED(ADDR, SIZE_KB)    ADDR & (SIZE_KB<<10)-1
//#if (LOGGER_DEF_FLASH_NUM_OF_DEBUG_SECTORS == 1)
//#error num of debug secotrs must be at least 2
//#endif    

#define LOGGER_DEF_MIN(X,Y)            (X)<(Y)?(X):(Y)    
#define LOGGER_DEF_MIN3VARS(A, B, C)   (A)<(B)?LOGGER_DEF_MIN(A,C):LOGGER_DEF_MIN(B,C)

#define LOGGER_DEF_GET_ALL_FLASH_SIZE_KB            0xFFFF          

#define LOGGER_DEF_GEN_CONFIG           gLoggerManager.Config.General
#define LOGGER_DEF_SOCKET_CONFIG        gLoggerManager.Config.Socket
#define LOGGER_DEF_FTP_CONFIG           gLoggerManager.Config.Ftp

#define LOGGER_DEF_SET_CONFIG(VAL, IN_VAL, NO_VAL, DEF_VAL)      VAL = IN_VAL == NO_VAL?DEF_VAL:IN_VAL

#define LOGGER_DEF_IS_RX_BUFFER_EMPTY()   (gLoggerManager.RxBuffer.pWrite == gLoggerManager.RxBuffer.pRead)

typedef int8 LOGGER_STATUS;

/* logger statuses */
#define LOGGER_STATUS_INVALID_PARAM         -14    /* logger invalid paramter error */
#define LOGGER_STATUS_SEND_LOG_FILE_FAILED  -13    /* logger send log file failed */
#define LOGGER_STATUS_CLOCK_ERROR           -12    /* logger read date and time error */
#define LOGGER_STATUS_NVR_READ_ERROR        -11    /* logger read NVR paramters error */
#define LOGGER_STATUS_ERROR_PARAM           -10    /* logger function got error in input parameters */
#define LOGGER_STATUS_UART_WRITE_ERROR      -9     /* logger status UART write command failed */
#define LOGGER_STATUS_FLASH_READ_ERROR      -8     /* logger status FLASH read command failed */
#define LOGGER_STATUS_FLASH_WRITE_ERROR     -7     /* logger status FLASH write command failed */
#define LOGGER_STATUS_FLASH_ERASE_ERROR     -6     /* logger status FLASH erase command failed */
#define LOGGER_STATUS_EEPROM_READ_ERROR     -5     /* logger status EEPROM read command failed */
#define LOGGER_STATUS_EEPROM_WRITE_ERROR    -4     /* logger status EEPROM write command failed */
#define LOGGER_STATUS_SOCKET_SEND_ERROR     -3     /* logger status send data socket failed */
#define LOGGER_STATUS_OPEN_SOCKET_FAILED    -2     /* logger status open socket failed */
#define LOGGER_STATUS_ERROR                 -1     /* logger general error */
#define LOGGER_STATUS_OK                     0     /* logger command executed successfuly */
#define LOGGER_STATUS_BUSY                   1     /* logger command could not be executed for now */

/* logger blank 4 bytes eeprom paramters */
#define LOGGER_DEF_BLANK_NVR_PARAM_32       0xFFFFFFFF

typedef enum {
    e_LOGGER_CONF_TYPE_GENERAL              =0,
    e_LOGGER_CONF_TYPE_SOCKET               =1,
    e_LOGGER_CONF_TYPE_FTP                  =2
}LoggerConfigType_e;

typedef enum {
    e_LOGGER_MSG_STATUS_OK,
    e_LOGGER_MSG_STATUS_FAIL,
}LoggerMsgStatus_e;
typedef enum {
    e_LOGGER_MSG_ERROR_DES_OK,
    e_LOGGER_MSG_ERROR_DES_NO_FILE_TO_SEND,
}LoggerMsgErrorDesc_e;
/* logger remote server configuration structure */
typedef struct {
    char DomainNameIp[LOGGER_CONFIG_SOCKET_MAX_IP_LEN];        
    uint16 RemotePort;
    uint16 LocalPort;
    uint8 SocketType;
    uint8 DomainNameLen;
}LoggerSocketConfig_t;

typedef struct {
    uint8 Attributes;
    uint16 FileSize;
    char Username[LOGGER_CONFIG_FTP_MAX_USER_NAME_LEN];
    char Password[LOGGER_CONFIG_FTP_MAX_PASSWORD_LEN];
    char Path[LOGGER_CONFIG_FTP_MAX_PATH_LEN];    
}LoggerFtpConfig_t;
                  
/* logger configuration structure */
typedef struct {
    BOOL IsEnabled;                 /* is logger feature enabled */
    BOOL IsPrintoutEnabled;         /* Is RS232 printout enabled */
    uint8 Attributes;               /* feature attributes */
    uint8 DestType;                 /* FLASH or Socket */
    uint8 Mode;                     /* Logger mode - push or pull */
    uint8 LogLevel;                 /* the current log level for printing debug messages */
    uint16 ResolutionMS;            /* the resolution of the debug log output in ms */
    uint16 FlashSize;               /* the flash size for storing log file */    
    uint32 ModuleMask;              /* the task ID participating in debug logs */
}LoggerGenConfig_t;

typedef struct {
    LoggerFtpConfig_t Ftp;
    LoggerSocketConfig_t Socket;
    LoggerGenConfig_t General;   
}LoggerConfig_t;

typedef struct {
    uint8  Year;                   /* Year value */
    uint8   Month;                  /* Month 1-12 */
    uint8   Day;                    /* DayOfMonth 1-31 */
    uint8   Hour;                   /* hour of day 0-23 */
    uint8   Minute;                 /* minute of hour 0-59 */
    uint8   Second;                 /* Seconds 0-59 */
}LoggerDateAndTime_t;

/* logger CB data structure */
typedef struct {
    LOGGER_STATUS (*OpenSocketCb)(LoggerSocketConfig_t *pSocketConfig);                         /* open TCP\UDP socket callback */
    LOGGER_STATUS (*CloseSocketCb)(void);                                                       /* close TCP\UDP socket callback */
    LOGGER_STATUS (*SendSocketDataCb)(uint8 *pBuff, uint16 Len, uint8 Mode);                    /* send TCP\UDP socket callback */
    LOGGER_STATUS (*SendUartDataCb)(char *pStr, uint16 StrLen);                                 /* send UART data callback */
    LOGGER_STATUS (*SendFlashDataCb)(uint32 Address, char *pStr, uint16 *pStrLen);              /* send FLASH data callback */
    LOGGER_STATUS (*ReadFlashDataCb)(uint32 Adrdress, char *pStr, uint16 StrLen);               /* read FLASH data callback */
    LOGGER_STATUS (*EraseFlashSectorCb)(uint32 Address, uint8 NumOfSectors, uint16 FlashSize);  /* erase FLASH sector */  
    LOGGER_STATUS (*SendFtpFileCb)(LoggerSocketConfig_t *pSocketConfig, 
                                   LoggerFtpConfig_t *pFtpConfig, uint32 Address, 
                                   int FileSize, char FileName[],  uint8 FileNameLen);          /* send FTP data callback */
    LOGGER_STATUS (*GetDateAndTimeCb)(LoggerDateAndTime_t *pDateAndTime);                       /* get time and date callback */
    LOGGER_STATUS (*SaveNVRParamsCb)(uint32 Address, void *pBuff, uint8 SizeBytes);             /* save internal logger DB in NVR callback */
    LOGGER_STATUS (*ReadNVRParamsCb)(uint32 Address, void *pBuff, uint8 SizeBytes);             /* read internal logger DB in NVR callback */
    LOGGER_STATUS (*GetPanelIdCb)(char PanelIdStr[LOGGER_CONFIG_PANEL_ID_SIZE_BYTES]);          /* get the panel ID value in string */
    LOGGER_STATUS (*ReadConfigCb)(LoggerConfigType_e ConfigType, void *pConfig);
    LOGGER_STATUS (*WriteConfigCb)(LoggerConfigType_e ConfigType, void *pConfig);
}LoggerCB_t;


#endif //__LOGER_DEFS_H
