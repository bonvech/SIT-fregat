/**
 * \file led.cpp
 * \brief Класс led. LED calibration
 *
 * Class led. Functions for board of LED calibration
 * \date 06.02.2009  modified 2019-10-31
 */

#ifndef _LED
#define _LED                            ///< LED class label
#define LED_FILE "./config/lcd.config"  ///< File to read LED configuration

#include "i2c.cpp"
//#include "bar.cpp"


/// Structure to hold LED configuration
struct lconfig
{
    unsigned int c[9]; ///<  channel amplitudes
    unsigned int m;    ///< delay of LED pulse after trigger
    unsigned int b;    ///< include LED pulse in event: onoff
    unsigned int l;    ///< onoff external start: 1 for On or 0 for Off to prohibit external start
    unsigned int w;    ///< onoff generator and +12V for barometers
    unsigned int d;    ///< light pulse duration
};


/// Class for LED board
class led: public i2c
{
  private:
    /// \todo check fifosum and fifo 
    int  fifo[22];          ///< unknown variable to count fifosum
    long fifosum;           ///< unknown variable to count fifosum

  public:
    char info[80];          ///< string to write debug info
    unsigned char led_mask; ///< byte to write to 0xD address
    unsigned char manager;  ///< byte to write to 0xB address
    int banner_external;    ///< flag to prohibit external pulses 

    struct lconfig ReadConf; ///< structure to read from config file
    struct lconfig Conf;     ///< structure to hold work parametes


    led();

  private:
    //  I2C interface  // obrashenie k zhelezy
    void SDA(unsigned char Data);
    void SCL(unsigned char Data);
    unsigned char SDAin(void);

    //  LED board functions
    void SetAddr(unsigned int addr);
    unsigned char ReadDAC( unsigned char AddrDev, unsigned char *Data);
    unsigned char led_init();

  public:
    unsigned char led_prepare();
    unsigned char ban_external(unsigned char onoff);
    unsigned char led_in_event_onoff(unsigned char onoff);
    unsigned char generator_12_onoff( unsigned char onoff );
    unsigned char bar_onoff(unsigned char onoff );
    unsigned char led_pulse();
    unsigned char set_trigger_delay(unsigned char delay);
    unsigned char led_pulseN(unsigned int nn);
    unsigned char led_pulseN_sec(unsigned int nn);
    unsigned int  read_chan_amp(unsigned char Chan);
    unsigned char set_chan_amp(unsigned char chan, unsigned char Data);
    int set_led_duration( unsigned char Data);
    int sum_to_fifo(int val);
    int read_ADC(char *message);

    //---------------------------------------------------------
    // new functions:
    int read_config_from_file();
    int print_config(struct lconfig Conf);
    int import_config(); // write led configuration ReadConf to Conf
    int set_config();
    int save_config();
    int test_config(struct lconfig ReadConf);
    int set_config_from_file();
};


//---------------------------------------------------------
//---------------------------------------------------------
/// constructor
led::led()
{
    fifosum = 0;
    for(int z = 0; z < 22; z++)
    {
        fifo[z] = 0;
    }
    for(int z = 0; z < 8; z++)
    {
        Conf.c[z] = 20;
        ReadConf.c[z] = 20;
    }
    Conf.c[8] = 255;
    ReadConf.c[8] = 255;
    Conf.m = 5;
    Conf.b = 1; 
    Conf.l = 1; // ban_external
    Conf.w = 1;
    Conf.d = 5;

    BaseAddr    = BaseAddrLED;
    ADCSubAddr2 = 0x00;
    led_mask = 0;
    manager = 10;
    banner_external = 1; // flag to prohibit external pulses 

    led_init();
}


//---------------------------------------------------------
/// I2C SDA function
void led::SDA(unsigned char Data)
{
    if (!(Data)) { outb((inb(BaseAddr)  |  2),  (BaseAddr));
                    outb((inb(BaseAddr)  |  2),  (BaseAddr));}
    else         { outb((inb(BaseAddr) & (~2)), (BaseAddr));
                    outb((inb(BaseAddr) & (~2)), (BaseAddr));}
}


