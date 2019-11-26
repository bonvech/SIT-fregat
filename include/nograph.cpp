/**
 * \file nograph.cpp
 * \brief Часть класса fadc_board. Работа с буфером FADC
 *
 * Функции, выполняемые во время измерений. Часть класса fadc_board
 * \date Modified 2019.10.31
 */


public:

/** ----------------------------------------------------------------
 * Reading events from buffer and write it to binary data file (fout)\n
 *
 * \param flag    write (0) or not (not 0) to file fkadf with name Event\n
 * \return 0
 */
unsigned int get_event(int flag)
{
    unsigned char a = 0;
    unsigned int i = 0, j = 0, dj = 0;
    short int fadc_data = 0;
    unsigned short int kadr_data = 0;

    fprintf(fout, "k");
    if(!flag)
    {
        //Conv.tInt = STROBE - STROBB; //Buf2; //BUF2;
        //fprintf(fkadr, "b%c%c",Conv.tChar[1], Conv.tChar[0]);
        fprintf(fkadr, "%d ", STROBE - STROBB);
        //Conv.tInt = CHANMAX;
        //fprintf(fkadr, "c%c%c",Conv.tChar[1], Conv.tChar[0]);
        fprintf(fkadr, "%d ", CHANMAX);
        fprintf(fkadr, "k\n");
    }
    read_3RG(); // read registers 3 times

    for(a = 1; a <= AddrOn[0]; a++)
    {
        BaseAddr = AddrOn[a];
        //fprintf(fkadr, "FADC = %d\n", a);
        for (i = 0; i < Buf2; i++)
        {
            for(j = 0; j < CHANMAX; j++)
            {
                dj = (2 * (int)(j % 4)) + 0x10 * (int)(j/4);
                //fadc_data = ( inw( BaseAddr+dj ) & 1023);  // read word
                fadc_data = ( inw( BaseAddr + dj ) );  // read word
                Conv.tInt = fadc_data;
                fprintf(fout, "%c%c", Conv.tChar[1], Conv.tChar[0]); // print to file

                // ----- print kadr to virtual event file
                if(!flag)
                {
                    //if(j%2)
                    //{
                        //if((i > STROBB) && (i <= STROBE)) 
                        //{
                            //fprintf(fkadr, "%c%c", Conv.tChar[1], Conv.tChar[0]); // print to file
                            kadr_data = fadc_data;
                            fprintf(fkadr, "%d ", kadr_data); // print to file
                        //}
                    //}
                }
#ifdef PIEDESTAL
                if(i < 6)
                {
                    fadc_data = fadc_data  & 1023;  // read word
                    printf( " %3i", fadc_data);
                    if(dout) fprintf(dout, " %3i", fadc_data);
                }
#endif
            }
            if(!flag)
                fprintf(fkadr, "\n");

#ifdef PIEDESTAL
            if(i < 6)
            {
                print_debug((char*)"\n");
            }
#endif
        }
        if(!flag)
            fprintf(fkadr, "\n");

#ifdef PIEDESTAL
        print_debug((char*)"==========\n");
#endif
    }

#ifdef PIEDESTAL
    printf( "\n");
#endif
    //fflush(fout);

    if(!flag) 
    {
        fprintf(fkadr, "eof\n");
        fflush(fkadr);
    }
    return 0;
}


/** --------------------------------------------------------------
 * Read FADC buffer flags
 * \return 1 - there is data in the buffer\par 
 *         0 - no data
 */
unsigned short read_buffer_flag(void)
{
    unsigned int i=0, flags = 0;
    unsigned short reg = 0;

    for(i = 1; i <= AddrOn[0]; i++)
    {
        BaseAddr = AddrOn[i];
        //if ( !(inw(BaseAddr + 8) & 1) ) //if there is data in buffer
            //return 1;
        reg = inw(BaseAddr + 8)   ;
        if ( !(reg & 1) ) //if there is data in buffer
        {
            flags ++;
            //printf("Addr = %4xh : %4xh : 1\n", BaseAddr, reg);
        } 
        //else
        //{
        //    printf("Addr = %4xh : %4xh :  0\n", BaseAddr, reg);
        //}
    }
    return flags;
    //return 0;
}


