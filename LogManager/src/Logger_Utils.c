#include <stdio.h>
#include <stdarg.h>
#include "Logger_Defs.h"
#include "Logger_Manager.h"
#include "Logger_Func.h"
#include "Logger_Release.h"
#include "RTC_API.h"
#include "clock.h"
#include "SSP_SPI_API.h"
#include "FLASH_API.h"

                /* ========================================== *
                 *          D E F I N I T I O N S             *
                 * ========================================== */

#define CALC_REMAIN_BUFF_SIZE(P_READ, P_WRITE, BUF_SIZE)    ((P_READ>P_WRITE)?(P_READ-P_WRITE):(BUF_SIZE - (P_WRITE - P_READ)))
#define CALC_BUF_SIZE_TO_READ(P_READ, P_WRITE, P_TAIL)      (P_WRITE>=P_READ)?(P_WRITE - P_READ):(P_TAIL - P_READ)

#define P_BUFF_ITER                                         gLoggerManager.RxBuffer.pWrite
#define P_BUFF_TAIL                                         gLoggerManager.RxBuffer.pTail
#define P_BUFF_HEAD                                         gLoggerManager.RxBuffer.pHead
#define P_BUFF_WRITE                                        gLoggerManager.RxBuffer.pWrite
#define P_BUFF_READ                                         gLoggerManager.RxBuffer.pRead

                /* ========================================== *
                 *     P R I V A T E     V A R I A B L E S    *
                 * ========================================== */
extern LoggerManager_t gLoggerManager;
                /* ========================================== *
                 *     P R I V A T E     F U N C T I O N S    *
                 * ========================================== */


/**
 * <pre>
 * static inline int mini_itoa(int InputNum, char* buff, int base, int MaxLen)
 * </pre>
 *  
 * this function is a customize lean itoa function - transform number to string
 *
 * @param   InputNum  [in]       The input number to transform
 * @param   buff      [out]      The output number in string
 * @param   base      [in]       The base number (decimal, hexa)
 * @param   MaxLen    [in]       The maximum length of the ouput string in bytes
 *
 * @return the length of the output string number in bytes
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static inline int mini_itoa(IN int InputNum, OUT char* buff, IN int base, IN int MaxLen)
{
    int len = 0, i;
    int origNum = InputNum;
    /* make sure we dont exceed the maximum length */
    if(MaxLen == 0)
        return 0;
    
    if(base == 10)
    {
        /* base number is decimal */
        for(len=0; InputNum; InputNum = InputNum/10, len++);
        if(origNum<0)
        {
            /* this is a negative number add the minus sign */
            origNum = -origNum;
            *buff++ = '-';
            MaxLen--;
            len++;
        }
        
        /* if we exceed the maximum lenght - transform only a part fot the number */
        if(len > MaxLen)
            len = MaxLen;
        
        /* converts digits from 0 to 9 */
        for(i = len ; i > 0; origNum/=10 , i--)
            buff[i - 1] = (origNum % 10) + '0';
    }
    if(base == 16)
    {
       /* this is a hexa decimal base */       
       uint32 HexNum = (uint32)InputNum;
       
       for(len=0; HexNum; HexNum = HexNum>>4, len++);
       
       /* in hexa make sure we always have at list 2 digits unless we exceed the maximum len */
       if(len < 2)
           len = 2;
       
       if(len > MaxLen)
           len = MaxLen;
       
       for(i=len; i>0; i--)
        {
            char digit = origNum&0xF;
            /* converts digits from 0 to F */
            buff[i - 1] = digit>9?digit - 10 + 'A': digit + '0'; 
            origNum = origNum>>4;
        }
    }
    if(!len)
    {
        /* if length is 0 return the digit 0 */
        len = 1;
        buff[0] = '0';
    }
    /* return a pointer to the end of the string */
    return len;
}


/**
 * <pre>
 * static inline void Logger_ParseAndWriteNumber(int InputNum, int base, int *pMaxSize, uint8 LeftOverSize)
 * </pre>
 *  
 * this function writes integer\hexa number to string into a cyclic buffer
 *
 * @param   InputNum        [in]       The input number to transform
 * @param   base            [in]       The base number (decimal, hexa)
 * @param   pMaxSize        [inout]    The maximum size allowed till the end of the cyclic buffer 
 * @param   LeftOverSize    [in]       The maximum size allowed after the wrap arround
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static inline void Logger_ParseAndWriteNumber(IN int InputNum, IN int base, INOUT int *pMaxSize, IN uint8 LeftOverSize)
{
    /* maximum number of digit possible are 2^32 = 10 digits +1 (-) sign */
    char NumStr[11];
    
    uint8 NumOfDigits = mini_itoa(InputNum, NumStr, base, 11);
    
    uint8 i;
    
    int MaxSize = *pMaxSize;
    
    /* simple case no split needed */
    if(NumOfDigits < MaxSize)
    {
        
        for(i=0; NumOfDigits; *P_BUFF_ITER++ = NumStr[i++], MaxSize--, NumOfDigits--);
        
                        
    }
    /* singual case - need to split to 2 messages */
    else
    {
        for(i = 0; MaxSize; *P_BUFF_ITER++ = NumStr[i++], MaxSize--, NumOfDigits--);
        
        /* wrap around */
        P_BUFF_ITER = P_BUFF_HEAD;
        
        MaxSize = LeftOverSize;
        
        /* continue from the buffer head */
        for(; NumOfDigits && MaxSize; *P_BUFF_ITER++ = NumStr[i++], MaxSize--, NumOfDigits--);

    }    
    *pMaxSize = MaxSize;
    if(P_BUFF_WRITE == P_BUFF_TAIL)
    {
        P_BUFF_WRITE = P_BUFF_HEAD;
    }

}

