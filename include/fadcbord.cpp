/**
 * \file fadcbord.cpp
 * \brief Класс fadc_board
 *
 * Описание класса fadc_board
 */
#ifndef _FADC_BOARD
#define _FADC_BOARD     ///< to prevent multiple including

#define DELAY 100       ///< delay between fadc output

#include "fadc.cpp"

// extern functions and variables
void timestamp_to_file(FILE *ff);
void print_debug(char * message);
void print_time_ms(FILE *ff);
char * sprint_freq(char text[], double freq);


extern input_parameters Work;
extern int TunkaNumber;

//-------------------------------------------------------------
//     ******************     MAIN BLOCK   ******************
//-------------------------------------------------------------
/// Class for FADC board
class fadc_board : public fadc
{
    char debug[250];          ///< String for debug information
    /// address of counters
    unsigned short count_addr[9];
    int THR[BOARD+1][9];           ///< array to hold threshold levels
    unsigned int   SerNum[16];     ///< Serial numbers of FADC boards
    ssize_t        BIN_size;   ///< Size of file with firmware
    unsigned short RG1put;     ///< RG1put bytes to write to RG1 register of fadc board
    unsigned short Buf1;     ///< Trigger shift size
public:
    unsigned short Buf2;     ///< Buffer size
    unsigned int   AddrOn[16];     ///< Adresses of FADC boards
    /// array to write event from buffer
    short int event_data[66000];   // BOARD * CHANMAX * 512  // 8 * 16 * 512
    int last_bin;              ///< index of last number in event_data

    /// constructor
    fadc_board() 
    {
        BIN_size = 0;
        Buf1 = Work.buf1;
        Buf2 = Work.buf2;
        RG1put = 0;
        last_bin = 0;
        unsigned short count_addr_init[9] = {8, 0xA, 0xC, 0x1A, 0x1C, 0x2A, 0x2C, 0x3A, 0x3C};
        for(short i = 0; i < 9; i++)
            count_addr[i] = count_addr_init[i];

        debug[0] = 0;

        init();
    }


    //  Methods
private:
    int   init_fadc_boards(void);
    int   boot_fadc(unsigned int BasAddr, char* buffer);
    int   boot_fadc_boards(void);
    int   test_boards_are_boot(void);
    char* read_bin_file(char* buffer);
    unsigned char read_from_registers(void);
    unsigned char write_to_registers(void);
    unsigned char Read1ResReg(unsigned char AddrDev, unsigned char AddrReg, unsigned short *RegRes);
    float read_fadc_temp(unsigned int addr);
    float kod_2_fadc_temp(unsigned int kod);

public:
    int   init(void);
    int   boot(void);
    int   test_fadc_boards(void);
    float read1_average_fadc_temp(unsigned int addr);
    unsigned short turn_off_fadc_boards(void);

#include "levels.cpp"
#include "nograph.cpp"
#include "tunka_sync.cpp"

};  // end of class fadc_board



/** ---------------------------------------------------------
 * \brief   init all FADC boards
 * \return  kk  number of tested FADC boadrs
 *
 *  Initialization and test of FADC boards
 */
int fadc_board::init(void)
{
    int kk = 0;
    unsigned int ll = 0;
    unsigned int i = 0;
    char info[200] = "";

    BaseAddr = FADC_IO; // address of FADC

    sprintf(info, "\nFADC BaseAddr = %x\n", BaseAddr);
    print_debug(info);

    /// Init FADC boards
    kk = init_fadc_boards();
    sprintf(info,"\n  Result:   %2i devices found on addreses:", kk);
    print_debug(info);

    /// Print board information: serial numbers and adresses
    for(i = 1; i<= AddrOn[0]; i++)
    {
        sprintf(info," N%2i on %4xh; ", SerNum[i], AddrOn[i]);
        print_debug(info);
    }

    /// Test if FADC boards are boot
    ll = test_boards_are_boot();
    sprintf(info,"\n  Result:   %i board(s) are booted\n", ll);
    print_debug(info);

    // ---------------------------
    /// Test boards
    kk = test_fadc_boards();
    sprintf(info,"  Result:   %i board(s) are tested\n", kk);
    print_debug(info);

    /// Start boards
    start_fadc_boards();

    return kk;
}

