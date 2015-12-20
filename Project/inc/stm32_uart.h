#ifndef STM32_UART_H
#define STM32_UART_H

#include "stm32f10x.h"

void  STM32_UART_Init(uint32_t baudrate);
uint8_t STM32_UART_PutChar (uint8_t ch);
uint8_t STM32_UART_GetChar (void);


#endif
