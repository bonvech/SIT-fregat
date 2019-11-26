/**
 * \file operate.cpp
 * \brief
 * Функции, выполняемые во время основного цикла измерений. Вентиллятор.
 */


#define TERMOSTAB 21.0  ///< Temperature to stabilize
#define VENT_MAX 254    ///< Ventillator max code
#define VENT_ON  253    ///< Ventillator work code
#define SEC5 10         ///< time delta to make 5sec file
#define SEC60 60        ///< time delta to make every minute file


int FileNum = 0;        ///< number of output data file
int NumBar  = 3;        ///< number of barometers in use
unsigned int  sec5 = 0; ///<  number to make flag to write 5sec file
char debug[100];        ///< debug string



// --- functions --- 
int Every_sec(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent);
int Every_min(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent, led &LED, barometer Bar[]);
unsigned char GetEvent(fadc_board &Fadc, trigger_board &Trigger,  SiPM &Vip);
int simulate_event(fadc_board &Fadc, trigger_board &Trigger);
int Before(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent, led &LED);
int After(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent);
unsigned short Operate(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent, led &LED, barometer Bar[]);
void print_time();
void get_time_ms();
int check_current(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger);
int check_temperature(fadc_board &Fadc, lvps_dev &Vent);
int read_threshould_from_file();
unsigned int bars_init(led &LED, barometer Bar[]);
unsigned int bars_read(led &LED, barometer Bar[], char *message_out);
int save_eventnumber_to_file();
int read_command_file(void);
//int print_log_parameters(FILE *fileout);
int print_everymin_parameters(FILE *fileout);



//================================================
// ================= Every_sec  ==================
/** -------------------------------------------------------
 * \brief Every second function to control parameters
 * \todo delete vip from arguments
 * \todo check Trigger functions
 *
 * Reading of compass, GPS, inclinometer, temperature.\n
 * Every SEC5 seconds writes info to file '5s.data' and debug file
 */
int Every_sec(fadc_board &Fadc,       SiPM &vip,
              trigger_board &Trigger, lvps_dev &Vent)
{
    sec5 ++;
    check_temperature(Fadc, Vent);

    if(!(sec5 % SEC5))
    {
        get_time_ms();

        f5sec = freopen(EVERYSEC_FILE, "wt", f5sec);
        if(f5sec)
        {
            fprintf(f5sec, "%s\n%s\n", time_out, msc_out);
            fprintf(f5sec, "Temperatures: B: %.1f T: %.1f\n", Last.temp_bot, Last.temp_top);
            fflush(f5sec);
        }
        if(stdout) printf("\r");
        print_debug(time_out);
    }

    Trigger.pps_read_time();
    Trigger.pps_read_time();
    return 0;
}


//================================================
// ================= Every_min  ==================
/** -------------------------------------------------------
 *  \brief Every minute MINI function to control parameters of turned off detector
 *
 * Read the temperature, barometers, ventillator, LED.\n
 * Every min writes info to files '1m.data' and debug
 */
void Every_min_mini(lvps_dev &Vent, led &LED, barometer Bar[])
{
    //FILE *flog = NULL;

    /// -- read barometers
    bars_read(LED, Bar, bar_out);

    /// -- read other parameters
    Vent.read_vip_ADC(adc_out);
    Vent.read_power_temp(pwr_out);
    LED.read_ADC(led_out);

    /// -- print all parameters to stdout
    if(stdout) print_everymin_parameters(stdout);

    // -- print all parameters to file EveryMin
    ffmin = freopen(EVERYMIN_FILE, "wt", ffmin);
    if(ffmin)
    {
        print_everymin_parameters(ffmin);
        fflush(ffmin);
    }

/*
    // -- print all parameters to Log file
    if((flog = fopen(LOG_FILE, "at")) != NULL)
    {
        print_everymin_parameters(flog);
        fclose(flog);
    }
 */
}


/** -------------------------------------------------------
 *  \brief Every minute function to control parameters
 *
 * Reading of temperature, current, trigger status, barometers, vip, ventillator,  mosaic temperature.\n
 * Every min writes info to files '1m.data' and debug
 */
