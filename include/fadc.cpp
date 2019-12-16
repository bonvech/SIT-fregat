/**
 * \file fadc.cpp
 * \brief Класс fadc. Базовый для плат FADC.
 * \date 28.09.2006. Modified November 2019
 *
 * Описание класса fadc
 */


#ifndef _FADC
#define _FADC       ///< to prevent multiple including


/** -------------------------------------------------------------
 * Class fadc for FADC I2C interface
 */
class fadc : public i2c
{
  public:
    // constructor
    fadc()
    {
       BaseAddr    = 0x320;
       ADCSubAddr2 = 0x23;
    }

 private:
    // obrashenie k zhelezy
    void SDA(unsigned char Data)
    {
#ifdef LINUX
        if (!(Data)) {outb((inb(BaseAddr+0xE)  |  2),  (BaseAddr+0xE));
                      outb((inb(BaseAddr+0xE)  |  2),  (BaseAddr+0xE));}
        else         {outb((inb(BaseAddr+0xE) & (~2)), (BaseAddr+0xE));
                      outb((inb(BaseAddr+0xE) & (~2)), (BaseAddr+0xE));}
#else
    if (!(Data)) {outp((BaseAddr+0x2E), (inp(BaseAddr+0x2E)  |  2));
                  outp((BaseAddr+0x2E), (inp(BaseAddr+0x2E)  |  2));}
    else         {outp((BaseAddr+0x2E), (inp(BaseAddr+0x2E) & (~2)));
                  outp((BaseAddr+0x2E), (inp(BaseAddr+0x2E) & (~2)));}
#endif
    }

    void SCL(unsigned char Data)
    {
#ifdef LINUX
    if (!(Data)) {outb((inb(BaseAddr+0xE)  |  1),  (BaseAddr+0xE));
                  outb((inb(BaseAddr+0xE)  |  1),  (BaseAddr+0xE));}
    else         {outb((inb(BaseAddr+0xE) & (~1)), (BaseAddr+0xE));
                  outb((inb(BaseAddr+0xE) & (~1)), (BaseAddr+0xE));}
#else
    if (!(Data)) {outp((BaseAddr+0x2E), (inp(BaseAddr+0x2E)  |  1));}
    else         {outp((BaseAddr+0x2E), (inp(BaseAddr+0x2E) & (~1)));}
    if (!(Data)) {outp((BaseAddr+0x2E), (inp(BaseAddr+0x2E)  |  1));}
    else         {outp((BaseAddr+0x2E), (inp(BaseAddr+0x2E) & (~1)));}
#endif
    }

    unsigned char SDAin(void)
    {       //char tmp = inp(BaseAddr+1);
#ifdef LINUX
        SDA(1);
    if ((inb(BaseAddr+0xE) & 8))
        { return 1;}
#else
    //for HVPS
    //if ((inp(BASEADDR+0xE) & 0x10)) { return 1;}
    //return 0;
#endif
    return 0;    }





//public:

/*    void SetChannelAddr(unsigned char ChannelAddr)
    {
#ifdef LINUX
    outb(((inb(BaseAddr) & 0x40) | ChannelAddr), BaseAddr);
#else
    outp(BaseAddr, ((inp(BaseAddr) & 0x40) | ChannelAddr));
#endif
    }
*/
/*    //////////////////////////////////////////////////////////////
   unsigned char ReadDAC(unsigned char AddrDev, unsigned char *Data)
    //read Data from DAC whit addr AddrDev
    {
    unsigned char tmpAddr;
    if (AddrDev==0) {tmpAddr = 0x2C;}
    else if (AddrDev==1) {tmpAddr = 0x2D;}
        else {return 0;}
    return RX8(tmpAddr,Data);
    }
*/

    /////////////////////////////
 /*   unsigned char Read1ResReg(unsigned char AddrDev, unsigned char AddrReg, unsigned int *RegRes)
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
*/
};  // end of class fadc

#endif
