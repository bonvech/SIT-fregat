/**
 * \file trigger.cpp
 * \brief Класс trigger_board. Плата триггера.
 * 
 * Функции для работы триггера.
 * \date 2011.02.01 - changes to use new modification, 2019.08 - some 
 */
// 2007.01
// trigger software
// 


#ifndef _TRIGGER_BOARD
#define _TRIGGER_BOARD

#define DATA     0x0224
#define ADDR     0x0226
#define CFG      0x022E

#define UID      0x0010
#define TGBTIME  0x0020
#define TGBNUM   0x0022
#define TGBCTRL  0x0024
#define PPSBTIME 0x0028
#define PPSBNUM  0x002A
#define PPSBCTRL 0x002C
#define FIXTIME  0x0030
#define FIXCTRL  0x0034
#define TGCTRL   0x0044
#define CFGI     0x0048
#define CFGO     0x004A
#define TGLTHR   0x0054  // local  trigger
#define TGGTHR   0x0056  // global trigger
#define RSRVA    0x0058
#define RSRVB    0x005A
#define RSRVC    0x005C

// channel event status
#define EVTB01   0x0060
#define EVTB23   0x0062
#define EVTB45   0x0064
#define EVTB67   0x0066
#define EVTB89   0x0068
#define EVTBAB   0x006A
#define EVTBCD   0x006C

// channel permit status
#define EVTENB01 0x0070
#define EVTENB23 0x0072
#define EVTENB45 0x0074
#define EVTENB67 0x0076
#define EVTENB89 0x0078
#define EVTENBAB 0x007A
#define EVTENBCD 0x007C

// control bits
#define POP     6
#define CLR     5
#define CLRRPTR 4
#define CLROVRF 3
#define NEMPTY  4
#define OVRF    3
#define TGDIS   1
#define TGEN    1
#define TGSIM  0
#define TGINEN 5
#define TGLEDTGL 9
#define TGLEDSIM 8
#define TGOUTTGS 6
#define TGOUTTGF 5
#define TGCHTGS  2
#define TGCHTGF  1

enum {TG, PPS, FIX};

class trigger_board
{
public:

    ssize_t        BIN_size; // size of fpga*.bin file
    unsigned int   BaseAddr; //, CurAddr ;
    unsigned short TriggerOnOff[7];  // to store EVTENB data
    char debug[255];

    // costructor
    trigger_board()
    {
        unsigned char ii = 0;

        BaseAddr = 0x220;

        BIN_size  = 0;
        for(ii = 0; ii <7; ii++)
        {
            TriggerOnOff[ii] = 0;
        }
#ifndef NO_BOOT
        if(boot_trigger_board())
        {
            sprintf(debug, "\n\n TRIGGER boot ERROR!!!!!\n");
            print_debug(debug);
            if(dout) fflush(dout);
            while(1)
            {
                /// if error in trigger booting
                /// \todo сделать корректное завершение работы всей программы при невозможности загрузить плату триггера
                ;
            }            
        }
        
        //status();
        trigger_init();
        trigger_extra_init();
        status();
#endif
    }

    //---------------------------------------------------------
    //---------------------------------------------------------
public:

    unsigned short int ReadData()
    {
        unsigned short int Data;
        //outw(Addr,ADDR);
        Data = inw(DATA);
        return Data;
    }

    unsigned short int ReadAddr()
    {
        unsigned short int Data;
        //outw(Addr,ADDR);
        Data = inw(ADDR);
        return Data;
    }

    int WriteData( unsigned short int Data)
    {
        unsigned short int res = 0;

        outw(Data, DATA);
        return res;
    }

    int SetAddr( unsigned short int Addr)
    {
        //unsigned short int res = 0;

        outw(Addr, ADDR);
        return 0;
    }

private:
//////////////////////////////////////
    unsigned char SetRegOnOff(unsigned short int Addr, unsigned char Reg, unsigned char OnOf)
    {
        unsigned short int Da = 0;

        SetAddr(Addr);
        Da = ReadData();
        if(OnOf)  Da = set_bit_1(Da, Reg);
        else      Da = set_bit_0(Da, Reg);
        WriteData(Da);
        //printf("SetRg: Ad=%x Da=%i Rg=%i OnOf=%i\n", Addr, Da, Reg, OnOf);
        if(dout) fprintf(dout, "SetRg: Ad=%x Da=%i Rg=%i Onf=%i\n", Addr, Da, Reg, OnOf);
        return 0;
    }

