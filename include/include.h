/**
 * \file
 * Данный файл содержит в себе подключение стандартных библиотек, библиотек аппаратуры, 
 * а также определения основных констант и глобальных переменнтых
 *
 * \date июнь 2019 года
 * \brief Определение констант, подключение библиотек.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/io.h>
#include <sys/perm.h>


#define PROC_TO_KILL   "diag"                          ///< filename of process to kill

#define FILE_NAME      "/home/Tunka/Data/s"            ///< first part of filename to write binary data
#define LOG_FILE       "/home/Tunka/Data/log/log.txt"  ///< filename to print log
#define ENABLE_FILE    "/var/www/htdocs/enable.txt"    ///< file to enable status
#define EVERYMIN_FILE  "/var/www/htdocs/1m.data"       ///< file to write every min log
#define EVERYSEC_FILE  "/var/www/htdocs/5s.data"       ///< file to write every sec log
#define WORK_FILE      "./log/fadc_work.dat"           ///< output file to write fadc information

#define LEVELS_CONFIG_FILE  "./config/levels.config"   ///< File to read levels from
#define LEVELS_INPUT_FILE   "./config/levels.inp"      ///< File to read levels from first
#define LEVELS_LOG_FILE     "./log/levels.dat"         ///< File to write levels to


// --- for levels.cpp
#define BOARD 8       ///< number of FADC boards with pmt [1-8]
#define CHANPMT 8     ///< number of PMT channels on one FADC board
#define CHANMAX 16    ///< number of ADC channels on one FADC board

// --- for nograph.cpp
#define RG1ADDR 10  ///< address of RG1 register
#define RG0ADDR 8   ///< address of RG0 register
#define STROBB 0    ///< Strob begin // 150
#define STROBE 512  ///< Strob end   // 350


// --- for high.cpp
#define HVCHANN 64              ///< number of vip channels
#define MAXCURR 10              ///< maximum current, 100 mkA
#define WORKCURR 3              ///< work current, 30 mkA
#define LPT_IO     0x378        ///< LPT FADC input/outpu t ports
#define FADC_IO    0x320        ///< FADC input/output ports
#define BaseAddrLED  0x4320     ///< Base Address for LED 


// --- FADCBOARD
// trigger2_width + int2_width + led_delay2 (RG0,3,6,9)
// int2_width: 7=200ns (25ns x 8) - dlitelnost vorot integririvanija
/// Какая-то ширина
#define INT2_WIDTH 0x7           // !!!!!!! change here   and in levels.cpp  !!!!!!!
/// Trigger
#define TRG2 (100+0x700+0x3000)  // !!!!!!! change here   and in levels.cpp  !!!!!!!

/// glubina buffera 2-go urovnja (RG2,5,8,11) = 512
#define BUFF2  400 //510 //512

/// glubina buffera 1-go urovnja (ADC1(5,9,13)/DP, ADC3(7,11,15)/DP) = 256 \n 256 - для 2013 г. \n 230 - для случая, когда синхроимпульс выключен, сдвиг калибровочного кадра = 0
#define BUF1  130 // 230



/// Union type to conver Int to bytes
union CharInt_converter
{
    unsigned char tChar[4]; ///< 4 bytes for integer
    unsigned int  tInt;     ///< integer
} Conv; ///< name of union to use to convert int to bytes


/// Structure to hold onOff data
struct time_onoff
{
    time_t time_on; ///< on time
    time_t time_of; ///< off time
};


/// structure to hold termometeres and ventillators 
struct temp
{
    float  temp_top;         ///< top termometere
    float  temp_bot;         ///< bottom termomert
    unsigned char high_inn;  ///< inn ventilator
    unsigned char high_out;  ///< out ventillator
} Last;                      ///< structure to hold last temperature and ventillator voltage


/// Structure to hold work parameters
struct input_parameters
{
    struct time_onoff timeOnOff;       ///< Structure to hold onOff data in all parameters
    time_t   period;                   ///< period to change data files and checvk apparatus, 1200 sec
    unsigned short trigger_onoff[112]; ///< channels trigger array
    unsigned short hvtrig[112];        ///< hv triggers
    unsigned short autolevels;         ///< Auto set levels flag: 1 - set auto, 0 - read from file
    unsigned short buf1;    ///< Trigger shift size
    unsigned short buf2;    ///< FADC Buffer size
    unsigned short master;  ///< trigger
    unsigned short gmaster; ///< global trigger
    unsigned short lmin;    ///< min level in fadc channel
    unsigned short lmax;    ///< max level in fadc channel
    unsigned char  umax;    ///< max U_high in hv channel
    unsigned char  umin;    ///< min U_high in hv channel
    float rate;             ///< rate in one channel
    float maxcur;           ///< maximal current
    float workcur;          ///< work current
    int hvchan;             ///< number of vip channels
    int onscreen;           ///< printf to screen(1) or no (0)
    int wait;               ///< wait begin time or start now
    int off;                ///< off apparatus or no
} Input,  ///< Structure to read from config files
Default,  ///< Default parameters
Work;     ///< Structure of parameters to work with


FILE *dout;   ///< debug out file
FILE *fwork;  ///< debug out file
FILE *fout;   ///< data out file
FILE *ffmin;  ///< every min file
FILE *f5sec;  ///< every 5min file


int  EventNumber = 0;       ///< number of current event
int  LastEventNumber = 0;   ///< number of start event of current minute
int  EventNumberBefore = 0; ///< number of start event of current period
int  TunkaNumber = 0;       ///< last syncro number from HISCORE

int  commandin[21] = {0};   ///< commands from file string


char vip_out[1024]  = "Mosaic is not available now";  ///< vip ADC info string
char time_out[100]  = "time_out init string";         ///< time_ms info string 
char adc_out[1024]  = "adc_out init string";          ///< Vent ADC info string
char pwr_out[1024]  = "pwr_out init string";          ///< power info string
char msc_out[1024]  = "msc_out init string";          ///< info string
char bar_out[1024]  = "bar_out init string";          ///< barometer info string
char freq_out[256]  = "frequency init string";        ///< event frequency info string
char kadr_out[256]  = "event counter init string";    ///< event frequency info string
char led_out[4096]  = "led_out init string";          ///< LED info string
char gps_bstamp[30] = "gps_bstamp init string";       ///< gps_bstamp info string
char gps_sstamp[30] = "gps_sstamp init string";       ///< gps_sstamp info string
char chfreq_out[400] = "channel frequency init str";  ///< channel frequency info string - 321 byte
char chtext_out[400] = "channel numbering text str";  ///< channel number text info string - 321 byte
char tunka_out[128] = "Tunka syncro text str";  ///< channel number text info string - 321 byte


/**
 * Fregat input files
 */
#include "bit.cpp"
#include "ioports.cpp"
#include "lvpsdev.cpp"
#include "fadcbord.cpp" //includes "levels.cpp"
#include "trigger.cpp"
#include "sipm.cpp"
#include "readinp.cpp"
#include "led.cpp"
#include "bar.cpp"
#include "files.cpp"
#include "operate.cpp" // includes "nograph.cpp"
