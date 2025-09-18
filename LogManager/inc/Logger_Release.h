/*--------------------------------------------------------------------------------------
 * Author: Joshua Levi
 * Date: 17/01/2021
 * Description: Logger Release notes
 * -------------------------------------------------------------------------------------
 
#define LOGGER_VERSION  "1.11"

/*==========================================================================
 *   DATE           : 27/06/2021
 *   Version number : 1.11
 *   Description    : Initial implementation
 //==========================================================================

    Requirement Num | Implemented          |  Remarks
    ----------------+----------------------+----------------------------------------------
    R3.1                Yes
    R3.2                Yes
    R3.3                Yes
    R3.4                Partial             TCP is currently not supported
    R3.5                Yes
    R3.6.1              Yes
    R3.6.2              Yes
    R3.6.3              Yes             
    R3.6.4              No                  We need to have a newer IAR compiler to support this
    R3.6.5              Yes                 This paramter is configurable per platform now
    R3.6.6              Yes
    R3.7.1              Yes                 
    R3.7.2              Yes                 
    R3.7.3              Partial             Automatic mode is currently not supported
    R3.7.4              Yes
    R3.7.5              Yes                 
    R3.7.6              Yes
    R3.8.1              Yes                  
    R3.8.2              Yes                  
    R3.8.3              Yes                  
    R3.8.4              Yes                  
    R3.8.5              Yes                  
    R3.9.2              Yes                 
    R3.9.3              Yes             
    R3.9.4              Yes                  
    R3.9.5              Yes             
    R3.9.6              Yes             
    R3.9.7              Yes             
    R3.9.8              Yes
    R3.9.9              Yes                 
    R3.9.10             Yes                 
    R3.9.11             Partial             Currently only enabled logger is not supported after reset
    R3.10.1             Yes
    R3.10.2             No                  This requirement was for debug, will not be supported
    R3.10.3             Yes


Known issus:
------------

1. when working in pull mode and stressing the log file (send 60 bytes message every 10 msec), sometimes some of the logs are not writen to the flash correctly
   I suspecet this is due to the FLASH driver poor realtime performance, as I didnt see such problem when working with RS232 or UDP
*/