    unsigned char CheckReg(unsigned short int Addr, unsigned char Reg)
    {
        unsigned short int Da = 0;
        unsigned char bit = 0;

        SetAddr(Addr);
        Da = ReadData();
        bit = get_bit(Da, Reg);
        sprintf(debug, "\nCheckReg: Addr=%x Da=%i Reg=%i bit = %i\n", Addr, Da, Reg, bit);
        print_debug(debug);

        return bit;
    }

public:
 // -------------------------------------------------------
 // ///////////////////////////////////////////////////////
/* unsigned char set_threshould(unsigned short int Thre)
 {
     //if((Thre !=1 ) || (Thre != 3) || (Thre != 7) )
     if(!((Thre == 1 ) || (Thre == 3) || (Thre == 7)) )
     {
         printf("\nError: set_threshold: trigger TGTHR is out of possible values!!\n");
         if(dout) fprintf(dout, "\nError: set_threshold: trigger TGTHR is out of possible values!!\n");
         return 1; // error
     }
     SetAddr(TGTHR);
     WriteData(Thre);
     sprintf(debug, "\nSset_threshold: trigger TGTHR is set= %i\n", Thre);
     print_debug(debug);
     return 0;
 }*/

    // ///////////////////////////////////////////////////////
    /// Set local trigger LN
    unsigned char set_local_threshould(unsigned short int Thre)
    {
        //if((Thre !=1 ) || (Thre != 3) || (Thre != 7) )
        if(!((Thre == 0 ) ||(Thre == 1 ) || (Thre == 2) || (Thre == 3) || (Thre == 7)) )
        {
            sprintf(debug, "\nError: set_threshold: trigger TGLTHR is out of possible values!!\n");
            print_debug(debug);
            return 1; // error
        }
        SetAddr(TGLTHR);
        WriteData(Thre);
        sprintf(debug,"Sset_threshold: trigger TGLTHR is set= %i\n", Thre);
        print_debug(debug);
        return 0;
    }
    
    // ///////////////////////////////////////////////////////
    /// Set local trigger GN
    unsigned char set_global_threshould(unsigned short int Thre)
    {
        //if((Thre !=1 ) || (Thre != 3) || (Thre != 7) )
        //if(!((Thre == 0 ) ||(Thre == 1 ) || (Thre == 3) || (Thre == 7)) )
        if(Thre > 20 ) 
        {
            sprintf(debug, "\nError: set_threshold: trigger TGGTHR is out of possible values > 20 !!\n");
            print_debug(debug);
            return 1; // error
        }
        SetAddr(TGGTHR);
        WriteData(Thre);
        sprintf(debug,"Sset_threshold: trigger TGGTHR is set= %i\n", Thre);
        print_debug(debug);
        return 0;
    }
    
    // ///////////////////////////////////////////////////////
    /// \brief OnOff all channels
    void OnOff_all_channels(unsigned short int OnOff)
    {
        unsigned int i = 0;

        for(i=0; i<=6; i++)
        {
            SetAddr(EVTENB01 + (2*i));
            if(OnOff)  WriteData(65535);
            else       WriteData(0);
        }
    }

    /// set TriggerOnOff[7] to trigger board
    void set_trigger(void)
    {
        unsigned int i = 0;

        for(i = 0; i <= 6; i++)
        {
            SetAddr(EVTENB01 + (2*i));
            WriteData(TriggerOnOff[i]);
        }
    }

    /** ----------------------------------------------------------
     * \brief To add or exclude channel from TriggerOnOff\par
     * 
     * \param ChanNum channel number counting from 0
     * \param OnOff switch OnOff channel to/from trigger
     * \return 0
     */     
    int addOnOff_channel(unsigned char ChanNum, unsigned char OnOff)
    {
        unsigned char  addr, bit;
        unsigned short int trig = 0;

        addr = ChanNum;
        bit  = ChanNum;

        addr >>= 4;   // devide by 16
        bit  <<= 4;   // remain of deviding by 16
        bit  >>= 4;  

        trig = TriggerOnOff[addr];

        if(OnOff) trig = set_bit_1(trig, bit);
        else      trig = set_bit_0(trig, bit);

        TriggerOnOff[addr] = trig;

        return 0;
    }