/**
 * <pre>
 * static inline char *Logger_ParseAndWriteString(char *pStr, int *pMaxSize, uint8 LeftOverSize, char DEL)
 * </pre>
 *  
 * this function writes input string into the cyclic buffer until a delimineter is reached or NULL character is reached or until a maximum size reached.
 *
 * @param   pStr            [in]       The input string
 * @param   pMaxSize        [inout]    The maximum size allowed till the end of the cyclic buffer 
 * @param   LeftOverSize    [in]       The maximum size allowed after the wrap arround
 * @param   DEL             [in]       Optional.The delimineter character marks the end of the input buffer
 *                                     if DEL value is 0xFF than delimineter is not valid.
 *
 * @return the pointer of the input buffer address (after the write operation)
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static inline char *Logger_ParseAndWriteString(IN char *pStr, INOUT int *pMaxSize, IN uint8 LeftOverSize, IN char DEL)
{
    int MaxSize = *pMaxSize;
    
    if(DEL == 0xFF)
    {
        /* delimineter is not valid, search for either NULL character or maximum limit */
        if(!LeftOverSize)                    
        {
            /* no wrap around condition, write the entire string */
            for(;*pStr && MaxSize; *P_BUFF_ITER++ = *pStr++, MaxSize--);
        }
        else
        {
            for(;*pStr && MaxSize; *P_BUFF_ITER++ = *pStr++, MaxSize--);
            
            if(*pStr)
            {
                /* wrap around */
                P_BUFF_ITER = P_BUFF_HEAD;
                
                if(MaxSize == 0)
                {
                    MaxSize = LeftOverSize;
                }
                
                /* continue from the buffer head */
                for(;*pStr && MaxSize; *P_BUFF_ITER++ = *pStr++, MaxSize--);
            }
        }
    }
    else
    {
        if(!LeftOverSize)                    
        {
            for(;*pStr && *pStr != DEL && MaxSize; *P_BUFF_ITER++ = *pStr++, MaxSize--);
        }
        else
        {
            for(;*pStr && *pStr != DEL && MaxSize; *P_BUFF_ITER++ = *pStr++, MaxSize--);
            
            if(*pStr && MaxSize == 0)
            {
                /* wrap around */
                P_BUFF_ITER = P_BUFF_HEAD;
                
                MaxSize = LeftOverSize;
                
                /* continue from the buffer head */
                for(;*pStr && *pStr != DEL && MaxSize; *P_BUFF_ITER++ = *pStr++, MaxSize--);
            }
        }
        
    }
    if(P_BUFF_WRITE == P_BUFF_TAIL)
    {
        P_BUFF_WRITE = P_BUFF_HEAD;
    }
    if(MaxSize == 0)
    {
        MaxSize = LeftOverSize;
    }
    *pMaxSize = MaxSize;

    return pStr;
}

/**
 * <pre>
 * static inline void Logger_ParseAndWriteRawData(char *pInBuff, int BufLen, int *pMaxSize, uint8 LeftOverSize)
 * </pre>
 *  
 * this function writes input binary buffer into the cyclic buffer transform into ascii
 *
 * @param   pInBuff         [in]    The input binary buffer
 * @param   BufLen          [in]    The input binary buffer length in bytes
 * @param   pMaxSize        [inout]    The maximum size allowed till the end of the cyclic buffer 
 * @param   LeftOverSize    [in]       The maximum size allowed after the wrap arround
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static inline void Logger_ParseAndWriteRawData(IN char *pInBuff, IN int BufLen, INOUT int *pMaxSize, IN uint8 LeftOverSize)
{   
    
    int MaxSize = *pMaxSize;
    uint8 digit;
    /* simple case no split needed */
    
    if(BufLen+BufLen<= MaxSize)
    {        
        for(;BufLen; BufLen--, pInBuff++, MaxSize-=2)
        {
            digit = (*pInBuff&0xF0)>>4;
            /* converts digits from 0 to F */
            *P_BUFF_ITER++ = digit>9?digit - 10 + 'A': digit + '0';
            digit = *pInBuff&0xF;
            *P_BUFF_ITER++ = digit>9?digit - 10 + 'A': digit + '0';
            
            if(P_BUFF_ITER == P_BUFF_TAIL)
            {
                /* wrap around */
                P_BUFF_ITER = P_BUFF_HEAD;                
            }
        }
    }
    else
    {
        /* singular case were we need to wrap around and the size left to the end of buffer is odd */
        if(MaxSize & 1)
        {
            *P_BUFF_ITER++ = ' ';
            MaxSize--;
        }
        
        for(;MaxSize>0; BufLen--, pInBuff++, MaxSize-=2)
        {
            digit = (*pInBuff&0xF0)>>4;
            /* converts digits from 0 to F */
            *P_BUFF_ITER++ = digit>9?digit - 10 + 'A': digit + '0';
            digit = *pInBuff&0xF;
            *P_BUFF_ITER++ = digit>9?digit - 10 + 'A': digit + '0';
        }
        if(P_BUFF_ITER == P_BUFF_TAIL)
        {
            /* wrap around */
            P_BUFF_ITER = P_BUFF_HEAD;
        
            MaxSize = LOGGER_DEF_MIN(LeftOverSize, BufLen + BufLen);

            for(;MaxSize>1; MaxSize-=2, pInBuff++)
            {
                digit = (*pInBuff&0xF0)>>4;
                /* converts digits from 0 to F */
                *P_BUFF_ITER++ = digit>9?digit - 10 + 'A': digit + '0';
                digit = *pInBuff&0xF;
                *P_BUFF_ITER++ = digit>9?digit - 10 + 'A': digit + '0';
            }
        }
    }
    *pMaxSize = MaxSize;
}

