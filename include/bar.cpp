/**
 * \file bar.cpp
 * \date 07.02.2009 Modification 2012, 2019.10.30
 * \brief Класс barometer. Board of LED calibration
 *
 * Класс barometer.
 */

#ifndef _BAR
#define _BAR          ///< to prevent multiple including
#include "i2c.cpp"

#define AddrEEPROM  0x50       ///< Address EEPROM
#define AddrBAROMETER 0x8      ///< Address of barometer
#define MaxPress 1100          ///< Maximal pressure 1100Hpa

//class led;


/// Class for one barometer
class barometer: public i2c
{
  public:
    unsigned int cc[8];   ///< koefficients of barometer module
    unsigned int aa[5];   ///< Sensor Specific Parameters A,B,C,D

    // constructor
    barometer()
    {
        BaseAddr    = BaseAddrLED; // default for 1-st barometer
    }

 private:
    //  I2C interface // obrashenie k zhelezy
    void SDA(unsigned char Data)
    {
        if (!(Data)) { outb((inb(BaseAddr)  |  2),  (BaseAddr));
                        outb((inb(BaseAddr)  |  2),  (BaseAddr));}
        else         { outb((inb(BaseAddr) & (~2)), (BaseAddr));
                        outb((inb(BaseAddr) & (~2)), (BaseAddr));}
    }

    void SCL(unsigned char Data)
    {
        if (!(Data)) {outb((inb(BaseAddr)  |  1),  (BaseAddr));
                        outb((inb(BaseAddr)  |  1),  (BaseAddr));}
        else         {outb((inb(BaseAddr) & (~1)), (BaseAddr));
                        outb((inb(BaseAddr) & (~1)), (BaseAddr));}
    }

    unsigned char SDAin(void)
    {
        SDA(1);
        if ((inb(BaseAddr) & 8))
            { return 1;}
        return 0;
    }

    // ================================================ //
    // ================================================ //
    //  Barometer functions
    // ================================================ //

 public:
    /// Set base address
    void SetAddr(unsigned int addr)
    {
        BaseAddr = addr;
        printf("BaseAddr = %xh\n", BaseAddr);
    }

 private:
    /// Read coefficient CN whit number AddrDev = 0from EEPROM
    unsigned int ReadC( unsigned char Dev, unsigned int *Data)
    {
        unsigned int tmpAddr;

        switch(Dev)
        {
            case 1:  tmpAddr = 16;
                        break;
            case 2:  tmpAddr = 18;
                            break;
            case 3:  tmpAddr = 20;
                        break;
            case 4:  tmpAddr = 22;
                        break;
            case 5:  tmpAddr = 24;
                            break;
            case 6:  tmpAddr = 26;
                            break;
            case 7:  tmpAddr = 28;
                        break;
            default: return 0;
        }

        TX_reg_NoStop(AddrEEPROM, tmpAddr);
        return RX16(AddrEEPROM, Data);
    }


    /// Read Sensor Specific Parameters A-D whith number AddrDev = 0 from EEPROM
    unsigned int ReadS( unsigned char Dev, unsigned char *Data)
    {
        unsigned int tmpAddr;

        switch(Dev)
        {
            case 1:  tmpAddr = 30;
                        break;
            case 2:  tmpAddr = 31;
                        break;
            case 3:  tmpAddr = 32;
                        break;
            case 4:  tmpAddr = 33;
                        break;
            default: return 0;
        }
        //printf("BaseAddr=%x\n", BaseAddr);
        TX_reg_NoStop(AddrEEPROM, tmpAddr);
        return RX8(AddrEEPROM, Data);
    }

 public:
    /// Read all koeff to dimention cc[8]
    unsigned int read_coef()
    {
        unsigned int Dat = 0;
        unsigned char i = 0, ADat = 0;
        unsigned int result = 0;

/*         if((bout = fopen("debuger.out", "a")) == NULL)
         {
             fprintf(stderr, "Debug file is not open!");
             if(dout) fprintf(dout, "Debug file is not open!");
         }
*/
        for (i = 1; i <= 7; i++)
        {
            if( !ReadC(i, &Dat))
            {
                printf(" Error in ReadC[%i] !!\n", i);
                if(dout)  fprintf(dout, " Error in ReadC[%i]  !!\n", i);
                result ++;
                //return 1;
            }
            else
            {
                cc[i] = Dat;
                printf(" C[%i] = %i\n", i, cc[i]);
                if(dout)  fprintf(dout, " C[%i] = %i\n", i, cc[i]);
            }
        }

        for (i = 1; i <= 4; i++)
        {
            if( !ReadS(i, &ADat))
            {
                printf(" Error in ReadS[%i] !!\n", i);
                if(dout)  fprintf(dout, " Error in ReadS[%i]  !!\n", i);
                result ++;
                //return 1;
            }
            else
            {
                aa[i] = ADat;
                printf(" A[%i] = %i\n", i, aa[i]);
                if(dout)  fprintf(dout, " A[%i] = %i\n", i, aa[i]);
            }
        }

        //if(bout) fclose(bout);
        return result; //         return 0;
     }

