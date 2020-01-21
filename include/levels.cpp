/**
 * \file levels.cpp
 * \brief Класс fadc_board. Управление порогами
 *
 * Функции, связанные с выставлением порогов измерительных каналов. Часть класса fadc_board.
 */


/** ----------------------------------------------------------
 * \brief Set threshold levels in all measuring channels.
 *
 * Выставление порогов срабатывания измерительных каналов.
 */
void levels(void)
{
    struct timeval tv0, tv1;
    long delta = 0;

    gettimeofday(&tv0, NULL);

    if(Work.autolevels)
        levels_auto();
    else
        if( set_THR_from_file((char*) LEVELS_INPUT_FILE) != 0)
            set_THR_from_file((char*) LEVELS_CONFIG_FILE);

    print_THR_to_file();
    print_THR_to_configfile();


    // count and print time
    gettimeofday(&tv1, NULL);
    delta = tv1.tv_sec - tv0.tv_sec;
    sprintf(debug, "\n==levels==> delta = %ld sec\n", delta);
    print_debug(debug);
    if(dout) fflush(dout);
}


/** ----------------------------------------------------------
 * \brief Set threshold levels in all measuring channels.
 *
 * Процедура автоматического выставления порогов срабатывания измерительных каналов.
 */
void levels_auto(void)
{
    unsigned short ii = 1, jj = 1; //, need = 1;
    int lmax = 8191, dl = 100;

    /// -- set all levels to max
    lmax = Work.lmax;
    for(ii = 1; ii <= AddrOn[0]; ii++)
    {
        BaseAddr = AddrOn[ii];
        set_RGs( lmax, BaseAddr);     //ADC0(4,8,12)/THR
        set_RGs( lmax, BaseAddr+4);   //ADC2(6,10,14)/THR
        for(jj = 0; jj <= CHANPMT; jj++)
            THR[ii][jj] = lmax;       // CHANPMT=8
    }

    /// -- set all thresholds 
    for(dl = lmax/2; dl >= 1; dl /= 2 )
    {
        set_THR();
        correct_THR(dl);
    }
    sprintf(debug, "\n");
    print_debug(debug);

    /// -- set thresholds more 10 times
    for(jj = 0; jj <10; jj++)
    {
        set_THR();
        correct_THR(1);
    }
    sprintf(debug, "\n");
    print_debug(debug);

    // -- set thresholds more 20 times plus 1 unit every time 
    for(jj = 1; jj <20; jj++)
    {
        set_THR();
        correct_THR_plus(1);
        //THR_save(jj); //
    }
}


/** ----------------------------------------------------------
 *  \brief Set RGs: write int word to address
 *  \param word    number to write
 *  \param address address to write
 */
void set_RGs(int word, unsigned int address)
{
    short i = 0;
    for(i=0; i<=3; i++) outw( word, address + 0x10*i);  //RG0
}


/** ----------------------------------------------------------
 *  \brief Set threshold levels to all boards
 * 
 *  Set threshold levels from array THR[][] to all channels of all FADC boards
 */
unsigned char set_THR(void)
{ 
    unsigned short int jj = 0, ii = 0;
    unsigned short THR_addr[9] = {8,0x0,0x4,0x10,0x14,0x20,0x24,0x30,0x34}; // address of regs to write threshold levels

    sprintf(debug, "\nTHR: ");
    print_debug(debug);
    for(ii = 1; ii <= AddrOn[0]; ii++ )
    {
        for(jj = 1; jj <= CHANPMT; jj++) // CHANPMT=8
        {
            outw( THR[ii][jj], AddrOn[ii] + THR_addr[jj]);  //RG0
            sprintf(debug, " %5i", THR[ii][jj]);
            print_debug(debug);
        }
        sprintf(debug, "\n");
        print_debug(debug);
    }
    print_THR_to_file();
    return 0;
}


/** ----------------------------------------------------------
 *  \brief  Set levels from file 
 *
 * Процедура выставления порогов. 
 * Пороги читаются из конфигурационного файла LEVELS_CONFIG_FILE or LEVELS_INPUT_FILE.
 *  \return 0  -- OK\par 
 *          1  -- error  in file opening\par 
 *          2  -- errors in file reading\par
 *          3  -- errors in testing levels
 */