/**
 * <pre>
 * static inline void Logger_ParseAndWriteBinBuff(unsigned char *pInBuff, int BufLen, int *pMaxSize, uint16 LeftOverSize)
 * </pre>
 *  
 * this function writes input binary buffer into the cyclic buffer as is (w/o transform into ascii)
 *
 * @param   pInBuff         [in]    The input binary buffer
 * @param   BufLen          [in]    The input binary buffer length in bytes
 * @param   pMaxSize        [inout]    The maximum size allowed till the end of the cyclic buffer 
 * @param   LeftOverSize    [in]       The maximum size allowed after the wrap arround
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static inline void Logger_ParseAndWriteBinBuff(IN unsigned char *pInBuff, IN int BufLen, INOUT int *pMaxSize, IN uint16 LeftOverSize)
{
    int MaxSize = *pMaxSize;
    
    /* sanity */
    if(BufLen > LOGGER_CONFIG_MAX_RX_MESSAGE_SIZE_BYTES)
    {
        BufLen = LOGGER_CONFIG_MAX_RX_MESSAGE_SIZE_BYTES;
        //OSAL_ASSERT(0);
    }
    /* simple case no split needed */
    if(BufLen<= MaxSize)
    {
        P_BUFF_ITER = (char*)OSAL_MemCopy(P_BUFF_ITER, pInBuff, BufLen);
        
        MaxSize -= BufLen;
        
        /* singular case after memcopy we reached the end of buffer */
        if(P_BUFF_ITER == P_BUFF_TAIL)
        {
            P_BUFF_ITER = P_BUFF_HEAD;
        }
    }
    else
    {
        P_BUFF_ITER = (char *)OSAL_MemCopy(P_BUFF_ITER, pInBuff, MaxSize);
        /* wrap around */
        if(P_BUFF_ITER == P_BUFF_TAIL)
        {
            P_BUFF_ITER = P_BUFF_HEAD;
        
            pInBuff+= MaxSize;

            MaxSize = LOGGER_DEF_MIN(BufLen - MaxSize, LeftOverSize);
        
            if(MaxSize)
            {
                P_BUFF_ITER = (char*)OSAL_MemCopy(P_BUFF_ITER, pInBuff, MaxSize);
            }
        }
        MaxSize = 0;
    }
    *pMaxSize = MaxSize;
}

/**
 * <pre>
 * static void Logger_DateAndTimeWrite2digits(int *pMaxSize, uint16 LeftOverSize, uint8 Num, char DEL)
 * </pre>
 *  
 * this function transforms 2 digits to ascii and a deliminiter afterwards into the cyclic buffer
 *
 * @param   pMaxSize        [inout]    The maximum size allowed till the end of the cyclic buffer 
 * @param   LeftOverSize    [in]       The maximum size allowed after the wrap arround
 * @param   Num             [in]       The number containing 2 digits
 * @param   DEL             [in]       The delimineter charcter to write after 2 digits.
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static void Logger_DateAndTimeWrite2digits(INOUT int *pMaxSize, IN uint16 LeftOverSize, IN uint8 Num, IN char DEL)
{
    char TwoDigits[2];
    OSAL_num2decstr(Num, (uint8*)TwoDigits, 2);
    
    *P_BUFF_ITER++=TwoDigits[0];
    if(--(*pMaxSize) == 0)
    {
        P_BUFF_ITER = P_BUFF_HEAD;
        *pMaxSize = LeftOverSize;
    }
    *P_BUFF_ITER++=TwoDigits[1];
    if(--(*pMaxSize) == 0)
    {
        P_BUFF_ITER = P_BUFF_HEAD;    
        *pMaxSize = LeftOverSize;
    }
    *P_BUFF_ITER++= DEL;
    if(--(*pMaxSize) == 0)
    {
        P_BUFF_ITER = P_BUFF_HEAD;    
        *pMaxSize = LeftOverSize;
    }
}
/**
 * <pre>
 * static inline void Logger_WriteTimeAndDate(int *pMaxSize, uint16 LeftOverSize)
 * </pre>
 *  
* this function writes current date and time into the cyclic buffer in form of DD/MM/YY HH:MM:SS
 *
 * @param   pMaxSize        [inout]    The maximum size allowed till the end of the cyclic buffer 
 * @param   LeftOverSize    [in]       The maximum size allowed after the wrap arround
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static inline void Logger_WriteTimeAndDate(INOUT int *pMaxSize, IN uint16 LeftOverSize)
{
#define DATE_AND_TIME_STR_SIZE 18    
    RTC_Time_t TimeTable;
    
    /* sanity dont write date and time if you dont have the size for it */
    if((*pMaxSize) + LeftOverSize < DATE_AND_TIME_STR_SIZE)
    {
        return;
    }
    RTC_GetTimeAndDate(&TimeTable);
    
    Logger_DateAndTimeWrite2digits(pMaxSize, LeftOverSize, TimeTable.DayOfMonth, '/');
    Logger_DateAndTimeWrite2digits(pMaxSize, LeftOverSize, TimeTable.Month, '/');
    Logger_DateAndTimeWrite2digits(pMaxSize, LeftOverSize, TimeTable.Year - CLOCK_BASE_YEAR, ' ');
    Logger_DateAndTimeWrite2digits(pMaxSize, LeftOverSize, TimeTable.Hour, ':');
    Logger_DateAndTimeWrite2digits(pMaxSize, LeftOverSize, TimeTable.Minute, ':');
    Logger_DateAndTimeWrite2digits(pMaxSize, LeftOverSize, TimeTable.Second, ' ');        
}

