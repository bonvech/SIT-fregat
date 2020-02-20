/**
 * \file tunka_sync.cpp
 * \brief Часть класса fadc_board. Работа c синхроимпульсом от HISCORE
 *
 * Функции выделения и определения номера синхроимпульса от HISCORE.
 * \date 2020.01.30
 */

//int GetTunkaNumber();
//int CalculateTunkaNumber(int *Syncro);

//extern short int event_data[];

// ==================================================================
/**
 * Get syncro pulse and if syncro pulse exists then calculate Tunka syncro number.
 * \brief   Get syncro pulse and calculate Tunka syncro number
 * \warning TunkaNumber is global
 * \return  TunkaNumber
 */
int GetTunkaNumber()
{
    int threshold = 304, amax = 0, amin = 1000;
    int i = 0, k = 0, bit = 0, n = 0;
    int pulse[2 * Buf2];
    int ind = 0, base = 0;
    int Syncro[100] = {0};
    const short chanmax = 16;

    for (i = 0; i < 2 * Buf2; i++)
    {
        pulse[i] = 0;
    }

    base = 7 * Buf2 * chanmax;
    for (i = 0; i < Buf2; i++)
    {
        ind = base + i * chanmax;
        for( int ch = 15; ch >= 14; ch--) // for ch = 15 and 14
        {
            bit = event_data[ind + ch];
            if(bit & 1024)  pulse[n] = 0;
            else            pulse[n] = bit & 1023;

            if(pulse[n] > amax)
                amax = pulse[n];
            if((pulse[n] < amin) && (pulse[n] > 50) )
                amin = pulse[n];
            n++;
        }
    }

    ///  Calculate threshold
    threshold = (amax + amin) / 2;
    //printf("%d: %d %d %d\n", EventNumber, amin, amax, threshold);

    // pass all small values in the pulse
    k = -1;
    for(i = 0; (pulse[i] < threshold) && (i < 2* Buf2); i++);
    k = i;

    /// If syncro pulse exists - calculate it.
    if( (k > 0) && (k < 2 * Buf2 - 60) && (threshold - amin > 50) )
    {
        for(i = k; i < k + 60; i++)
        {
            if(pulse[i] > threshold)
                bit = 1;
            else
                bit = 0;
            Syncro[i-k] = bit;
            //printf(",%1d", bit);
        }
        /// Calculate Tunka syncro pulse number
        TunkaNumber = CalculateTunkaNumber(Syncro);
    }
    /// If syncro pulse does not exists - number = -1 
    else 
    {
        TunkaNumber = -1;
    }

    return TunkaNumber;
}


// ==================================================================
/**
 * Calculate Tunka syncro_pulse number from array Syncro
 */
int CalculateTunkaNumber(int *Syncro)
{
    int bit[16] = { 4, 7, 10, 13, 16, 20, 23, 26, 29, 32, 36, 39, 42, 45, 48, 52};
    int result1 = 0;
    int result2 = 0;
    int result = 0;

    /// Calculate two syncro numbers
    for(int i = 15; i >= 0; i--)
    {
        result1 *= 2;
        result1 += Syncro[bit[i]];
        result2 *= 2;
        result2 += Syncro[bit[i] + 1];
    }
    printf("Syncro: %d %d %d ", TunkaNumber, result1, result2);

    result = result2;
    /// If two numbers are different - compare it with syncro number of previous event
    if(result1 != result2)
    {
        if(result < TunkaNumber)
            result = result1;
        if(result < TunkaNumber)
            result = -2; // Wrong number
    }
    /// If two numbers are smaller or equal to previous syncro number - wrong number
    if( (TunkaNumber >=0) and (result <= TunkaNumber) )
            result = -2; // Wrong number

    return result;
}