int set_THR_from_file(char* fname)
{
    FILE *fthr = NULL;
    char  buf[ 1200] = {0};
    char  line[1200] = {0};
    int   kk = 0, err = 0;
    int   Lev[BOARD+1][9];
    unsigned char  ii = 0, jj = 0;
    //unsigned short lpar = 0; //, lnum = 0;
    int lpar = 0, linesize=0; //, lnum = 0;


    /// -- open config file --------------------------
    if((fthr = fopen(fname, "r")) == NULL)
    {
        if(dout) fprintf(dout, "Data file %s is not open!", fname);
        return 1;  // error
    }
    sprintf(debug,  "\n===> LEVELS from \"%s\"  <=====\n", fname);
    print_debug(debug);


    /// -- read levels from config file ----------
    err = 0;
    while( fgets(line, sizeof(line), fthr) != NULL )
    {
        if( (line[0] == '\n') || (line[0] == '/') )
        {
            continue;
        }
        if(dout) fprintf(dout, "%s", line);
        linesize = strlen(line);      //printf("strlen = %d\n", linesize);
        if(linesize < 100) continue; // no reading

        kk = sscanf(line, "%s", buf);
        if(!kk) continue; // no reading

        kk = 0;
        if( !strncmp(buf,"T:", 3) ) // in 3 first positions
        {
            fseek(fthr, -linesize, SEEK_CUR);
            kk = fscanf(fthr, "%s", buf); // read "T:"
            //printf("buf=%s \n", buf);

            for(ii = 1; ii <= AddrOn[0]; ii++ )
            {
                for(jj = 1; jj <= CHANPMT; jj++) // CHANPMT=8
                {
                    kk = fscanf(fthr, "%u", &lpar); // read level unsigned short int
                    //printf("\nla=%d l=%d k = %d", ii, lpar, kk);
                    if(kk == 1)  // one number was read
                    {
                        Lev[ii][jj] = lpar;
                    }
                    else err++;
                }
            }
        }
    }
    fclose(fthr);

    if (err)
    {
        sprintf(debug, "\nRead_levels: %d errors!", err);
        print_debug(debug);
        fprintf(stderr, "%s", debug);
        return 2;
    }
    else
    {
        sprintf(debug, "\nRead_levels: OK!");
        print_debug(debug);
    }

    /// -- test levels ---------------------------
    err = 0;
    sprintf(debug, "Lev: ");
    print_debug(debug);
    for(ii = 1; ii <= AddrOn[0]; ii++ )
    {
        for(jj = 1; jj <= CHANPMT; jj++) // CHANPMT=8
        {
            sprintf(debug, "%d ", Lev[ii][jj]);
            print_debug(debug);
            if((Lev[ii][jj] < Work.lmin) || (Lev[ii][jj] > Work.lmax))
            {
                err++;
            }
        }
    }
    if (err)
    {
        sprintf(debug, "\nTest_levels: %d errors!", err);
        print_debug(debug);
        fprintf(stderr, "%s", debug);
        return 3;
    }
    else
    {
        sprintf(debug, "\nTest_levels: OK!");
        print_debug(debug);
    }


    /// -- write levels to array THR -------------------
    for(ii = 1; ii <= AddrOn[0]; ii++ )
    {
        for(jj = 1; jj <= CHANPMT; jj++) // CHANPMT=8
        {
            THR[ii][jj] = Lev[ii][jj];
        }
    }


    /// -- set levels ----------------------------
    set_THR(); // and print_THR_to_file();


    if(dout) fprintf(dout, "===> end of file \"%s\" <=====\n", fname);
    if(dout) fprintf(dout, "=============================================\n");
    return 0;
}


/** ----------------------------------------------------------
 *  \brief print_THR_to log file
 *
 *  Print threshold levels to file LEVELS_LOG_FILE("./log/levels.dat")
 */
int print_THR_to_file(void)
{
    FILE *fthr;
    unsigned char ii = 0, jj = 0;
    char name[] = LEVELS_LOG_FILE; //"./log/levels.dat";

    if((fthr = fopen(name, "a")) == NULL)
    {
        if(dout) fprintf(dout, "Data file \"%s\" is not open!", name);
        return 1;  // error
    }

    timestamp_to_file(fthr);
    print_time_ms(fthr);

    fprintf(fthr, "T:\t");
    for(ii = 1; ii <= AddrOn[0]; ii++ )
    {
        for(jj = 1; jj <= CHANPMT; jj++) // CHANPMT=8
        {
            fprintf(fthr, "%5i\t", THR[ii][jj]);
        }
    }

    fprintf(fthr, " \n");
    fflush(fthr);
    fprintf(dout, "levels file %s closing ... ", name);
    fclose(fthr);
    fprintf(dout, ". done \n");
    return 0;
}


