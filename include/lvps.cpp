// sdvig_reg from "hvps.cpp"
// 01.06.06
/**
 *  \file lvps.cpp
 *  \brief Класс lvps. 
 * 
 *  Class lvps. Base class for lvps_devices.
 */
#ifndef _LVPS
#define _LVPS  ///< to prevent multiple including

//#include <stdio.h>
#ifndef LINUX
#include <conio.h>
#endif

#include "i2c.cpp"


/// Class for lvps I2C interface
class lvps: public i2c
{
  public:

    // constructor
    lvps()
    {
        BaseAddr = 0x378;  
        ADCSubAddr2 = 0x21;
    }

 protected:
    // obrashenie k zhelezy
    /** -------------------------------------------------------
    *  \brief SDA for lvps
    */
    void SDA(unsigned char Data)
    {
#ifdef LINUX
        if (!(Data)) {outb((inb(BaseAddr+2)  |  2),  (BaseAddr+2));
                      outb((inb(BaseAddr+2)  |  2),  (BaseAddr+2));}
        else         {outb((inb(BaseAddr+2) & (~2)), (BaseAddr+2));
                      outb((inb(BaseAddr+2) & (~2)), (BaseAddr+2));}
#else
        if (!(Data)) {outp((BaseAddr+2), (inp(BaseAddr+2)  |  2));
                      outp((BaseAddr+2), (inp(BaseAddr+2)  |  2));}
        else         {outp((BaseAddr+2), (inp(BaseAddr+2) & (~2)));
                      outp((BaseAddr+2), (inp(BaseAddr+2) & (~2)));}
#endif
    }

    /** -------------------------------------------------------
    *  \brief  SCL for lvps
    */
    void SCL(unsigned char Data)
    {
#ifdef LINUX
        if (!(Data)) {outb((inb(BaseAddr+2)  | 1),   (BaseAddr+2));
                      outb((inb(BaseAddr+2)  | 1),   (BaseAddr+2));}
        else         {outb((inb(BaseAddr+2) & (~1)), (BaseAddr+2));
                      outb((inb(BaseAddr+2) & (~1)), (BaseAddr+2));}
#else
        if (!(Data)) {outp((BaseAddr+2), (inp(BaseAddr+2)  | 1));
                      outp((BaseAddr+2), (inp(BaseAddr+2)  | 1));}
        else         {outp((BaseAddr+2), (inp(BaseAddr+2) & (~1)));
                      outp((BaseAddr+2), (inp(BaseAddr+2) & (~1)));}
#endif
    }

    /** -------------------------------------------------------
    *  \brief SDA reading for lvps 
    */
    unsigned char SDAin(void)
    {       //char tmp = inp(BaseAddr+1);
#ifdef LINUX
        if ((inb(BaseAddr+1) & 0x40))
#else
        if ((inp(BaseAddr+1) & 0x40))
#endif
            { return 1;}
        return 0;
    }

public:
    /** -------------------------------------------------------
    *  \brief  Set Channel Address to work with
    *  \date   08.07.2019 Добавлена блокировка всех vip каналов при выставлении канала lvps\n
    *          19.08.2019 Убрана блокировка всех vip каналов
    */
    void SetChannelAddr(unsigned char ChannelAddr)
    {     
        // set channel adress
        outb(((inb(BaseAddr) & 0x40) | ChannelAddr), BaseAddr);
    }
    
    /** -------------------------------------------------------
    *  \brief  Read Data from DAC\par 
    *  Read Data from DAC whit addr AddrDev
    */
    unsigned char ReadDAC(unsigned char AddrDev, unsigned char *Data)
    {
        unsigned char tmpAddr;
        if (AddrDev==0)      {tmpAddr = 0x2C;}
        else if (AddrDev==1) {tmpAddr = 0x2D;}
        else {return 0;}
        return RX8(tmpAddr,Data);
    }
    
    /** -------------------------------------------------------
    *  \brief  Read all result registers (64bit)
    */
    unsigned char ReadResRegs(unsigned char AddrDev, unsigned int *ResData[4])
    {
        
        if(!TX8(AddrDev, 0x00, 0x0F))
            return 0;
        
        return RX64(AddrDev, ResData);
    }

    /** -------------------------------------------------------
    *   \brief  Read one result register (16bit) 
    */
    unsigned char Read1ResReg(unsigned char AddrDev, unsigned char AddrReg, unsigned int *RegRes)
    {
        unsigned char tmpAddr = AddrReg;
        
        //tmpAddr = 1 << (tmpAddr-1);
        
        printf(" Addr=%d", tmpAddr);
        if(!TX8(AddrDev, 0x00, tmpAddr))
        {
            printf("Error in Read1ResReg!!");
            return 0;
        }
        
        return RX16(AddrDev, RegRes);
    }
    
};  // end of class lvps

#endif