int Every_min(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent, led &LED, barometer Bar[])
{
    int  dk = 0;
    //FILE *flog = NULL;

    Trigger.trigger_prohibit();
    get_time_ms();

    check_temperature(Fadc, Vent);
    dk = check_current(Fadc, vip,  Trigger);
    Trigger.status();
    Trigger.pps_read_time();
    Trigger.pps_read_time();

    // ----------- read barometers --------------
    bars_read(LED, Bar, bar_out);

    // ----------- read other parameters --------
    vip.read_vip_ADC(vip_out);
    Vent.read_power_temp(pwr_out);
    Vent.read_vip_ADC(adc_out);

    LED.read_ADC(led_out);

    // --- stdout: print all  parameters --------
    print_everymin_parameters(stdout);
    print_everymin_parameters(  dout);

    // --- ffmin: print all  parameters --------
    ffmin = freopen(EVERYMIN_FILE, "wt", ffmin);
    if(ffmin)
    {
        print_everymin_parameters(ffmin);
        fflush(ffmin);
    }

/*
*   // ------ log: print all  parameters --------
    if((flog = fopen(LOG_FILE, "at")) != NULL)
    {
        print_everymin_parameters(flog);
        fclose(flog);
    }
    */

    fflush(stdout);
    fflush(  dout);

    if(dk == 2) return 2; // no channels to trigger
    //if(dk < 2) return 2;  // no channels to trigger
    Trigger.trigger_permit();
    return 0;
}


//================================================
// =================   Before   ==================
/** -------------------------------------------------------
 *  \brief Procedure to run before work period
 */
int Before(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent, led &LED)
{
    char info[100] = {""};

    // Open new binary data file
    open_data_file(Fadc);

    // Test and reset FADC channels
    check_temperature(Fadc, Vent);
    Fadc.test_fadc_boards();
    Fadc.reset_channels();
    check_temperature(Fadc, Vent);

    // Set trigger local and global master threshoulds
    read_threshould_from_file();
    Trigger.set_local_threshould(Work.master);
    Trigger.set_global_threshould(Work.gmaster);
    Trigger.set_trigger();   // set EVNTB01-89

    // Set configurations from file
    LED.set_config_from_file();
    Fadc.set_THR_from_file();

    //-----------------
    //simulate_event(Fadc, Trigger);
    Trigger.status();
    Trigger.pps_read_time();
    Trigger.pps_read_time();

    // Reset FADC
    Fadc.reset_channels();
    Fadc.reset_counters();
    Fadc.start_counters();

    check_temperature(Fadc, Vent);
    vip.measure_high();

    // -- print all parameters to file EveryMin
    if(ffmin)
    {
        ffmin = freopen(EVERYMIN_FILE, "wt", ffmin);
        print_everymin_parameters(ffmin);
        fflush(ffmin);
    }

    sprintf(info, "\n---------------------------------------\n");
    print_debug(info);
    sprintf(info, "Waiting for signal ... \n");
    print_debug(info);

    Trigger.trigger_permit();
    fflush(dout);
    return 0;
}


//================================================
// =================    After   ==================
/** -------------------------------------------------------
 *  \brief Procedure to run after the work period
 */
int After(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent)
{
    /// Prohibit trigger
    Trigger.trigger_prohibit();
    Trigger.status();

    /// Stop FADC counters
    //Fadc.print_THR_to_file();
    Fadc.stop_counters();
    Fadc.read_counters();

    check_temperature(Fadc, Vent);
    vip.measure_high(); // 2010.03.11
    time_to_file();

    /// Close binary data file
    if(fout) fclose(fout);

    fflush(dout);
    return 0;
}


//================================================
// =================   Period   ==================
/** -------------------------------------------------------
 *  \brief Procedure to run every work period
 */
int Period(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent, led &LED)
{
    After( Fadc, vip, Trigger, Vent);
    Before(Fadc, vip, Trigger, Vent, LED);

    return 0;
}


/** -------------------------------------------------------
 *  \brief Simulate event
 *  \deprecated Старая функция. Закомментирована.
 */