/**
 * <pre>
 * static inline int Logger_ParseAndWriteRxMessage(uint8 level, char *fmt, va_list ap)
 * </pre>
 *  
 * this function parse a customized printf style RX log message and write into into a cyclic buffer
 *
 * @param   level        [in]       The log level
 * @param   fmt          [in]       a pointer to input string
 * @param   ap           [in]       a pointer to arbitrary list of parameters to print
 *
 * @return the length of the writen (parsed) log message in bytes
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static inline int Logger_ParseAndWriteRxMessage(IN uint8 level, IN char *fmt, IN va_list ap)
{   
    int d, Len;
    char c, *s, *base = P_BUFF_ITER;
    
    /* max size is the minimum of the maximum message allowed and the buffer size left till the end of buffer */
    int MaxSize = LOGGER_DEF_MIN(LOGGER_CONFIG_MAX_RX_MESSAGE_SIZE_BYTES, P_BUFF_TAIL - P_BUFF_ITER);
    
    /* in case we have a buffer wrap arround case we need to calculate the left over */    
    uint8 BuffLeftOverSize = LOGGER_CONFIG_MAX_RX_MESSAGE_SIZE_BYTES - MaxSize;
    
    uint16 RemainBuffSize = CALC_REMAIN_BUFF_SIZE((uint32)P_BUFF_READ, (uint32)P_BUFF_WRITE, (uint32)LOGGER_CONFIG_DOUBLE_BUFFER_SIZE_BYTES);
    
    gLoggerManager.RxBuffer.Flags.IsLowMemory = (LOGGER_CONFIG_MAX_RX_MESSAGE_SIZE_BYTES > RemainBuffSize);

    if(gLoggerManager.RxBuffer.Flags.IsLowMemory)
    {
        /* update for debug */
        gLoggerManager.RxBuffer.LowMemoryCnt++;
        Logger_SetTxResolution(LOGGER_RESOLUTION_TYPE_HIGH);
    }
    /* start collecting the arguments */
    
    if(MaxSize <= 0)
    {
        OSAL_ASSERT(0);
    }
    /* go over the list of paramters or until the maximum size has reached */
    while(*fmt && MaxSize>0)
    {
        fmt = Logger_ParseAndWriteString(fmt, &MaxSize, BuffLeftOverSize, '%');
        if(!*fmt++ || !*fmt) 
        {
            break;
        }
        switch (*fmt) {
            case 's':              /* string */
                s = va_arg(ap, char *);
                Logger_ParseAndWriteString(s, &MaxSize, BuffLeftOverSize, 0xFF);
            break;
            
            case 'd':              /* int */
            case 'i':
                    d = va_arg(ap, int);
                    Logger_ParseAndWriteNumber(d, 10, &MaxSize, BuffLeftOverSize);
            break;

            case 'p':
            case 'x':              /* hexa */                    
                    d = va_arg(ap, int);
                    Logger_ParseAndWriteNumber(d, 16, &MaxSize, BuffLeftOverSize);                   
            break;            
            
            case 'c':              /* char */
                c = (char) va_arg(ap, int);
                *P_BUFF_ITER++ = c;
                MaxSize--;
            break;
            
            case 'r':              /* raw data */
                s = va_arg(ap, char *);
                d = va_arg(ap, unsigned char);
                    Logger_ParseAndWriteRawData(s, d, &MaxSize, BuffLeftOverSize);
             break;
             
            case 'b':               /* binary data */
                s = va_arg(ap, char *);
                d = va_arg(ap, short);
                Logger_ParseAndWriteBinBuff((unsigned char *)s, d, &MaxSize, BuffLeftOverSize);
            break;
            
            case 'T':               /* time and date data */
                Logger_WriteTimeAndDate(&MaxSize, BuffLeftOverSize);
            break;
                
            default:
                /* it was not a special char, need to copy this char aswell! */
                if(MaxSize >=2)
                {
                    /* TODO: need to make sure wrap around works well here as well */
                    *P_BUFF_ITER++ = '%';
                    *P_BUFF_ITER++ = *fmt;
                    MaxSize -= 2;
                }
        }
        fmt++;
    }
    /* calculate actual massage length */
    //Len = BuffLeftOverSize == 0 ?(int)(P_BUFF_ITER - base): 
    //    (int)(P_BUFF_TAIL - base + P_BUFF_ITER - P_BUFF_HEAD);
    Len = (int)(P_BUFF_ITER - base)<0?LOGGER_CONFIG_DOUBLE_BUFFER_SIZE_BYTES + (int)(P_BUFF_ITER - base): (int)(P_BUFF_ITER - base);

        /* check buffer overrun conditions:
           1. we had a low memory 
           2. we have more space in the buffer after the write operation */
    OSAL_ASSERT(Len >= 0);
    if(gLoggerManager.RxBuffer.Flags.IsLowMemory)
    {
        
         uint16 NewRemainBufSize = CALC_REMAIN_BUFF_SIZE((uint32)P_BUFF_READ, (uint32)P_BUFF_WRITE, (uint32)LOGGER_CONFIG_DOUBLE_BUFFER_SIZE_BYTES);
         
         if(NewRemainBufSize > RemainBuffSize)
         {
             /*mark the special sign for buffer overrun*/
            *P_BUFF_READ = LOGGER_CONFIG_BUFFER_OVERRUN_MARK_CHAR;
    
            /* set the start reading pointer to the new buffer start */
            P_BUFF_READ = P_BUFF_WRITE + 1;
            
            /* update the run over counter for debug */
            gLoggerManager.RxBuffer.RunOverCnt++;
         }
    }
    return Len;
    
}