/** --------------------------------------------------------------
 * Start FADC boards
 */
unsigned int start_fadc_boards()
{
    unsigned char i = 0;

    for(i = 1; i <= AddrOn[0]; i++)
    {
        BaseAddr = AddrOn[i];
        set_RGs( BUF1, BaseAddr+2);   //ADC1(5,9,13)/DP
        set_RGs( BUF1, BaseAddr+6);   //ADC3(7,11,15)/DP
        set_RGs( TRG2, BaseAddr+8);   //RG0(3,6,9)
        set_RGs( Buf2, BaseAddr+12);  //RG2(5,8,11)   // 

        set_RGs( 2000, BaseAddr);     //ADC0(4,8,12)/THR
        set_RGs( 2000, BaseAddr+4);   //ADC2(6,10,14)/THR

        // reset  channels
        //set_RGs(    0, BaseAddr+10);  //RG1(4,7,10)
        // permit channels work
        //set_RGs(    7, BaseAddr+10);  //RG1(4,7,10)
    }
    read_3RG();
    return 0;
}


/** --------------------------------------------------------------
 * Reset FADC channels
 */
unsigned int reset_channels()
{
    for(unsigned int i = 1; i <= AddrOn[0]; i++)
    {
        BaseAddr = AddrOn[i];

        // reset  channels and buffer
        //set_RGs(    0, BaseAddr+10);  //RG1(4,7,10)
        RG1put  = set_bit_0(RG1put,  0); // buffer reset
        RG1put  = set_bit_0(RG1put,  1); // prohibit trigger1
        RG1put  = set_bit_0(RG1put,  2); // prohibit trigger2
        set_RGs( RG1put, BaseAddr+10);   //  //RG1(4,7,10)

        // permit channels work
        //set_RGs(    7, BaseAddr+10);  //RG1(4,7,10)
        RG1put  = set_bit_1(RG1put,  0); // buffer works
        RG1put  = set_bit_1(RG1put,  1); // permit trigger1
        RG1put  = set_bit_1(RG1put,  2); // permit trigger2
        set_RGs( RG1put, BaseAddr+10);   // permit //RG1(4,7,10)
    }

    read_3RG(); // read registers 3 times

    return 0;
}


/** --------------------------------------------------------------
 * Permit FADC channels to count
 */
unsigned int permit_channels()
{
    for(unsigned int i = 1; i <= AddrOn[0]; i++)
    {
        BaseAddr = AddrOn[i];
        // permit channels work
        //set_RGs(    7, BaseAddr+10);  //RG1(4,7,10)
        RG1put  = set_bit_1(RG1put,  0); // buffer works
        RG1put  = set_bit_1(RG1put,  1); // permit trigger1
        RG1put  = set_bit_1(RG1put,  2); // permit trigger2
        set_RGs( RG1put, BaseAddr+10);   // permit //RG1(4,7,10)
    }
    return 0;
}


/** --------------------------------------------------------------
 * \brief Prohibit channel counting
 */
unsigned int prohibit_channels()
{
    for(unsigned int i = 1; i <= AddrOn[0]; i++)
    {
        BaseAddr = AddrOn[i];

        // permit channels work
        //set_RGs(    1, BaseAddr+10);  //RG1(4,7,10)
        RG1put  = set_bit_0(RG1put,  1); // ban trigger1
        RG1put  = set_bit_0(RG1put,  2); // ban trigger2
        set_RGs( RG1put, BaseAddr+10);   // ban //RG1(4,7,10)
    }
    return 0;
}


/** --------------------------------------------------------------
 * Check fifo error
 * \return 1 if there is error in RG0: bit 8
 */
unsigned int fifo_err(void)
{
    unsigned short in = 0, address = 0, out = 0;

    for(unsigned int i = 1; i <= AddrOn[0]; i++)
    {
        BaseAddr = AddrOn[i];
        address = BaseAddr + RG0ADDR;//RG0

        //
        for(int j = 0; j <= 3; j++)
        {
            in = inw(address + 0x10*j);  //RG0
            if(get_bit(in, 8))
            {
                if(dout) fprintf(dout, "\nFifo_err = 1, addr = %xh", address + 0x10*j);
                fprintf(stdout, "\nFifo_err = 1, addr = %xh", address + 0x10*j);
                //print_binary1(in);
                //if(dout) fprintf(dout, "\n");
                //fprintf(stdout, "\n");
                //return 1;
                out ++;
            }
        }
  }
  return out;
}