    /** -------------------------------------------------------
    *  \brief fill TriggerOnOff[7]
    *
    *  Function fills TriggerOnOff[7] (array to write to EVENTVB) according to Work.TriggerOnOff[112]
    */    
    int fill_TriggerOnOff(unsigned short *trigger)
    {
        int   ii = 0, j = 0, bit = 0;
        unsigned short word = 0, drow = 0; //, bit = 0;

        for(ii = 0, word = 0, j = 0; ii < 112; ii++)
        {
            if(trigger[ii])
                word += 1;

            sprintf(debug, "%1i", trigger[ii]);
            print_debug(debug);
            if(!((ii+1)%16))
            {
                TriggerOnOff[j] = word;
                j++;
                word = 0;
                sprintf(debug, "\n");
                print_debug(debug);
            }
            else
            {
                word <<= 1;
            }
        }

        for(ii = 0; ii < 7; ii++)
        {
            word = TriggerOnOff[ii];
            drow = 0;
            for(j = 0; j < 16; j++)
            {
                bit = get_bit(word, j);
                if(bit) drow = set_bit_1(drow, 15-j);
                else    drow = set_bit_0(drow, 15-j);
            }
            TriggerOnOff[ii] = drow;
        }

        sprintf(debug, "TriggerOnOff:\n");
        print_debug(debug);
        for(j = 0; j < 7; j++)
        {
            print_binary1(TriggerOnOff[j]);
            sprintf(debug, "\n");
            print_debug(debug);
        }

        return 0;
    }

    // ------------------------------------------------------
    /////////////////////////////////////////////////////////
    unsigned char clear_all_counters()
    {
        SetRegOnOff(TGBCTRL,  CLR,     1);
        SetRegOnOff(TGBCTRL,  CLRRPTR, 1);
        SetRegOnOff(TGBCTRL,  CLROVRF, 1);
        SetRegOnOff(PPSBCTRL, CLR,     1);
        SetRegOnOff(PPSBCTRL, CLRRPTR, 1);
        SetRegOnOff(PPSBCTRL, CLROVRF, 1);

        return 0;
    }

    // ------------------------------------
    void trigger_prohibit(void)
    {
        SetRegOnOff(TGCTRL, TGDIS,  1);
    }

    // ------------------------------------
    void trigger_permit(void)
    {
        SetRegOnOff(CFGI,   TGEN,   1);  // permit trigger
        SetRegOnOff(TGCTRL, TGDIS,  0);  // no prohibit trigger
        SetRegOnOff(TGCTRL, TGSIM,  0);  //
#ifdef PIEDESTAL
        trigger_prohibit();
#endif
    }


    // ------------------------------------
    void trigger_sim(void)
    {
        SetRegOnOff(TGCTRL, TGDIS,  1);
        SetRegOnOff(TGCTRL, TGSIM,  1);  // simulate trigger
        SetRegOnOff(TGCTRL, TGSIM,  0);  // 1 - 0 in TGSIM trigger
    }

    // ------------------------------------
    short trigger_init()
    {
        // set TGTHR threshould 1,3 or 7
        set_local_threshould(1);
        set_global_threshould(0);
        
        // prohibit run to all channels
        OnOff_all_channels(0);

        // clear all counters = 0
        clear_all_counters();

        return 0;
    }

    // ------------------------------------
    void trigger_extra_init()
    {
        // permit exterior trigger
        SetRegOnOff(CFGI,   TGINEN,   0);
        // LED:
        SetRegOnOff(CFGO,   TGLEDTGL, 1);
        //SetRegOnOff(CFGO,   TGLEDSIM, 1);
        // OUT:
        //SetRegOnOff(CFGO,   TGOUTTGS, 1);
        SetRegOnOff(CFGO,   TGOUTTGF, 1);
        // CH:
        //SetRegOnOff(CFGO,   TGCHTGS,  1);
        SetRegOnOff(CFGO,   TGCHTGF,  1);
        // SIM trigger:
        SetRegOnOff(TGCTRL, TGSIM,    0);
    }

    
    /** -------------------------------------------------------
     * \brief  check buffer is empty
     * \return 0 - is empty\par
     *         1 - is full
     */ 
    unsigned char buffer_is_not_empty(unsigned short int Kind)
    {
        //sprintf(debug, "NotEmpty:");
        //print_debug(debug);
        //return CheckReg(TGBCTRL, NEMPTY);
        if(Kind == TG)  SetAddr(TGBCTRL);
        if(Kind == PPS) SetAddr(PPSBCTRL);
        return (unsigned char) inw(DATA);
    }

    
    /** -------------------------------------------------------
     * \brief check buffer overflowing
     * \return 0 - no overflow\par
     *         1 - there is overflow
     * */
    unsigned char buffer_is_overflow(unsigned short int Kind)
    {
        if(Kind == TG)   return CheckReg(TGBCTRL,  OVRF);
        if(Kind == PPS)  return CheckReg(PPSBCTRL, OVRF);
        if(dout) fprintf(dout, "\nError in beffer_is_overflow\n");
        return 0;
    }


