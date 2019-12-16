/**
 * \file sipm.cpp (old hvpstest.cpp)
 * \brief Класс SiPM. Источники высоковольтного напряжения питания ФЭУ. 
 * \date  November 2018. Modified November 2019
 *
 * Class SiPM. New version for one VIP for SiPM.
 */

#include "hvcalibr.h"


#define HVDIM 112       ///< Dimension of hv arrays from 112 reduced to 1
#define HIGHMAX 254     ///< max kod to high voltage DAC
#define HIGHMIN 100     ///< min kod to high voltage DAC
#define H_HIGHMAX 150   ///< max kod to high voltage DAC to HAMAMATSU
#define H_HIGHMIN 100   ///< min kod to high voltage DAC to HAMAMATSU

#define VIP_ADDR 26    ///< Address of SiPM VIP
#define VIP_SUBADDR 0  ///< SubAddress of SiPM VIP


extern struct input_parameters Work;
extern char vip_out[1024];

const int CurKoef = 100;  ///< Coeff to write current as integer


/** -------------------------------------------------------------
 * Class for SIPM board
 */
class SiPM : public lvps
{

public:
    int ChanAddr;                 ///< Channel Address
    unsigned char SubAddr;        ///< SubAddress
    static const unsigned char sipm_addr = VIP_ADDR * 2 + VIP_SUBADDR; ///< SiPM address

    unsigned char On;             ///< flag
    float current;                ///< param to hold SiPM current
    float Up;                     ///< param to hold SiPM supply Voltage
    unsigned char highv;          ///< value of set High voltage kod


    unsigned char hvwork[HVDIM];  ///< marker of channel OnOff
    unsigned int  pmt_cur[HVDIM]; ///< array of currents in channels: int(cur * CurKoef)
    //char info[200];               ///< string to write debug info
    char debug[230];              ///< string to write debug


    /// constructor
    SiPM() 
    {
        lvps();
        ChanAddr = VIP_ADDR;
        SubAddr  = VIP_SUBADDR;
        //sipm_addr = VIP_ADDR * 2 + VIP_SUBADDR;
        sprintf(debug, "SiPM started");
        On = 0;
        for(int ii = 0; ii < HVDIM; ii++)
        {
            pmt_cur[ii] = 0;
        }

        self_test();
    }


private:
    /** -------------------------------------------------------
    *  \brief Set SubAddress
    *
    */
    void SetSubAddr(unsigned char addr)
    {
        SubAddr = addr;
    }

public:
    int self_test();
    int SetChanHigh(unsigned char ii, unsigned char high);
    int high();
    int high_scout(unsigned char Addr, unsigned char SubAddr);
    unsigned char measure_high(void);
    unsigned int read_vip_ADC(char *message);
    float measure_current(void);
    int check_current();
    int print_currents(FILE *ff);
    int print_currents_to_binary(FILE *ff);
    void turn_off(void);
    void check_turn_off(void);
}; // class SiPM




/** -------------------------------------------------------
 *  \brief test high  voltage channels
 *  \date July 2019 изменено на единственный канал
 *
 *  Проверяется возможность и правильность записи и чтения в каналы ЦАП
 */
