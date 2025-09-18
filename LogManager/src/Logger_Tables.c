const char * LoggerLevelTableStr[] = {
    "",
    "CRITICAL",
    "ERROR",
    "WARNING",
    "INFO",
    "DETAILS",
    "FLOW"
};

const char * LoggerStateStr[] = {
    "DOWN",
    "INIT",
    "OPEN_SOCKET",
    "UPLOAD_FILE",
    "READY",
};

const char * LoggerDestTypeStr[] = {
    "NONE",
    "FLASH",
    "SOCKET",
};

const char * LoggerModeStr[] = {
    "PUSH",
    "PULL",
    "TEST",
    "NONE",
};

const char * LoggerSocketTypeStr[] = {
    "TCP",
    "UDP",
    "FTP",
};