/**
 * <pre>
 * static inline LOGGER_STATUS Logger_EraseSector(IN uint32 NextSectorAddress)
 * </pre>
 *  
 * this function erases a log flash sector
 *
 * @param   NextSectorAddress        [in]    The address of the begining of the sector to erase
 *
 * @return LOGGER_STATUS_OK for success or a negative ineger for error
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

static inline LOGGER_STATUS Logger_EraseSector(IN uint32 NextSectorAddress)
{
    
    /* erase flash sector */
    if(FLASH_SectorErase(SSP1, 0, NextSectorAddress, FLASH_ATTR_USED_FROM_INTERRUPT, &gLoggerManager.FlashMng.CmdStatus, NULL) != OSAL_STATUS_OK)
    {
        OSAL_ASSERT(0);
        return LOGGER_STATUS_FLASH_ERASE_ERROR;
    }        
        
    /* if we have just erased the current read address, update it to the next sector */
    if(gLoggerManager.FlashMng.ReadAddr == NextSectorAddress)
    {
        gLoggerManager.FlashMng.ReadAddr = LOGGER_DEF_FLASH_NEXT_SECTOR_ADDRESS(gLoggerManager.Config.General.FlashSize, gLoggerManager.FlashMng.ReadAddr, LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB);
    }
    
    return LOGGER_STATUS_OK;
}

/**
 * <pre>
 * static inline uint16 Logger_FlashGetSizeToWrite(uint32 WriteAddress, uint32 EndAdderss, uint16 SizeNeeded, BOOL IsUrgent)
 * </pre>
 *  
 * this function gets the size of available flash size to write log messages to.
 *
 * @param   WriteAddress        [in]    The address in flash of the next message to write to
 * @param   EndAdderss          [in]    The address in flash of the maximum location to write to
 * @param   SizeNeeded          [in]    The input log message size needed in bytes
 * @param   IsUrgent            [in]    The priority of the input log message (1 urgent 0 not urgent)
 *
 * @return the available size to write in bytes
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static inline uint16 Logger_FlashGetSizeToWrite(IN uint32 WriteAddress, IN uint32 EndAdderss, IN uint16 SizeNeeded, IN BOOL IsUrgent)
{
    /* default status value FLASH write error failed */
    uint16 SizeToWrite = SizeNeeded;
    uint8 LeftOver;
    uint8 NumOfPages;
    
    
    /* for optimization, we write data in page size, unless we have a low memory event */
    if(WriteAddress + SizeNeeded > EndAdderss)
    {
        SizeToWrite = EndAdderss - WriteAddress;
        
    }
#if 1    
    else
    {        
        if(!IsUrgent)
        {
            /* if address is aligned */
            uint8 PageOffset = WriteAddress &0xFF;
            if(!PageOffset)
            {
                /* write only pages */
                SizeToWrite = SizeNeeded&0xFF00;
            }
            else if(PageOffset + SizeNeeded >= 256)
            {                        
                LeftOver =  256 - PageOffset;
                NumOfPages = ((SizeNeeded - LeftOver)&0xFF00)>>8;
                                               
                SizeToWrite = (NumOfPages<<8) + LeftOver;
            }
            else
            {    
                SizeToWrite = 0;
            }
        }
    }