int simulate_event(fadc_board &Fadc, trigger_board &Trigger)
{
/*    int dk = 0;
    if(Trigger.buffer_is_not_empty(TG) )
    {
        printf("SIM: NotEmpty:");
        if(dout) fprintf(dout, "SIM: NotEmpty: ");
        dk = GetEvent(Fadc, Trigger);
        printf( "\n k+%i \n", dk);
        if(dout) fprintf(dout, "k+%i\n", dk);
    }
    Trigger.trigger_sim();   // simulate event to control
    if(fout) fprintf(fout, "s");
    if(Trigger.buffer_is_not_empty(TG) )
    {
        printf("SIM: NotEmpty:");
        if(dout) fprintf(dout, "SIM: NotEmpty: ");

        dk = GetEvent(Fadc, Trigger);
        printf( "\n k+%i \n", dk);
        if(dout) fprintf(dout, "k+%i\n", dk);
    }*/
    return 0;
}


//================================================
// ================= GetEvent  ===================
/** -------------------------------------------------------
 *  \brief Get event if there is any event in buffer
 *  \return number of detected events
 */
unsigned char GetEvent(fadc_board &Fadc, trigger_board &Trigger, SiPM &Vip)
{
    unsigned char Over = 0, Err = 0, num = 0;
    struct timeval tv0 = {0};
    int Inclination = 0, Magnitation = 0;
    char info[100] = {0};

    get_time_ms();

    num = Trigger.buffer_event_number(TG);
    //sprintf(info, " num = %i\n", num);
    //print_debug(info);

    if(num == 4)
    {
        if(Trigger.buffer_is_overflow(TG))
        {
            Trigger.trigger_prohibit();
            Fadc.prohibit_channels();
            Over = 1;
            print_debug((char*) " Over = 1\n");
        }
    }

    // !!!!!!! print kadr to virtual event file
    //2019!!!fkadr = freopen("event", "w", fkadr); // 2018
    //if(fkadr) fprintf(fkadr, "%2i\n", EventNumber + 1); 

    //  --------- get events from buffer -------
    for(int i = 0; i < num; i++)
    {
        // ---------- read event  --------
        EventNumber ++;
        //sprintf(event_out, "<K%05d>g%5s", EventNumber, gps_bstamp);
        fprintf(fout, "<K%05d>g%c%c%c%c%c", EventNumber,
                gps_bstamp[0],gps_bstamp[1],gps_bstamp[2],gps_bstamp[3],gps_bstamp[4]);
        sprintf(info, "\n<K%05d>  Time: %13s   ",  EventNumber, gps_sstamp);
        print_debug(info);


        // -- print localtime (sec)
        gettimeofday(&tv0, NULL);
        Conv.tInt = tv0.tv_sec;
        fprintf(fout, "t%c%c%c%c", Conv.tChar[3], Conv.tChar[2], Conv.tChar[1], Conv.tChar[0]);
        //strncat(event_out, tmp, 5);

        // -- print trigger time
        Conv.tInt = Trigger.tg_read_time();
        fprintf(fout, "e%c%c%c%c", Conv.tChar[3], Conv.tChar[2], Conv.tChar[1], Conv.tChar[0]);
        //strncat(event_out, tmp, 5);

        // -- print Inclinometer
        Conv.tInt = Inclination;
        fprintf(fout, "I%c%c%c%c", Conv.tChar[3], Conv.tChar[2], Conv.tChar[1], Conv.tChar[0]);
        //strncat(event_out, tmp, 5);

        // -- print Magnitometer
        Conv.tInt = Magnitation;
        fprintf(fout, "m%c%c%c%c", Conv.tChar[3], Conv.tChar[2], Conv.tChar[1], Conv.tChar[0]);
        //strncat(event_out, tmp, 5);

        // -- print event_out to event file
        //fprintf(fout, "%s", event_out); 

        // -- print currents to event file
        //Vip.print_currents(stdout);
        Vip.print_currents_to_binary(fout);

        // -- print fadc data to event file
        Fadc.get_event(i);
        fprintf(fout, "</K%05d>", EventNumber);
        //fprintf(fout, "%s", event_out);
        fflush(fout);

        //sprintf(info, "t-%3i-%3i-%3i-%3i\n", Conv.tChar[3], Conv.tChar[2], Conv.tChar[1], Conv.tChar[0]);
        //print_debug(info);

        // check fifo_err
        if(!Err)
        {
            if(Fadc.fifo_err())
            {
                if(!Over)
                {
                    Trigger.trigger_prohibit();
                    Fadc.prohibit_channels();
                    Err = 1;
                    print_debug((char*)"OverErr = 1\n");
                }
            }
        }
    }
    save_eventnumber_to_file();

    /// if overflow or error - clear buffer
    if((Over) || (Err))
    {
        Trigger.clear_buffer_overflow(TG);
        Fadc.reset_channels();
        sprintf(info, "Fadc.fifo_err() = %i after\n", Fadc.fifo_err());
        print_debug(info);
        Trigger.trigger_permit();
    }

    return num;
}


