/**
 * \file lvpsdev.cpp
 * \brief Class lvps_dev. Вентиллятор.
 * \date 05.02.2008
 */
#ifndef _LVPS_DEV
#define _LVPS_DEV   ///< to prevent multiple including

#include "lvps.cpp"


// extern function
void print_debug(char * message);


/// Class for lvps devices
class lvps_dev: public lvps
{
  public:
    // constructor
    lvps_dev()
    {
        BaseAddr    = 0x378;  ///< BaseAddress
        ADCSubAddr2 = 0x21;   ///< ADC subaddres
    }


    //////////////////////////////////////////////////////////////
    /** -------------------------------------------------------
    *  \brief read_fadc_temp
    */
    float read_fadc_temp(unsigned int addr)
    {
        unsigned int Res1 = 0;
        float Tem1 = 0.;

        if(!RX16(addr, &Res1))
        {
            printf("Error in T RX16 reading!!!!");

            return 0;
        }
        Tem1 = kod_2_fadc_temp(Res1);
        //printf("T = %6.2f oC", Tem1); // --- lena
        if(dout) fprintf(dout, "T=%ikod =%6.2f oC", Res1, Tem1);
        return Tem1;
    }


    /** -------------------------------------------------------
    *  \brief set_inn_vent
    *  \param high kod to write to inn ventillator (is one byte integer: 0 - 255) 
    */
    int set_inn_vent(unsigned char high)
    {
        //printf(                "inn vent: high = %i\n", high);
        SetChannelAddr(57);
        if( !WriteDAC(0, high))
        {
            printf("Error in DAC`s transmittion\n");
            return 1;
        }
        return 0;
    }

    /** -------------------------------------------------------
    *  \brief set_out_vent
    *  \param high kod to write to out ventillator (is one byte integer: 0 - 255) 
    */
    int set_out_vent(unsigned char high)
    {
        //printf(                "out vent: high = %i\n", high);
        SetChannelAddr(58);
        if( !WriteDAC(0, high))
        {
            printf("Error in DAC`s transmittion\n");
            return 1;
        }
        return 0;
    }


    /** -------------------------------------------------------
    * \brief read_power_temp\n
    * Write power supply temperature to debug string
    * \param message - string to write power supply temperature
    */
    unsigned int read_power_temp(char *message)
    {
        if(dout) fprintf(dout, "\nread_power_temp: ");

        SetChannelAddr(59); // adress of VIP block temperature
        float Tem1 = read_fadc_temp(0x49);

        sprintf(message, "T_power_supply = %5.1f oC", Tem1);
        print_debug(message);

        return 0;
    }


    /** -------------------------------------------------------
    *  \brief read_vip_ADC \par 
    *  Read vip ADC and write to degug file and string message
    */
    unsigned int read_vip_ADC(char *message)
    {
        unsigned char  iii = 0, i = 0;
        unsigned int   MData[4];
        unsigned int   kod[10], tmppp = 0;
        float U5 = 0.,  Uac = 0., I = 0.;   //U15 = 0.,

        SetChannelAddr(56); // adress of ADC of vip
        for(i=0; i<4; i++)  MData[i] = 1;

        /// Read ADC
        if(!ReadADCs(0,(unsigned int *)&MData))  // read ADC
        {
            printf("Error in ADC`s reading\n");
            if(dout) fprintf(dout, "!measure_high: Error in ADC`s reading\n");
            return 1;
        }
        else
        {
            for(iii=0; iii<4; iii++)
            {
                tmppp = 0;
                tmppp = MData[iii];
                tmppp &= 0x0FFF; // set 4 major bit to 0
                kod[iii] = tmppp;
                //printf("kod%i=[%d]  ", iii, kod[iii]);
            }
        }

        // Print data result
        //printf("\nread_vip_ADC: ");
        //printf("CH0[%3i]  CH1[%3i]  CH2[%3i]  CH3[%3i]\n\n", kod[0], kod[1],kod[2],kod[3]);
        if(dout) fprintf(dout, "\nComputer block: CH0[%3i] CH1[%3i] CH2[%3i] CH3[%3i]   CH0-CH2=[%4i]\n\n", kod[0], kod[1],kod[2],kod[3], kod[0]-kod[2]);

        //U15 = kod_to_U15(kod[0]);
        I   = kod_to_I(  kod[1]);
        U5  = kod_to_U5( kod[2]);
        Uac = kod_to_Uac(kod[3]);

        //printf("U15=%6.2fV   U5=%4.2fV  Uac=%6.2fV  I=%6.2fA\n ", U15, U5, Uac, I);
        sprintf(message, "Computer block: U5 = %4.2f V  Uac = %5.2f V  I = %4.2f A  ", U5, Uac, I);
        print_debug(message);

        return 0;
    }


private:

    /** -------------------------------------------------------
    *  \brief kod_2_fadc_temp
    */
    float kod_2_fadc_temp(unsigned int kod)
    {
        float temp = 0.;
        short int skod = 0;

        skod = (short int) kod;
        //printf("\n    T1 %i", skod);
        skod >>= 6;
        //printf("    T1 %i", skod);
        temp = (float)skod;
        temp /= 4.;

        return temp;
    }


    /** -------------------------------------------------------
    *  \brief CH0 to U 15V
    */
    float kod_to_U15(float kod)
    {
        float tmp = 0.;

        tmp  = 1.011216 * kod;
        tmp /= 100.;
        return tmp;
    }


    /** -------------------------------------------------------
    *  \brief CH2 to U 5V
    */
    float kod_to_U5(float kod)
    {
        float tmp = 0.;

        tmp  = 1.00945 * kod;
        tmp /= 100.;
        return tmp;
    }


    /** -------------------------------------------------------
    *  \brief CH1 to I,mkA
    */
    float kod_to_I(float kod)
    {
        float tmp = 0.;

        //printf("I= %.0f kod", kod);
        //tmp  = 0.01476 * kod;
        //tmp += 0.0144;
        tmp  = 0.0154396 * kod;
        tmp -= 0.05082;
        return tmp;
    }


    /** -------------------------------------------------------
    *  \brief CH3 to U_Vaccumulators
    */
    float kod_to_Uac(float kod)
    {
        float tmp = 0;

        tmp  = 0.01004 * kod;
        tmp += 0.00508;
        return tmp;
    }

};  // end of class lvps_dev

#endif
