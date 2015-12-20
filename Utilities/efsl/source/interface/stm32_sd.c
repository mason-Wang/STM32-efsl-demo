#include "interface/stm32_sd.h"
#include "interface/stm32_spi.h"

/* 512 bytes for each sector */
#define SD_SECTOR_SIZE	512

/* token for write operation */
#define TOKEN_SINGLE_BLOCK	0xFE
#define TOKEN_MULTI_BLOCK	0xFC
#define TOKEN_STOP_TRAN		0xFD

/* Local variables */
static uint8_t CardType;
//static SDCFG SDCfg;

/* Local Function Prototypes */
static uint8_t  STM32_SD_SendCmd     (uint8_t cmd, uint32_t arg);
static bool 	STM32_SD_ReadDataBlock ( uint8_t *buff,	uint32_t cnt);
static bool 	STM32_SD_WirteDataBlock (const uint8_t *buff, uint8_t token);
static bool 	STM32_SD_WaitForReady (void);

/* wait until the card is not busy */
static bool STM32_SD_WaitForReady (void)
{
	uint8_t res;
	/* timeout should be large enough to make sure the write operaion can be completed. */
	uint32_t timeout = 400000;

	STM32_SPI_SendByte(0xFF);
	do {
		res = STM32_SPI_RecvByte();
	} while ((res != 0xFF) && timeout--);

	return (res == 0xFF ? true : false);
}

/* Initialize SD/MMC card. */
bool STM32_SD_Init (void) 
{
	uint32_t i, timeout;
	uint8_t cmd, ct, ocr[4];
	bool ret = false;
	
	/* Initialize SPI interface and enable Flash Card SPI mode. */
	STM32_SPI_Init ();

	/* At least 74 clock cycles are required prior to starting bus communication */
	for (i = 0; i < 80; i++) {	   /* 80 dummy clocks */
		STM32_SPI_SendByte (0xFF);
	}

	ct = CT_NONE;
	if (STM32_SD_SendCmd (GO_IDLE_STATE, 0) == 0x1)
	{
		timeout = 50000;
		if (STM32_SD_SendCmd(CMD8, 0x1AA) == 1) {	/* SDHC */
			/* Get trailing return value of R7 resp */
			for (i = 0; i < 4; i++) ocr[i] = STM32_SPI_RecvByte();		
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {	/* The card can work at vdd range of 2.7-3.6V */
				/* Wait for leaving idle state (ACMD41 with HCS bit) */
				while (timeout-- && STM32_SD_SendCmd(SD_SEND_OP_COND, 1UL << 30));	
				/* Check CCS bit in the OCR */
				if (timeout && STM32_SD_SendCmd(READ_OCR, 0) == 0) {		
					for (i = 0; i < 4; i++) ocr[i] = STM32_SPI_RecvByte();
					ct = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;
				}
			} else {  /* SDSC or MMC */
				if (STM32_SD_SendCmd(SD_SEND_OP_COND, 0) <= 1) 	{
					ct = CT_SD1; cmd = SD_SEND_OP_COND;	/* SDSC */
				} else {
					ct = CT_MMC; cmd = SEND_OP_COND;	/* MMC */
				}
				/* Wait for leaving idle state */
				while (timeout-- && STM32_SD_SendCmd(cmd, 0));			
				/* Set R/W block length to 512 */
				if (!timeout || STM32_SD_SendCmd(SET_BLOCKLEN, SD_SECTOR_SIZE) != 0)	
					ct = CT_NONE;				
			}
		}
	}
	CardType = ct;
	STM32_SPI_Release();

	if (ct) {			/* Initialization succeeded */
		ret = true;
		if ( ct == CT_MMC ) {
			//STM32_SPI_SetSpeed(SPI_SPEED_20MHz);
		} else {
			//STM32_SPI_SetSpeed(SPI_SPEED_20MHz);
		}
	} else {			/* Initialization failed */
		STM32_SPI_Select ();
		STM32_SD_WaitForReady ();
		STM32_SPI_DeInit();
	}
	
	return ret;
}