//---------------------------------------------------------
/// I2C SCL function
void led::SCL(unsigned char Data)
{
    if (!(Data)) {outb((inb(BaseAddr)  |  1),  (BaseAddr));
                    outb((inb(BaseAddr)  |  1),  (BaseAddr));}
    else         {outb((inb(BaseAddr) & (~1)), (BaseAddr));
                    outb((inb(BaseAddr) & (~1)), (BaseAddr));}
}


//---------------------------------------------------------
/// I2C SDAin finction
unsigned char led::SDAin(void)
{
    SDA(1);
    if ((inb(BaseAddr) & 8))
        { return 1;}
    return 0;
}


// ================================================ //
// ================================================ //
//  LED board functions
// ================================================ //
// ================================================ //


//---------------------------------------------------------
/// Set base address
void led::SetAddr(unsigned int addr)
{
    BaseAddr = addr;
    //if(dout) fprintf(stdout, "LED: BaseAddr = %xh\n", BaseAddr);
    if(dout) fprintf(  dout, " LED: BaseAddr = %xh ", BaseAddr);
}


//---------------------------------------------------------
/// Read Data from DAC with addr AddrDev = 0
unsigned char led::ReadDAC( unsigned char AddrDev, unsigned char *Data)
{
    unsigned char tmpAddr;

    if (AddrDev == 0) 
        tmpAddr = 0x2C;
    //else if (AddrDev==1) {tmpAddr = 0x2D;}
    else 
        return 0;
    return RX8(tmpAddr, Data);
}


//---------------------------------------------------------
/// init LED
unsigned char led::led_init()
{
    manager = set_bit_1(manager, 0); // prohibit external trigger for LED pulses
    manager = set_bit_0(manager, 1); // set bit 2 of manager(addr 0xB) to 1 = work position
    outb(manager, BaseAddrLED + 0xB);
    //printf("\nBaseAddrLED + 0xB = %xh, byte = %i\n", BaseAddrLED + 0xB, manager);

    // lamp off:
    //if(set_chan_amp(8, 255)) printf("\nError in set_chan_amp!!");
    return 0;
}


//-----------------------------------------------------
/// Prepare LED - run before measurements start:
// set work parameters
unsigned char led::led_prepare()
{
    set_led_duration(20);

    manager = set_bit_1(manager, 0); // prohibit external trigger for LED pulses
    manager = set_bit_0(manager, 1); //
    manager = set_bit_0(manager, 2); //
    manager = set_bit_0(manager, 3); //
    manager = set_bit_0(manager, 4); //
    manager = set_bit_0(manager, 5); //
    manager = set_bit_1(manager, 6); //
    manager = set_bit_0(manager, 7); //
    outb(manager, BaseAddrLED + 0xB);
    //printf("\nBaseAddrLED + 0xB = %xh, byte = %i\n", BaseAddrLED + 0xB, manager);
    return 0;
}


//---------------------------------------------------------
/// Permit extermal trigger
unsigned char led::ban_external(unsigned char onoff)
{
    // set bit 1 of manager(addr 0xB) to on/off
    if(onoff)   manager = set_bit_1(manager,0);
    else        manager = set_bit_0(manager,0);

    banner_external = onoff;

    outb(manager, BaseAddrLED + 0xB);
    printf("\nBaseAddrLED + 0xB = %xh, byte = %i\n", BaseAddrLED + 0xB, manager);
    return 0;
}


//---------------------------------------------------------
/// Include or not LED pulse in event
unsigned char led::led_in_event_onoff(unsigned char onoff)
{
    // set bit 1 of manager(addr 0xB) to on/off
    if(onoff)   manager = set_bit_1(manager,1);
    else        manager = set_bit_0(manager,1);

    outb(manager, BaseAddrLED + 0xB);
    printf("\nBaseAddrLED + 0xB = %xh, byte = %i\n", BaseAddrLED + 0xB, manager);
    return 0;
}