/** ---------------------------------------------------------
 * \brief   init all FADC boards
 * \return  kk  number of tested FADC boadrs
 *
 *  Initialization and test of FADC boards
 */
int fadc_board::boot(void)
{
    int kk = 0;
    unsigned int ll = 0;
    unsigned int i = 0;
    char info[200] = "";


    /// Test if FADC boards are boot
    ll = test_boards_are_boot();
    sprintf(info,"\n  Result:   %i board(s) are booted\n\n", ll);
    print_debug(info);

    /// Boot boards
    if(ll != AddrOn[0])  // if tested != found --> boot boards
    {
        // ----  boot fadc boards -----------------------
        kk = boot_fadc_boards();
        sprintf(info,"\n  Result:   %i board(s) are booted\n\n", kk);
        print_debug(info);

        // ----- print fadc boards to file "./log/fadc_work.dat" ---
        if((fwork = fopen( WORK_FILE, "a+")) == NULL)
        {
            sprintf(info, "\n  Error: file \"%s\" is not open!\n", WORK_FILE);
            print_debug(info);
        }
        else
        {
            timestamp_to_file(fwork);
            fprintf(fwork, "FADC %i", AddrOn[0]);
            for(i = 1; i<= AddrOn[0]; i++)
            {
                fprintf(fwork,"  %4x ",  AddrOn[i]);
            }
            fprintf(fwork, "\n");
            fclose(fwork);
        }
    }

    // ---------------------------
    /// Test boards
    kk = test_fadc_boards();
    sprintf(info,"  Result:   %i board(s) are tested\n\n", kk);
    print_debug(info);

    /// Start boards
    start_fadc_boards();

    return kk;
}


/** ---------------------------------------------------------
    * \brief boot_fadc_boards
    * \return   kk -number of booted FADC boadrs
    * 
    *  Boot FADC boards
    */
int  fadc_board::boot_fadc_boards(void)
{
    short unsigned int i = 0, at = 0;
    char* buff = {0};
    int response = 0;

    buff = read_bin_file(buff);
    if(!buff)
    {
        printf("\nSorry.. buff file cannot be read...\n");
        if(dout) fprintf(dout, "\nSorry.. buff file cannot be read...\n");
        return 0;
    }
    for(i = 1; i<= AddrOn[0]; i++)
    {
        at = 0;
        printf("\nBoot... ");
        fflush(stdout);

        if(boot_fadc(AddrOn[i], buff)) //  1 -- OK
        {
            printf("\b\b\b\b\b\b\b\r %2i)N%2i on %4xh is boot; ", i, SerNum[i], AddrOn[i]);
            if(dout) fprintf(dout, "\n N%2i on %4xh is boot; ", SerNum[i], AddrOn[i]);
        }
        else //   0 -- error
        { 
            while(!boot_fadc(AddrOn[i], buff))
            {
                at ++;
                printf("\b\b\b\b\b.. boot %i ..", at);
                fflush(stdout); 
                if(at >= 10)
                {
                    printf("\n\n\n10 attempts to boot board N %2i failed!!!!\n to be or not to be????", i);
                    printf("input [1] - for stop programm, [2] - for new 10 attempts");
                    scanf("%i", &response);
                    if(response == 2)
                    {
                        at = 0;
                        continue;
                    }
                }
            }
            printf("\r%2i) N%2i on %4xh is boot; ", i, SerNum[i], AddrOn[i]);
            if(dout) fprintf(dout, "\n N%2i on %4xh is boot; ", SerNum[i], AddrOn[i]);
            fflush(dout);
        }
    }

    free(buff);
    return i-1;
}