    unsigned char clear_buffer_overflow(unsigned short int Kind)
    {
        if(Kind == TG)
        {
            SetRegOnOff(TGBCTRL,  CLR,     1);
            SetRegOnOff(TGBCTRL,  CLRRPTR, 1);
            SetRegOnOff(TGBCTRL,  CLROVRF, 1);
        }
        else if(Kind == PPS)
        {
            SetRegOnOff(PPSBCTRL, CLR,     1);
            SetRegOnOff(PPSBCTRL, CLRRPTR, 1);
            SetRegOnOff(PPSBCTRL, CLROVRF, 1);
        }

        return 0;
    }

    unsigned char clear_buffer(unsigned short int Kind)
    {
        if(Kind == TG)  SetRegOnOff(TGBCTRL,  CLR,  1);
        if(Kind == PPS) SetRegOnOff(PPSBCTRL, CLR,  1);
        return 0;
    }

    // returns number of events in trigger buffer
    unsigned short int buffer_event_number(unsigned short int Kind)
    {
        unsigned short int Da = 0;

        if(Kind == TG)  SetAddr(TGBNUM);
        if(Kind == PPS) SetAddr(PPSBNUM);
        //SetAddr(TGBNUM);
        Da = ReadData();
        return Da;
    }

private:
//     unsigned int get_TIME(unsigned short int TimeAddr)
//     {
//          unsigned short int ttime = 0;
//          unsigned int   trigtime = 0;
// 
//          SetAddr(TimeAddr);
// 
//          ttime =   ReadData();
//          Conv.tInt = ttime;
//          if(fout) fprintf(fout, "%c%c", Conv.tChar[1], Conv.tChar[0]);
//          trigtime  = ttime;
//          trigtime  <<= 16;
//          if(dout) fprintf(dout, "1:%i ",ttime);
// 
//          ttime =   ReadData();
//          Conv.tInt = ttime;
//          if(fout) fprintf(fout, "%c%c", Conv.tChar[1], Conv.tChar[0]);
//          trigtime  += ttime;
//          if(dout) fprintf(dout, "2:%i ",ttime);
// 
//          return trigtime;
//     }

public:

    unsigned int tg_read_time(void)
    {
         unsigned short int ttime = 0;
         unsigned int   trigtime = 0;

         SetAddr(TGBTIME);

         ttime =   ReadData();
         //Conv.tInt = ttime;
         //if(fout) fprintf(fout, "%c%c", Conv.tChar[1], Conv.tChar[0]);
         trigtime  = ttime;
         trigtime  <<= 16;
         //(dout) fprintf(dout, "tg:%i ",ttime);

         ttime =   ReadData();
         //Conv.tInt = ttime;
         //if(fout) fprintf(fout, "%c%c", Conv.tChar[1], Conv.tChar[0]);
         //if(dout) fprintf(dout, "_2:%i ",ttime);
         trigtime  += ttime;
         if(dout) fprintf(dout, "trigtime: %i ",trigtime);
         
         SetRegOnOff(TGBCTRL,  POP,     1);
         SetRegOnOff(TGBCTRL,  POP,     0);

         return trigtime;
    }
//     unsigned char get_times(void)
//     {
//         unsigned int ttim = 0;
// 
//         ttim = get_TIME(TGBTIME);
//         printf( "\n TGBTIME = %u ", ttim);
//         if(dout) fprintf(dout,"\n TGBTIME = %u ", ttim);
//         print_binary1(ttim);
//         ttim = get_TIME(PPSBTIME);
//         printf( " PPSBTIME = %u ", ttim);
//         if(dout) fprintf(dout," PPSBTIME = %u ", ttim);
//         ttim = get_TIME(FIXTIME);
//         printf( " FIXTIME = %u\n", ttim);
//         if(dout) fprintf(dout," FIXTIME = %u\n", ttim);
// 
//          return 0;
//     }