/** ----------------------------------------------------------
 *  \brief print_THR_to_configfile
 * 
 *  Print threshold levels to file LEVELS_CONFIG_FILE = "./config/levels.config"
 */
int print_THR_to_configfile(void)
{
    FILE *fthr;
    unsigned char ii = 0, jj = 0;
    char name[] = LEVELS_CONFIG_FILE; //"./config/levels.config";

    if((fthr = fopen(name, "w")) == NULL)
    {
        if(dout) fprintf(dout, "Data file \"%s\" is not open!", name);
        return 1;  // error
    }

    fprintf(fthr, "//N:\t");
    for(ii = 1; ii <= 112; ii++ )
    {
        fprintf(fthr, "%5i\t", ii);
    }
    fprintf(fthr, "\n");
    fprintf(fthr, "T:\t");
    for(ii = 1; ii <= AddrOn[0]; ii++ )
    {
        for(jj = 1; jj <= CHANPMT; jj++) // CHANPMT=8
        {
            fprintf(fthr, "%5i\t", THR[ii][jj]);
        }
    }

    //fflush(fthr);
    fprintf(fthr, " \n");
    fflush(fthr);
    fprintf(dout, "levels file  \"%s\" closing ... ", name);
    fclose(fthr);
    fprintf(dout, ". done \n");
    return 0;
}


/** ----------------------------------------------------------
 *  \brief One step of levels correction.
 *
 *  One step of threshold levels correction. Correct levels in array THR[][] according to counting rate.\par
 *  The minimum rate is 0.8 Hz.
 */
unsigned short int correct_THR(unsigned short dlev)
{
    /// address of counters
    unsigned short count_addr[9] = {8,0xA,0xC,0x1A,0x1C,0x2A,0x2C,0x3A,0x3C};
    unsigned short sta = 0,  dis = 2, rate = 1;
    unsigned short ii = 0,   jj = 0;
    unsigned short lmin = 2, rtime = 4;
    unsigned int   lmax = 32767, lev = 1023;
    float rmax = 0.,   rmin = 0.0;
    float maxkoef = 1.1, minkoef = 0.9;
    float Rate = 0.;

    lmin = Work.lmin;
    lmax = Work.lmax;
    Rate = Work.rate;

    if(Rate <= 1.0) //if(Rate == 1)
    {
        rmin = 0.8;
        rmax = 2.;
    }
    else
    {
        rmin = Rate * minkoef;
        rmax = Rate * maxkoef;
    }
    rmin *= rtime;
    rmax *= rtime;

    /// --- start all boards to count
    for(ii = 1; ii<= AddrOn[0]; ii++)
    { 
        BaseAddr = AddrOn[ii];
        RG1put  = 0;         // stop
        RG1put  = set_bit_0(RG1put,  8); // set stop
        RG1put  = set_bit_0(RG1put, 10); // set stop
        set_RGs( RG1put, BaseAddr+10);   // stop    //RG1(4,7,10)

        RG1put  = set_bit_1(RG1put,  9); // set default counter1
        RG1put  = set_bit_1(RG1put, 11); // set default counter2
        set_RGs( RG1put, BaseAddr + 10); // default //RG1(4,7,10)

        RG1put  = set_bit_1(RG1put,  0); // set buffer works
        RG1put  = set_bit_1(RG1put,  1); // set permit trigger1
        RG1put  = set_bit_1(RG1put,  2); // set permit trigger2
        RG1put  = set_bit_0(RG1put,  9); // set default counter1
        RG1put  = set_bit_0(RG1put, 11); // set default counter2
        RG1put  = set_bit_1(RG1put,  8); // set start counter1
        RG1put  = set_bit_1(RG1put, 10); // set start counter2
        set_RGs( RG1put, BaseAddr + 10); // start //RG1(4,7,10)
    }
    sleep(rtime); // count some time

    /// --- stop  all boards
    for(ii = 1; ii<= AddrOn[0]; ii++)
    {
        BaseAddr = AddrOn[ii];
        RG1put  = set_bit_0(RG1put, 8);  // stop
        RG1put  = set_bit_0(RG1put, 10); // stop
        set_RGs( RG1put, BaseAddr+10);   // stop //RG1(4,7,10)
    }

    /// --- analyze and correct each THR level
    sprintf(debug, "\nR(max = %5.1f):\n", rmax);
    print_debug(debug);
    for(ii = 1; ii <= AddrOn[0]; ii++ )
    {
        BaseAddr = AddrOn[ii];
        for(jj = 1; jj <= CHANPMT; jj++) // CHANPMT=8
        {
            // if channel if off - skip level change
            if( !Work.trigger_onoff[ (ii - 1) * CHANPMT + jj - 1] )
            {
                sprintf(debug, " NO%d  ", (ii - 1) * CHANPMT + jj - 1);
                print_debug(debug);
                continue;
            }

            // check count
            lev = THR[ii][jj];
            rate = inw(BaseAddr + count_addr[jj]);
            sprintf(debug, " %5i", rate);
            print_debug(debug);

            // if rate is big
            if(rate > rmax)
            {
                lev += dlev;
                if(lev > lmax) lev = lmax;
                sprintf(debug, "u");
                print_debug(debug);
            }
            else  // if rate is small
            {
                if(rate < rmin)
                {
                    sta = inw(BaseAddr + 0x8 + 0x10*(int)((jj-1)/2));
                    dis = 9 + (int(jj-1)%2);
                    if(get_bit(sta,dis)) // if diskriminator vyshe poroga - porog nizok
                    {
                        lev += dlev;
                        if(lev < lmin) lev = lmin;
                        sprintf(debug, "+");
                        print_debug(debug);
                    }
                    else
                    {
                        lev -= dlev;
                        if(lev < lmin) lev = lmin;
                    }
                }
                sprintf(debug, "d");
                print_debug(debug);
            }
            THR[ii][jj] = lev; //thr matrix (!?)
        }
        sprintf(debug, "\n");
        print_debug(debug);
    }
    return 0;
}