/*****************************************************************************
	Send a Command to Flash card and get a Response
	cmd:  cmd index
	arg: argument for the cmd
	return the received response of the commond
*****************************************************************************/
static uint8_t STM32_SD_SendCmd (uint8_t cmd, uint32_t arg) 
{
	uint32_t r1, n;

	if (cmd & 0x80) {	/* ACMD<n> is the command sequence of CMD55-CMD<n> */
		cmd &= 0x7F;
		r1 = STM32_SD_SendCmd(APP_CMD, 0);   /* CMD55 */
		if (r1 > 1) return r1; /* cmd send failed */
	}

	/* Select the card and wait for ready */
	STM32_SPI_DeSelect();
	STM32_SPI_Select();
	if (STM32_SD_WaitForReady() == false ) return 0xFF;

	STM32_SPI_SendByte (0xFF);	 /* prepare 8 clocks */ 
	STM32_SPI_SendByte (cmd);	 
	STM32_SPI_SendByte (arg >> 24);
	STM32_SPI_SendByte (arg >> 16);
	STM32_SPI_SendByte (arg >> 8);
	STM32_SPI_SendByte (arg);
   /* Checksum, should only be valid for the first command.CMD0 */
	n = 0x01;							/* Dummy CRC + Stop */
	if (cmd == GO_IDLE_STATE) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	STM32_SPI_SendByte(n); 

	if (cmd == STOP_TRAN) STM32_SPI_RecvByte ();		/* Skip a stuff byte when stop reading */

	n = 10;		/* Wait for a valid response in timeout of 10 attempts */
	do {
		r1 = STM32_SPI_RecvByte ();
	} while ((r1 & 0x80) && --n);

	return (r1);		/* Return with the response value */
}

/*****************************************************************************
	Read "count" Sector(s) starting from sector index "sector",
	buff <- [sector, sector+1, ... sector+count-1]
	if success, return true, otherwise return false
*****************************************************************************/ 
bool STM32_SD_ReadSector (uint32_t sector, uint8_t *buff, uint32_t count)
{
	/* Convert to byte address if needed */
	if (!(CardType & CT_BLOCK)) sector *= SD_SECTOR_SIZE;	

	if (count == 1) {	/* Single block read */
		if ((STM32_SD_SendCmd(READ_BLOCK, sector) == 0)
			&& STM32_SD_ReadDataBlock(buff, SD_SECTOR_SIZE))
			count = 0;
	} else {				/* Multiple block read */
		if (STM32_SD_SendCmd(READ_MULT_BLOCK, sector) == 0) {
			do {
				if (!STM32_SD_ReadDataBlock(buff, SD_SECTOR_SIZE)) break;
				buff += SD_SECTOR_SIZE;
			} while (--count);
			STM32_SD_SendCmd(STOP_TRAN, 0);	/* STOP_TRANSMISSION */
		}
	}
	STM32_SPI_Release();

	return count ? false : true;	
}

/*****************************************************************************
	read specified number of data to specified buffer.
	buff:  Data buffer to store received data
	cnt:   Byte count (must be multiple of 4, normally 512)
*****************************************************************************/
static bool STM32_SD_ReadDataBlock ( uint8_t *buff,	uint32_t cnt)
{
	uint8_t token;
	uint32_t timeout;
	
	timeout = 20000;
	do {							/* Wait for data packet in timeout of 100ms */
		token = STM32_SPI_RecvByte();
	} while ((token == 0xFF) && timeout--);
	if(token != 0xFE) return false;	/* If not valid data token, return with error */

#if USE_FIFO
	STM32_SPI_RecvBlock_FIFO (buff, cnt);
#else
	do {	/* Receive the data block into buffer */
		*buff++ = STM32_SPI_RecvByte ();
		*buff++ = STM32_SPI_RecvByte ();
		*buff++ = STM32_SPI_RecvByte ();
		*buff++ = STM32_SPI_RecvByte ();
	} while (cnt -= 4);
#endif /* USE_FIFO */
	STM32_SPI_RecvByte ();						/* Discard CRC */
	STM32_SPI_RecvByte ();

	return true;					/* Return with success */
}

/*****************************************************************************
	Write "count" Sector(s) starting from sector index "sector",
	buff -> [sector, sector+1, ... sector+count-1]
	if success, return true, otherwise return false
*****************************************************************************/  
bool STM32_SD_WriteSector (uint32_t sector, const uint8_t *buff, uint32_t count) 
{
	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert to byte address if needed */	

	if (count == 1) {	/* Single block write */
		if ((STM32_SD_SendCmd(WRITE_BLOCK, sector) == 0)
			&& STM32_SD_WirteDataBlock(buff, TOKEN_SINGLE_BLOCK))
			count = 0;
	} else {				/* Multiple block write */
		if (CardType & CT_SDC) STM32_SD_SendCmd(SET_WR_BLK_ERASE_COUNT, count);
		if (STM32_SD_SendCmd(WRITE_MULT_BLOCK, sector) == 0) {
			do {
				if (!STM32_SD_WirteDataBlock(buff, TOKEN_MULTI_BLOCK)) break;
				buff += 512;
			} while (--count);
		#if 1
			if (!STM32_SD_WirteDataBlock(0, TOKEN_STOP_TRAN))	/* STOP_TRAN token */
				count = 1;
		#else
			STM32_SPI_SendByte(TOKEN_STOP_TRAN);
		#endif
		}
	}
	STM32_SPI_Release(); 
	return count ? false : true;
}