 private: 
     /// Read AD
     unsigned int read_AD(unsigned char id, unsigned int *Data)
     //unsigned int read_AD(unsigned char id, unsigned int *Data)
     {
         unsigned char addr = 0xF0;
         if(id == 2)   addr = 0xE8;

         TX8(0x77, 0xFF, addr);
         usleep(50000); // delay 50 ms
         TX_reg_NoStop(0x77, 0xFD);

         return RX16(0x77, Data);
     }

public:
    /// Measure pressure
    unsigned int pressure_measure(void)
    {
        unsigned int Dat = 0;	    

        //if(!read_AD(1, &Dat)) ;
        if(!read_AD(1, &Dat))
        {
            printf("Error in pressure\n");
            return 0;
        }
        //printf("pressure = %i\n", Dat);

        return Dat;
    }

    /// Measure temperature
    unsigned int temperature_measure(void)
    {
        unsigned int Dat = 0;

        if(!read_AD(2, &Dat))
        {
            printf("Error in temperature\n");
            return 0;
        }
        //printf("temperature = %i\n", Dat);
        return Dat;
    }

    int calc_bar_temp(unsigned int pp, unsigned int tt, char *message);     
    unsigned int read_bar_temp(char *message);
    void CalculateAltitude(double Press);

}; // class end


// --------------------------------------------------------
/** -------------------------------------------------------
*  \brief Calculate 2**xx
*/
int pow2_x (int xx)
{
    int res=1;
    res <<= xx;
    return res;
}


/** -------------------------------------------------------
*  \brief Read ptressure anf temperature
*/
unsigned int barometer::read_bar_temp(char *message)
{
    unsigned int dd[3] = {0};
    int p = 0;

    dd[1] = pressure_measure();
    dd[2] = temperature_measure();
    //printf("dd1 = %6i dd2 = %6i\n", dd[1], dd[2]);

    if(dd[1] and dd[2])
        p = calc_bar_temp(dd[1],dd[2], message);

    return p;
}


/** -------------------------------------------------------
*  \brief Calculate values of pressure & temperature 
*
*  Calculate values of pressure & temperature from kods pp & tt in INT numbers
* \param pp - pressure kod
* \param tt - temperature kod
* \param message - message to write out info
*/
int barometer::calc_bar_temp(unsigned int pp, unsigned int tt, char *message)
{
    double dUT = 0., qqq = 0., ppp = 0.;
    double P  = 0.0, T  = 0.0, OFF = 0.0, SENS = 0.0;
    long double X = 0.0;
    float c[8];
    int a[5];
    unsigned int i = 0;
    //cc[8] = {7, 13919, 1793, 198, 131, 3052, 5066, 2500};
    //float cc[8] = {7, 29908, 3724, 312, 441, 9191, 3990, 2500};
    //int   aa[5] = {4, 1, 4, 4, 9};

    //pp = 42610; tt = 36508;
    //pp = 30036; tt = 4107;
    //printf("tt = %6i pp = %6i\n", tt, pp);
    pp<<=16;
    pp>>=16;
    tt<<=16;
    tt>>=16;
    for(i = 0; i< 8; i++)
    {
        c[i] = (float) cc[i];
        if(i < 5) 
            a[i] = aa[i];
        //printf("%i : cc[i]=%i, c[i]=%f, aa[i]=%i, a[i]=%i\n", i, cc[i], c[i], aa[i], a[i]);
    }

    // ---------------
    // -- 1 step: --
    dUT  = (float)tt - c[5];
    //printf("dUT = %f\n", dUT);
    qqq  = dUT * dUT; // 128=2^7
    //printf("qqq = %f\n", qqq);
    if(tt < c[5])
    {
        qqq *= (float) a[2];
        //printf("qqq = %f, a[2]=%i\n", qqq, a[2]);
    }
    else
    {
        qqq *= (float) a[1];
        //printf("qqq = %f, a[1]=%i\n", qqq, a[1]);
    }

    ppp = (128.0 * 128.0 *(float)pow2_x(a[3]) );
    //printf("qqq = %f, ppp = %f\n", qqq, ppp);
    qqq /= ppp;
    dUT -= qqq;
    //printf("dUT = %f\n", dUT);

    qqq = 0;
    T  = dUT * c[6];
    T /= 65536.0; // 65536 = 2^16
    T = T + 250.0;
    qqq = dUT/pow2_x(a[4]);
    T -= qqq;	
    //printf(" T = %f\n",  T);

    // ---------------
    // -- step 2 --
    OFF = (c[4] - 1024.0) * dUT;
    OFF /= 16384.0;  // 2^14
    OFF = (c[2] + OFF ) * 4.0;
    //printf("OFF = %f\n", OFF);

    SENS = c[1] + (c[3]* dUT / 1024.0);
    //printf("SENS = %lf\n", SENS);

    X = SENS * ((float)pp - 7168.0)/16384.0 - OFF;
    //printf("X = %Lf\n", X);

    P = X * 10.0/ 32.0 + c[7];
    //printf("P = %f\n", P);

    T = T/10;
    P = P/1;//-T;

    /// \ todo Correct pressure info format
    //sprintf(message, "Bar:  T[ %5i ] = %5.1f C  P[ %5i ] = %6.2f kPa (%4.1f mm w)\n", tt, T, pp, P/100, P/0.981);
    sprintf(message, "Bar:  T = %5.1f oC  P = %6.2f kPa\n", T, P/100);
    print_debug(message);

    return P;
}