//---------------------------------------------------------
/// On(1)/off(0) generator & _12V for barometers
//  !!!!!
unsigned char led::generator_12_onoff( unsigned char onoff )
{
    // set bit 2 of manager(addr 0xB) to on/off generator and +12V
    if(onoff) manager = set_bit_1(manager,2);
    else      manager = set_bit_0(manager,2);

    sprintf(info,"LED: man = %i, addr =%xh\n", manager, BaseAddrLED + 0xB);
    print_debug(info);

    outb(manager, BaseAddrLED + 0xB);
    usleep(50000);
    return 0;
}


//---------------------------------------------------------
/// On(1)/off(0) barometer
//  !!!!!
unsigned char led::bar_onoff(unsigned char onoff )
{
    // set bit 3 of manager(addr 0xB) to on/off
    if(onoff)   manager = set_bit_1(manager,3);
    else        manager = set_bit_0(manager,3);

    outb(manager, BaseAddrLED + 0xB);
    return 0;
}


//---------------------------------------------------------
/// Make LED pulse
unsigned char led::led_pulse()
{
    manager = set_bit_0(manager,0); // set bit 0 of manager(addr 0xB) to 1
    outb(manager, BaseAddrLED + 0xB);
    //printf("\nBaseAddrLED + 0xB = %xh, byte = %i\n", BaseAddrLED + 0xB, manager);
    manager = set_bit_1(manager, 0); // set bit 0 of manager(addr 0xB) to 0 = start LED pulse
    outb(manager, BaseAddrLED + 0xB);
    //printf("\nBaseAddrLED + 0xB = %xh, byte = %i\n", BaseAddrLED + 0xB, manager);
    if(!banner_external) 
    {
        manager = set_bit_0(manager,0); //  permit external trigger for pulses
        outb(manager, BaseAddrLED + 0xB);
    }
    return 0;
}


//---------------------------------------------------------
/// Set LED delay after trigger
unsigned char led::set_trigger_delay(unsigned char delay)
{
    delay   = delay << 4;      // dddd0000
    manager = manager & 15;    // 0000mmmm
    manager = delay | manager; // ddddmmmm

    outb(manager, BaseAddrLED + 0xB);
    //printf("\nBaseAddrLED + 0xB = %xh, byte = %i\n", BaseAddrLED + 0xB, manager);
    return 0;
}


//---------------------------------------------------------
/// Make N LED pulses
unsigned char led::led_pulseN(unsigned int nn)
{
    unsigned int ii = 0;

    for(ii = 0; ii < nn; ii ++)
    {
        led_pulse();
        usleep(1000);
    }
    return 0;
}


//---------------------------------------------------------
/// Make N LED pulses with step 1 sec
unsigned char led::led_pulseN_sec(unsigned int nn)
{
    unsigned int ii = 0;

    for(ii = 0; ii < nn; ii ++)
    {
        led_pulse();
        sleep(1);
    }
    return 0;
}


//---------------------------------------------------------
/// Read DAC in LED channel
unsigned int led::read_chan_amp(unsigned char Chan)
{
    unsigned char Dat = 0;
    unsigned int  addr = BaseAddrLED + Chan - 1;

    //printf("read_chan_amp in\n");
    SetAddr(addr);
    if(!ReadDAC(0, &Dat))
    {
        print_debug((char*)"ReadDAC failed in recieve\n");
        return 1;
    }

    sprintf(info,"DAC[%1i] = %3i \n", Chan, Dat);
    print_debug(info);
    return 0;
}