/** ---------------------------------------------------------
    * \brief read_bin_file
    * \param buffer - 
    * \return  0 - error\n
    *          buffer - buffer of read FADC board bin configuration
    *
    *  Read FADC board firmware from "fadc_fpga.bin" file
    */
char* fadc_board::read_bin_file(char* buffer)
{
    FILE *f0;
    int kk;

    if((f0 = fopen("fadc_fpga.bin","rb")) == NULL)
    {
        printf("\nError: fadc_fpga.bin is  not open!!!\n");
        if(dout) fprintf(dout, "\nError: fadc_fpga.bin is  not open!!!\n");
        return 0;
    }

    if( (kk = fseek(f0, 0, SEEK_END)) )
    {
        printf("Error: in fseek");
        fclose(f0);
        return 0;
    }
    BIN_size = ftell(f0);
    //printf("f0_size=%d\n", f0_size);
    if(BIN_size == -1L)
    {
        printf("Error: in fteel");
        fclose(f0);
        return 0;
    }
    if( (kk = fseek(f0, 0, SEEK_SET)) )
    {
        printf("Error: in fseek");
        fclose(f0);
        return 0;
    }

    if((buffer = (char*)calloc(BIN_size, sizeof(char))) == NULL)
    //if(((void *)buffer = calloc(BIN_size, sizeof(char))) == NULL)
    {
        printf("Error: in calloc !!\n");
        fclose(f0);
        return 0;
    }

    kk = fread(buffer, sizeof(char), BIN_size, f0);
    //printf("fread: kk = %d\n", kk);
    if(ferror(f0))
    {
        printf("Error: ferror in fread \n");
        fclose(f0);
        free(buffer);
        return 0;
    }

    fclose(f0);
    return buffer;
}


/** ---------------------------------------------------------
    * \brief boot_fadc
    * \param BasAddr - FADC board base address
    * \param buffer  - firmware to boot
    * \return   1 -- OK\par
    *           0 -- Error in booting
    *
    * Boot FADC with address BasAddr
    */
int fadc_board::boot_fadc(unsigned int BasAddr, char* buffer)
{
    int Fadc_io_contr = BasAddr + 0xF;
    short int kk = 1;
    //short delay = 100;
    ssize_t  num = 0;
    unsigned char ii = 0, i = 0;
    unsigned char b = 0, b1 = 0, rez = 0 ;

    outb(64, Fadc_io_contr-1);
    usleep(1);
    outb(8, Fadc_io_contr);
    usleep(1);
    outb(9, Fadc_io_contr);
    usleep(1);

    do
    {
        b1=((inb(Fadc_io_contr)) & 3);
    }
    while( (b1!=0) );

    outb(8, Fadc_io_contr);

    do
    {
        b1=((inb(Fadc_io_contr)) & 3);
    }
    while( (b1==0) );

    printf("         "); 
    //printf("E-%i F-%i \n", inb(Fadc_io_contr-1), inb(Fadc_io_contr) );
    //printf("BIN_size = %i \n", BIN_size);
    for(num = 0; num < BIN_size; num++)
    {
        b = buffer[num];
        kk  = 1;
        rez = 0;
        if(! (num%10000) ) printf("\b\b\b\b\b\b\b%7zd", num); 
        fflush(stdout);
        for(ii=0; ii < 8; ii++)
        {
            rez = b & kk;
            if ( rez )
            {
                outb(10, Fadc_io_contr);
                for(i = 0; i < DELAY; i++);
                outb(14, Fadc_io_contr);
                for(i = 0; i < DELAY; i++);
            }
            else
            {
                outb( 8, Fadc_io_contr);
                for(i = 0; i < DELAY; i++);
                outb(12, Fadc_io_contr);
                for(i = 0; i < DELAY; i++);
                //printf("0");
            };
            kk *= 2;
        }
    }

    for(ii = 0; ii < 10; ii++)
    {
        outb(12, Fadc_io_contr);
        for(i = 0; i < DELAY; i++);
        outb( 8, Fadc_io_contr);
        for(i = 0; i < DELAY; i++);
    }

    ii = (inb(Fadc_io_contr)) & 3;  // get bit DONE
    outb(0, Fadc_io_contr);
    printf("\a");

    if (ii != 2)
        return 0; // error 
    return 1;  // OK
}