//New version of GetEvent->only resets buffer
/*
unsigned char GetEvent(fadc_board &Fadc, trigger_board &Trigger)
{ //if Trigger bufer not empty!
    unsigned char num = 0;
    num = Trigger.buffer_event_number(TG);
    Trigger.trigger_prohibit();
    Fadc.prohibit_channels();
    Trigger.clear_buffer_overflow(TG);
    Fadc.reset_channels();
    Trigger.trigger_permit();
    return num;
}
*/


//================================================
// ================= operate  ====================
/** -------------------------------------------------------
 *  \brief The main function of measuring procedure
 *
 *  Проверяет время. Запускает процедуры каждую секунду, минуту и период.\n
 *  Проверяет буфер. Считывает события.\n
 *  Читает входной командный файл. Прекрацает выполнение, если поступила команда.
 *
 *  \return 0 - OK\n
 *          код команды - если читался командный файл
 */
unsigned short Operate(
    fadc_board &Fadc, SiPM &vip, trigger_board &Trigger,
    lvps_dev &Vent, led &LED, barometer Bar[])
{
    struct timeval tv00, tv0min, tv0sec, tv1;
    unsigned int   kadr = 0, res = 0, ii = 0;
    unsigned char  dk   = 0;
    char info[255]  = "";
    char debug[255] = "";
    long period = 300; // sec - period
    time_t timeoff = 0;
    timeoff = Work.timeOnOff.time_of;
    period  = Work.period;

    Before(Fadc, vip, Trigger, Vent, LED);
    gettimeofday(&tv00, NULL);
    tv0sec.tv_sec = tv00.tv_sec;
    tv0min.tv_sec = tv00.tv_sec;

    while(1)
    {
        gettimeofday(&tv1, NULL);
        // --- Every sec
        if((tv1.tv_sec-tv0sec.tv_sec)>0)
        {
            Every_sec(Fadc, vip, Trigger, Vent);
            gettimeofday(&tv0sec, NULL);
        }
        // --- Every min
        if((tv1.tv_sec-tv0min.tv_sec) > 60)
        {
            printf("\n\n====================\nEvery min");
            if( Every_min(Fadc, vip, Trigger, Vent, LED, Bar) == 2) goto STOP;
            gettimeofday(&tv1, NULL);
            tv0min.tv_sec = tv1.tv_sec;
            printf("Every min end\n====================\n\n");
        }
        // --- Period
        if((tv1.tv_sec-tv00.tv_sec) > period)
        {
            sprintf(debug, "\n\n---------------------\nEvery period:");
            print_debug(debug);
            Period(Fadc, vip, Trigger, Vent, LED);
            gettimeofday(&tv00, NULL);
            gettimeofday(&tv1, NULL);
            tv0min.tv_sec = tv1.tv_sec;
            tv0sec.tv_sec = tv1.tv_sec;
            sprintf(debug, "Every period end\n---------------------\n\n");
            print_debug(debug);
        }

        // --- Run finished -> exit
        if( tv1.tv_sec > timeoff )
        { 
            print_debug((char*)"Time Off!!\n");
            goto STOP;
        }

        // --- new events !!
        if(Trigger.buffer_is_not_empty(TG) )
        { 
            if (Fadc.read_buffer_flag())    //if there is data in FADC buffers
            {
                ;
            }
            //sprintf(debug, "\nTG:NotEmpty:");
            //print_debug(debug);
            dk = GetEvent(Fadc, Trigger, vip);
            kadr += dk;
            sprintf(debug, "k+%i=%i\n", dk, kadr);
            print_debug(debug);
        }

        // ---------   read command file -----------
        res = read_command_file();
        if(res > 0)
        {
                if(stdout) fprintf(stdout, "read_command_file: %d commands\n", res);
                if(  dout) fprintf(  dout, "read_command_file: %d commands\n", res);
                for(ii = 1; ii <= res; ii++)
                {
                    strcpy(info,"");
                    if(commandin[ii] == 1) // exit
                    {
                        strcpy(info, "\ncommand EXIT in file\n");
                        res = 1;
                    }
                    if(commandin[ii] == 2) //levels
                    {
                        strcpy(info, "\ncommand LEVELS in file\n");
                        res = 2;
                    }
                    if(commandin[ii] == 4) //high off
                    {
                        strcpy(info, "\ncommand HIGH OFF in file - === EXIT\n");
                        res = 1;
                    }
                    if(commandin[ii] == 5) //led off
                    {
                        strcpy(info, "\ncommand LED OFF in file - NOT SUPPORTED\n");
                        continue;
                    }
                    print_debug(info);
                    goto STOP;
                }
        }
        // --------- end of read command file -----------
        res = 0;

    }   // end of while

STOP:
    After(Fadc, vip, Trigger, Vent);
    printf("\n");
    return res;
}


