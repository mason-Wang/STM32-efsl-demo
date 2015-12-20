#include "stm32_uart.h"

/* init uart0 */
void  STM32_UART_Init(uint32_t baudrate)
{
	USART_InitTypeDef USART_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;


	/* Enable GPIOx clock */
  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	/* Enable USARTx clocks */
  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* Configure USARTx_Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

  	/* Configure USARTx_Rx as input floating */
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* USARTx configuration ------------------------------------------------------*/
  /* USARTx configured as follow:
        - BaudRate   
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
	USART_InitStructure.USART_BaudRate = baudrate;
  	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  	USART_InitStructure.USART_StopBits = USART_StopBits_1;
  	USART_InitStructure.USART_Parity = USART_Parity_No ;
  	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
 	 /* Configure the USARTx */ 
	USART_Init(USART1, &USART_InitStructure);
	/* Enable the USARTx */
  	USART_Cmd(USART1, ENABLE);
}

/* Write one character to UART0 */
uint8_t STM32_UART_PutChar (uint8_t ch) 
{
	USART_SendData(USART1, ch);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
  	{
  	}
	return ch;
}

/*  Read one character from UART0   (blocking read)	*/
uint8_t STM32_UART_GetChar (void) 
{
	while (USART_GetITStatus(USART1, USART_IT_RXNE) == RESET);
	return (USART_ReceiveData(USART1) & 0xff);
}