//---------------------------------------------------------
/// Set amplitude in LED channel
unsigned char led::set_chan_amp(unsigned char chan, unsigned char Data)
{
    unsigned char out = 0;
    unsigned int addr = BaseAddrLED + chan - 1;

    //printf("set_chan_amp in\n");
    SetAddr(addr);
    if( !WriteDAC(0x0, Data))  //
    {
        out = 1;
        sprintf(info, "\nset_LED_amp: Error in DAC`s transmittion !!! chan = %d\n", (unsigned int)chan);
        print_debug(info);
    }
    print_debug((char*)"WriteDAC O'K");
    return out;
}


//---------------------------------------------------------
/// Set LED duration
int led::set_led_duration( unsigned char Data)
{
    sprintf(info, "  BaseAddrLED + 0xC = %xh  ", BaseAddrLED + 0xC);
    print_debug(info);
    outb(Data, BaseAddrLED + 0xC);
    return 0;
}


//---------------------------------------------------------
/// Add to fifo current value of CH0-CH2
int led::sum_to_fifo(int val)
{
    int i = 0;
    // ---- shift fifo
    //printf("sum_to_fifo: Enter\n");
    if( fifo[0] == 20)
    {
        fifosum -= fifo[1];
        for(i = 1; i <= 19; i++)
        {
            fifo[i] = fifo[i+1];
        }
        fifo[0] = 19;
    }

    //add value to fifo
    if( fifo[0] < 20)
    {
        fifo[0] ++;
        fifo[fifo[0]] = val;
        fifosum += val;
    }
    else
    {
        printf("sum_to_fifo: Error!");
    }

/*            for(i = 1; i <= fifo[0]; i++)
    {
        printf("   fifo[%4d] = %2d\n", i, fifo[i]);
    }
    printf("sum_to_fifo: Exit\n");
*/
    return (long) fifosum * 10 /fifo[0];
}


//---------------------------------------------------------
/// Read and print ADC registers (Pressure)
int led::read_ADC(char *message)
{
    unsigned int   Data[4];
    unsigned short i = 0; //, ii = 0;
    unsigned int   tmp = 0; //, n = 0;
    //unsigned long  sum = 0;
    char mes[100];
    //float dp = 0.;

    strcpy(message, "LED: ");
    SetAddr(BaseAddrLED + 0x000E);
    if(!ReadADCs(0x00, (unsigned int*)&Data))
    {
        print_debug((char*)"LED:  No device\n");
        return 1;
    }
    for(i = 0; i < 4; i++) // read ADC
    {
        tmp  = Data[i];
        tmp &= 0x0FFF;
        sprintf(mes, " CH%1i[%4i]", i, tmp);
        strcat(message, mes);
    }

    /*
    tmp  = Data[0] - Data[2];
    tmp &= 0x0FFF;
    dp = tmp;
    //if(tmp < 0) tmp = 0;
    //if(tmp) sum = sum_to_fifo(tmp);
    //printf("   dP[%4d]  aver = %5ld\n", tmp, sum);
    //sprintf(mes, "   dP[%4d]  aver = %4ld.%ld\n", tmp, sum/10, sum%10);
    dp = (dp - 5.) * 2.74;
    sprintf(mes, "   DeltaP = %7.3f kPa  (%6.2f mm w)\n", dp/1000, dp/9.81);
    strcat(message, mes);
    */

    print_debug(message);
    return 0;
}

/*
* int read_ADC(void)
{
    unsigned int   Data[4];
    unsigned short i = 0;
    unsigned int   tmp = 0;
    unsigned long  sum = 0;
    char mes[100];

    strcpy(out, "LED: ");
    SetAddr(BaseAddrLED + 0x000E);
    if(!ReadADCs(0x00, (unsigned int*)&Data))
    {
        printf("LED:  No device\n");
        if(dout) fprintf(dout, "  No device\n");
        return 1;
    }
    for(i = 0; i < 4; i++) // read ADC
    {
        tmp  = Data[i];
        tmp &= 0x0FFF;
        sprintf(mes, " CH%1i[%4i]", i, tmp);
        strcat(out, mes);
    }

    tmp  = Data[0] - Data[2];
    tmp &= 0x0FFF;
    if(tmp < 0) tmp = 0;
    if(tmp) sum = sum_to_fifo(tmp);
    //printf("   dP[%4d]  aver = %5ld\n", tmp, sum);
    //sprintf(mes, "   deltaP = [%4d] =  aver = %4ld.%ld\n", (tmp-5)*2.74, sum/10, sum%10);
    sprintf(mes, "   deltaP = %6.2f =  aver = %4ld.%ld\n", (tmp-5)*2.74, sum/10, sum%10);
    strcat(out, mes);

    //if(stdout) fprintf(stdout, "\n%s", message);
    if(  dout) fprintf(  dout, "\n%s", out);
    return 0;
}
*/