/** ---------------------------------------------------------
    * \brief init_fadc_boards
    * \return number of inituated fadc boards
    */
int  fadc_board::init_fadc_boards(void)
{
    unsigned int  Data[4];
    unsigned char AddrNum = 0;
    unsigned int  sernum = 0;//, ResData = 0;
    short int     ii = 0;

    for(ii = 0; ii < BOARD; ii++)
    {
        BaseAddr = FADC_IO + ii*0x400;
        if(!ReadADCs(0x00, (unsigned int*)&Data))
        {
            //printf("  Error in ADC reading!!!\n");
            //printf("  No device\n");
            continue;
        }

        printf(" i=%2i  Ad=%5xh  Ports: %3i %3i",
            ii, BaseAddr, inb(BaseAddr+0xE), inb(BaseAddr+0x01+0xE));
        if(dout) fprintf(dout," i=%2i  Ad=%5xh  Ports: %3i %3i",
            ii, BaseAddr, inb(BaseAddr+0xE), inb(BaseAddr+0x01+0xE));

        // test fadc_board
        // ON ADC
        if(!TX8(0x20, 0x02, 0x09 ))
        {
            printf("Error in ADC transfer!!!\n");
            if(dout) fprintf(dout, "Error in ADC transfer!!!\n");
        }
        else
        {
            printf("  __ADC ON__  ");
            if(dout) fprintf(dout, "  __ADC ON__  ");
        }

        printf(" Ports: %3i %3i", inb(BaseAddr+0xE), sernum=inb(BaseAddr+0x01+0xE));
        fprintf(dout," Ports: %3i %3i", inb(BaseAddr+0xE), sernum=inb(BaseAddr+0x01+0xE));
        sernum >>= 2;
        printf("   num = %3d\n", sernum);
        fprintf(dout,"   num = %3d\n", sernum);

        AddrNum++;
        AddrOn[AddrNum] = BaseAddr;
        SerNum[AddrNum] = sernum;
    }
    AddrOn[0] = AddrNum;
    BaseAddr  = AddrOn[1];

    return AddrNum;
}


/** ---------------------------------------------------------
    * \brief test_fadc_boards
    * \return number of good boards
    *
    * test_fadc_boards
    */
int fadc_board::test_fadc_boards(void)
{
    unsigned short i = 0, ii = 0;
    unsigned int   tmp = 0;
    unsigned int   Data[4];
    int  boot = 0;
    char mes[100] = {0}, message[200] = {0};;

    for(ii = 1; ii <= AddrOn[0]; ii++)
    {
        BaseAddr = AddrOn[ii];
        sprintf(message, "FADC%02i: ", ii);

        if(!ReadADCs(0x00, (unsigned int*)&Data))
        {
            if(stdout) fprintf(stdout, "%s  No device\n", message);
            if(  dout) fprintf(  dout, "%s  No device\n", message);
            continue;
        }

        boot++;  // number of good boards
        for(i = 0; i < 4; i++) // read ADC
        {
            tmp  = Data[i];
            tmp &= 0x0FFF;
            sprintf(mes, " CH%i[%4i]", i, tmp);
            strcat(message, mes);
            //printf("  CH%d=[%4d]  ", i, tmp); //Data[i]);
            //if(dout) fprintf(dout, "  CH%d=[%4d]  ", i, tmp); //Data[i]);
        }

        if(stdout) fprintf(stdout, "%s\n", message);
        if(  dout) fprintf(  dout, "%s\n", message);

        // read temperature
        read_fadc_temp(0x49);
        read_fadc_temp(0x4A);
    }
    BaseAddr = AddrOn[1];
    return boot; // number of good boards
}


