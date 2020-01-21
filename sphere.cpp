/**
 * \file   sphere.cpp
 * \author ptrwww@mail.ru
 * \version 3.0
 * \brief Программа автономной работы установки СФЕРА в Тунке
 * \date  октябрь-декабрь 2019 года
 *
 * Данный файл содержит основную программу для автономной работы
 * установки SIT c мозаикой SiPM в наземном варианте в Тункинской долине.
 *
 * Установка работает по расписанию включений в файле config/sdate.inp, пока оно имеется.
 *
 * По сравнению с аэростатным вариантом убраны модули GPS, компас и инклинометер. 
 * 49 измерительных каналов (8 плат).
 * Канал 64 принимает сихроимпульс от установки Тунка.
 * Канал 29 дублируется на 39 канал.
*/


#define LINUX 1   ///< Flag to compile for Linux
#define NO_SCOUT  ///< No scout current before On high voltage
//#define NO_BOOT
//#define NO_SET_HIGH
//#define NO_SET_LEVELS

#include "include/include.h"


int open_ports_and_files();
int wait_start_time(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent, led &LED, barometer Bar[]);
int wait_enable(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent, led &LED, barometer Bar[]);
int prepare_for_measurements(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent, led &LED, barometer Bar[]);
void close_ports_and_files();


/** ------------------------------------------------------------------------
 *
 *
 */
int main (void)
{
    // -------------------------------------
    ///        1) Initialization
    // -------------------------------------

    open_ports_and_files();

    /// \todo чтение файла пока не появится корректная дата начала и конца
    int res = read_input_files();
    if(res < 0)
    {
        print_debug((char*)"Errors in reading input TIME file!");
        close_ports_and_files();
        return -1;
    }


    // -------------------------------------
    ///         2) Declare and init devices  
    // -------------------------------------

    SiPM       vip;         // class vip, on  // init vip
    fadc_board Fadc;        // class fadc on  // init and boot fadc
    lvps_dev   Vent;        // class lvps - ventillator
    trigger_board Trigger;  // class trigger // boot and init trigger
    led       LED;          // LED calibration system
    barometer Bar[NumBar];  // Two barometers

    /// Fill array TriggerOnOff according to input file
    Trigger.fill_TriggerOnOff( Work.trigger_onoff);
    bars_init(LED, Bar);

    Every_sec_mini(Fadc, Trigger, Vent);
    Every_min_mini( vip, Vent, LED, Bar);
    if(dout) fflush(dout);


    // -------------------------------------
    ///        3.0) Wait ENABLE signal
    // -------------------------------------

READ_START_TIME:

    //  Read start time from file
    Work.timeOnOff = read_date_from_file();
    if(dout) fflush(dout);
    if(Work.timeOnOff.time_on < 10)
    {
        print_debug((char*)"\n\nDATA PANIC!!!!!\nError in reading DATE information!!!\n Check file \"config/sdate.inp\"!");
        goto EXIT;
    }


    // -------------------------------------
    ///        3.1) Wait ENABLE signal or stop time
    // -------------------------------------

WAIT_ENABLE:

    res = wait_enable(Fadc, vip, Trigger, Vent, LED, Bar);
    if(dout) fflush(dout);
    if(res == 1) // exit command
        goto EXIT;
    if(res == 2) // Time off
    {
        Fadc.turn_off_fadc_boards();
        goto READ_START_TIME;
    }

    // Enable to run or command start in command file
    sprintf(msc_out, "Status: Boot FADC");
    print_status_to_file();
    Fadc.init();
    Fadc.boot();


    // -------------------------------------
    ///        3.2) Wait start time
    // -------------------------------------

    res = wait_start_time(Fadc, vip, Trigger, Vent, LED, Bar);
    if(dout) fflush(dout);
    if(res == 1)  // Exit command
        goto EXIT;
    if(res == 2)  // DISABLE
        goto WAIT_ENABLE;


    // -------------------------------------
    ///         4) Start time
    //  Set high, set levels
    // -------------------------------------
LEVELS:

    res = prepare_for_measurements(Fadc, vip, Trigger, Vent, LED, Bar);
    if(dout) fflush(dout);
    if(res == 1) // Exit command
        goto EXIT;

    // -------------------------------------
    ///         5) Main run 
    // -------------------------------------

    // Main Operation loop
    res = Operate(Fadc, vip, Trigger, Vent, LED, Bar);

    // after exit from main loop:
    if(dout) fflush(dout);

    if(res == 2) // levels
    {
        sprintf(msc_out, "Status: Command to set levels received");
        print_status_to_file();
        goto LEVELS;
    }

    vip.turn_off();
    if(res == 3) // Disable
    {
        sprintf(msc_out, "Status: DISABLE reseived");
        print_status_to_file();
        goto WAIT_ENABLE;
    }
    if(res == 0) // Time off
    {
        sprintf(msc_out, "Status: Time OFF");
        print_status_to_file();

        Fadc.turn_off_fadc_boards();
        write_disable();
        FileNum = 0;

        //  Open new debug file
        print_debug((char*)"debug close:\n");
        if(dout) fflush(dout);
        sleep(10);  // 10sec
        if(dout) fclose(dout);
        printf("               closed\n");  // !!!! no print to debug in this place !!!

        open_debug_file();
        print_debug((char*)"debug open! \n");
        if(dout) fflush(dout);

        goto WAIT_ENABLE;
    }


    // -------------------------------------
    // -------------------------------------
    ///         6) Exit
    // -------------------------------------
EXIT:

    sprintf(msc_out, "Status: Exit. Turn off VIP and FADC");
    print_status_to_file();
    if(dout) fflush(dout);

    ///     Turn off VIP and FADC
    if( Work.off)
    {
        vip.turn_off();
        Fadc.turn_off_fadc_boards();
    }
    sprintf(vip_out, "Mosaic is not available now");
    sprintf(msc_out, "Status: Exit");
    print_status_to_file();

    print_debug((char*)"\ninn_vent off:\n");
    Vent.set_inn_vent(0);
    print_debug((char*)"out vent off:\n");
    Vent.set_out_vent(0);

    
    // -------------------------------------
    ///         6) Close ports and files
    // -------------------------------------
    close_ports_and_files();

    return res;
}


