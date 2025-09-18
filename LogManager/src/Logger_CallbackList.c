#include "LogManager.h"

/* below is the logger callback function. every platform needs to fill in its own implementation to every callback so the feature can work */

LoggerCB_t gLoggerCbList {
    WeRLogger_OpenSocketCb,
    NULL,
    NULL,
    WeRLogger_SendUartDataCb,
    WeRLogger_SendFlashDataCb,
    WeRLogger_EraseFlashSectorCb,
    NULL,
    WeRLogger_GetDateAndTimeCb,
    NULL,
    WeRLogger_ReadNVRParamsCb,
    WeRLogger_GetPanelIdCb,
};