//---------------------------------------------------------
/** \brief Read LED configuration from file
 *
 *  \return  0  -- OK\n
 *          -1  -- error in file opening\n
 *         err  -- number of errors in file reading
 */ 
int led::read_config_from_file()
{
    char  buf[100]  = "";
    char  line[250] = "";
    int   kk = 0, err = 0;
    unsigned short lpar = 0, lnum = 0;
    FILE *fpar = NULL;

    // ------ open config file --------------------------
    if((fpar = fopen( LED_FILE, "r")) == NULL)
    {
        sprintf(buf, "\n  Error: file \"%s\" is not open!\n", LED_FILE);
        print_debug(buf);
        return -1;
    }
    sprintf(buf, "\n=== Read LED Config from \"%s\" \n", LED_FILE);
    print_debug(buf);

    // ------- read parameters from config file ----------
    while( fgets(line, sizeof(line), fpar) != NULL )
    {
        //printf("line:%s", line);
        if( (line[0] == '\n') || (line[0] == '/') )
        {
            continue;
        }
        if(dout) fprintf(dout, "%s", line);

        kk = sscanf(line, "%s", buf);
        if(!kk) continue;
        //if( !strcmp(buf,"/") ) continue;

        lpar = 0;
        lnum = 0;
        kk = 0;
        if( !strcmp(buf,"c") )
        {
            kk = sscanf(line, "%s %hu %hu", buf, &lnum, &lpar);
            if(kk > 2)
            {
                if((lnum > 0) && (lnum < 9)) 
                        ReadConf.c[lnum] = lpar;
            }
            else        err++;
            continue;
        }
        if( !strcmp(buf,"d") )
        {
            kk = sscanf(line, "%s %hu", buf, &lpar);
            if(kk > 1)  ReadConf.d = lpar;
            else        err++;
            continue;
        }
        if( !strcmp(buf,"m") )
        {
            kk = sscanf(line, "%s %hu", buf, &lpar);
            if(kk > 1)  ReadConf.m = lpar;
            else         err++;
            continue;
        }
        if( !strcmp(buf,"b") )
        {
            kk = sscanf(line, "%s %hu", buf, &lpar);
            if(kk > 1)  ReadConf.b = lpar;
            else        err++;
            continue;
        }
        if( !strcmp(buf,"l") )
        {
            kk = sscanf(line, "%s %hu", buf, &lpar);
            if(kk > 1)  ReadConf.l = lpar;
            else        err++;
            continue;
        }
        if( !strcmp(buf,"w") )
        {
            kk = sscanf(line, "%s %hu", buf, &lpar);
            if(kk > 1)  ReadConf.w = lpar;
            else        err++;
            continue;
        }
    }
    fclose(fpar);
    sprintf(info, "===> end of file \"%s\" <=====\n", LED_FILE);
    print_debug(info);
    return err; // number of errors
}


//---------------------------------------------------------
/// Print LED config to file
int led::print_config(struct lconfig Conf)
{
    char info[20] = "";

    print_debug((char*)"LED.Config:\n");
    for(int i = 1; i <= 8; i++)
    {
        sprintf(info,"c %d %d\n", i, Conf.c[i]);
        print_debug(info);
    }
    sprintf(info, "d %d\n", Conf.d);  print_debug(info);
    sprintf(info, "m %d\n", Conf.m);  print_debug(info);
    sprintf(info, "b %d\n", Conf.b);  print_debug(info);
    sprintf(info, "l %d\n", Conf.l);  print_debug(info);
    sprintf(info, "w %d\n", Conf.w);  print_debug(info);

    return 0;
}


