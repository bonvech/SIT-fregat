//#include "ioports.h"
long dev_port; //
int  port1;
int  port;

/*-----------------------------------------------------------*/
/*                                                           */
/*            Get I/O Ports access                           */
/*                                                           */
/*-------------  Open I/O ports access  ---------------------*/
void GetIOPortsAccess(void)
{
    // for adress > 0x3ff
    if((dev_port = open("/dev/port", O_RDWR))==-1)
        printf("\n/dev/port access - NO!\n");

    port1 = iopl(3);
    //port  = ioperm(0x0, LPT_IO+0xF, LPT_IO);
    port  = ioperm(LPT_IO, 0xF, 1);
    if(port==-1)
    {
        printf("\nI/O port access for LPT - NO!\n");
    }

    port  = ioperm(FADC_IO, 0x3F, 1);
    if(port==-1)
    {
        printf("\nI/O port access for FADC - NO!\n");
        exit(1);
    }

}
/*-----------------------------------------------------------*/

/*-------------- Close I/O ports access ---------------------*/
void CloseIOPortsAccess(void)
{
    close(dev_port);

    port  = ioperm(LPT_IO, 0xF, 0);   // close permition
    port  = ioperm(FADC_IO, 0x3F, 0); // close permition

}

/******************* END ************************************/
