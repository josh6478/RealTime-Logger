
/* ---------------------------  Logger hard coded configuration ---------------------------------*/
#define LOGGER_CONFIG_REMOTE_SERVER_DNS_LEN                  64             /* remote server DNS length in bytes */
#define LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB                   64             /* FLASH sector size in KB */
#define FLASH_CONFIG_SECTOR_SIZE_THERSHOLD_KB                50             /* FLASH sector thershold for preparing the next sector for writing (earase sector) */    
#define LOGGER_CONFIG_DOUBLE_BUFFER_SIZE_BYTES               1024           /* double buffer size in bytes */
#define LOGGER_CONFIG_NVR_ADDRESS                            0x7F60         /* the address to read\write NVR parmeters */
#define LOGGER_CONFIG_NVR_SIZE_BYTES                         16             /* the size of internal logger NVR DB */
#define LOGGER_CONFIG_MAX_RX_MESSAGE_SIZE_BYTES              256            /* maximum logger message RX packet in bytes */
#define LOGGER_CONFIG_MAX_TX_MESSAGE_SIZE_BYTES              256            /* maximum logger message TX packet in bytes */
#define LOGGER_CONFIG_FLASH_START_ADDRESS                    0x000A0000     /* the flash start address of debug messages */
#define LOGGER_CONFIG_OPEN_SOCKET_TIMEOUT_MS                 5000           /* the maximum timeout it takes to open a TCP\UDP socket */
#define LOGGER_CONFIG_PANEL_ID_SIZE_BYTES                    9              /* the size of the panel ID in bytes including null terminator */
#define LOGGER_CONFIG_FLASH_MIN_SEGMENT_SIZE_KB              128            /* the minimum size of flash debug log file allowed */
#define LOGGER_CONFIG_FLASH_MAX_SEGMENT_SIZE_KB              256            /* the maximum size of flash debug log file allowed */
#define LOGGER_CONFIG_LOW_RESOLUTION_MS                      10             /* the low (regular) logger TX resolution in ms */
#define LOGGER_CONFIG_HIGH_RESOLUTION_MS                     1              /* the high logger TX resolution in ms */
#define LOGGER_CONFIG_MAX_SECTOR_ERASE_TIMEOUT_MS            3000           /* the maximum timeout for sector erase in ms */
#define LOGGER_CONFIG_FTP_FILE_NAME_LEN                      25             /* the length of FTP log file name */
/* Socket configuration */
#define LOGGER_CONFIG_SOCKET_MAX_IP_LEN                      64             /* the maximum length of ip or domain name allowed */

/* FTP configuration */
#define LOGGER_CONFIG_FTP_MAX_PATH_LEN                       64             /* the maximum length of FTP path name allowed including null terminator */
#define LOGGER_CONFIG_FTP_MAX_USER_NAME_LEN                  11             /* the maximum FTP user name allowed including null terminator */
#define LOGGER_CONFIG_FTP_MAX_PASSWORD_LEN                   11             /* the maximum FTP password length allowed including null terminator */

#define LOGGER_CONFIG_BUFFER_OVERRUN_MARK_CHAR               '^'            /* a special non alphabetic char to mark buffer over */                                
/* --------------------------- Logger default configuration -------------------------------------*/
#define LOGGER_CONFIG_DEFAULT_FLASH_SEGMENT_SIZE_KB          LOGGER_CONFIG_FLASH_MAX_SEGMENT_SIZE_KB  /* default FLASH segment size in KB */

/* Logger remote debug server configuration */
#define LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_DNS              "192.168.0.116"            /* default remote server IP */
#define LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_DNS_LEN          14                         /* default remote server DNS length */
#define LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_PORT             6969                       /* default remote server port */
#define LOGGER_CONFIG_DEFAULT_LOCAL_PORT                     1101                       /* default logger local port */
#define LOGGER_CONFIG_DEFAULT_SOCKET_TYPE                    LOGGER_SOCKET_TYPE_UDP     /* default socket type is UDP */
#define LOGGER_CONFIG_DEFAULT_FLASH_SIZE                     LOGGER_CONFIG_DEFAULT_FLASH_SEGMENT_SIZE_KB
#define LOGGER_CONFIG_DEFAULT_RESOLUTION_MS                  LOGGER_CONFIG_LOW_RESOLUTION_MS
#define LOGGER_CONFIG_DEFAULT_FILE_SIZE_KB                   LOGGER_CONFIG_DEFAULT_FLASH_SEGMENT_SIZE_KB                      
#define LOGGER_CONFIG_DEFAULT_ATTRIBUTES                     0

/* Logger default manage configuration */
#define LOGGER_CONFIG_DEFAULT_MODE_TYPE                      LOGGER_MODE_TYPE_PUSH      /* default mode type */
#define LOGGER_CONFIG_DEFAULT_LOG_LEVEL                      LEVEL_FLOW                 /* default log level */
#define LOGGER_CONFIG_DEFAULT_LOG_MASK                       LOGGER_MASK_NO_TASK        /* default log mask */
#define LOGGER_CONFIG_DEFAULT_IS_PRINTOUT_ENABLED            TRUE                       /* default printout enabled */
#define LOGGER_CONFIG_DEFAULT_DESTINATION_TYPE               LOGGER_DEST_TYPE_SOCKET    /* default destination type */
#ifdef OSAL_DEBUG_MODE
#define LOGGER_CONFIG_DEFAULT_FEATURE_IS_ENABLED             TRUE                       /* default feature is enabled */
#else
#define LOGGER_CONFIG_DEFAULT_FEATURE_IS_ENABLED             FALSE                      /* default feature is disabled */
#endif

/* FTP default configuration */
#define LOGGER_CONFIG_DEFAULT_FTP_USERNAME                   "erez"                     /* default FTP username */
#define LOGGER_CONFIG_DEFAULT_FTP_PASSWORD                   "1"                        /* default FTP password */
#define LOGGER_CONFIG_DEFAULT_FTP_DNS                        "212.29.221.88"            /* default FTP DNS */
#define LOGGER_CONFIG_DEFAULT_FTP_PATH                       "\\josh"                   /* default FTP path */

