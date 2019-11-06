/**
 * \file i2c.cpp
 * \date 01.06.06  last update 2011-01-18
 *
 * \brief Абстрактный базовый класс для i2c устройств
 *
 * Abstract base class for i2c objects\n
 * Contains commom functions for I2C\n
 * for fadc & hvps\n
 * for Linux
 */

#ifndef _I2C
#define _I2C  ///< to prevent multiple including


/// Base class for all devices of the SPHERE detector
class i2c
{
  public:

    int BaseAddr;    ///< Base address of device
    int ADCSubAddr2; ///< ADC subaddress

  private:
    ///   obrashenie k zhelezy
    virtual void SDA(unsigned char Data) = 0;
    virtual void SCL(unsigned char Data) = 0;
    virtual unsigned char SDAin(void) = 0;

    /// I2C Start function
    void i2c_Start(void)
    {
        SDA(1);
        SCL(1);
        SDA(0);
        SCL(0);
    }

    /// I2C Stop function
    void i2c_Stop(void)
    {
        SCL(0);
        SDA(0);
        SCL(1);
        SDA(1);
        SDA(1);
    }

    /// I2C Take Ask function
    unsigned char i2c_TakeACK(void)
    {
        unsigned char tmp = 0;

        SDA(1);
        SCL(1);
        //if (SDAin()) {return 0;}
        //else {return 1;}
        tmp = SDAin();
        SCL(0);
        return tmp;
    }

    /// I2C ACK function
    void i2c_ACK(void)
    {
        SDA(0);
        SCL(1);
        SCL(0);
    }

    /// I2C SetAddr function
    void i2c_SetAddr(unsigned char Addr, unsigned char RW)
    {
        // На шину последовательно выводится адрес с 6 бита
        // по 0, затем бит RW
        unsigned char i;
        for (i=0; i<7;i++)
        {
            Addr=Addr<<1;
            if (Addr & 0x80) { SDA(1);}
            else {SDA(0);}
            SCL(1);
            SCL(0);
        }
        if (RW>0) {SDA(1);}
        else {SDA(0);}
        SCL(1);
        SCL(0);
    }


    /// I2C Send data function
    void i2c_SendData(unsigned char Data)
    {
        unsigned char i;
        for(i=0; i<8; i++)
        {
            if (Data & 0x80) { SDA(1);}
            else {SDA(0);}
            SCL(1);
            SCL(0);
            Data=Data<<1;
        }
    }


    unsigned char i2c_ReceiveData(void)
    {
        unsigned char tmpData=0;
        unsigned char i;

        SDA(1);
        for(i=0; i<7; i++)
        {
            SCL(1);
            tmpData|=SDAin();
            //tmpD[i] = tmpData;
            tmpData<<=1;
            SCL(0);
            //	printf("GOT: %d\n", tmpData);
        }
        SCL(1);
        tmpData|=SDAin();
        SCL(0);
        return tmpData;
    }


  ///////////////////////////////////////
  /// процедуры пользователя для чтения и записи данных по i2c

protected:
    /// Transmit one
    unsigned char TX_reg(unsigned char AddrDev, unsigned char AddrReg)
    {
        i2c_Start();
        i2c_SetAddr(AddrDev,0);
        if (i2c_TakeACK())
        {
            i2c_Stop();
            printf("i2c:TX_reg:Error1\n");return 0;
        }
        i2c_SendData(AddrReg);
        if (i2c_TakeACK())
        {
            i2c_Stop();
            printf("i2c:TX_reg:Error2\n");return 0;
        }
        i2c_Stop();
        return 1;
    }


    /// Transmit one byte with no stop
    /// \return 0 - Error\n
    ///         1 - OK
    unsigned char TX_reg_NoStop(unsigned char AddrDev, unsigned char AddrReg)
    {
        i2c_Start();
        i2c_SetAddr(AddrDev,0);
        if (i2c_TakeACK())
        {
            i2c_Stop();/*printf("Error1\n")*/;
            return 0;
        }
        i2c_SendData(AddrReg);
        if (i2c_TakeACK())
        {
            i2c_Stop();
            /*printf("Error2\n");*/
            return 0;
        }
        return 1;
    }