#endif    
    return SizeToWrite;    
}

/**
 * <pre>
 * static inline uint16 Logger_SendPacketFlash(char *pStr, uint16 StrLen)
 * </pre>
 *  
 * this function sends the flash an ascii format log messages buffer from RAM  
 *
 * @param   pStr            [in]    The input log messages RAM buffer to write
 * @param   StrLen          [in]    The input buffer length in bytes
 *
 * @return the size of input buffer writen to flash in bytes
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
static inline uint16 Logger_SendPacketFlash(IN char *pStr, IN uint16 StrLen)
{
    
    uint32 FlashEndAddress =  LOGGER_DEF_FLASH_END_ADDRESS(LOGGER_CONFIG_FLASH_START_ADDRESS, gLoggerManager.Config.General.FlashSize);
    
    uint16 SizeToWrite = Logger_FlashGetSizeToWrite(gLoggerManager.FlashMng.WriteAddr, FlashEndAddress, StrLen, gLoggerManager.RxBuffer.Flags.IsLowMemory || gLoggerManager.RxBuffer.Flags.IsUrgent);

    if(SizeToWrite == 0)
    {
        return 0;
    }
    OSAL_ASSERT(SizeToWrite + gLoggerManager.RxBuffer.pRead <= gLoggerManager.RxBuffer.pTail);
    /* sanity check */
    if(gLoggerManager.pCbList->SendFlashDataCb != NULL)
    {
        if(gLoggerManager.pCbList->SendFlashDataCb(gLoggerManager.FlashMng.WriteAddr, pStr, &SizeToWrite) == LOGGER_STATUS_OK)
        {
            /* we need to prepare the next sector */
            uint16 CurrentSectorUsageKB = (gLoggerManager.FlashMng.WriteAddr - gLoggerManager.FlashMng.CurrSectorAddr)>>10;
            /* find the FLASH debug end address */ 
            
            if(!gLoggerManager.FlashMng.Flags.IsNextSectorErased)
            {
                if(CurrentSectorUsageKB >= FLASH_CONFIG_SECTOR_SIZE_THERSHOLD_KB)
                {
                    uint32 NextSectorAddress = LOGGER_DEF_FLASH_NEXT_SECTOR_ADDRESS(gLoggerManager.Config.General.FlashSize, gLoggerManager.FlashMng.CurrSectorAddr, LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB);

                    if(Logger_EraseSector(NextSectorAddress) == LOGGER_STATUS_OK)
                    {
                        /* mark that we have erased the next sector */
                        gLoggerManager.FlashMng.Flags.IsNextSectorErased = 1;
                    }
                }
            }
            else 
            {
                if(gLoggerManager.FlashMng.CmdStatus == 0 && CurrentSectorUsageKB >= LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB)
                {
                    gLoggerManager.FlashMng.Flags.IsNextSectorErased = 0;
                    gLoggerManager.FlashMng.CurrSectorAddr = LOGGER_DEF_FLASH_NEXT_SECTOR_ADDRESS(gLoggerManager.Config.General.FlashSize, gLoggerManager.FlashMng.CurrSectorAddr, LOGGER_CONFIG_FLASH_SECTOR_SIZE_KB);
                }
            }
            /* update the Flash write pointer */
            gLoggerManager.FlashMng.WriteAddr += SizeToWrite;
            /* sanity check - flash write pointer should  not be bigger than flash size */
            
            if(gLoggerManager.FlashMng.WriteAddr > FlashEndAddress)
            {
                OSAL_ASSERT(FALSE);
            }
            else if(gLoggerManager.FlashMng.WriteAddr == FlashEndAddress)
            {
                /* need to wrap around flash address */
                gLoggerManager.FlashMng.WriteAddr = LOGGER_CONFIG_FLASH_START_ADDRESS;
            }
            /* save in eeprom the new write address for next reset */
            if(gLoggerManager.pCbList->SaveNVRParamsCb != NULL)
            {
                gLoggerManager.pCbList->SaveNVRParamsCb(LOGGER_CONFIG_NVR_ADDRESS, &gLoggerManager.FlashMng, LOGGER_CONFIG_NVR_SIZE_BYTES);
            }
        }
    }
    else
    {
        /* we have configured destination type FLASH but no flash write callback was initiated */
        OSAL_ASSERT(0);
        return 0;
    }
    
    return SizeToWrite;
}