/*****************************************************************************
	Write 512 bytes
	buffer: 512 byte data block to be transmitted
	token:  0xFE -> single block
			0xFC -> multi block
			0xFD -> Stop
*****************************************************************************/ 
static bool STM32_SD_WirteDataBlock (const uint8_t *buff, uint8_t token)
{
	uint8_t resp, i;

	STM32_SPI_SendByte (token);		/* send data token first*/

	if (token != TOKEN_STOP_TRAN) {
#if USE_FIFO
		STM32_SPI_SendBlock_FIFO (buff);
#else
      /* Send data. */
      for (i = 512/4; i ; i--) { 
         STM32_SPI_SendByte (*buff++);
		 STM32_SPI_SendByte (*buff++);
		 STM32_SPI_SendByte (*buff++);
		 STM32_SPI_SendByte (*buff++);
      }
#endif /* USE_FIFO */
		STM32_SPI_SendByte(0xFF);					/* 16-bit CRC (Dummy) */
		STM32_SPI_SendByte(0xFF);

		resp = STM32_SPI_RecvByte();				/* Receive data response */
		if ((resp & 0x1F) != 0x05)		/* If not accepted, return with error */
			return false;

		if ( STM32_SD_WaitForReady() == false)  /* Wait while Flash Card is busy. */
			return false;
	}

	return true;
}

/* Read MMC/SD Card device configuration. */
bool STM32_SD_ReadCfg (SDCFG *cfg) 
{	   
	uint8_t i;	
	uint16_t csize;
	uint8_t n, csd[16];
	bool retv = false;
	
	/* Read the OCR - Operations Condition Register. */
	if (STM32_SD_SendCmd (READ_OCR, 0) != 0x00) goto x;
	for (i = 0; i < 4; i++)	cfg->ocr[i] = STM32_SPI_RecvByte ();
  	
	/* Read the CID - Card Identification. */
	if ((STM32_SD_SendCmd (SEND_CID, 0) != 0x00) || 
		(STM32_SD_ReadDataBlock (cfg->cid, 16) == false))
		goto x;

    /* Read the CSD - Card Specific Data. */
	if ((STM32_SD_SendCmd (SEND_CSD, 0) != 0x00) || 
		(STM32_SD_ReadDataBlock (cfg->csd, 16) == false))
		goto x;

	cfg -> sectorsize = SD_SECTOR_SIZE;

	/* Get number of sectors on the disk (DWORD) */
	if ((cfg->csd[0] >> 6) == 1) {	/* SDC ver 2.00 */
		csize = cfg->csd[9] + ((uint16_t)cfg->csd[8] << 8) + 1;
		cfg -> sectorcnt = (uint32_t)csize << 10;
	} else {					/* SDC ver 1.XX or MMC*/
		n = (cfg->csd[5] & 15) + ((cfg->csd[10] & 128) >> 7) + ((cfg->csd[9] & 3) << 1) + 2;  // 19
		csize = (cfg->csd[8] >> 6) + ((uint16_t)cfg->csd[7] << 2) + ((uint16_t)(cfg->csd[6] & 3) << 10) + 1; // 3752
		cfg -> sectorcnt = (uint32_t)csize << (n - 9); // 3842048
	}

	cfg->size = cfg -> sectorcnt * cfg -> sectorsize; // 512*3842048=1967128576Byte (1.83GB)

	/* Get erase block size in unit of sector (DWORD) */
	if (CardType & CT_SD2) {			/* SDC ver 2.00 */
		if (STM32_SD_SendCmd(SD_STATUS /*ACMD13*/, 0) == 0) {		/* Read SD status */
			STM32_SPI_RecvByte();
			if (STM32_SD_ReadDataBlock(csd, 16)) {				/* Read partial block */
				for (n = 64 - 16; n; n--) STM32_SPI_RecvByte();	/* Purge trailing data */
				cfg->blocksize = 16UL << (csd[10] >> 4);
				retv = true;
			}
		}
	} else {					/* SDC ver 1.XX or MMC */
		if ((STM32_SD_SendCmd(SEND_CSD, 0) == 0) && STM32_SD_ReadDataBlock(csd, 16)) {	/* Read CSD */
			if (CardType & CT_SD1) {			/* SDC ver 1.XX */
				cfg->blocksize = (((csd[10] & 63) << 1) + ((uint16_t)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
			} else {					/* MMC */
				// cfg->blocksize = ((uint16_t)((buf[10] & 124) >> 2) + 1) * (((buf[11] & 3) << 3) + ((buf[11] & 224) >> 5) + 1);
				cfg->blocksize = ((uint16_t)((cfg->csd[10] & 124) >> 2) + 1) * (((cfg->csd[10] & 3) << 3) + ((cfg->csd[11] & 224) >> 5) + 1);
			}
			retv = true;
		}
	}

x:	STM32_SPI_Release();
    return (retv);
}