int SiPM::self_test()
{
    unsigned char ii = 0;
    unsigned char kod = 0;
    unsigned char err[112] = {0};

    /// -- write numbers to channels
    print_debug( (char*)"\nHV Channels Self-Test: write numbers to HV channels:\n");
    ii = sipm_addr;
    sprintf(debug, " ii=%2i-%2i-%i", ii, (int)ii/2, ii%2);
    print_debug(debug);

    if(SetChanHigh(ii, ii))  // set high = 0
    {   // error
        sprintf(debug, "self_test: Error in DAC`s transmittion chan = %i", ii);
        print_debug(debug);
    }


    // ------- read numbers
    /// -- read actual high value from chanels
    print_debug( (char*) "\n\nREAD numbers from HV channels:\n");
    ii = sipm_addr;

    SetChannelAddr(26); // !!! for SiPM in 2018 year
    SetSubAddr(0);      // !!! for SiPM in 2018 year

    sprintf(debug,  " ii=%2i-%2i-%i", ii, ii/2, ii%2);
    print_debug(debug);

    if(!ReadDAC(SubAddr, &kod))
    {
        print_debug( (char*) " self_test: ReadDAC failed in recieve ");
        //return 1; // error in read DAC
        kod = 200;
    }
    else
    {
        //printf("= %3i", kod);
        if(dout) fprintf(dout," kod = %3i", kod);
    }

    // diagnostics
    if(ii == kod)
    {
        print_debug( (char*)"  OK");
    }
    else
    {
        print_debug( (char*)" ERR");
        err[0] ++;
        err[err[0]] = ii;
    }

    /// -- write 0 to channels -------------
    print_debug( (char*)"\n\nWrite 0 to HV channels:");
    ii = sipm_addr;
    if(SetChanHigh(ii, 0))  // set high = 0
    {   // error
        sprintf(debug,  "self_test: Error in DAC`s transmittion chan = %i", ii);
        print_debug(debug);
    }

    /// -- print diagnostics results
    print_debug( (char*)"\n\nHV Channels Self-Test: ");
    if(err[0])  // there are errors in channels
    {
        sprintf(debug, "Errors in %i channels:", err[0]);
        print_debug(debug);
        for(ii = 1; ii <= err[0]; ii++)
        {
            sprintf(debug, " %2i", err[ii]);
            print_debug(debug);
        }
    }
    else  // -- no errors
    {
        print_debug( (char*)" Success");
    }
    print_debug( (char*)"\n");

    return err[0];
}


/** -------------------------------------------------------
 *  \brief set voltage high to one channel with Addr = ii/2 and SubAddr = ii%2
 *  \return 0 -- OK\n
 *          2 -- error in write to DAC
 */
int SiPM::SetChanHigh(unsigned char ii, unsigned char high)
{
    unsigned char kod = 0;

    SetChannelAddr(int(ii/2)); // 2011 year
    SetSubAddr(ii%2);
    if(dout) fprintf(dout, "\n ii=%2i-%2i-%i", ii, (int)ii/2, ii%2);

    // ---- read actual high value ----
    if(!ReadDAC(SubAddr, &kod))
    {
        sprintf(debug, "\n ii=%2i--%2i--%i ", ii, (int)ii/2 , ii%2);
        print_debug(debug);
        print_debug((char*)" SCH: ReadDAC failed in recieve, kod set to 0. ");
        kod = 0;
    }
//       else
//       {
//           printf("DAC[%3i]", kod);
//           if(dout) fprintf(dout, "DAC[%3i]", kod);
//       }

    //printf(" high = %i", high);
    if(dout) fprintf(dout, " high = %i", high);
    if( (high - kod) > 5) // new value is biggest
    {
        print_debug((char*) " sleep ");
        if(stdout) fflush(stdout);
        usleep(500000);
    }

    // --- set high
    if(!WriteDAC(SubAddr, high))  // set high kods
    {
            print_debug((char*) "SCH: Error in DAC`s transmittion - set high!!");
            return 2;  // error in write to DAC
    }
    return 0; // no errors
}


/** -------------------------------------------------------
*  \brief measure high in all work channels
*  \date 2018.11 changed to one channel (2010.03.11)
*/
/*
int  measure_all_high()
{
    unsigned char ii = 0;

    print_debug((char*) "\nMeasure high\n");

    ii = VIP_ADDR * 2 + VIP_SUBADDR;
    SetChannelAddr(int(ii/2));
    SetSubAddr(ii%2);

    if(dout) fprintf(dout, "i = %2i--%2i--%i: ", ii, ChanAddr, SubAddr);
    if(stdout) printf("i = %2i: ", ii);

    measure_high();
    return 0;
}
*/