/* ----------------------------------------------- */
/* ----------------------------------------------- */
/* ----------------------------------------------- */
/* ----------------------------------------------- */

/** -----------------------------------------------
 * Initialization procedures before programm start
 */
int open_ports_and_files()
{
    int  res = 0;

    //  print info status
    sprintf(msc_out, "Status: Open ports and files");
    print_status_to_file();


    Last.temp_top = 0;
    Last.temp_bot = 0;
    Last.high_inn = 0;
    Last.high_out = 0;

    GetIOPortsAccess();
    open_debug_file();


    // -------------------------------------
    // Open files for online telemetry monitoring 
    res = open_telemetry_files();
    if(res < 0)
    {
        sprintf(debug, "\n\nErrors to open telemetry files >%d!\n\n", res);
        print_debug(debug);
    }

    // -------------------------------------
    // Kill another programm that works with apparatus
    if(search_competitor()) // if LOG_FILE is changing
    {
        kill_competitor();  // alpha:  kill process PROC_TO_KILL 
    }

    // fill chtext_out[] string
    fill_chtext();

    //  print info status
    sprintf(msc_out, "Status: Initialization");
    print_status_to_file();

    return 0;
}


/** -----------------------------------------------
 * Wait programm start
 * \return 0 - time to start\n
 *         1 - time to exit: exit command in command file\n
 *         2 - DISABLE status
 * 
 */