/** ----------------------------------------------------------
 *  \brief One step of levels correction. Only increase.
 * 
 *  One step of threshold levels correction. Correct levels in array THR[][].
 *  Only increase levels, not decrease
 */
unsigned short int correct_THR_plus(unsigned short dlev)
{
    // count_addr - address of count
    unsigned short count_addr[9] = {8,0xA,0xC,0x1A,0x1C,0x2A,0x2C,0x3A,0x3C};
    unsigned short rtime = 4;
    unsigned short sta = 0,  dis = 2, rate = 1;
    unsigned short ii = 0,   jj = 0;
    int   lmax = 1023, lev = 2048;
    float rmax = 0.,   rmin = 0.;
    float maxkoef = 1.1, minkoef = 0.9;
    float Rate = 0.;

    //lmin = Work.lmin;
    lmax = Work.lmax;
    Rate = Work.rate;

    if(Rate == 1.0) //if(Rate == 1)
    {
        rmin = 0.8;
        rmax = 2.;
    }
    else
    {
        rmin = Rate * minkoef;
        rmax = Rate * maxkoef;
    }
    rmin *= rtime;
    rmax *= rtime;

    sprintf(debug, "\n");
    print_debug(debug);

    // start  all boards to count
    for(ii = 1; ii<= AddrOn[0]; ii++)
    { 
        BaseAddr = AddrOn[ii];
        RG1put  = 0;         // stop
        RG1put  = set_bit_0(RG1put,  8); // stop
        RG1put  = set_bit_0(RG1put, 10); // stop
        set_RGs( RG1put, BaseAddr+10);   // stop    //RG1(4,7,10)
        RG1put  = set_bit_1(RG1put,  9); // default counter1
        RG1put  = set_bit_1(RG1put, 11); // default counter2
        set_RGs( RG1put, BaseAddr + 10);   // default //RG1(4,7,10)
        RG1put  = set_bit_1(RG1put,  0); // buffer works
        RG1put  = set_bit_1(RG1put,  1); // permit trigger1
        RG1put  = set_bit_1(RG1put,  2); // permit trigger2
        RG1put  = set_bit_0(RG1put,  9); // default counter1
        RG1put  = set_bit_0(RG1put, 11); // default counter2
        RG1put  = set_bit_1(RG1put,  8); // start counter1
        RG1put  = set_bit_1(RG1put, 10); // start counter2
        set_RGs( RG1put, BaseAddr + 10);   // start //RG1(4,7,10)
    }
    // count some time
    sleep(rtime);
    // stop
    for(ii = 1; ii<= AddrOn[0]; ii++)
    {
        BaseAddr = AddrOn[ii];
        RG1put  = set_bit_0(RG1put, 8);  // stop
        RG1put  = set_bit_0(RG1put, 10); // stop
        set_RGs( RG1put, BaseAddr+10);   // stop //RG1(4,7,10)
    }

    // analyze each THR level
    sprintf(debug, "  R: ");
    print_debug(debug);

    for(ii = 1; ii <= AddrOn[0]; ii++ )
    {
        BaseAddr = AddrOn[ii];
        for(jj = 1; jj <= CHANPMT; jj++) // CHANPMT=8
        {
            // check count
            rate = inw(BaseAddr + count_addr[jj]);                
            lev = THR[ii][jj];
            sprintf(debug, " %5i", rate);
            print_debug(debug);

            // if rate big
            if(rate > rmax)
            {
                lev += dlev;
                if(lev > lmax) lev = lmax;
            }
            else  // if rate small
            {
                if(rate < rmin)
                {
                    sta = inw(BaseAddr + 0x8 + 0x10*(int)((jj-1)/2));
                    dis = 9 + (int)((jj-1)%2);
                    //printf("d%1i",get_bit(sta,dis));
                    if(get_bit(sta,dis)) // if diskriminator vyshe poroga - porog nizok
                    {
                        if(dout) fprintf(dout, "+");
                        lev += dlev;
                    }
                }
            }
            THR[ii][jj] = lev; //thr matrix (!?)
        }
    }
    return 0;
}