/**
 * <pre>
 * static inline LOGGER_STATUS Logger_SendPacket(char *pStr, int StrLen, BOOL IsPrintoutEnabled, uint8 DestType)
 * </pre>
 *  
 * this function sends an ascii format log messages buffer from RAM, to the configured destination (FLASH, socket, RS232)  
 *
 * @param   pStr                    [in]    The input log messages RAM buffer to write
 * @param   StrLen                  [in]    The input buffer length in bytes
 * @param   IsPrintoutEnabled       [in]    Should we send log to RS232 as well (currently we can't send log to more than one destination simultenously,
                                            but the design it was supposed to be supported)
 * @param   DestType                [in]    The destination type:
 *                                          LOGGER_DEST_TYPE_NONE - no destination type (RS232 can be still enabled)
 *                                          LOGGER_DEST_TYPE_FLASH - destination type is flash (log file in PULL mode)
 *                                          LOGGER_DEST_TYPE_SOCKET - destination type is socket (in PUSH mode)
 *
 * @return the available size to write in bytes
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

static inline LOGGER_STATUS Logger_SendPacket(IN char *pStr, IN int StrLen, IN BOOL IsPrintoutEnabled, IN uint8 DestType)
{
    
    LOGGER_STATUS Status = LOGGER_STATUS_OK;
    
    if(StrLen <= 0)
    {
        OSAL_ASSERT(0);
        return LOGGER_STATUS_ERROR;
    }
    /* TODO: need to take care of wrap arround */
    if(StrLen > LOGGER_CONFIG_MAX_TX_MESSAGE_SIZE_BYTES)
        StrLen = LOGGER_CONFIG_MAX_TX_MESSAGE_SIZE_BYTES;
        
    switch (DestType)
    {
        /* do nothing */
        case LOGGER_DEST_TYPE_NONE:
        break;
        
        /* send to FLASH */
        case LOGGER_DEST_TYPE_FLASH:
            if(gLoggerManager.State == e_LOGGER_STATE_READY)
            {
                StrLen = Logger_SendPacketFlash(pStr, StrLen);
            }
        break;
        /* send to socket */
        case LOGGER_DEST_TYPE_SOCKET:
            if(gLoggerManager.pCbList->SendSocketDataCb != NULL && gLoggerManager.State == e_LOGGER_STATE_READY)
            {
                Status  = gLoggerManager.pCbList->SendSocketDataCb((uint8 *)pStr, StrLen, LOGGER_MODE_TYPE_PUSH);
            }
        break;
        
        default:
            /* invalid destination type */
            OSAL_ASSERT(0);
    }
    if(IsPrintoutEnabled)
    {
        /* send the message to uart callback */
        if(gLoggerManager.pCbList->SendUartDataCb != NULL)
            Status = gLoggerManager.pCbList->SendUartDataCb(pStr, StrLen);
    }
    if(Status == LOGGER_STATUS_OK && StrLen != 0)
    {
        /* update the numbers of TX packets sent */
        gLoggerManager.RxBuffer.TxCounter++;
        /* update the RX read pointer */
        gLoggerManager.RxBuffer.pRead += StrLen;
        
        /* wrap around case */
        if(gLoggerManager.RxBuffer.pRead == gLoggerManager.RxBuffer.pTail)
        {
            gLoggerManager.RxBuffer.pRead = gLoggerManager.RxBuffer.pHead;
        }

    }
    else if(Status != LOGGER_STATUS_OK)
    {
        gLoggerManager.RxBuffer.BusyCnt++;
    }
    return Status;
}

                /* ========================================== *
                 *     P U B L I C     F U N C T I O N S      *
                 * ========================================== */

/**
 * <pre>
 * void Logger_printf(uint8 level, char *fmt, ...)
 * </pre>
 *  
 * this function is a customized printf log for debug. it's currently support the following parameters:
 *                                  %c - prints an ascii character
 *                                  %s - prints a string (must be null terminated)
 *                                  %d or %i - prints a decimal number
 *                                  %x - prints an hexadecimal number
 *                                  %r - translate (and prints) a binary buffer to an hexadecimal buffer. (must get size as well)
 *                                  %b - prints a binary buffer (without translate to hexa. must get size as well)
 *                                  %T - prints the current date and time in format DD/MM/YY HH:MM:SS
 *                                  
 * @param   level                   [in]    The log level
 * @param   fmt                     [in]    The input string
 * @param   ...                     [in]    a list of printf style arbitrary parameters (must match to the % in the string).
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_printf(uint8 level, char *fmt, ...)
{
    int Len;
    
    va_list ap;
    
    uint8 TaskId = OSAL_GetSelfTaskId();
    
    /* check the condition to use the logger */
    if(!LOGGER_DEF_GEN_CONFIG.IsEnabled || (LOGGER_DEF_GEN_CONFIG.LogLevel != LEVEL_CRITICAL && (~LOGGER_DEF_GEN_CONFIG.ModuleMask & (1<<TaskId) || LOGGER_DEF_GEN_CONFIG.LogLevel < level)))
        return;
    
    /* first lock mutex */

     OSAL_PortMutexCapture(gLoggerManager.RxBuffer.pMutex);

    va_start(ap, fmt);

    /* parse the arguments to string and write it to the ciruclar buffer */
    Len = Logger_ParseAndWriteRxMessage(level, fmt, ap);
    
    va_end(ap);

    if(Len <= 0)
    {
        //OSAL_ASSERT(FALSE);
        goto EXIT;
    }

    
    /* update the numbers of Rx debug messages */
    gLoggerManager.RxBuffer.RxCounter++;
    
    /* if log level is critical we want the logger to printout ASAP */
    if(LEVEL_CRITICAL == level)
    {
        Logger_FlushRxBuffer();    
    }
    
EXIT:    
    /* releae mutex */
    OSAL_PortMutexRelease( gLoggerManager.RxBuffer.pMutex);

}