    /// Transmit one byte
    /// \return 0 - Error\n
    ///         1 - OK
    unsigned char TX8(unsigned char AddrDev, unsigned char AddrReg, unsigned char Data)
    {
        i2c_Start();
        i2c_SetAddr(AddrDev,0);
            if (i2c_TakeACK())
            {i2c_Stop(); return 0;}
        i2c_SendData(AddrReg);
            if (i2c_TakeACK())
            {i2c_Stop(); return 0;}
        i2c_SendData(Data);
            if (i2c_TakeACK())
            {i2c_Stop(); return 0;}
        i2c_Stop();
        return 1;
    }


    /// Transmit 2 bytes
    /// \return 0 - Error\n
    ///         1 - OK
    unsigned char TX16(unsigned char AddrDev, unsigned char AddrReg, unsigned int Data)
    {
        Conv.tInt = Data;
        i2c_Start();
        i2c_SetAddr(AddrDev,0);
            if (i2c_TakeACK())
            {i2c_Stop();return 0;}
        i2c_SendData(AddrReg);
            if (i2c_TakeACK())
            {i2c_Stop();return 0;}
        i2c_SendData(Conv.tChar[1]);
            if (i2c_TakeACK())
            {i2c_Stop();return 0;}
        i2c_SendData(Conv.tChar[0]);
            if (i2c_TakeACK())
            {i2c_Stop();return 0;}
        i2c_Stop();
        return 1;
    }


    /// Read one byte
    /// \return 0 - Error\n
    ///         1 - OK
    unsigned char RX8(unsigned char AddrDev, unsigned char *GotData)
    {
        i2c_Start();
        i2c_SetAddr(AddrDev,1);
        if (i2c_TakeACK())
        {
            i2c_Stop();
            return 0;
        }
        *GotData = i2c_ReceiveData();
        SDA(1); // требование datasheet к AD7994
        SCL(1);
        SCL(0);
        i2c_Stop();
        return 1;
    }


    /// Read 2 bytes
    /// \return 0 - Error\n
    ///         1 - OK
    unsigned char RX16(unsigned char AddrDev, unsigned short *GotData)
    {
        i2c_Start();
        i2c_SetAddr(AddrDev,1);
        if (i2c_TakeACK())
            {i2c_Stop();return 0;}
        Conv.tChar[1] = i2c_ReceiveData();
        i2c_ACK();
        Conv.tChar[0] = i2c_ReceiveData();
        SDA(1); // требование datasheet к AD7994
        SCL(1);
        SCL(0);
        i2c_Stop();
        *GotData = Conv.tInt;
        return 1;
    }

    /// read 2 bytes
    /// \return 0 - Error\n
    ///         1 - OK
    unsigned char RX16(unsigned char AddrDev, unsigned int *GotData)
    {
        i2c_Start();
        i2c_SetAddr(AddrDev,1);
        if (i2c_TakeACK())
            {i2c_Stop();return 0;}
        Conv.tChar[1] = i2c_ReceiveData();
        i2c_ACK();
        Conv.tChar[0] = i2c_ReceiveData();
        SDA(1);// datasheet AD7994
        SCL(1);
        SCL(0);
        i2c_Stop();
        *GotData = Conv.tInt;
        return 1;
    }

    /// read 4 bytes
    unsigned char RX32(unsigned char AddrDev, unsigned int *Data)
    {
        //short i = 0;
        //data[2] = {0,0};
        //unsigned short data[2];
        //unsigned char *ptr;

        i2c_Start();
        i2c_SetAddr(AddrDev,1);
        if (i2c_TakeACK())
        {i2c_Stop();return 0;}

        Conv.tChar[3] = i2c_ReceiveData();
        i2c_ACK();
        Conv.tChar[2] = i2c_ReceiveData();
        i2c_ACK();       
        Conv.tChar[1] = i2c_ReceiveData();
        i2c_ACK();
        Conv.tChar[0] = i2c_ReceiveData();

        //printf("\n %x ", Conv.tChar[3]);
        //printf(" %x ", Conv.tChar[2]);
        //printf(" %x ", Conv.tChar[1]);
        //printf("\n");
        //for(i = 0; i < 4; i++)
        //  printf(" %x ", Conv.tChar[3-i]);
        //printf("\n");

        SDA(1);  // требование datasheet к AD7994
        SCL(1);
        SCL(0);
        i2c_Stop();

        *Data = Conv.tInt;

        return 1;
    }