/** -------------------------------------------------------
*  \brief set optimal high voltage in SIPM vip
*  \date 2018.11, 2019.11
*/
int SiPM::high()
{
    unsigned char high = 0, highset = 0;
    unsigned char flag = 1;
    unsigned char need = 1;
    unsigned highmax = HIGHMAX;
    unsigned highmin = HIGHMIN;
    float cur = 0;
    float Workcur = 0;

    // --- set variables, print debug
    On = 1;
    highmax = Work.umax;
    highmin = Work.umin;
    cur     = Work.workcur;
    Workcur = Work.workcur; //I_to_kod(Work.workcur);
    print_debug((char*)"\n==========================\nSetHighVoltage high() procedure:");
    sprintf(debug, "\nWorkcur = %.1f mA\nTurn on ", Workcur );
    print_debug(debug);

    // -------- turn on HV  -----------   
    need   = 1;
    //highv[sipm_addr]  = 0; //code value of high voltage
    highv = 0; //code value of high voltage
    hvwork[sipm_addr] = 1; // flag onoff

    if(SetChanHigh(sipm_addr, 0))  // set high = 0
    {   // error
        sprintf(debug, "\nset high: Error in DAC`s transmittion\n");
        print_debug(debug);
        need  = 0;
        hvwork[sipm_addr] = 0;
    }
    else
    {   // turn on
        if(!TX8(0x20+SubAddr,0x02,0x09)) // turn on
        {
            sprintf(debug, "set high: Error in ADC`s setting ---> NO ON\n");
            print_debug(debug);
            need   = 0;
            hvwork[sipm_addr] = 0;
            /// \todo iskluchit iz triggera hvtrig[ii] = 0;
        }
        else
        {
            print_debug((char*) " +> ON");
        }
    }
    fflush(stdout);
    sleep(10); // 10 sec sleep


    /// ----------- measure high ------------
    //measure_all_high();  // measure high in all work channels
    measure_high();  // measure high in channel


    /// --- search optimal high to provide current =  workcur by increasing high
    sprintf(debug, "\n--------------------------------------------------\n!!!CHANGE high");
    print_debug(debug);

    // --- change high voltage ----
    for (high = highmin; high <= highmax; high++) // 14.02.12
    {
        // check need
        if( !need ) break;
        if( !flag ) break;

        /// -- set high --
        sprintf(debug, "\n---------------------------------\n++ HIGH = %3i", high);
        print_debug(debug);

        highset = high;
        if(SetChanHigh(sipm_addr, highset))  // set high
        {
            sprintf(debug,"\n  ER: ii=%2i  adr=%2i  sad=%i need=%i ===> ", sipm_addr, ChanAddr, SubAddr, need);
            print_debug(debug);
        }
        sleep(1); // 1 sec sleep after high voltage setting

        /// -- measure current --
        flag = 0;
        SetChannelAddr(VIP_ADDR);
        SetSubAddr(VIP_SUBADDR);
        cur = measure_current();
        pmt_cur[sipm_addr] = int(cur*CurKoef);

        sprintf(debug, "  >>>  cur = %6.3f mA", cur);  
        print_debug(debug); 

        // -- test current --
        if(cur > Workcur )
        {
            need  = 0;      // stop
            //highv[sipm_addr] = high;   //
            highv = high;   //
        }
        else flag++ ;

        //if(high > 254)       highv[sipm_addr] = 254;
        //if(high >= highmax)  highv[sipm_addr] = highmax;
        if(high > 254)       highv = 254;
        if(high >= highmax)  highv = highmax;

        sprintf(debug, "    ==> flag = %3i", flag);
        print_debug(debug);
    }

    // -----------------------
    // measure high in all work channels
    sprintf(debug, "\n--------------------------------------------------\n");
    print_debug(debug);
    measure_high();

    //if(highv[sipm_addr] == 0)    hvwork[sipm_addr] = 0;
    if(highv == 0)    hvwork[sipm_addr] = 0;

    // print results:
    sprintf(debug,"\n--------------------------------------------------\nResults:_");
    print_debug(debug);
    //sprintf(debug, "\ni = %2i--%2i--%1i, high = %3i", sipm_addr, sipm_addr/2 + 1, sipm_addr%2, highv[sipm_addr]);
    sprintf(debug, "\ni = %2i--%2i--%1i, high = %3i", sipm_addr, sipm_addr/2 + 1, sipm_addr%2, highv);
    print_debug(debug);

    if(!hvwork[sipm_addr])
    {
        sprintf(debug, " NoWork !!!!!!!! ");
        print_debug(debug);
    }
    print_currents(stdout);
    print_currents(dout);

    return 0;
}