int wait_start_time(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent, led &LED, barometer Bar[])
{
    struct tm* ptm0;
    time_t time0;
    char info[200] = "Info init string";
    struct timeval tv1, tv0sec, tv0min;
    int res = 0;

    // print start time to info status message
    ptm0 = localtime(&Work.timeOnOff.time_on);
    strftime(info, sizeof(info), "%Y-%m-%d %H:%M:%S", ptm0);
    sprintf(msc_out, "Status: Waiting for start time: %s", info);
    print_status_to_file();

    Every_min_mini(vip, Vent, LED, Bar);

    gettimeofday(&tv1,    NULL);
    tv0sec.tv_sec = tv1.tv_sec;
    tv0min.tv_sec = tv1.tv_sec;

    time(&time0);
    sprintf(info, "\nstart= %ld, now= %ld time0= %ld\n", Work.timeOnOff.time_on, tv1.tv_sec, time0);
    print_debug(info);

    while( (Work.wait) && (tv1.tv_sec < Work.timeOnOff.time_on))
    {
        gettimeofday(&tv1, NULL);
        /// Every sec routine
        if((tv1.tv_sec - tv0sec.tv_sec) > 0)
        {
            Every_sec_mini(Fadc, Trigger, Vent);
            //gettimeofday(&tv0sec, NULL);
            tv0sec.tv_sec = tv1.tv_sec;
        }

        /// Every minute:
        if((tv1.tv_sec-tv0min.tv_sec) > SEC60)
        {
            Every_min_mini(vip, Vent, LED, Bar);
            tv0min.tv_sec = tv1.tv_sec;
        }

        ///  Read status file
        res = read_enable();
        // if DISABLE - exit
        if(res == 0)
            return 2;

        ///  Read command file
        res = 0;
        res = read_command_file();
        if(res > 0)
        {
            sprintf(info, "read_command_file: %d commands\n", res);
            print_debug(info);
            for(int ii = 1; ii <= res; ii++)
            {
                strcpy(info,"");
                if(commandin[ii] == 1) // exit
                {
                    strcpy(info, "\ncommand EXIT in file\n");
                    print_debug(info);
                    if(dout) fflush(dout);
                    //goto EXIT;
                    return 1;
                }
                if(commandin[ii] == 3) //start
                {
                    strcpy(info, "\ncommand START in file\n");
                    Work.wait = 0;
                }
                print_debug(info);
                if(dout) fflush(dout);
            }
        }
        //  end of read command file
    }
    print_debug((char*) "Exit from wait_start_time()\n");
    if(dout) fflush(dout);
    return 0;
}


/** -----------------------------------------------
 * \brief Wait enable status\n
 * Read enable status from enable file or command file to start now.
 * \return 0 - Enable to run or command start in command file\n
 *         1 - command to exit\n
 *         2 - time if off
 */
int wait_enable(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent, led &LED, barometer Bar[])
{
    int    res = 0;
    char   info[200] = "Info init string";
    struct tm*     ptm0;
    struct timeval tv1, tv0sec, tv0min;

    kadr_out[0] = '\0';
    freq_out[0] = '\0';
    chfreq_out[0] = 0;


    Every_min_mini(vip, Vent, LED, Bar);


    // -----------------------------------------------
    // print status to info status message
    sprintf(msc_out, "Status: Waiting for ENABLE ");

    gettimeofday(&tv1, NULL);
    // if now is before start time
    if(tv1.tv_sec < Work.timeOnOff.time_on)
    {
        // print start time to info status message
        ptm0 = localtime(&Work.timeOnOff.time_on);
        strftime(info, sizeof(info), "%Y-%m-%d %H:%M:%S", ptm0);
        strcat(msc_out, "and Start time: ");
        strcat(msc_out, info);
    }

    // print stop time
    strcat(msc_out, "  or Stop time: ");
    ptm0 = localtime(&Work.timeOnOff.time_of);
    strftime(info, sizeof(info), "%Y-%m-%d %H:%M:%S", ptm0);
    strcat(msc_out, info);
    print_status_to_file();

    // start waiting loop
    gettimeofday(&tv1, NULL);
    tv0sec.tv_sec = tv1.tv_sec;
    tv0min.tv_sec = tv1.tv_sec;
    while(tv1.tv_sec < Work.timeOnOff.time_of)
    {
        gettimeofday(&tv1, NULL);

        ///  Read status file
        res = read_enable();
        // if ENABLE - exit
        if(res == 1)
            return 0;

        /// Every sec routine
        if((tv1.tv_sec - tv0sec.tv_sec) > 0)
        {
            Every_sec_mini(Fadc, Trigger, Vent);
            //gettimeofday(&tv0sec, NULL);
            tv0sec.tv_sec = tv1.tv_sec;
        }

        /// Every minute:
        //if(!(sec5 % SEC60))
        if((tv1.tv_sec-tv0min.tv_sec) > SEC60)
        {
            Every_min_mini(vip, Vent, LED, Bar);
            tv0min.tv_sec = tv1.tv_sec;
        }

        ///  Read command file
        res = 0;
        res = read_command_file();
        if(res > 0)
        {
            sprintf(info, "read_command_file: %d commands\n", res);
            print_debug(info);
            for(int ii = 1; ii <= res; ii++)
            {
                strcpy(info,"");
                if(commandin[ii] == 1) // exit
                {
                    strcpy(info, "\ncommand EXIT in file\n");
                    print_debug(info);
                    //goto EXIT;
                    return 1;
                }
                if(commandin[ii] == 3) //start
                {
                    strcpy(info, "\ncommand START in file\n");
                    print_debug(info);
                    Work.wait = 0;
                    return 0;
                }
                print_debug(info);
            }
        }
        //  end of read command file
    }
    print_debug((char*) "Exit form wait_enable()\n");
    if(dout) fflush(dout);
    return 2; // time off
}


