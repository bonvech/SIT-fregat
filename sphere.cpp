/**
 * \file  sphere.cpp
 * \author ptrwww@mail.ru
 * \version 2.0
 * \brief Программа автономной работы установки СФЕРА в Тунке
 * \date  октябрь 2019 года
 *
 * Данный файл содержит в себе основную программу для автономной работы 
 * установки СФЕРА-2 c SiPM в наземном варианте в Тункинской долине.
 *
 * Убраны модули GPS, компас и инклинометер. 49 измерительных каналов (8 плат).
 * Канал 64 принимает сихроимпульс от установки Тунка.
*/


#define LINUX 1   ///< Flag to compile for Linux
#define NO_SCOUT  ///< No scout current before On high voltage
//#define NO_BOOT
//#define NO_SET_HIGH
//#define NO_SET_LEVELS


#include "include/include.h"

int main (void)
{
    int  res   = 0, ii = 0;
    long delta = 0;
    struct timeval tv0, tv1, tv0sec;
    struct tm* ptm0;
    time_t time0;
    char info[200];

    Last.temp_top = 0;
    Last.temp_bot = 0;
    Last.high_inn = 0;
    Last.high_out = 0;

    // -------------------------------------
    /// 1) Initialization 
    // -------------------------------------
    GetIOPortsAccess();
    open_debug_file();
    print_time();

    /// \todo чтение файла пока не появится корректная дата начала и конца
    res = read_input_files();
    if(res < 0)
    {
        printf("Errors in reading input TIME file!");
        return -1;
    }
    //printf("Onscreen: %i", Work.onscreen);

    // -------------------------------------
    /// Open files for online telemetry monitoring 
    res = open_telemetry_files();
    if(res < 0)
    {
        sprintf(info, "Errors to open telemetry files %d!", res);
        print_debug(info);
    }

    // -------------------------------------
    /// Kill another programm that works with apparatus
    if(search_competitor()) // if LOG_FILE is changing
    {
        kill_competitor();  // alpha:  kill process PROC_TO_KILL 
    }

    // -------------------------------------
    /// 2) Declare and init devices  
    // -------------------------------------

    // print info status
    sprintf(msc_out, "Status: Initialization");
    print_status_to_file();

    SiPM  vip;              // class vip, on  // init vip
    fadc_board Fadc ;       // class fadc on  // init and boot fadc
    lvps_dev   Vent;        // class lvps - ventillator
    trigger_board Trigger;  // class trigger // boot and init trigger
    led  LED;               // LED calibration system
    barometer Bar[3];       // Three barometers

    /// Fill array TriggerOnOff according to input file
    Trigger.fill_TriggerOnOff( Work.trigger_onoff); 
    bars_init(LED, Bar);

    Every_sec(Fadc, vip, Trigger, Vent);
    Every_min_mini( Vent, LED, Bar);
    if(dout) fflush(dout);

    // -------------------------------------
    /// 3) Wait start time
    // -------------------------------------

    // print start time to info status message
    ptm0 = localtime(&Work.timeOnOff.time_on);
    strftime(info, sizeof(info), "%Y-%m-%d %H:%M:%S", ptm0);
    sprintf(msc_out, "Status: Waiting for start time: %s", info);
    print_status_to_file();

    gettimeofday(&tv1,    NULL);
    gettimeofday(&tv0sec, NULL);
    time(&time0);
    sprintf(info, "start= %ld, now= %ld time0= %ld\n", Work.timeOnOff.time_on, tv1.tv_sec, time0);
    print_debug(info);

    while( (Work.wait) && (tv1.tv_sec < Work.timeOnOff.time_on))
    {
        gettimeofday(&tv1, NULL);
        /// Every sec routine
        if((tv1.tv_sec - tv0sec.tv_sec) > 0)
        {
            Every_sec(Fadc, vip, Trigger, Vent);
            gettimeofday(&tv0sec, NULL);
        }

        /// Every minute:
        if(!(sec5 % SEC60))
        {
            Every_min_mini( Vent, LED, Bar);
        }

        ///  Read command file
        res = 0;
        res = read_command_file();
        if(res > 0)
        {
            sprintf(info, "read_command_file: %d commands\n", res);
            print_debug(info);
            for(ii = 1; ii <= res; ii++)
            {
                strcpy(info,"");
                if(commandin[ii] == 1) // exit
                {
                    strcpy(info, "\ncommand EXIT in file\n");
                    print_debug(info);
                    goto EXIT;
                }
                if(commandin[ii] == 3) //start
                {
                    strcpy(info, "\ncommand START in file\n");
                    Work.wait = 0;
                }
                print_debug(info);
            }
        }
        //  end of read command file
    }

    // -------------------------------------
    // -------------------------------------
    ///         4) Start time 
    // -------------------------------------

    sprintf(msc_out, "Status: Start");
    print_status_to_file();

    // -------------------------------------
    ///         Set lamp on and configure LEDs
    LED.set_config_from_file();

    // -------------------------------------
    ///         Set high voltage 
    sprintf(msc_out, "Status: Set High voltage");
    print_status_to_file();

#ifndef NO_SET_HIGH
    gettimeofday(&tv0, NULL);
#ifndef NO_SCOUT
    //res = vip.high_scout(1,1);  // probe if high voltage setting is possible
    res = vip.high_scout(ADDR,SUBADDR);  // !!! 2018: SiPM: probe if high voltage setting is possible
#endif
    if(!res) vip.high();  // set high voltage
    else
    {
        sprintf(info, "No conditions for high on!!: res = %i\n", res);
        print_debug(info);
        goto EXIT;
    }
    gettimeofday(&tv1, NULL);
    delta = tv1.tv_sec - tv0.tv_sec;
    sprintf(info, "\n==high==> delta = %ld sec\n", delta);
    print_debug(info);
    if(dout) fflush(dout);
#endif

    // -------------------------------------
    ///          Set levels 
LEVELS:
    sprintf(msc_out, "Status: Set levels");
    print_status_to_file();

#ifndef NO_SET_LEVELS
    gettimeofday(&tv0, NULL);
    Fadc.levels();
    gettimeofday(&tv1, NULL);
    delta = tv1.tv_sec - tv0.tv_sec;
    printf("\n==levels==>delta = %ld sec\n", delta);
    if(dout) fprintf(dout, "\n==levels==> delta = %ld sec\n", delta);
    if(dout) fflush(dout);
#endif

    // -------------------------------------
    ///          Trigger status
    Trigger.status();

    // -------------------------------------
    ///           Open new debug file
    print_debug((char*)"debug close:\n");
    if(dout) fflush(dout);
    sleep(10);  // 10sec
    if(dout) fclose(dout);
    printf("               closed\n"); // !!!! no print to debu in this place !!!
    open_debug_file();
    print_debug((char*)"debug open! \n");
    if(dout) fflush(dout);


    // -------------------------------------
    // -------------------------------------
    ///         5) Main run 
    // -------------------------------------

    // print stop time to message
    ptm0 = localtime(&Work.timeOnOff.time_of);
    strftime(info, sizeof(info), "%Y-%m-%d %H:%M:%S", ptm0);
    sprintf(msc_out, "Status: Operation. Waiting for stop time: %s", info);
    print_status_to_file();


    // Main Operation loop
    Every_min_mini( Vent, LED, Bar);
    res = Operate(Fadc, vip, Trigger, Vent, LED, Bar);
    if(res == 2) // levels
        goto LEVELS;


    // -------------------------------------
    // -------------------------------------
    ///         6) Exit
    // -------------------------------------
EXIT:

    sprintf(msc_out, "Status: Exit. Turn off VIP and FADC");
    print_status_to_file();

#ifndef NO_OFF
    ///     Turn off VIP and FADC
    if( Work.off)
    {
        vip.turn_off();
        Fadc.turn_off_fadc_boards();
    }
    sprintf(vip_out,"Mosaic is not available now");
    sprintf(msc_out, "Status: Exit");
    print_status_to_file();
#endif

    print_debug((char*)"\ninn_vent off:\n");
    Vent.set_inn_vent(0);
    print_debug((char*)"out vent off:\n");
    Vent.set_out_vent(0);



    /// Close files
    if(dout)   fclose(dout);
    if(stderr) fclose(stderr);
    if(stdout) fclose(stdout);
    if(fkadr)  fclose(fkadr);
    if(ffmin)  fclose(ffmin);

    sprintf(msc_out, "Status: Bye! I do not work now.");
    print_status_to_file();
    if(f5sec)  fclose(f5sec);

    /// Close Ports
    CloseIOPortsAccess();

    return res;
}