/** -------------------------------------------------------
 *  \brief high scout
 *  \return 0 - OK\par 
 *          1 - high illumination\par
 *          2 - error in channel
 */
int SiPM::high_scout(unsigned char Addr, unsigned char SubAddr)
{
    float cur = 0, Maxcur = 0;

    sprintf(debug, "Reconnaissance: \n"); 
    print_debug(debug);
    if( (Addr > 55) || (SubAddr > 1))
    {
        sprintf(debug, "Bad channel numbers \n"); 
        print_debug(debug);
        return 2;
    }

    /// -- turn on HV on one channel
    sprintf(debug, "Turn on \n"); 
    print_debug(debug);

    SetChannelAddr( Addr);
    SetSubAddr(SubAddr);

    sprintf(debug, " adr=%2i  sad=%i\n", ChanAddr, SubAddr ); 
    print_debug(debug);

    if(!WriteDAC(SubAddr, 0))  // set high==0 kod
    {
        sprintf(debug, " Error in DAC`s transmit: NO set high==0kod "); 
        print_debug(debug);
        return 2;
    }
    if(!TX8(0x20+SubAddr,0x02,0x09)) // turn on
    {
        sprintf(debug, " Error in ADC`s setting: NO turn on "); 
        print_debug(debug);
        return 2;
    }

    sleep(10);
    measure_high();

    Maxcur = Work.maxcur;; // !!!2018  I_to_kod(Work.maxcur);
    sprintf(debug, "Maxcur  = %.3f mA\n", Maxcur);
    print_debug(debug);

    cur = measure_current();
    sprintf(debug, "I = %4.2f mkA\n", cur); 
    print_debug(debug);

    if(cur > Maxcur ) // current > MAXCUR
    {
        sprintf(debug, "\n===> ERROR:  Current > MAXCUR \n");
        print_debug(debug);
        return 1;
    }
    sprintf(debug, "\n===> SUCCES:  Current < MAXCUR \n");
    print_debug(debug);
    return 0;
}


/** -------------------------------------------------------
 *  \brief measure_high in one actual channel
 *  \return: 1 - Error in ADC`s reading\par
 *           0 - OK
 */
unsigned char SiPM::measure_high(void)
{
    read_vip_ADC(vip_out);
    return 0;
}


/** -------------------------------------------------------
 *  \brief read_vip_ADC \par
 *
 *  Read vip ADC and write to debug file and string message
 *  \return: 1 - Error in ADC`s reading\par
 *           0 - OK
 * \warning Up and current are class SiPM members
 */
unsigned int SiPM::read_vip_ADC(char *message)
{
    unsigned char  iii = 0;
    unsigned int   MData[4] = {1,1,1,1};
    unsigned int   kod[10];
    float I = 0.;
    float T = 0.;
    float Upositive = 0;
    float Unegative = 0;


    /// -- read ADC
    SetChannelAddr(VIP_ADDR); // adress of ADC of vip
    if(!ReadADCs(0,(unsigned int *)&MData))  // read ADC
    {
        sprintf(debug, "!Mosaic:read_vip_ADC: Error in ADC`s reading\n\n");
        print_debug(debug);
        return 1;
    }
    else
    {
        for(iii=0; iii<4; iii++)
        {
            MData[iii] &= 0x0FFF; // set 4 major bit to 0
            kod[iii] = MData[iii];
        }
    }

    sprintf(debug, "\nMosaic: measure_high: CH0[%3i] CH1[%3i] CH2[%3i] CH3[%3i]\n", 
                            kod[0], kod[1], kod[2], kod[3]);
    print_debug(debug);

    /// -- calculate Up, I, T
    Upositive = float(kod[1]) / 2000.;
    Unegative = kod_to_Up(kod[0]);
    Up  = -1 * (Unegative - Upositive);
    I   = kod_to_I(kod[1], kod[2]);
    T   = kod_to_T(kod[3]);
    current = I;

    // if CH0 - very small (high is off) - show Uemf = CH0/2000
    if(Up > 30.) //if(kod[0] < 400)
        Up = float(kod[0]) / 2000.;

    /// \todo change vip_out message
    sprintf(debug, "Mosaic: T = %5.1f oC  Up = %6.2f V  Ianode = %5.3f mA   ", T, Up, I);
    print_debug(debug);
    strcpy(message, debug);

    return 0;
}


