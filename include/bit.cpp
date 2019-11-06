/**
 * \file bit.cpp
 * \brief to make bit operations with numbers
 *
 * \date 20.10.2006 Modified 25.01.2010, reformatted November 2019
 */

#ifndef _BIT_CPP
#define _BIT_CPP ///< to prevent multiple including

extern FILE *dout;

int print_binary(int number);
unsigned int set_bit_1(unsigned int number, short bit);
unsigned int set_bit_0(unsigned int number, short bit);
unsigned int get_bit(  unsigned int number, short bit);


/** ----------------------------------------------------------------
 * \brief Set bit to 1
 * \param   number  number
 * \param   bit     bit to set 1
 * \return new number
 * 
 * Set bit of number to 1
 */
unsigned int set_bit_1(unsigned int number, short bit)
{
   unsigned int p = 1;

   if(bit > 31) return 0;

   if(bit > 0)  p <<= bit;
 
   return number |= p;
}

/** ----------------------------------------------------------------
 * \brief Set bit to 0
 * \param   number  number
 * \param   bit     bit to set 0
 * \return new number
 * 
 * Set bit of number to 0
 */
unsigned int set_bit_0(unsigned int number, short bit)
{
   unsigned int p = 1;

   if(bit > 31) return 0;
   if(bit > 0)  p <<= bit;

   //number ^= p;
   //number &= ~p;
   return number &= ~p;
}


/** ----------------------------------------------------------------
 * \brief Get bit of number
 * \param   number  number
 * \param   bit     bit to get
 * \return bit value
 *
 * Get bit of number 
 */
unsigned int get_bit(unsigned int number, short bit)
{
   unsigned int p = 1;
 
   if(bit > 31) return 0;
   if(bit > 0)  p <<= bit;

   if( number & p)
       return 1;
   else
       return 0;
}


/** ----------------------------------------------------------------
 * \brief Print binary
 * \param   number  number
 * \return 0
 *
 * Print number in binary format
 */
int print_binary(int number)
{
    unsigned short i = 0, p = 1;

    printf("\nb0: " );
    for(i = 0; i <= 15; i++)
    {
        if(number&p)
            printf("1");
        else
            printf("0");
        p <<= 1;
    }

    return 0;
}


/** ----------------------------------------------------------------
 * \brief Print binary1
 * \param   number  number
 * \return 0
 *
 * Print number in binary format
 */
int print_binary1(int number)
{
    unsigned short p = 1;
    unsigned short bins = 15;
    signed short i = 15;  // !! signed !!
    char ch;

    p <<= bins ;

    for( i = bins; i >= 0; i--)
    {
        if( number & p)
            ch = '1';
        else
            ch = '0';

        printf("%c", ch);
        if(dout) fprintf(dout, "%c", ch);
        if( !(i%8) ) printf(" ");
        p >>= 1;
    }

    return 0;
}


/** ----------------------------------------------------------------
 * \brief Old Print binary1 
 * \param   number  number
 * \return 0
 *
 * Print number in binary format. Old redaction
 */
int print_binary0(int number)
{
    signed short i = 0; //, p = 0;

    //printf("\nb1: " );
    for( i = 15; i >= 0; i--)
    {
        //p = get_bit(number,i);
        printf("%1i",get_bit(number,i));
        if(dout) fprintf(dout, "%1i",get_bit(number,i));
    }

    return 0;
}


/*  switch(bit)
{
    case  0: p =    1; break;
    case  1: p =    2; break;
    case  2: p =    4; break;
    case  3: p =    8; break;
    case  4: p =   16; break;
    case  5: p =   32; break;
    case  6: p =   64; break;
    case  7: p =  128; break;
    case  8: p =  256; break;
    case  9: p =  512; break;
    case 10: p = 1024; break;
    case 11: p = 2048; break;
    case 12: p = 4096; break;
    case 13: p = 8192; break;
    case 14: p = 16384; break;
    case 15: p = 32768; break;
    case 16: p = 65536; break;
    }
    */

#endif