/** -----------------------------------------------
 * Procedures before measurement start.
 * Set high, set levels.
 * \return 0 OK\n
 *         1 no condition for high
 */
int prepare_for_measurements(fadc_board &Fadc, SiPM &vip, trigger_board &Trigger, lvps_dev &Vent, led &LED, barometer Bar[])
{
    //int  res = 0;
    long delta = 0;
    struct timeval tv0, tv1;

    sprintf(msc_out, "Status: Start");
    print_status_to_file();


    // read and set input parameters from file
    read_and_set_params_from_file();

    // -------------------------------------
    ///         Set lamp on and configure LEDs
    LED.set_config_from_file();

    // -------------------------------------
    ///         Set high voltage 
    sprintf(msc_out, "Status: Set High voltage");
    print_status_to_file();
    gettimeofday(&tv0, NULL);

#ifndef NO_SCOUT
    // !!! 2018: SiPM: probe if high voltage setting is possible
    if( res = vip.high_scout(ADDR,SUBADDR) ) // errors
    {
        sprintf(debug, "No conditions for high on!!: res = %i\n", res);
        print_debug(debug);
        //goto EXIT;
        return 1;
    }
#endif

    // set high voltage
    vip.high();  // set high voltage

    gettimeofday(&tv1, NULL);
    delta = tv1.tv_sec - tv0.tv_sec;
    sprintf(debug, "\n==high==> delta = %ld sec\n", delta);
    print_debug(debug);
    if(dout) fflush(dout);

    Every_min_mini(vip, Vent, LED, Bar);

    // -------------------------------------
    ///          Set levels 

    sprintf(msc_out, "Status: Set levels");
    print_status_to_file();

    Fadc.levels();
//     gettimeofday(&tv0, NULL);
// 
//     Fadc.levels();
// 
//     gettimeofday(&tv1, NULL);
//     delta = tv1.tv_sec - tv0.tv_sec;
//     printf("\n==levels==>delta = %ld sec\n", delta);
//     if(dout) fprintf(dout, "\n==levels==> delta = %ld sec\n", delta);
//     if(dout) fflush(dout);

    // -------------------------------------
    ///          Trigger status
    Trigger.status();

    // -------------------------------------
    ///           Open new debug file
    print_debug((char*)"debug close:\n");
    if(dout) fflush(dout);
    sleep(10);  // 10sec
    if(dout) fclose(dout);
    printf("               closed\n"); // !!!! no print to debug in this place !!!

    open_debug_file();
    print_debug((char*)"debug open! \n");
    if(dout) fflush(dout);

    return 0;
}


/** -----------------------------------------------
 * Finish procedures after programm finish
 */
void close_ports_and_files()
{
    /// Close Ports
    CloseIOPortsAccess();

    sprintf(msc_out, "Status: Bye! I do not work now.");
    print_status_to_file();

    /// Close files
    if(f5sec)  fclose(f5sec);
    if(ffmin)  fclose(ffmin);
    if(dout)   fclose(dout);
    //if(stderr) fclose(stderr);
}
