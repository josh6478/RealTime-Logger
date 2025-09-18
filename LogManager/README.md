
# Realtime Logger

A lightweight, high-throughput remote logging system for embedded panels that supports **Push** and **Pull** modes, double-buffered, low-latency transmission, and multiple backends (Socket/FTP/UART). Designed to coexist with real-time workloads such as live video streaming without impacting performance.

> This README is a developer-facing summary and integration guide extracted from the system design. It focuses on how to use, integrate, and test the logger module.

---

## Table of Contents
- [Overview](#overview)
- [Key Features](#key-features)
- [Operating Modes](#operating-modes)
- [Architecture](#architecture)
- [Message Format](#message-format)
- [Public API](#public-api)
- [Integration Callbacks](#integration-callbacks)
- [Configuration](#configuration)
- [Control & Diagnostics](#control--diagnostics)
- [Assumptions & RT Constraints](#assumptions--rt-constraints)
- [Testing Strategy (Summary)](#testing-strategy-summary)
- [Example](#example)
- [FAQ](#faq)
- [License](#license)

---

## Overview

**WeR Logger** enables remote debug and telemetry prints (DIM = Debug Information Message) from the panel to a remote server. It supports on-device buffering (Pull) and live streaming (Push), selective verbosity by module and level, and simultaneous mirroring to UART.

Typical use cases:
- Field diagnostics with minimal server load (Pull).
- Beta/bring-up sites that stream live warnings/errors (Push).
- Post-mortem analysis via FLASH dumps or FTP file uploads.

## Key Features

- **Transports**: UDP/TCP sockets (live streaming), FTP (file upload), UART (local).
- **Two modes**:
  - **Pull**: buffer DIMs to FLASH; upload by command or threshold.
  - **Push**: stream DIMs from RAM immediately after reboot.
- **Selective logging**:
  - Levels: `ERROR`, `WARNING`, `INFO`, `DETAILS`, `FLOW`.
  - Per-module mask with optional “always-send ERROR” bypass.
- **Low footprint / RT friendly**:
  - 1024-byte static RAM buffer (configurable double-buffer of 255-byte packets).
  - Write path ≤ 0.1 ms (logger) and ≤ 5 ms (lower layers) per 255 B.
- **Resilience**:
  - Wrap-around FLASH segment; resume write position across reboots.
  - COMM owns socket lifecycle; logger is stateless wrt reconnects.
- **Observability**: counters, pointers, queue snapshots, socket stats.

## Operating Modes

### Pull Mode
- DIMs are stored in **FLASH** (wrap-around segment; default ≥ 128 KB).
- Trigger FTP **PUT** of a Debug Information File (DIF) via command or when segment reaches a threshold.
- DIF file name includes **panel serial**, **date**, **time**.

### Push Mode
- After reboot, open UDP/TCP socket and **stream DIMs live**.
- First DIM includes **panel ID**, **serial**, **date**, **time**.
- No retransmissions; UDP includes a **sequence number** in header.

> In both modes, messages can **also** mirror to **UART**.

## Architecture

```
Application Tasks  -->  Logger (double buffer, filters)  -->  Destinations
                                               |                + UART (COM)
                                               |                + Socket (UDP/TCP) via COMM
                                               + FLASH (wrap)   + FTP (PUT) from FLASH
```

- **Application Layer**: calls `DBG_PRINT*` and Logger API.
- **Logger**: parses/filters DIMs, enqueues to double buffer, triggers TX (Socket/FLASH/UART).
- **COMM Layer**: session types (`ECOP`, `RSU`, `DEBUG`), priority queues, multi-socket, modem selection (Ethernet primary, GSM secondary).
- **Ethernet/GSM Midlayers**: socket open/close, errors, stats; owned by COMM.

## Message Format

### Start (session opening)
```
DATE:DD-MM-YY TIME:HH-MM-SS CP:XXXXXXXX LEVEL:<Info|...> MASK:0xZZ
```

### Each DIM
```
<LEVEL:> <TaskIdHex> <FunctionName> [<Line>] [<<|>>] <DebugMessage>\r\n\0
```

**Constraints**
- Max message length **255** chars including control chars.
- Overrun is marked with a **non-alphabetic sentinel**.
- Header fields: Level, Module/Task ID, Function, optional Line/Flow, message.

## Public API

```c
// Lifecycle
LOGGER_STATUS Logger_InitApi(...);
LOGGER_STATUS Logger_UnintApi(void);

// Configuration
LOGGER_STATUS Logger_SetLevelApi(Level_t level);              // ERROR..FLOW
LOGGER_STATUS Logger_SetModuleMaskApi(uint8_t bitmap);        // per-module mask
LOGGER_STATUS Logger_SetModeApi(Mode_t mode);                 // PUSH | PULL
LOGGER_STATUS Logger_SetRemoteServerApi(RemoteServer_t* rs);  // FTP/TCP/UDP details
LOGGER_STATUS Logger_SetFLASHSegmentSizeApi(uint16_t kb);

// Enable/Disable & printout
LOGGER_STATUS Logger_EnableApi(void);
LOGGER_STATUS Logger_DisableApi(void);
LOGGER_STATUS Logger_EnablePrintoutApi(void);   // UART mirror ON
LOGGER_STATUS Logger_DisablePrintoutApi(void);  // UART mirror OFF

// Actions
LOGGER_STATUS Logger_PullDebugMessagesApi(uint16_t sizeKB); // FLASH -> FTP
LOGGER_STATUS Logger_DumpDebugMessagesApi(uint16_t sizeKB); // FLASH -> UART

// Macros (examples)
DBG_PRINT(level, "msg(%d)\n", val);
DBG_PRINT_LINE(level, "msg");
DBG_PRINT_ENTRY(level);
DBG_PRINT_EXIT(level);
```

## Integration Callbacks

The logger is transport-agnostic; you provide these callbacks:

```c
LOGGER_STATUS OpenSocketCb(LoggerSocketConfig_t* cfg);
LOGGER_STATUS CloseSocketCb(void);
LOGGER_STATUS SendSocketDataCb(uint8_t* buf, uint16_t len);
LOGGER_STATUS SendUartDataCb(uint8_t* buf, uint16_t len);
LOGGER_STATUS SendFlashDataCb(uint8_t* addr, uint8_t* buf, uint16_t len);
LOGGER_STATUS SendFtpDataCb(uint8_t* addr, uint8_t sizeKB, char filename[]);
LOGGER_STATUS SaveLoggerNVRCb(uint8_t* addr, void* buf, uint8_t size);
LOGGER_STATUS ReadLoggerNVRCb(uint8_t* addr, void* buf, uint8_t size);
```

**Notes**
- COMM owns socket persistence (constant vs temporary sessions).
- UDP should add per-DIM sequence number.
- On reboot, resume FLASH write pointer from NVR/EEPROM snapshot.

## Configuration

Define (project-specific) constants:

```c
// Hard-coded
#define LOGGER_CONFIG_DOUBLE_BUFFER_SIZE_BYTES     (/* e.g., 1024 */)
#define LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB         (/* e.g., 4 */)
#define LOGGER_CONFIG_REMOTE_SERVER_DNS_LEN        (/* e.g., 64 */)
#define LOGGER_CONFIG_NVR_SIZE_BYTES               (/* e.g., 32 */)
#define LOGGER_CONFIG_NVR_ADDRESS                  (/* platform addr */)
#define LOGGER_CONFIG_MAX_MESSAGE_SIZE             (255)
#define LOGGER_CONFIG_DOUBLE_BUFFER_TX_PAKCET_SIZE (255)

// Defaults
#define LOGGER_CONFIG_DEFAULT_FLASH_SEGMENT_SIZE_KB   (128)
#define LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_DNS       ("debug.example.com")
#define LOGGER_CONFIG_DEFAULT_REMOTE_SERVER_PORT      (10001)
#define LOGGER_CONFIG_DEFAULT_LOCAL_PORT              (0)
#define LOGGER_CONFIG_DEFAULT_SOCKET_TYPE             (UDP_OR_TCP)
#define LOGGER_CONFIG_DEFAULT_MODE_TYPE               (PULL_OR_PUSH)
#define LOGGER_CONFIG_DEFAULT_LOG_LEVEL               (LEVEL_INFO)
#define LOGGER_CONFIG_DEFAULT_IS_PRINTOUT_ENABLED     (false)
```

Runtime configuration is supported via API and persists across reboot.

## Control & Diagnostics

- **Remote/Local commands** (WeR manager or RS232):
  - Enable/Disable, Set Mode (requires disabled state), Set Remote Server, Change Level, Change Module Mask, Set FLASH Segment Size, Pull DIF, Dump DIF.
- **Diagnostics**:
  - Logger counters (RX messages, TX packets), double-buffer snapshot, TX/RX pointers + occupancy.
  - COMM/Ethernet socket stats (TX/RX byte counters, connection time, state, IP/port).

## Assumptions & RT Constraints

- 100 MHz CPU; video RT uses ~138 Kbps.
- Logger designed for **≤ 50 Kbps** sustained DIM throughput; combined system target ≈ **200 Kbps**.
- Lower layers must complete **255B write ≤ 5 ms**. Logger path ≤ **0.1 ms** per write.
- Use responsibly: avoid tight-loop prints, keep messages short, don’t enable all modules at once.

## Testing Strategy (Summary)

- **Comm Positives**: Ethernet/GSM connect, backup channel failover, FTP PUT, concurrent sockets.
- **Comm Negatives**: wrong APN/DNS/MAC/routing; unresponsive modules; socket/file errors; TX/RX error paths.
- **Stress/Stability**: video + logger coexistence, timeouts, burn tests, server up/down.
- **Acceptance/Regression**: core scenarios across modes and transports.

## Example

```c
DBG_PRINT(LEVEL_DETAILS, "Handle trigger event TrigId(%d) IsForce(%d)\n", TrigId, IsForce);

if(!RulesDb_IsRuleExist(...)) {
    DBG_PRINT(LEVEL_WARNING, "RULE(%d) DOES NOT EXIST!\n", RuleId);
    return;
}

DBG_PRINT_ENTRY(LEVEL_FLOW);
// ...
DBG_PRINT_EXIT(LEVEL_FLOW);
```

**Sample DIMs**
```
INFO: 01 Rules_CalcDurationMsec Calculate minutes(20)\r\n\0
FLOW: 01 Rules_SubscribeDateTrigger 246 >>\r\n\0
FLOW: 01 Rules_SubscribeDateTrigger 320 <<\r\n\0
```

**Start banner (Push)**
```
DATE:22-12-15 TIME:17-51-00 CP:000031A8 LEVEL:Info MASK:0xFF
```

## FAQ

**Q: Can I stream to socket and still mirror to UART?**  
Yes. UART can be enabled in parallel with socket/FLASH.

**Q: What happens on UDP packet loss?**  
No retransmissions; DIMs may be lost. Consider TCP for reliability or Pull mode for completeness.

**Q: How are sockets managed?**  
COMM handles opening/reopening and prioritization. Logger just writes via callbacks.

## License

Free - No license needed

---

> _Tip_: Keep messages short and informative; avoid logging in tight loops and idle polling to maintain RT safety.
