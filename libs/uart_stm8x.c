/** @file uart_stm8x.c
 *
 * @author Wassim FILALI  STM8L
 *
 * @compiler IAR STM8
 *
 *
 * $Date: 26.10.2016
 * $Revision:
 *
 */

#include "uart_stm8x.h"

//to know which DEVICE_STM8 we have
#include "deviceType.h"

#if DEVICE_STM8L == 1
	#include <iostm8l151f3.h>
#elif DEVICE_STM8S == 1
	#include <iostm8s103f3.h>
#else
  #error needs a file deviceType.h with DEVICE_STM8L 1  or S 1 defined
#endif



#if DEVICE_STM8L == 1

void uart_init()
{
        //Enable USART Peripheral Clock
        CLK_PCKENR1_PCKEN15 = 1;
	//  Reset the UART registers to the reset values.
	//
	USART1_CR1 = 0;
	USART1_CR2 = 0;
	USART1_CR4 = 0;
	USART1_CR3 = 0;
	USART1_CR5 = 0;
	USART1_GTR = 0;
	USART1_PSCR = 0;
	//
	//  Now setup the port to 115200,n,8,1.
	//
	USART1_CR1_M = 0;        //  8 Data bits.
	USART1_CR1_PCEN = 0;     //  Disable parity.
	//CR3_STOP = 0 =>  1 stop bit.
        
        //BR1 [11:4]=0x08 BR2[15:12 - 3:0]=0x0a
        //0x008a => 138       
        //  Set the baud rate registers to 115200 baud 
        //  based upon a 16 MHz system clock.
	USART1_BRR2 = 0x0a;      //  Set the baud rate registers to 115200 baud
	USART1_BRR1 = 0x08;      //  based upon a 16 MHz system clock.
	//
	//  Disable the transmitter and receiver.
	//
	USART1_CR2_TEN = 0;      //  Disable transmit.
	USART1_CR2_REN = 0;      //  Disable receive.
	//
	//  Set the clock polarity, lock phase and last bit clock pulse.
	//
	USART1_CR3_CPOL = 0;//1
	USART1_CR3_CPHA = 0;//1
	USART1_CR3_LBCL = 0;//1
	//
	//  Turn on the UART transmit, receive and the UART clock.
	//
	USART1_CR2_TEN = 1;
	USART1_CR2_REN = 0;
	USART1_CR3_CLKEN = 0;//Clock Pin Disabled (UART not USART)
}

void putc(char c)
{
	while (USART1_SR_TXE == 0);          //  Wait for transmission to complete.
	USART1_DR = c;
}

//
//  Send a message to the debug port (UART1).
//
void printf(char const *ch)
{
	while (*ch)
	{
	while (USART1_SR_TXE == 0);          //  Wait for transmission to complete.
	USART1_DR = (unsigned char) *ch;     //  Put the next character into the data transmission register.
	ch++;                               //  Grab the next character.
	}
}

#elif DEVICE_STM8S == 1
void uart_init()
{
	//
	//  Clear the Idle Line Detected bit in the status register by a read
	//  to the UART1_SR register followed by a Read to the UART1_DR register.
	//
	unsigned char tmp = UART1_SR;
	tmp = UART1_DR;
	//
	//  Reset the UART registers to the reset values.
	//
	UART1_CR1 = 0;
	UART1_CR2 = 0;
	UART1_CR4 = 0;
	UART1_CR3 = 0;
	UART1_CR5 = 0;
	UART1_GTR = 0;
	UART1_PSCR = 0;
	//
	//  Now setup the port to 115200,n,8,1.
	//
	UART1_CR1_M = 0;        //  8 Data bits.
	UART1_CR1_PCEN = 0;     //  Disable parity.
	UART1_CR3_STOP = 0;     //  1 stop bit.
	UART1_BRR2 = 0x0a;      //  Set the baud rate registers to 115200 baud
	UART1_BRR1 = 0x08;      //  based upon a 16 MHz system clock.
	//
	//  Disable the transmitter and receiver.
	//
	UART1_CR2_TEN = 0;      //  Disable transmit.
	UART1_CR2_REN = 0;      //  Disable receive.
	//
	//  Set the clock polarity, lock phase and last bit clock pulse.
	//
	UART1_CR3_CPOL = 1;
	UART1_CR3_CPHA = 1;
	UART1_CR3_LBCL = 1;
	//
	//  Turn on the UART transmit, receive and the UART clock.
	//
	UART1_CR2_TEN = 1;
	UART1_CR2_REN = 1;
	UART1_CR3_CKEN = 1;
}

void putc(char c)
{
	UART1_DR = c;
	while (UART1_SR_TXE == 0);          //  Wait for transmission to complete.
}

//
//  Send a message to the debug port (UART1).
//
void printf(char const *ch)
{
	while (*ch)
	{
	UART1_DR = (unsigned char) *ch;     //  Put the next character into the data transmission register.
	while (UART1_SR_TXE == 0);          //  Wait for transmission to complete.
	ch++;                               //  Grab the next character.
	}
}

#else  
	#error needs a file deviceType.h with DEVICE_STM8L 1  or S 1 defined
#endif
