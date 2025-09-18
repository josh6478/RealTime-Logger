
enum {
    e_LOGGER_CMD_DEBUG_START,
    e_LOGGER_CMD_DEBUG_STOP
};

void LoggerDebug_PrintRAMstatus(void);
void LoggerDebug_PrintConfig(void);
void LoggerDebug_TestStartStop(uint8 IsStart, uint16 Timeout, uint8 NumOfTasks);