    unsigned char status (void)
    {
        unsigned int addr = 0;
        unsigned short ii = 0;
        //printf("\n __Trigger status:___");
        if (dout) fprintf(dout, "\n ___Trigger status:___");
        for(addr=0x0020; addr <=0x007E; addr = addr+0x0002)
        {
            if((addr == TGBTIME) || (addr == PPSBTIME) || (addr == FIXTIME))
                continue;
            SetAddr(addr);
            ii = ReadData();
            //printf("\n ad= %xh  ", addr);
            if(dout) fprintf(dout, "\n ad= %xh  ", addr);
            print_binary_to_dbgfile(ii);
        }
        printf("\n");
        if(dout) fprintf(dout, "\n");

        return 0;
    }

    void pop_buffer(unsigned short int Kind)
    {
        int addr = 0;

        if(Kind == TG)  addr =  TGBCTRL;
        if(Kind == PPS) addr = PPSBCTRL;
        SetRegOnOff(addr,  POP,     1);
        SetRegOnOff(addr,  POP,     0);
    }

    // read PPS signal every second
    // return:  (unsigned int) pps time
    // errors:   0 - no pps   (-1) - pps overfull
    unsigned int pps_read_time(void)
    {
        unsigned short int ttime = 0, trigt = 0, inb = 0;
        unsigned int   trigtime = 0;

        // read NEMPTY buffer
        SetAddr(PPSBCTRL);
        inb = inw(DATA);
        //printf("NEMPTY: %xh\n", inb);

        if(inb == 0)
        {
            //printf("No PPS\n");
            return 0;
        }
        // if PPS is overfull
        if(inb == 0x18)
        {
            for(inb = 1; inb <= 5; inb ++)
            {
                SetRegOnOff(PPSBCTRL,  POP,     1);
                SetRegOnOff(PPSBCTRL,  POP,     0);
            }
            return 0;
        }

        // read number of PPS in buffer
        SetAddr(PPSBNUM);
        inb = inw(DATA);

        //if > 1- pop extra PPS
        //printf("PPS: %i\n", inb);
        while(inb > 1)
        {
            SetRegOnOff(PPSBCTRL,  POP,     1);
            SetRegOnOff(PPSBCTRL,  POP,     0);
            inb -= 1;
        }

        // --- read last PPS time
        SetAddr(PPSBTIME);
        ttime =   ReadData();
        Conv.tInt = ttime;
        if(fout) fprintf(fout, "p%c%c", Conv.tChar[1], Conv.tChar[0]);
        if(dout) fprintf(dout, "pps: _1:%xh ",ttime);
        trigt  = ttime;

        ttime =   ReadData();
        Conv.tInt = ttime;
        if(fout) fprintf(fout, "%c%c", Conv.tChar[1], Conv.tChar[0]);
        if(dout) fprintf(dout, "_2:%xh \n",ttime);
        trigtime  =  ttime;
        trigtime  <<= 16;
        trigtime  += trigt;

        SetRegOnOff(PPSBCTRL,  POP,     1);
        SetRegOnOff(PPSBCTRL,  POP,     0);

        //fprintf(stdout, "----------------------------------> PPS: %xh\n", trigtime);
        return trigtime;

    }

// ////////////////////////////////////////////////////////

private:
    // ---------------------------------------------------------
    // ---------------------------------------------------------
    unsigned char  boot_trigger_board(void)
    {
        char* buff = {0};

        buff = read_bin_file(buff);
        if(!buff)
        {
            printf("\nSorry.. trigger buff file cannot be read...\n");
            if(dout) fprintf(dout, "\nSorry.. TRIGGER buff file cannot be read...\n");
            return 1; // error
        }
        printf("\nBoot trigger... \n");
        if(dout) fprintf(dout, "\nBoot trigger... \n");

        if(boot_fadc(BaseAddr, buff))
        {
            printf("\t Trigger board is boot \n");
            if(dout) fprintf(dout, "\t Trigger board is boot \n");
        }
        else
        {
            printf("\n\n\n\n PANIC!! ERROR!!! Trigger board is NOT boot !!!!!!!!!!!");
            if(dout) fprintf(dout, "\n\n\n\n\n PANIC!! ERROR!!! Trigger board is NOT boot !!!!!!!! ");            
            return 2; // ERROR
            //halt();
        }

        free(buff);
        return 0; // OK
    }

