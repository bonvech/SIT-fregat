/**
 *  \file  hvcalibr.h
 *  \brief Калибровочные кривые
 * 
 *  Данный файл содержит в себе функции калибровок 
 *  CH0 - SiPM negative volatge
 *  CH1 - direct voltage on 1.9V
 *  CH2 - 100Om voltage on 1.9V
 *  CH3 - temperature detector
 */
#ifndef _HV_CALIBR
#define _HV_CALIBR

float kod_to_Up(float kod);
float kod_to_I(int ch1, int ch2);
float kod_to_T(int kod);

/// CH0 to U_VH1
float kod_to_Up(float kod)
{
    float tmp = 0;

    //tmp = -245.36 + 0.06 * kod;  // 2013
    tmp = 0.01514 * kod - 36.6;  // 2018
    return tmp;
}

/// CH1 and CH2 to current
float kod_to_I(int ch1, int ch2)
{
    float tmp = 0.005012;
    return tmp * (ch2-ch1);
}

/// CH3 to temperature T,oC
float kod_to_T(int kod)
{
    float x = (kod/2.0 - 500) * 0.1;
    return x;
}


/*
float kod_to_VH1(float kod);
float kod_to_Up(float kod);
float kod_to_I(float kod);
float kod_to_T(float kod);
*/
// CH1 to I,mkA
/*float kod_to_I(float kod)
{
    float tmp = 0.;

    tmp = 123.83 - 0.030244 * kod;
    return tmp;
}
*/

/*
// I,mkA to CH1
int I_to_kod(float cur)
{
    float tmp = 0.;

    tmp = 4095 - 33.06 * cur;
    return (int)tmp;
}
*/

/// CH2 to Upit
/*float kod_to_Up(float kod)
{
    float tmp = 0.;

    tmp = 0.0013 +0.00641 * kod;
    return tmp;
}*/

// CH4 to temperature T,oC
/*float kod_to_T(float kod)
{
    float tmp = 0.;

    if(kod < 3585) tmp = 150.56 - 0.0392 * kod;
    else           tmp = 356.06 - 0.0965 * kod;

    return tmp;
}
*/
#endif