/** -------------------------------------------------------
 *  \brief print time with milliseconds to stdout and debug file
 * 
 *  Print actual time to stdout and debug file
 */
void print_time()
{
    struct timeval tv;
    struct tm* ptm;
    char time_string[40];
    long milliseconds;

    gettimeofday(&tv, NULL);
    ptm = localtime (&tv.tv_sec);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", ptm);
    milliseconds = tv.tv_usec/1000;
    printf("%s.%03ld\n", time_string, milliseconds);
    if(dout) fprintf(dout, "%s.%03ld\n", time_string, milliseconds);
}


/** -------------------------------------------------------
 *  \brief get_time_ms
 *
 *  Get actual time to string time_out
 */
void get_time_ms()
{
    struct timeval tv;
    struct tm* ptm;
    char time_string[40];
    long milliseconds;
    unsigned char gph = 0, gpm = 0, gps = 0;

    gettimeofday(&tv, NULL);
    ptm = localtime (&tv.tv_sec);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", ptm);
    milliseconds = tv.tv_usec/1000;
    sprintf(time_out, "%s.%03ld", time_string, milliseconds);

    gph = (unsigned char) ptm->tm_hour;
    gpm = (unsigned char) ptm->tm_min;
    gps = (unsigned char) ptm->tm_sec;
    sprintf(gps_sstamp,"%02d:%02d:%02d.%03ld", gph, gpm, gps, milliseconds);

    Conv.tInt = (unsigned short) milliseconds;
    sprintf(gps_bstamp,"%c%c%c%c%c",gph, gpm, gps,
                                    Conv.tChar[1], Conv.tChar[0]);
}


/** -------------------------------------------------------
 *  \brief print_time_ms to file\par
 *  Get actual time to string time_out
 */
void print_time_ms(FILE *ff)
{
    fprintf(ff, "%s ", time_out);
}


/** -------------------------------------------------------
 *  \brief check_current
 *  \return 0 - OK, vip.check_current() вернул 0 \par
 *          1 - выключенные каналы исключены из триггера\par
 *          2 - vip.check_current() вернул 2
 */
int check_current(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger)
{
    int res = 0, ii = 0;
    res = vip.check_current();
    if( !res) return 0;
    if(res == 2) return 2;
    if(res == 1)
    {
        // exclude off channels from trigger
        for(ii = 0; ii <= Work.hvchan-1; ii++)
        {
            if( vip.hvwork[ii] )  continue;
            if( Input.hvtrig[ii] >= Work.hvchan) continue;
            Trigger.addOnOff_channel(Input.hvtrig[ii], 0);
        }
    }
    return 1;
}