    // ---------------------------------------------------------
    // return 0 if error
    // ---------------------------------------------------------
    char* read_bin_file(char* buffer)
    {
        FILE *f0;
        int kk;

        if((f0 = fopen("trig_fpga.bin","rb")) == NULL)
        {
            printf("\nError: trig_fpga.bin is  not open!!!\n");
            if(dout) fprintf(dout, "\nError: trig_fpga.bin is  not open!!!\n");
            return 0;
        }

        if( (kk = fseek(f0, 0, SEEK_END)) )
        {
            printf("Error: in fseek");
            if(dout) fprintf(dout,"Error: in fseek");
            fclose(f0);
            return 0;
        }
        BIN_size = ftell(f0);
        //printf("f0_size=%d\n", f0_size);
        if(BIN_size == -1L)
        {
            printf("Error: in fteel");
            if(dout) fprintf(dout, "Error: in fteel");
            fclose(f0);
            return 0;
        }
        if( (kk = fseek(f0, 0, SEEK_SET)) )
        {
            printf("Error: in fseek");
            if(dout) fprintf(dout, "Error: in fseek");
            fclose(f0);
            return 0;
        }

        if((buffer = (char*)calloc(BIN_size, sizeof(char))) == NULL)
        {
            printf("Error: in calloc !!\n");
            if(dout) fprintf(dout,"Error: in calloc !!\n");
            fclose(f0);
            return 0;
        }

        kk = fread(buffer, sizeof(char), BIN_size, f0);
        if(ferror(f0))
        {
            printf("Error: ferror in fread \n");
            if(dout) fprintf(dout, "Error: ferror in fread \n");
            fclose(f0);
            free(buffer);
            return 0;
        }

        fclose(f0);
        return buffer;
    }


    //---------------------------------------------------------
    //---------------------------------------------------------
    int boot_fadc(unsigned int BasAddr, char* buffer)
    {
        int Fadc_io_contr = BasAddr + 0xF;
        short int kk = 1;
        ssize_t  num = 0;
        unsigned char ii = 0;
        unsigned char b = 0, b1 = 0, rez = 0 ;

        outb(64, Fadc_io_contr-1);
        outb(8,  Fadc_io_contr);
        outb(9,  Fadc_io_contr);

        do
        {
            b1=((inb(Fadc_io_contr)) & 3);
        }while( (b1!=0) );

        outb(8, Fadc_io_contr);
        do
        {
            b1=((inb(Fadc_io_contr)) & 3);
        } while( (b1==0) );

        for(num = 0; num < BIN_size; num++)
        {
            b = buffer[num];
            kk  = 1;
            rez = 0;
            for(ii=0; ii < 8; ii++)
            {
                rez = b & kk;
                if ( rez )
                {
                    outb(10, Fadc_io_contr);
                    outb(14, Fadc_io_contr);
                }
                else
                {
                    outb( 8, Fadc_io_contr);
                    outb(12, Fadc_io_contr);
                };
                kk *= 2;
            }
        }

        for(ii=0; ii<10; ii++)
        {
            outb(12, Fadc_io_contr);
            outb( 8, Fadc_io_contr);
        }

        ii = (inb(Fadc_io_contr)) & 3;
        printf("INIT = %i, DONE = %i\n", get_bit(ii, 0), get_bit(ii,1));
        if(dout) fprintf(dout, "INIT = %i, DONE = %i ", get_bit(ii, 0), get_bit(ii,1));
        outb(0, Fadc_io_contr);

        if (ii != 2)  return 0;

        return 1;
    }

    int print_binary_to_dbgfile(int number)
    {
        unsigned short i = 0; //, p = 0;
        
        //printf(" b1: " );
        for(i=0; i<=15; i++)
        {
            //p = 
            get_bit(number,i);
            //printf("%1i",get_bit(number,i));
            if(dout) fprintf(dout, "%1i",get_bit(number,i));
        }
        
        return 0;
    }

};  // end of class
#endif