/** ----------------------------------------------------------
 * \brief Increase all levels up to 1 unit.
 * 
 *  Add 1 to all levels in array THR[][]
 */
/*
unsigned short int correct_THR_up(void)
{
    // count_addr - address of count
    //unsigned short count_addr[9] = {8,0xA,0xC,0x1A,0x1C,0x2A,0x2C,0x3A,0x3C};
    unsigned short ii = 0,   jj = 0;

    printf("\n");
    if(dout) fprintf(dout, "\n");

    for(ii = 1; ii <= AddrOn[0]; ii++ )
    {
        for(jj = 1; jj <= CHANPMT; jj++) // CHANPMT=8
        {
            THR[ii][jj] +=1;
        }
    }
    return 0;
}
*/


//----------------------------------------------------------
// set threshold levels in all channels on the board with address BasAddr
/*
unsigned char set_THR_plus(void)
{
    unsigned short int jj = 0, ii = 0;
    // adress of regs to write threshold levels
    unsigned short THR_addr[9] = {8,0x0,0x4,0x10,0x14,0x20,0x24,0x30,0x34}; 

    printf("\nTHR_A: ");
    if(dout) fprintf(dout, "\nTHR_A: ");
    for(ii = 1; ii <= AddrOn[0]; ii++ )
    {
        for(jj = 1; jj <= CHANPMT; jj++) // CHANPMT=8
        {
        outw( THR[ii][jj] + THR_ADD, AddrOn[ii] + THR_addr[jj]);  //RG0
        printf(" %5i", THR[ii][jj]);
        if(dout) fprintf(dout, " %5i", THR[ii][jj]);
        }
    }
    return 0;
}
*/


//  public:
//----------------------------------------------------------
// set threshold level in one channel with number chan
/*
unsigned char set_one_THR(unsigned short level, unsigned char chan)
{
    unsigned short int jj = 0, ii = 0;
    unsigned short THR_addr[9] = {8,0x0,0x4,0x10,0x14,0x20,0x24,0x30,0x34}; // adress of regs to write threshold levels

    ii = (chan-1)/8 + 1; // plate number in AddrOn
    jj = (chan-1)%8 + 1; // regs number in THR_addr

    outw( level, AddrOn[ii] + THR_addr[jj]);  //RG0
    printf("\n1THR: %5i\n", level);
    if(dout) fprintf(dout, "\n1THR: %5i\n", level);

    return 0;
}
*/
