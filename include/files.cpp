/**
 * \file files.cpp
 * \brief Функции для работы с файлами
 *
 * Данный файл содержит в себе определения функций, работающих с файлами.
*/


// extern functions and variables
void time_to_file();
void get_time_ms();
void get_and_print_time();
extern int FileNum;


// this file functions
int read_enable();
int write_disable();

void print_debug(char* message);
void print_status_to_file();
unsigned int init_data_file(FILE* file, fadc_board &Fadc);
char new_filename(char *filename, unsigned int filenum);
void time_to_file();
void timestamp_to_file(FILE *ff);
int open_debug_file();
int open_data_file(fadc_board &Fadc);
int open_telemetry_files();



/** -------------------------------------------------------
 * \brief Read enable message from file.
 * \return 0    Disable status\n
 *         1    Enable status\n
 *        -1    Error in file reading
 */
int read_enable()
{
    FILE *fin;
    char line[200];
    int status = 0;


    if((fin = fopen(ENABLE_FILE , "rt")) == NULL)
    {
        if(dout) fprintf(dout, "\nEnable status file %s is not open!", ENABLE_FILE);
        return -1;
    }

    while( fgets(line, sizeof(line), fin) != NULL )
    {
        if( (line[0] == '\n') || (line[0] == '/') )
        {
            continue;
        }

        if(strstr(line, "Disable"))
            status = 0;
        if(strstr(line, "Enable"))
            status = 1;
    }

    fclose(fin);
    return status;
}


/** -------------------------------------------------------
 * \brief Read enable message from file.
 * \return 0    OK in writing Disable status\n
 *        -1    Error in file reading
 */
int write_disable()
{
    FILE * fout;
    int res = 0;

    res = read_enable();

    // if "Disable" - exit
    if(res == 0)
        return 0;

    // write "Disable"
    if((fout = fopen(ENABLE_FILE , "wt")) == NULL)
    {
        if(dout) fprintf(dout, "\nEnable status file %s is not open!", ENABLE_FILE);
        return -1;
    }

    fprintf(fout, "Disabled\n");
    fclose(fout);
    return 0;
}


/** -------------------------------------------------------
 *  \brief Print debug message to stdout and debug file.
 */
void print_debug(char* message)
{
    if(stdout != NULL)  fprintf(stdout,"%s", message);
    if(dout   != NULL)  fprintf(dout,  "%s", message);
}


/** -------------------------------------------------------
 * Print status line to 5sec file
 */
void print_status_to_file()
{
    get_time_ms();
    if(f5sec) f5sec = freopen(EVERYSEC_FILE, "wt", f5sec);
    else      f5sec =   fopen(EVERYSEC_FILE, "wt");

    if(f5sec == NULL)
        return;

    fprintf(f5sec,  "%s\n%s\n", time_out, msc_out);
    //fprintf(dout,  "%s\n%s\n", time_out, msc_out);
    fflush(f5sec);
}


/** -------------------------------------------------------
 *  Print to binary datafile begining some information about current detector configuration:
 *  BUF2, CHANMAX, 
 * AddrOn - number of fadc plates
 * \param file - FILE to write information
 * \param Fadc - FADC object
 * \return 0 - OK\n
 *         1 - error in file
 */
unsigned int init_data_file(FILE* file, fadc_board &Fadc)
{
    if( !file ) return 1; // no data file is open

    time_to_file();

    ///     Print BUF2 and CHANMAX to file
    Conv.tInt = Fadc.Buf2; //BUF2;
    fprintf(file, "b%c%c", Conv.tChar[1], Conv.tChar[0]);
    Conv.tInt = CHANMAX;
    fprintf(file, "c%c%c", Conv.tChar[1], Conv.tChar[0]);

    ///     Print fadc boards number to file
    Conv.tInt = Fadc.AddrOn[0];
    fprintf(file, "a%c%c", Conv.tChar[1], Conv.tChar[0]);
    fflush(file);

    return 0;
}