/** ---------------------------------------------------------
    * \brief test_boards_are_boot
    * \return 0 -- are no boot\n
    *         1 -- are boot
    *
    * test_boards_are_boot
    */
int fadc_board::test_boards_are_boot(void)
{
    unsigned short ii = 0, ll = 0;
    int  boot = 0;
    int  Fadc_io_contr = 0xF;

    for(ii = 1; ii <= AddrOn[0]; ii++)
    {
        BaseAddr = AddrOn[ii];
        Fadc_io_contr = BaseAddr + 0xF;

        ll = (inb(Fadc_io_contr)) & 3;
        if (ll == 2)  // DONE bit OK
        {
            boot ++;
        }

        // read temperature
        read_fadc_temp(0x49);
        read_fadc_temp(0x4A);
    }
    BaseAddr = AddrOn[1];
    return boot;
}


/** ---------------------------------------------------------
    * \brief read_from_registers
    * \return 1
    *
    * read_from_registers
    */
unsigned char fadc_board::read_from_registers(void)
{
    unsigned char j=0, k=0;
    unsigned int  ResData = 0, Res[12];

    for(j = 0; j<12; j++)
    {
        if(!ReadReg(0x20, (j+4), &ResData))
        {
            printf("Error in %d RegRes reading\n", j);
        }
        ResData <<= 4;
        ResData >>= 4;
        Res[j] = ResData;
    }

    printf("\n");
    printf("                CH1        CH2        CH3        CH4\n");

    for(j = 0; j<=2; j++)
    {
        if(j==0) printf("DATAlow  ");
        if(j==1) printf("DATAhigh ");
        if(j==2) printf("Hysteres ");

        for(k=0; k<=3; k++)
        {
            printf("  R%2d[%4d]", j+4+k*3, Res[j+k*3]);
        }
        printf("\n");
    }

    return 1;
}


/** ---------------------------------------------------------
    * \brief write_to_registers
    * \return  1 - OK\n
    *          0 - errors
    *
    * write_to_registers
    */
unsigned char fadc_board::write_to_registers(void)
{
    unsigned int   regdata = 255, reg = 0;
    unsigned char  regnum = 6;

    printf("\nRegister number [4-15] ==>");
    scanf("%u", &reg);
    regnum = (unsigned char)reg;
    if(!regnum)
        {printf("NO write to NULL register is posible!\n");
        return 0;}
    printf("regnum=%u", regnum);
    printf("         Data to write ==>");
    scanf("%d", &regdata);

    printf("1");
    if(!TX16(0x20, regnum, regdata))
        {printf("Error in WriteReg (TX16) transmittion\n");
            return 0;}
    printf("2");

    return 1;
}


/** ---------------------------------------------------------
    * \brief Read1ResReg
    * \return  reading data
    *
    * Read1ResReg
    */
unsigned char fadc_board::Read1ResReg(unsigned char AddrDev, unsigned char AddrReg, unsigned short *RegRes)
{
    unsigned char tmpAddr = AddrReg;

    printf(" Addr=%d", tmpAddr);
    if(!TX8(AddrDev, 0x00, tmpAddr))
    {
        printf("Error in Read1ResReg!!\n");
        return 0;
    }
    return RX16(AddrDev, RegRes);
}


/** ---------------------------------------------------------
    * \brief read_fadc_temp
    * \param addr - FADC board address
    * \return  reading temperature\n
    *          -100 - if error
    *
    * Read temperature of one fadc board with addres addr
    */
