/** @file ClockUartLed.h
 *
 * @author Wassim FILALI
 *
 * @compiler IAR STM8
 *
 *
 * $Date: 20.09.2015
 * $Revision:
 *
 */

#include "commonTypes.h"

//for InitialiseUART(); putc(char c);  printf(char const *message);
#include "uart_stm8x.h"

void UARTPrintfLn(char const *message);

void UARTPrintfHex(unsigned char val);

void UARTPrintfHexTable(unsigned char *pval,unsigned char length);

void UARTPrintfHexLn(unsigned char val);

void UARTPrintf_sint(signed int num);

void UARTPrintf_uint(U16_t num);