//***************************************************
//comparison table for pressure and altitude
//***************************************************
/// Standart atmosphere table
long Tab_BasicAltitude[80]={-6983,-6201,-5413,-4620,-3820,-3015,-2203,-1385,-560, 270,   //0.1m
                          // 1100  1090  1080  1070  1060  1050  1040  1030 1020 1010    //hpa
                            1108, 1953, 2805, 3664, 4224, 5403, 6284, 7172, 8068, 8972,
                                                                                            //910     //hpa
                            // 1000 990        980 970 960           950   940 930 920
                                9885, 10805,11734,12671,13617,14572,15537,16510,17494,18486,
                                                                                                        //hpa
                            // 900 890         880 870 860           850   840 830 820          810
                                19489,20502,21526,22560,23605,24662,25730,26809,27901,29005,
                                                                                                        //hpa
                            // 800 790         780 770 760           750   740 730 720          710
                                30121,31251,32394,33551,34721,35906,37106,38322,39553,40800,
                                                                                                        //hpa
                            // 700 690         680 670 660           650   640 630 620          610
                                42060,43345,44644,45961,47296,48652,50027,51423,52841,54281,
                                                                                                        //hpa
                            // 600 590         580 570 560           550   540 530 520          510
                                55744,57231,58742,60280,61844,63436,65056,66707,68390,70105,           //hpa
                            // 500 490         480 470 460           450   440 430 420          410
                                71854,73639,75461,77323,79226,81172,83164,85204,87294,89438};           //hpa
                            // 400 390         380 370 360           350   340 330 320          310



/** -------------------------------------------------------
*  \brief Calculate altitude 
*
*  Calculate values altitude from pressure
* \warning Возможно не работает
* \param Pressure - pressure kod
*/
void barometer::CalculateAltitude(double Pressure)
{
    unsigned char ucCount = 0;
    unsigned int  uiBasicPress = 0;
    unsigned int  uiBiasTotal = 0;
    unsigned int  uiBiasPress = 0;
    unsigned int  uiBiasAltitude = 0;
    unsigned int Press = 0;
    long Altitude = 0;

    Press = (unsigned int) Pressure;
    for( ucCount=0;      ; ucCount++ )
    {
            uiBasicPress = MaxPress-(ucCount*10);
            //if(uiBasicPress < (Press/100)) break;
            if(uiBasicPress < (Press/100)) break;
            //printf("uiBasicPress = %u MaxPress = %u ucCount = %i\n",  uiBasicPress, MaxPress, ucCount );
    }
    printf("uiBasicPress = %u MaxPress = %u ucCount = %i\n",  uiBasicPress, MaxPress, ucCount );

    uiBiasTotal = Tab_BasicAltitude[ucCount] - Tab_BasicAltitude[ucCount-1];
    printf("Tab_BasicAltitude[ucCount] = %li Tab_BasicAltitude[ucCount-1] = %li\n",  Tab_BasicAltitude[ucCount], Tab_BasicAltitude[ucCount-1] );

    uiBiasPress = Press - (long)(uiBasicPress*100);
    uiBiasAltitude = (long)uiBiasTotal * uiBiasPress/1000;
    printf("\nuiBiasTotal = %u, uiBiasPress = %u uiBiasAltitude = %u\n", uiBiasTotal, uiBiasPress, uiBiasAltitude );
    Altitude = Tab_BasicAltitude[ucCount] - uiBiasAltitude;
    ucCount = abs(Altitude % 10);         // four lose and five up
    printf("Altitude = %li, ucCount = %i\n", Altitude, ucCount);
    if(Altitude < 0)
    {
        if(ucCount > 4)
            Altitude -= 10-ucCount;
        else
            Altitude += ucCount;
    }
    else
    {
        if(ucCount > 4)
            Altitude += 10-ucCount;
        else
            Altitude -= ucCount;
    }
    printf("\nAltitude = %li\n", Altitude);
    return;
}
//================================================

#endif