/** -------------------------------------------------------
 *  \brief  check_temperature in electronic box
 *  \return 0 - OK\par
 *          1 - error in termometere\par
 *  Check temperature in the electronic box and correct it with ventillator
 */
int check_temperature(fadc_board &Fadc, lvps_dev &Vent)
{
    float delta = 0, tt = 0.;
    struct temp Now;

    Now.temp_bot = Fadc.read1_average_fadc_temp(0x49); // bottom
    Now.temp_top = Fadc.read1_average_fadc_temp(0x4A); // top
    if( Now.temp_bot < -95) return 1; // error in termometere
    if( Now.temp_top < -95) return 1;

    if( Now.temp_top <= TERMOSTAB ) // if low temperature
    {
        Now.high_out = 0;
        Now.high_inn = 0;
    }
    else
    {
        Now.high_inn = Last.high_inn;
        Now.high_out = Last.high_out;
        if(!Now.high_out) Now.high_out = VENT_ON;
        if(!Now.high_inn) Now.high_inn = VENT_ON;
        // OUT vent
        //
        delta = Now.temp_top - TERMOSTAB;
        if(delta > 3.0)
        {
            Now.high_out ++;
        }
        // control of rising of top temperature
        delta = Now.temp_top - Last.temp_top;
        if(delta > 0.2)  // T rise
        {
            printf(" del = %.2f ", delta);
            Now.high_out ++;
        }
        else
        {
            if(delta < -0.2) // T down
            {
                printf(" del = %.2f ", delta);
                if(Now.high_out > VENT_ON) Now.high_out --;
            }
        }
        // control the temperature delta
        // INN vent
        delta = Now.temp_top - Now.temp_bot;
        if(delta > 5.0)
        {
            // ON inn vent
            if(Last.high_inn >= VENT_ON)
            {
                tt = Last.temp_top - Last.temp_bot;
                tt = delta - tt;
                if(tt > 0.2) Now.high_inn ++;
            }
        }
        else
        {
            if(delta < 3.0)
            {
                // decrease inn vent
                if(Last.high_inn > VENT_ON) Now.high_inn --;
            }
        }
    }
    if(Now.high_inn > VENT_MAX) Now.high_inn = VENT_MAX;
    if(Now.high_out > VENT_MAX) Now.high_out = VENT_MAX;
    if(Now.high_inn - Last.high_inn)   Vent.set_inn_vent(Now.high_inn);
    if(Now.high_out - Last.high_out)   Vent.set_out_vent(Now.high_out);

    if(dout) fprintf(dout, "B: %.1f T: %.1f ou: %i in: %i\n",
        Now.temp_bot, Now.temp_top, Now.high_out, Now.high_inn);
    //if(!(sec5 % SEC5)) if(f5sec) fprintf(f5sec, "B: %.1f T: %.1f\n",         Now.temp_bot, Now.temp_top);

    Last.temp_top = Now.temp_top;
    Last.temp_bot = Now.temp_bot;
    Last.high_inn = Now.high_inn;
    Last.high_out = Now.high_out;
    return 0;
}


/** -------------------------------------------------------
 *  \brief check_current
 *  \return 0 - OK\par
 *          -1 - no input master file\par
 */
int read_threshould_from_file()
{
    int res = 0;

    res = read_master_from_file(Input);

    printf("Errors in read %s file: %i\n", MASTER_FILE, res);
    if(dout) fprintf(dout, "Errors in read %s file: %i\n", MASTER_FILE, res);
    printf("Input: master %d, gmaster %d\n", Input.master, Input.gmaster);
    if(dout) fprintf(dout, "Input: master %d, gmaster %d\n", Input.master, Input.gmaster);

    //print_param(Input);

    if(res == -1) // NO input master file
        return -1;
    if(!res)  // no errors
    {
        Work.master  = Input.master;
        Work.gmaster = Input.gmaster;
    }

    printf("\n Work: master %d, gmaster %d\n", Work.master, Work.gmaster);
    if(dout) fprintf(dout,"\nWork: master %d, gmaster %d\n", Work.master, Work.gmaster);
    //print_param(Work);

    return 0;
}