//---------------------------------------------------------
/**
 * \brief Write LED configuration to Conf structure
 *
 * Write LED configuration ReadConf from read structure to Conf structure
 * \return 0
 */
int led::import_config()
{
    int i = 0;

    print_debug((char*)"\nImport LED.Config:\n");

    for(i = 1; i <= 8; i++)
    {
        Conf.c[i] = ReadConf.c[i];
    }
    Conf.d = ReadConf.d;
    Conf.m = ReadConf.m;
    Conf.b = ReadConf.b;
    Conf.l = ReadConf.l;
    Conf.w = ReadConf.w;

    print_debug((char*)"... done\n");

    return 0;
}


//---------------------------------------------------------
/**
 * \brief Save LED configuration
 * Save LED configuration to file "./log/lcd.con"
 * \return 0 -- OK\n
 *         1 -- file opening error
 */
int led::save_config()
{
    FILE *fpar = NULL;
    int i = 0;
    char fname[100] = {"./log/lcd.con"};

    // ------ open config file --------------------------
    if((fpar = fopen(fname, "w")) == NULL)
    {
        sprintf(fname, "\nSave_config():  Error: file \"%s\" is not open!\n", fname);
        print_debug(fname);
        return 1;
    }
    printf("\n\n\nfile \"%s\" is open!\n", fname);
    print_debug((char*)"Save LED.Config:");

    // ------ print config file --------------------------
    fprintf(fpar,"// channel amplitudes\n");
    for(i = 1; i <= 8; i++)
    {
        fprintf(fpar, "c %d %d\n", i, ReadConf.c[i]);
    }

    fprintf(fpar,"\n// Baseaddress\n");
    fprintf(fpar,"a %x\n", BaseAddr);
    fprintf(fpar,"\n// light pulse duration\n");
    fprintf(fpar,"d %d\n", ReadConf.d);
    fprintf(fpar,"\n// set delay of LED pulse after trigger \n");
    fprintf(fpar,"m %d\n", ReadConf.m);
    fprintf(fpar,"\n// include led pulse in event: onoff\n");
    fprintf(fpar,"b %d\n", ReadConf.b);
    fprintf(fpar,"\n// onoff external start: 1 for On or 0 for Off to ban external start\n");
    fprintf(fpar,"l %d\n", ReadConf.l);
    fprintf(fpar,"\n// onoff generator and +12V\n");
    fprintf(fpar,"w %d\n", ReadConf.w);

    fclose(fpar);
    print_debug((char*)"... done\n");

    return 0;
}


//---------------------------------------------------------
/**
 * \brief Set LED configuration
 * 
 * Set LED configuration to LED board\n
 * \return 0
 */
int led::set_config()
{
    int  i = 0;

    print_debug((char*)"\nSet LED.Config:\n");
    for(i = 1; i <= 8; i++)
    {
        sprintf(info, "Setting LED c %d %d  ", i, Conf.c[i]);
        print_debug(info);
        if(stdout) fflush(stdout);
        if(dout)   fflush(dout);
        set_chan_amp(i, Conf.c[i]);
        print_debug((char*)"... done\n");
    }

    //printf("d %d\n", Conf.d);
    // d
    sprintf(info, "Setting LED d %d", Conf.d);
    print_debug(info);
    if(stdout) fflush(stdout);
    if(dout)   fflush(dout);
    set_led_duration(Conf.d);
    print_debug((char*)"... done\n");

    // m
    sprintf(info, "Setting LED m %d", Conf.m);
    print_debug(info);
    if(stdout) fflush(stdout);
    if(dout)   fflush(dout);
    set_trigger_delay(Conf.m);
    print_debug((char*)"... done\n");

    // b
    sprintf(info, "Setting LED b %d", Conf.b);
    print_debug(info);
    if(stdout) fflush(stdout);
    if(dout)   fflush(dout);
    led_in_event_onoff(Conf.b);
    print_debug((char*)"... done\n");

    // l
    sprintf(info, "Setting LED l %d", Conf.l);
    print_debug(info);
    if(stdout) fflush(stdout);
    if(dout)   fflush(dout);
    ban_external(Conf.l);
    print_debug((char*)"... done\n");

    // w
    sprintf(info, "Setting LED w %d  ", Conf.w);
    print_debug(info);
    if(stdout) fflush(stdout);
    if(dout)   fflush(dout);
    generator_12_onoff(Conf.w);
    print_debug((char*)"... done\n");

    return 0;
}