/** -------------------------------------------------------
 * \brief Generation of new outdata filename\n
 *
 *  Generation of new outdata filename
 *
 * \param *filename  строка для хранения нового имени файла
 * \param filenum    номер файла данных
 * example of using:
 *  \code
        char filename[100] = {"\0"};
        char f = *filename;
        f = new_filename(filename, num);
        printf("main: %s\n", filename);
 *  \endcode
*/
char new_filename(char *filename, unsigned int filenum)
{
    char time_string[40];
    struct timeval tv;
    struct tm* ptm;

    // make file name
    strcpy(filename, FILE_NAME);
    gettimeofday(&tv, NULL);
    ptm = localtime (&tv.tv_sec);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d_%H_%M", ptm);
    strcat(filename, time_string);

    sprintf(filename, "%s.%i", filename, filenum);
    printf("nf: %s\n", filename);

    return *filename;
}


/** -------------------------------------------------------
 * Prints current time to current binary data file *fout.
*/
void time_to_file()
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    Conv.tInt = tv.tv_sec;
    if(fout)   fprintf(fout, "t%c%c%c%c", Conv.tChar[3], Conv.tChar[2], Conv.tChar[1], Conv.tChar[0]);
    if(stdout) fprintf(stdout, "t-%3i-%3i-%3i-%3i\n", Conv.tChar[3], Conv.tChar[2], Conv.tChar[1], Conv.tChar[0]);
}


/** -------------------------------------------------------
 * Prints current time in seconds to text file *ff
 * \param *ff  file to print timestamp
*/
void timestamp_to_file(FILE *ff)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    //Conv.tInt = tv.tv_sec;
    if(ff)  fprintf(ff, "<t>%ld ", tv.tv_sec);  // Conv.tChar[3], Conv.tChar[2], Conv.tChar[1], Conv.tChar[0]);
}


/** -------------------------------------------------------
 * Open debug file with current time in filename
*/
int open_debug_file()
{
    char filename[100] = {""};
    char info[150] = {""};

    //printf("open_debug_file_start"); fflush(stdout);
    new_filename(filename, 0);
    //printf("%s",filename);

    strcat(filename,".dbg");
    if((dout = fopen(filename, "w")) == NULL)
    {
        fprintf(stderr, "\nDebug file %s is not open!", filename);
        return 1;
    }

    sprintf(info, "Debug file %s open!\n", filename);
    print_debug(info);
    get_and_print_time();

    if(stdout) fflush(stdout);
    if( dout ) fflush(dout);
    return 0;
}


/** -------------------------------------------------------
 * Open and init binary data file with current time in filename
*/
int open_data_file(fadc_board &Fadc)
{
    char filename[100] = {"\0"};
    char debug[100] = "";
    //  generate new file name
    new_filename(filename, FileNum++);

    //  open out data file
    if((fout = fopen(filename, "w+b")) == NULL)
    {
        if(dout) fprintf(dout, "Data file %s is not open!", filename);
        return 1;  // error
    }

    sprintf(debug, "\nFile %s is open!\n", filename);
    print_debug(debug);
    init_data_file(fout, Fadc);
    return 0;
}


/** -------------------------------------------------------
    Open debug files for online telemetry monitoring
    \return 0 - OK\n
            negative number - if errors
*/
int open_telemetry_files()
{
    int n = 0;

    if((fkadr = fopen("event", "wt")) == NULL)
    {
        if(dout) fprintf(dout, "\nData file \"kadr\" is not open!");
        n--;  // error
    }
    if((ffmin = fopen(EVERYMIN_FILE, "wt")) == NULL)
    {
        if(dout) fprintf(dout, "\nData file \"%s\" is not open!", EVERYMIN_FILE);
        n--;  // error
    }
    if((f5sec = fopen(EVERYSEC_FILE, "wt")) == NULL)
    {
        if(dout) fprintf(dout, "\nData file \"%s\" is not open!", EVERYSEC_FILE);
        n--;  // error
    }
    return n;
}
// ----------- files for online telemetry monitoring ------
/** ------------------------------------------------------- */