/**
 * <pre>
 * void Logger_SendPacketFromInt(void)
 * </pre>
 *  
 * this function sends the RAM log buffer to the configured destination (RS232, socket, FLASH).
 * NOTE: this function is called every x ms (configurable parameter LOGGER_CONFIG_DEFAULT_RESOLUTION_MS) from HW timer interrupt.
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/

void Logger_SendPacketFromInt(void)
{
    
    int BuffToRead = CALC_BUF_SIZE_TO_READ(gLoggerManager.RxBuffer.pRead, gLoggerManager.RxBuffer.pWrite, gLoggerManager.RxBuffer.pTail);
    
    OSAL_ASSERT(BuffToRead <= LOGGER_CONFIG_DOUBLE_BUFFER_SIZE_BYTES);
    /* check working conditions */
    if(!BuffToRead)
    {
        return;
    }
    /* send data till the end of buffer */
    Logger_SendPacket(gLoggerManager.RxBuffer.pRead, BuffToRead,
                              LOGGER_DEF_GEN_CONFIG.IsPrintoutEnabled, 
                              LOGGER_DEF_GEN_CONFIG.DestType);
    
    if(!LOGGER_DEF_IS_RX_BUFFER_EMPTY()&&gLoggerManager.Config.General.DestType != LOGGER_DEST_TYPE_FLASH)
    {
        BuffToRead = CALC_BUF_SIZE_TO_READ (gLoggerManager.RxBuffer.pRead, gLoggerManager.RxBuffer.pWrite, gLoggerManager.RxBuffer.pTail);
        /* send data till the end of buffer */
        Logger_SendPacket(gLoggerManager.RxBuffer.pRead, BuffToRead,
                              LOGGER_DEF_GEN_CONFIG.IsPrintoutEnabled, 
                              LOGGER_DEF_GEN_CONFIG.DestType);
    }
    if(gLoggerManager.RxBuffer.Flags.IsLowMemory || gLoggerManager.RxBuffer.Flags.IsUrgent)
    {
        gLoggerManager.RxBuffer.Flags.IsLowMemory = gLoggerManager.RxBuffer.Flags.IsUrgent = 0;
        Logger_SetTxResolution(LOGGER_RESOLUTION_TYPE_LOW);
    }
}

/**
 * <pre>
 * BOOL Logger_IsLoopBack(uint8 TaskId)
 * </pre>
 *  
 * this function check logger loopback conditions (cases in which the log print itself may cause log print in endless loop)
 * @param   TaskId                   [in]    the ID of the using task
 *
 * @return 1 if there is a loopback condition, or 0 otherwise
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
BOOL Logger_IsLoopBack(uint8 TaskId)
{
    
    return ((LOGGER_DEF_GEN_CONFIG.DestType == LOGGER_DEST_TYPE_SOCKET) && 
            (TaskId == TASK_MDL_COMM_STM_RX_MNG_ID                     ||             
            TaskId == TASK_MDL_ETH_MANAGER_ID                          ||
            TaskId == TASK_APP_COM_MANAGER_ID));
}

/**
 * <pre>
 * BOOL Logger_IsLoopBack(uint8 TaskId)
 * </pre>
 *  
 * this function returns true if the logger is in test mode
 *
 * @return 1 if the logger is in test mode, or 0 otherwise
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
BOOL Logger_IsTestMode(void)
{
    return (gLoggerManager.Config.General.Mode == LOGGER_MODE_TYPE_TEST);
}

/**
 * <pre>
 * void Logger_SetTestMode(void)
 * </pre>
 *  
 * this function set the logger in test mode
 *
 * @return none
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
void Logger_SetTestMode(void)
{
    gLoggerManager.Config.General.Mode = LOGGER_MODE_TYPE_TEST;
}

/**
 * <pre>
 * uint16 Logger_GetMaxBuffSize(void)
 * </pre>
 *  
 * this function returns the maximum logger buffer size allowed in one log message
 *
 * @return the maximum logger buffer size allowed in one log message
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
uint16 Logger_GetMaxBuffSize(void)
{
    return LOGGER_CONFIG_MAX_RX_MESSAGE_SIZE_BYTES;
}

/**
 * <pre>
 * BOOL Logger_IsReady(void)
 * </pre>
 *  
 * this function returns ture if the logger is ready to run
 *
 * @return 1 if the logger is ready to run, or 0 otherwise
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
BOOL Logger_IsReady(void)
{
    return (gLoggerManager.State == e_LOGGER_STATE_READY); 
}

/**
 * <pre>
 * BOOL Logger_IsInit(void)
 * </pre>
 *  
 * this function returns ture if the logger was already initilized
 *
 * @return 1 if the logger was already initilized, or 0 otherwise
 *
 * \defgroup LogManager
 * \ingroup LogManager
*/
BOOL Logger_IsInit(void)
{
    return (gLoggerManager.State >= e_LOGGER_STATE_INITIALIZING);
}

BOOL Logger_UploadLogFile(uint16 SizeKB)
{
    return Logger_SendDebugLog(SizeKB, LOGGER_DEST_TYPE_SOCKET);
}
