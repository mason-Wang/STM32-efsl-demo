/******************** (C) COPYRIGHT 2009 developer.cortex **********************
* File Name          : efsl_spi.h
* Author             : Xu Mingfeng
* Version            : V1.0.0
* Date               : 2009-10-28
* Description        : This file contains all the functions prototypes for
*
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __EFSL_SPI_H
#define __EFSL_SPI_H

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "debug.h"

/* Exported types ------------------------------------------------------------*/
/*************************************************************\
              hwInterface
               ----------
* FILE* 	imagefile		File emulation of hw interface.
* long		sectorCount		Number of sectors on the file.
\*************************************************************/
void STM32_SPI_Init(void);
void STM32_SPI_Select(void);
void STM32_SPI_DeSelect (void);
void STM32_SPI_DeInit( void );
uint8_t STM32_SPI_SendByte (uint8_t data);
uint8_t STM32_SPI_RecvByte (void);
void STM32_SPI_Release (void);
#endif /* __EFSL_SPI_H */
/******************* (C) COPYRIGHT 2009 developer.cortex *******END OF FILE****/