float fadc_board::read_fadc_temp(unsigned int addr)
{
    unsigned short Res1 = 0;
    float Tem1 = 0.;

    if(!RX16(addr, &Res1))
    {
        if(stdout) fprintf(stdout, "Error in T RX16 reading!!!!\n");
        if(  dout) fprintf(  dout, "Error in T RX16 reading!!!!\n");
        return -100;
    }
    Tem1 = kod_2_fadc_temp(Res1);
    //printf("    T1 = %6.2f oC", Tem1);
    //if(dout) fprintf(dout, "    T1 = %6.2f oC", Tem1);
    return Tem1;
}


/** ---------------------------------------------------------
    * \brief read_fadc_temp
    * \param addr - FADC board address
    * \return  reading temperature\n
    *          -100 - if error
    *
    * Читает температуры плат FADC с адресом addr и возвращает среднее значение температуры.
    *
    */
/// \todo запись всех температур в лог-файл
float fadc_board::read1_average_fadc_temp(unsigned int addr)
{
    //unsigned int Res1 = 0, Sum = 0;
    float Tem1 = 0., Tem = 0.;
    unsigned short ii = 0, num = 0;

    for(ii = 1; ii <= AddrOn[0]; ii++)
    {
        BaseAddr = AddrOn[ii];
        //printf("BaseAddr = %xh", BaseAddr);
        //if(dout) fprintf(dout,"BaseAddr = %xh", BaseAddr);

        Tem = read_fadc_temp(addr);
        //printf("ii = %i, Tem = %.2f num = %i\n", ii, Tem, num);
        //if(dout) fprintf(dout, "ii = %i, Tem = %.2f num = %i\n", ii, Tem, num);

        if(Tem < -90)
        {
            if(stdout) fprintf(stdout,"Error Temp: BaseAddr = %xh, addr = %xh \n", BaseAddr, addr);
            if(  dout) fprintf(  dout,"Error Temp: BaseAddr = %xh, addr = %xh \n", BaseAddr, addr);
            continue;
        }
        Tem1 += Tem;
        num ++;
        //printf("ii = %i, Tem = %.2f num = %i\n", ii, Tem, num);
    }
    //printf("   => 1Tem1 = %6.2f oC num = %i <=\n", Tem1, num);
    //if(dout) fprintf(dout, "   => 1Tem1 = %6.2f oC num = %i  <=\n", Tem1, num);

    if(num)  Tem1 /= (float)num;
    else     return -100; // error

    //Tem1 = kod_2_fadc_temp(Sum);
    //printf("    1T1av = %6.2f oC", Tem1);
    //if(dout) fprintf(dout, "    1T1av = %6.2f oC", Tem1);
    return Tem1;
}


/** ---------------------------------------------------------
    * \brief kod_2_fadc_temp
    * \param kod - number to convert to temperature degrees
    * \return  temp - temperature in degrees
    *
    * Convert temperature kod to degrees centigrade
    */
float fadc_board::kod_2_fadc_temp(unsigned int kod)
{
    float temp = 0.;
    unsigned short skod = 0;

    skod = (unsigned short) kod;
    //printf("\n    T1 %i", skod);
    skod >>= 6;
    //printf("    T1 %i", skod);
    temp = (float)skod;
    temp /= 4.;

    return temp;
}


/** ---------------------------------------------------------
 * \brief turn_off_fadc_boards
 * \return  0
 *
 * turn_off_fadc_boards
 */
unsigned short fadc_board::turn_off_fadc_boards(void)
{
    unsigned short ii = 0;

    for(ii = 1; ii<= AddrOn[0]; ii++)
    {
        BaseAddr = AddrOn[ii];
        if(!TX8(0x20, 0x02, 0x0C)) // error
        {
            printf("Error in ADC`s setting\n");
            if(dout) fprintf(dout, "Error in ADC`s setting\n");
        }
        else
        {
            printf(" i=%2i  Ad=%5xh __ADC OFF__\n", ii, BaseAddr);
            if(dout) fprintf(dout," i=%2i  Ad=%5xh  __ADC OFF__\n",
                ii, BaseAddr);
        }
        // pause some time
        sleep(1); // 1sec
    }
    return 0;
}

#endif
