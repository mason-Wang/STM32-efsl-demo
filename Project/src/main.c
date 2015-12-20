/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    11/20/2009
  * @brief   Main program body
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "monitor.h"
#include <string.h>
#include "efs.h"
#include "ls.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
void GPIO_Configuration(void);
void NVIC_Configuration(void);
void Set_System(void);
/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */

static char LogFileName[] = 
 "efsltst1.txt"; 				// OK
// "/efsltst1.txt"; 				// OK
//"/efsldir1/efsltst9.txt"; 				// OK
// "efsltstlongfilename.txt";	// do not support LFN

EmbeddedFile filer, filew;
uint16_t e;
uint8_t buf[513];

EmbeddedFileSystem  efs;
EmbeddedFile        file;
DirList             list;
uint8_t       file_name[13];
uint32_t        size;

int main(void)
{
	int8_t res;
  	/* Setup STM32 clock, PLL and Flash configuration) */
  	SystemInit();
  	GPIO_Configuration();
  	NVIC_Configuration();
  	STM32_UART_Init(115200);

	xprintf("\nMMC/SD Card Filesystem Test (P:STM32 L:EFSL)\n");

	xprintf("CARD init...");

	// init file system
	if ( ( res = efs_init( &efs, 0 ) ) != 0 ) {
		xprintf("failed with %i\n",res);
	}
	else 
	{
		xprintf("ok\n");
		xprintf("Directory of 'root':\n");
		
		// list files in root directory
		ls_openDir( &list, &(efs.myFs) , "/");
		while ( ls_getNext( &list ) == 0 ) {
			// list.currentEntry is the current file
			list.currentEntry.FileName[LIST_MAXLENFILENAME-1] = '\0';
			xprintf("%s, 0x%x bytes\n", list.currentEntry.FileName, list.currentEntry.FileSize ) ;
		}

		// read file
		if ( file_fopen( &filer, &efs.myFs , LogFileName , 'r' ) == 0 ) {
			xprintf("\nFile %s open. Content:\n", LogFileName);
			while ( ( e = file_read( &filer, 512, buf ) ) != 0 ) {
				buf[e]='\0';
				xprintf("%s\n", (char*)buf);
			}
			xprintf("\n");
			file_fclose( &filer );
		}
		else
			xprintf("File %s open failed.\n", LogFileName);	

		// write file
		if ( file_fopen( &filew, &efs.myFs , LogFileName , 'a' ) == 0 ) {
			xprintf("File %s open for append. Appending...", LogFileName);
			strcpy((char*)buf, "Appending this line on " __TIME__ " "__DATE__ "\r\n");
			if ( file_write( &filew, strlen((char*)buf), buf ) == strlen((char*)buf) ) {
				xprintf("ok\n");
			}
			else {
				xprintf("fail\n");
			}
			file_fclose( &filew );
		}
		else
			xprintf("File %s open failed.\n", LogFileName);

		// read file
		if ( file_fopen( &filer, &efs.myFs , LogFileName , 'r' ) == 0 ) {
			xprintf("\nFile %s open. Content:\n", LogFileName);
			while ( ( e = file_read( &filer, 512, buf ) ) != 0 ) {
				buf[e]='\0';
				xprintf("%s\n", (char*)buf);
			}
			xprintf("\n");
			file_fclose( &filer );
		}
		else
			xprintf("File %s open failed.\n", LogFileName);

		// close file system
		fs_umount( &efs.myFs ) ;
		xprintf("EFSL test complete.\n");

	}
             
  	/* Infinite loop */
  	while (1)
  	{    
  	}
}

/* 
 * GPIO_Configuration
 * ≈‰÷√IO 
 */
void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                         RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
                         RCC_APB2Periph_GPIOE, ENABLE);

  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  	GPIO_Init(GPIOA, &GPIO_InitStructure);
  	GPIO_Init(GPIOB, &GPIO_InitStructure);
  	GPIO_Init(GPIOC, &GPIO_InitStructure);
  	GPIO_Init(GPIOD, &GPIO_InitStructure);
  	GPIO_Init(GPIOE, &GPIO_InitStructure);

  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                         RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
                         RCC_APB2Periph_GPIOE, DISABLE);  
}

void NVIC_Configuration(void)
{
  /* Set the Vector Table base location at 0x08000000 */
  NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);

  /* 2 bit for pre-emption priority, 2 bits for subpriority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif


/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