/** -------------------------------------------------------
 *  \brief measure current in one channel
 *  \date november 2018
 *  \return: curr - float current
 */
float SiPM::measure_current(void)
{
    unsigned int MData[4] = {1,1,1,1};

    //------- read ADC -----
    if(!ReadADCs(SubAddr,(unsigned int *)&MData))  // read ADC
    {
        printf("Error in ADC`s reading\n");
        if(dout) fprintf(dout,"!measure_current: Error in ADC`s reading\n");
        return 0;
    }
    for(int iii = 0; iii < 4; iii++)
    {
        MData[iii] &= 0x0FFF; // set 4 major bit to 0
    }
    sprintf(debug, "\nMeasure current: %d %d %d %d", MData[0], MData[1], MData[2], MData[3]);
    print_debug(debug);
    return kod_to_I(MData[1], MData[2]);
}


/** -------------------------------------------------------
 *  \brief check current
 *  \return 0 - O'k
 *          1 - some channel was off
 *          2 - some channel was off and now no enough channels to trigger - go to OFF
 *  \warning current is a class SiPM member
 */
int SiPM::check_current()
{
    unsigned char flag = 0, n_work = 0;
    //float   cur = 0;
    float   Maxcur = 0; //, H_Maxcur = 0;  // max current in mA
    //unsigned char ii = sipm_addr;

    //Hvchan = Work.hvchan;
    //current = Work.maxcur;
    Maxcur  = Work.maxcur; // !!!2018 I_to_kod(Work.maxcur);
    sprintf( debug, "\n < CHECK CURRENT: Maxcur = %.3f mA", Maxcur);
    print_debug(debug);

    // -- measure current and off high if current > Maxcur
    if( !hvwork[sipm_addr] )
    {
        sprintf( debug, "\n\n!!!! TERRIBLE ERROR!!!  Do not work !!!!!!!\n     ");
        print_debug(debug);
    }

    // --- set channel address
    SetChannelAddr(VIP_ADDR);
    SetSubAddr(VIP_SUBADDR);

    // --- measure current and print
    current = measure_current();
    pmt_cur[sipm_addr] = int(current * CurKoef);  // pmt_cur[] is integer

    sprintf( debug, " current = %7.3f mA ", current);
    print_debug(debug);

    /// \todo out info for current
    if(ffmin) fprintf(ffmin, "\nSiPM current: %4.2f", current);
    if(ffmin) fflush(ffmin);
    
    // -- check current > Maxcur
    if( current > Maxcur  ) // if current > MAXCUR
    {
        //highv[ii]  = 0;  // set high = 0
        highv = 0;  // set high = 0
        hvwork[sipm_addr] = 0;  // off channel
        print_debug( (char*) " ===> ER:  Current > MAXCUR  => high_off\n");
        print_debug(debug);

        //if(!WriteDAC(SubAddr, highv[ii]))  // set high = 0
        if(!WriteDAC(SubAddr, highv))  // set high = 0
        {
            sprintf(debug, "ER: ii=%2i  adr=%2i  sad=%i ===> ", sipm_addr, ChanAddr, SubAddr);
            print_debug(debug);
            print_debug( (char*) "Error in DAC`s transmittion\n");
            print_debug(debug);
            sleep(1); //1 sec
        }
        flag++; // number of channels to OFF
    }
    else n_work ++; // number of ON channels

    sprintf(debug, "CHECK CURRENT />");
    print_debug(debug);

    /// -- if no channels to off - return 0=0k
    if( !flag ) return 0;

    sprintf(debug, "CHECK CURRENT: %2i channels to trigger\n", n_work);
    print_debug(debug);

    if( (n_work >= Work.master) || ( n_work >= Work.gmaster))
        return 1; // there are channels to run

    // --------------------
    // if no enough channels to trigger
    sprintf(debug, "CHECK CURRENT: NO enough channels to trigger!! Turn_off!!!\n");
    print_debug(debug);
    turn_off();

    return 2; // no enough channels to trigger
}