//  -------------------------------------------------------
/** -------------------------------------------------------
 *  \brief barometers initialization
 *  \return NumBar
 */
unsigned int bars_init(led &LED, barometer Bar[])
{
    int   i   = 0;
    int   tmp = 0;

    LED.bar_onoff(1);  // on barometer 0
    LED.bar_onoff(0);  // on barometer

    // set BaseAddresses to barometers
    tmp = BaseAddrLED + AddrBAROMETER;
    for(i = 0; i < NumBar; i++)
    {
        Bar[i].SetAddr(tmp++);
        if(Bar[i].read_coef())
        {
            printf("\nBar[%i] : Error in read_koef()!!", i);
            NumBar = i;
        }
        else
            printf("\nBar[%i] : OK in read_koef()!!", i);
    }

    // == end of barometers initialization ============
    return NumBar;
}


/** -------------------------------------------------------
 *  \brief read barometers
 *  \return 0
 */
unsigned int bars_read(led &LED, barometer Bar[], char *message_out)
{
    unsigned short  i = 0;
    char message[1024]= {""};
    char tmp[1024]= {""};

    strcpy(message_out, "");
    for(i = 0; i < NumBar; i++)
    {
        strcpy(message, "");
        strcpy(tmp, "");

        LED.bar_onoff(1);
        Bar[i].read_bar_temp(message);
        LED.bar_onoff(0);

        sprintf(tmp, "%i %s", i, message);
        strcat(message_out, tmp);
    }

    return 0;
}


/** -------------------------------------------------------
 *  \brief make_calibration
 */
void make_calibration(led &LED)
{
    int begin = 130, end = 0; // !! begin > end !!!
    int num = 100;            // number of light spots
    int ii = 0;

    for(ii = begin; ii < end; ii -= 5)
    {
        if(LED.set_chan_amp(8, ii))  printf("\nError in set_chan_amp!!  ii = %d", ii);
        else                         printf("lamp amplitude %d set\n", ii);
        sleep(60);      // 60 sec
        LED.led_pulseN_sec(num);
    }
    if(LED.set_chan_amp(8, 255))  printf("\nError in set_chan_amp!!  ii = %d", ii);
}


/** -------------------------------------------------------
 *  \brief print EventNumber to file
 */
int save_eventnumber_to_file()
{
    FILE *fevent;
    char info[200] = {0};

    if ( (fevent = fopen(ENUM_FILE,"w")) == NULL)
    {
        sprintf(info, "Error! File Data file %s not open!", ENUM_FILE);
        print_debug(info);
    }
    else
    {
        fprintf(fevent, "%d", EventNumber);
        sprintf(info, "EventNumber %d printed to file %s",  EventNumber, ENUM_FILE);
        //print_debug(info);
        fclose(fevent);
    }
    return 0;
}


/** -------------------------------------------------------
 *  \brief get file size
 */
long getFileSize(const char *fileName)
{
    struct stat file_stat;
    if( stat(fileName, &file_stat) == -1)  // file no exists
        return 0;
    return file_stat.st_size;
}


/** -------------------------------------------------------
 *  \brief search if file log.txt changes
 * 
 * Now this programm does not change this file!
 */
int search_competitor()
{
    long  fsize = -1, fsize1 = -1;
    const char filelog[100] = {LOG_FILE};

    // ----------- read log.txt file size
    fsize = getFileSize(filelog);
    fsize = getFileSize(filelog);
    sprintf(debug, "filesize = %ld\n", fsize);
    print_debug(debug);

    sleep(6);
    fsize1 = getFileSize(filelog);
    sprintf(debug, "filesize = %ld\n", fsize1);
    print_debug(debug);

    fsize -= fsize1;

    if(fsize > 0)
    {
        sprintf(debug, "filesize delta = %ld\n", fsize);
        print_debug(debug);
        sprintf(debug, "File %s changes! There is a competitor! \n", LOG_FILE);
        print_debug(debug);
        return 1;
    }

    sprintf(debug, "File %s doesn`t change! There is NOT a process-competitor! \n", LOG_FILE);
    print_debug(debug);
    return 0;
}