//---------------------------------------------------------
/**
 * \brief Test LED configuration
 * 
 * \return number of errors in LED configuration
 */
int led::test_config(struct lconfig ReadConf)
{
    int i = 0;
    int panic = 0;
    char debug[50];

    print_debug((char*)"\nTest LED.Config:\n");
    for(i = 0; i <= 8; i++)
    {
        if(ReadConf.c[i] > 255)
        {
            panic ++;
            sprintf(debug, "Error in c %d %d\n", i, ReadConf.c[i]);
            print_debug(debug);
        }
    }
    if(ReadConf.c[8] < 255) 
    {
        sprintf(debug, "Warning in c 8 %d\n", ReadConf.c[8]); 
        print_debug(debug);
    }

    // 0 < ReadConf.d < 255 
    if((ReadConf.d > 15) && (ReadConf.d < 3))
    {
        panic ++;
        sprintf(debug, "Error in d %d: check d: 3 < d < 15\n", ReadConf.d); 
        print_debug(debug);
    }

    // ReadConf.m <= 15
    if(ReadConf.m > 15) 
    {
        panic ++;
        sprintf(debug, "Error in m %d\n", ReadConf.m);
        print_debug(debug);
    }

    // b = 0, 1
    if((ReadConf.b != 1) &&  (ReadConf.b != 0))
    {
        panic ++;
        sprintf(debug, "Error in b %d:\n", ReadConf.b); 
        print_debug(debug);
    }

    // ReadConf.l = 0, 1
    if((ReadConf.l != 1) &&  (ReadConf.l != 0))
    {
        panic ++;
        sprintf(debug, "Error in l %d:\n", ReadConf.l); 
        print_debug(debug);
    }

    // ReadConf.w = 0, 1
    if((ReadConf.w != 1) &&  (ReadConf.w != 0))
    {
        panic ++;
        sprintf(debug, "Error in w %d:\n", ReadConf.w); 
        print_debug(debug);
    }

    print_debug((char*)" ... done. ");
    // ------ print summary --------
    if(panic)
    {
        sprintf(debug, "%d Errors in LED.Config found!!!\n Check %s file \"lcd.config\" !!!!\n", 
                        panic, LED_FILE);
        print_debug(debug);
        if(stdout) fflush(stdout);
        if(dout)   fflush(dout);
    }
    else
    {
        print_debug((char*)"OK\n");
    }

    return panic;
}


//---------------------------------------------------------
/**
 * \brief Set LED configuration from file
 * \return: 0 - OK\n
 *          1 - errors in file reading\n
 *          2 - errors in parameters in configuration file
 */
int led::set_config_from_file()
{
    print_debug((char*)"========== LED =================================\n");

    if(read_config_from_file() )
    {
        printf("Error in reading \"lcd.config\" file!");
        return 1;
    }
    print_config(ReadConf);  //  print values
    if(test_config(ReadConf))  // errors in check
    {
        return 2;
    }

    import_config(); // write LED configuration ReadConf to Conf
    set_config();    // set to registers
    save_config();   // save to file
    print_debug((char*)"================================================\n");

    return 0;
}

#endif