/** -------------------------------------------------------
 *  \brief print currents to file *ff
 *  \param ff file to write currents
 *  \return 0 - OK
 *          1 - error in file ff
 */
int SiPM::print_currents(FILE *ff)
{
    float  tok = pmt_cur[sipm_addr];

    if(ff == NULL)
    {
        print_debug((char*)"\n\nError in open file to write currents!!!\n");
        return 1;
    }

    fprintf(ff,"\nCurrent:  %7.3f mA", float(tok)/CurKoef);

    return 0;
}


/** -------------------------------------------------------
 *  \brief print currents to binary file
 */
int SiPM::print_currents_to_binary(FILE *ff)
{
    int  ii = 0;

    fprintf(ff,"i");
    //ii = VIP_ADDR * 2 + VIP_SUBADDR;
    /// \todo print only one currenr to binary file
    for(ii = 0; ii < HVDIM; ii++)
    {
        Conv.tInt = pmt_cur[ii];
        fprintf(ff,"%c%c%c",  Conv.tChar[2], Conv.tChar[1], Conv.tChar[0]);
    }
    fflush(ff);
    return 0;
}


/** -------------------------------------------------------
*  \brief turn off vip of SiPM
*/
void SiPM::turn_off(void)
{
    unsigned char ii = VIP_ADDR * 2 + VIP_SUBADDR;

    print_debug( (char*) "\n<VIP_OFF:\n");
    SetChannelAddr(int(ii/2));
    SetSubAddr(ii%2);

    sprintf(debug, "\n ii=%2i--%2i--%i", ii, ChanAddr, SubAddr);
    print_debug(debug);

    if(!WriteDAC(SubAddr, 0))  // set high = 0 kod
    {
        printf(" turn off: Error in DAC`s transmittion - set high \n");
        if(dout) fprintf(dout, "turn off: Error in DAC`s transmittion\n");
        printf("ER: ii=%2i  adr=%2i  sad=%i ===> ", ii, ChanAddr, SubAddr);
        if(dout) fprintf(dout,"ER: ii=%2i  adr=%2i  sad=%i ===> ",
                ii, ChanAddr, SubAddr);
    }
    //sleep(1); //1 sec
    usleep(100000); //0.1 sec

    if(!TX8(0x20+SubAddr,0x02,0x0C)) // turn off
    {
        print_debug( (char*) "turn off: Error in ADC`s setting ---> NO OFF");
    }
    usleep(100000); //0.1 sec
    On = 0;

    print_debug( (char*) "\nVIP_OFF>\n");

    sleep(1);
    check_turn_off();
}


/** -------------------------------------------------------
*  \brief check if vip are off
*/
void SiPM::check_turn_off(void)
{
    unsigned char ii = 0, iii = 0;
    int Hvchan = HVDIM;
    unsigned int   MData[4] = {0};
    unsigned int   kod[10] = {0};

    print_debug( (char*) "\n<VIP_OFF check:\n");
    //ii = VIP_ADDR * 2 + VIP_SUBADDR;
    for(ii = 0; ii < Hvchan; ii++)
    {
        //if(!hvwork[ii])  continue; if no working channel
        SetChannelAddr(int(ii/2));
        SetSubAddr(ii%2);

        printf("\n ii=%2i--%2i--%i", ii, ChanAddr, SubAddr);
        if(dout) fprintf(dout, "\n ii=%2i--%2i--%i", ii, ChanAddr, SubAddr);

        if(!ReadADCs(SubAddr,(unsigned int *)&MData))  // read ADC
        {
            sprintf(debug, " !check_turn_off: Error in ADC`s reading");
            print_debug(debug);
            continue;
        }

        for(iii=0; iii<4; iii++)
        {
            kod[iii] = MData[iii] & 0x0FFF; // set 4 major bit to 0
        }
        /*
        if(kod[2] == 4095)
        {
            if(dout)   fprintf(  dout, " OK");
            if(stdout) fprintf(stdout, " OK");
        }
        else
        */
        {
            sprintf(debug, " %d", kod[1]);
            print_debug(debug);
        }
        usleep(100000); //0.1 sec
    }

    print_debug( (char*)  "\nVIP_OFF_CHECK>\n");
}