/** -------------------------------------------------------
 *  \brief kill of another 'diag' process
 */
int kill_competitor(void)
{
    char syscom[100] = {0};
    if( search_competitor())
    {
        sprintf(syscom,"killall -9 %s; echo $?", PROC_TO_KILL);
        system(syscom);
        sleep(19);
    }
    return 0;
}


/** -------------------------------------------------------
 *  \brief read command file "command.in"
 *  \return -1 - error in file command.in reading\par
 *          -2 - error in file  clearing\par
 *          0 - file size = 0
 *          1 - command "exit" in file\par
 *          2 - command "levels"\par
 *          4 - command "high_off" in file\par
 *          5 - command "led_off" in file - NOT SUPPORTED\par
 */
int read_command_file(void)
{
    FILE *fcomin = NULL, *fcomout = NULL;
    time_t t;
    long fsize = -1;
    int  comnum = 0, il = 0, flag = 0;
    char filename[100] = "command.in";
    char fileout[100]  = "command.out";
    char line[80]   = "";
    char debug[80]  = "";
    char commands[6][15] = {"exit","exit","levels","start","high_off","led_off"};

    // ----------- check command file size
    fsize = getFileSize(filename);
    if(!fsize) return 0;

    // ----- open command file if its size > 3 byte
    if( (fcomin = fopen(filename, "r")) == NULL) 
    {
        sprintf(debug, "\n Error in command.in file reading! \n");
        print_debug(debug);
        return -1;
    }
    if( (fcomout = fopen(fileout, "a")) == NULL) 
    {
        sprintf(debug, "\n Error in open command.out file to write! \n");
        print_debug(debug);
    }
    else
    {
        time(&t);
        fprintf(fcomout, "\n\n%s\n", ctime(&t) );
    }
    sprintf(debug, "\nRead command file\n%s\n", ctime(&t) );
    print_debug(debug);

    // ------------ read command file
    while( fgets(line, sizeof(line), fcomin) != NULL )
    {
        if( (line[0] == '\n') || (line[0] == '/') )
            continue;

        // --- if line contains command - save command in commandin[]
        flag = 0;
        for(il = 1; il <= 5; il++)
        {
            if(strstr(line, commands[il]))
            {
                flag = 1;
                comnum ++;
                commandin[comnum] = il;
            }
        }
        commandin[0] = comnum;

        if(flag)
        {
            if(fcomout) fprintf(fcomout, "%s", line);
            sprintf(debug, "commandin: %s command = %d \"%s\"\n", 
                            line, commandin[comnum], commands[commandin[comnum]] );
            print_debug(debug);
        }
    }
    fclose(fcomin);
    if(fcomout) fclose(fcomout);

    // ---------- clear command.in file
    if( (fcomin = fopen(filename, "w")) == NULL) 
    {
        sprintf(debug, "\n Error in command.in file re_writing! \n");
        print_debug(debug);
        return -2;
    }
    fclose(fcomin);

    return comnum;
}


/** -------------------------------------------------------
 *  \brief print log parameters to file
 *  \deprecated Старая функция. Закомментирована.
 */
int print_log_parameters(FILE *fileout)
{
    /*
    time_t t;

    time(&t);
    fprintf(fileout, "%s\n%s\n%s\n",       ctime(&t), gps_out, bar_out); //, incl_out);
    fprintf(fileout, "%s\n%s%s\n%s\n%s\n", comp_out, adc_out, pwr_out, msc_out, led_out);
    fprintf(fileout, "-----------------------\n");
    */
    return 0;
}


/** -------------------------------------------------------
 *  \brief print every min log parameters to file
 */
int print_everymin_parameters(FILE *fileout)
{
    time_t t;

    time(&t);
    fprintf(fileout, "%s%s\n%s", ctime(&t), vip_out, bar_out); //, incl_out);
    fprintf(fileout, "%s\n%s%s\n", led_out, adc_out, pwr_out);
    //fprintf(fileout, "-----------------------\n");
    return 0;
}
// ----------------------------------------------
