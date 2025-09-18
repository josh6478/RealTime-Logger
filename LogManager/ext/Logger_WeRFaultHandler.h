#define LAST_HEAP_ADDRESS 0x20083FC0
#define MAGIC_NUM         0x06041978    
#define MAGIC_ADDRESS               ((unsigned long*)LAST_HEAP_ADDRESS)[0]    
#define R0_REG_ADDRESS              ((unsigned long*)LAST_HEAP_ADDRESS)[1]
#define R1_REG_ADDRESS              ((unsigned long*)LAST_HEAP_ADDRESS)[2]
#define R2_REG_ADDRESS              ((unsigned long*)LAST_HEAP_ADDRESS)[3]
#define R3_REG_ADDRESS              ((unsigned long*)LAST_HEAP_ADDRESS)[4]
//#define R12_REG_ADDRESS           ((unsigned long*)LAST_HEAP_ADDRESS)[5]
#define LR_REG_ADDRESS              ((unsigned long*)LAST_HEAP_ADDRESS)[5]
#define PC_REG_ADDRESS              ((unsigned long*)LAST_HEAP_ADDRESS)[6]
//#define PSR_REG_ADDRESS           ((unsigned long*)LAST_HEAP_ADDRESS)[8]
#define DBG_TASK_NAME_ADDRESS       ((char*)LAST_HEAP_ADDRESS + 28)
#define DBG_TASK_CMD_ADDRESS        ((unsigned short*)LAST_HEAP_ADDRESS)[16]
#define DBG_TASK_ID_ADDRESS         ((unsigned char*)LAST_HEAP_ADDRESS)[34]
#define TIME_AND_DATE_ADDRESS       ((unsigned char*)LAST_HEAP_ADDRESS)[35]

#define IS_FAULT_HANDLER()          (MAGIC_ADDRESS == MAGIC_NUM)
#define RESET_FAULT_HANDLER()       MAGIC_ADDRESS = 0


void Logger_PrintFaultHandler(void);
void Logger_RS232PrintFaultHandler(void);