/** --------------------------------------------------------------
 * Reset FADC counters
 */
unsigned int reset_counters()
{
    for(unsigned char i = 1; i <= AddrOn[0]; i++)
    {
        BaseAddr = AddrOn[i];

        // reset counters
        RG1put  = set_bit_1(RG1put,  9); // reset counter1
        RG1put  = set_bit_1(RG1put, 11); // reset counter2
        set_RGs( RG1put, BaseAddr+10);   // default //RG1(4,7,10)
        RG1put  = set_bit_0(RG1put,  9); // reset counter1
        RG1put  = set_bit_0(RG1put, 11); // reset counter2
        set_RGs( RG1put, BaseAddr+10);   // default //RG1(4,7,10)
    }
    return 0;
}


/** --------------------------------------------------------------
 * Stop FADC counters
 */
unsigned int stop_counters()
{
    for(unsigned char i = 1; i <= AddrOn[0]; i++)
    {
        BaseAddr = AddrOn[i];

        // reset counters
        RG1put  = set_bit_0(RG1put,  8); // stop counter1
        RG1put  = set_bit_0(RG1put, 10); // stop counter2
        set_RGs( RG1put, BaseAddr+10);   // stop //RG1(4,7,10)
    }
    return 0;
}


/** --------------------------------------------------------------
 * Start FADC counters
 */
unsigned int start_counters()
{
    for(unsigned char i = 1; i <= AddrOn[0]; i++)
    {
        BaseAddr = AddrOn[i];

        // reset counters
        RG1put  = set_bit_1(RG1put,  8); // start counter1
        RG1put  = set_bit_1(RG1put, 10); // start counter2
        set_RGs( RG1put, BaseAddr+10);   // start //RG1(4,7,10)
    }
    return 0;
}


/** --------------------------------------------------------------
 * Read FADC counters
 */
unsigned int read_counters()
{
    unsigned short rate = 1;
    unsigned short count_addr[9] = {8,0xA,0xC,0x1A,0x1C,0x2A,0x2C,0x3A,0x3C};
    FILE *fthr = NULL;

    printf("\n  R: ");
    if(dout) fprintf(dout,"\n  R: ");
    if(fout) fprintf(fout, "r");
    if((fthr = fopen("levels.dat", "a")) == NULL)
    {
        if(dout) fprintf(dout, "Data file \"levels.dat\" is not open!");
    }

    timestamp_to_file(fthr);
    if(fthr) fprintf(fthr,"R:\t");
    for(unsigned char i = 1; i <= AddrOn[0]; i++)
    {
        BaseAddr = AddrOn[i];
        for(int jj = 1; jj <= CHANPMT; jj++) // CHANPMT=8
        {
            // read  counters
            rate = inw(BaseAddr + count_addr[jj]);

            printf(" %5i", rate);
            if(dout) fprintf(dout, " %5i", rate);
            if(fthr) fprintf(fthr, "%5i\t", rate);
            Conv.tInt = rate;
            if(fout) fprintf(fout, "%c%c", Conv.tChar[1], Conv.tChar[0]); // print to file
        }
    }
    printf("\n");
    if(dout) fprintf(dout,"\n");
    if(fthr) fprintf(fthr,"\n");
    if(fthr) fclose(fthr);
    return 0;
}


/** --------------------------------------------------------------
 * Read 3 registers before read FADC
 */
unsigned int read_3RG()
{
    unsigned short rate = 1;

    for(unsigned char i = 1; i <= AddrOn[0]; i++)
    {
        BaseAddr = AddrOn[i]; 
        rate = inw(BaseAddr + 8);
        rate = inw(BaseAddr + 10);
        rate = inw(BaseAddr + 12);
    }
    if(rate);
    return 0;
}