    /** -------------------------------------------------------
     *  write Data to DAC with addr AddrDev
     * \brief Read 8 byte
     * \param AddrDev - device address
     * \param Data    - data array
     */
    unsigned char RX64(unsigned char AddrDev, unsigned int *Data[4])
    {
        unsigned char i = 0;

        i2c_Start();
        i2c_SetAddr(AddrDev,1);
        if (i2c_TakeACK())
        { i2c_Stop();
        return 0;}

        for (i=0; i<3; i++)
        {
            Conv.tChar[1] = i2c_ReceiveData();
            i2c_ACK();
            Conv.tChar[0] = i2c_ReceiveData();
            i2c_ACK();
            *Data[i]=Conv.tInt;
        }
        Conv.tChar[1] = i2c_ReceiveData();
        i2c_ACK();
        Conv.tChar[0] = i2c_ReceiveData();
        *Data[3]=Conv.tInt;

        SDA(1); // требование datasheet к AD7994
        SCL(1);
        SCL(0);
        i2c_Stop();

        return 1;
    }


    /** -------------------------------------------------------
     *  Write Data to DAC with addr AddrDev
     * \brief Write to DAC
     * \return 0 - Error\n
     *         result of reading if success
     */
    unsigned char WriteDAC(unsigned char AddrDev, unsigned char Data)
    {
        unsigned char tmpAddr = 0;

        if      (AddrDev==0) {tmpAddr = 0x2C;}
        else if (AddrDev==1) {tmpAddr = 0x2D;}
        else                 {return 0;}
        return TX8(tmpAddr,0x00,Data);
    }


    /** -------------------------------------------------------
     * \brief Read register
     * \param AddrDev - device address
     * \param AddrReg - register address
     * \param RegRes  - result
     *
     *  Read register with AddrReg from device with AddrDev address
     *
     */
    unsigned char ReadReg(unsigned char AddrDev, unsigned char AddrReg, unsigned int *RegRes)
    {
        unsigned char tmpReg=AddrReg;

        //if(tmpReg < 1) return 0;
        //tmpReg <<= 4;

        if(!TX8(AddrDev,0x00,tmpReg))
        return 0;

        return RX16(AddrDev, RegRes);
    }


    /** -------------------------------------------------------
    *  read 4 ADC`s chanels
    *  \brief Read ADC
    *
    *  \param AddrDev - device address
    *  \param Data - data 2 byte
    *  \return 0 - Error\n
    *          1 - OK
    *
    *   example of using:
        \code
        unsigned int Data[4];
        ReadADCs (0x01,(unsigned int *)&Data);
        \endcode
    */
    unsigned char ReadADCs(unsigned char AddrDev, unsigned int *Data)//read 4 ADC`s chanels
    {
        unsigned char tmpAddr = 0;
        unsigned char i = 0;

        if (AddrDev == 0)    {tmpAddr=0x20;}
        else if (AddrDev==1) {tmpAddr=ADCSubAddr2;}
                else            {return 0;}
            //printf("SubAddr2 = %i\n",ADCSubAddr2);
        if (!TX_reg_NoStop(tmpAddr,0xF0))
            {return 0;}

        i2c_Start();
        i2c_SetAddr(tmpAddr,1);
        if (i2c_TakeACK())
            {i2c_Stop();return 0;}
        for (i=0; i<3; i++)
        {
            Conv.tChar[1] = i2c_ReceiveData();
            i2c_ACK();
            Conv.tChar[0] = i2c_ReceiveData();
            //printf(" %d ", Conv.tChar[0]);
            i2c_ACK();
            Data[i]=Conv.tInt;
        }
        Conv.tChar[1] = i2c_ReceiveData();
        i2c_ACK();
        Conv.tChar[0] = i2c_ReceiveData();
        Data[3]=Conv.tInt;
        //printf("%d\n", Conv.tInt);

        SDA(1); // требование datasheet AD7994 
        SCL(1);
        SCL(0);
        i2c_Stop();
        return 1;
    }

}; // class i2c
#endif
