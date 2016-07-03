// ************ SPI TEST APP **************
// Test 1 - First test program, practically port of python script
// ----------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <iostream>
#include <errno.h> // error handling
#include <time.h> // for delay function
#include <stdint.h> // aliases for int types (unsigned char = uint8_t, etc...)
#include <unistd.h>
// for spi
#include <fcntl.h> // file control options
#include <sys/ioctl.h> // I/O control routines ( ioctl() function)
#include <linux/spi/spidev.h> // SPI options

#include "tm_stm32f4_fonts.h"

/* LCD settings */
#define ILI9341_WIDTH        480
#define ILI9341_HEIGHT       320
#define ILI9341_PIXEL        153600

/* Colors */
#define ILI9341_COLOR_WHITE			0xFFFF
#define ILI9341_COLOR_BLACK			0x0000
#define ILI9341_COLOR_RED      		0xF800
#define ILI9341_COLOR_GREEN			0x07E0
#define ILI9341_COLOR_GREEN2		0xB723
#define ILI9341_COLOR_BLUE			0x001F
#define ILI9341_COLOR_BLUE2			0x051D
#define ILI9341_COLOR_YELLOW		0xFFE0
#define ILI9341_COLOR_ORANGE		0xFBE4
#define ILI9341_COLOR_CYAN			0x07FF
#define ILI9341_COLOR_MAGENTA		0xA254
#define ILI9341_COLOR_GRAY			0x7BEF
#define ILI9341_COLOR_BROWN			0xBBCA

/* Transparent background, only for strings and chars */
#define ILI9341_TRANSPARENT			0x80000000

/**
 * @brief  Orientation
 * @note   Used private
 */
typedef enum {
	TM_ILI9341_Landscape,
	TM_ILI9341_Portrait
} TM_ILI9341_Orientation;

/**
 * @brief  LCD options
 * @note   Used private
 */
typedef struct {
	uint16_t width;
	uint16_t height;
	TM_ILI9341_Orientation orientation; // 1 = portrait; 0 = landscape
} TM_ILI931_Options_t;


//#define _DEBUG_
int spi;
uint16_t ILI9341_x;
uint16_t ILI9341_y;
TM_ILI931_Options_t ILI9341_Opts;

int delayus(int us) {
	struct timespec tim, timr;
	tim.tv_sec = 0;
	tim.tv_nsec = (long)(us * 1000);
	
	return nanosleep(&tim, &timr);
}

int delayms(int ms) {
	struct timespec tim, timr;
	tim.tv_sec = 0;
	tim.tv_nsec = (long)(ms * 1000000);
	
	return nanosleep(&tim, &timr);
}

int delays(int s) {
	struct timespec tim, timr;
	tim.tv_sec = s;
	tim.tv_nsec = 0;
	
	return nanosleep(&tim, &timr);
}


/* ************************************************************
	BASIC SPI OPERATIONS 
   ************************************************************ */

/*
	Name: spi_open
	Description: Open SPI device and configure mode
	Parameters:
		1. h : pointer int32 - return device handle
		2. spidev : string - spi device
		3. mode : uint8_t - spi mode
		4. bits : uint8_t - bits per word (normally 8)
		5. speed : uint32_t - SPI clk speed in MHz
	Returns:
		0 - on success, non-zero - fail, see function content for error return values. Alos check errno for more information.
*/
int spi_open(int *h, std::string spidev, uint8_t mode, uint8_t bits, uint32_t speed) {
	int r;
	uint8_t t8;
	uint32_t t32;
	//*h = 0; // reset device handle
	
	// open spi device
	*h = open(spidev.c_str(), O_RDWR); // open spi device for R/W
	if (*h < 0) { 
		// can't open
		*h = 0;
		return -1; // -1 for unable to open device
	}
	
	#ifdef _DEBUG_
		std::cout << "SPI.SETUP: OPEN=" << spidev << " , MODE=" << mode << " , BITS_PER_WORD=" << bits << " , SPI_CLK_SPEED_HZ=" << speed << std::endl;
	#endif
	
	// setup SPI mode
	r = ioctl(*h, SPI_IOC_WR_MODE, &mode); // set mode
	if (r < 0) {
		return -2; // -2 for unable to set SPI mode
	}
	
	r = ioctl(*h, SPI_IOC_RD_MODE, &t8); // read mode
	if (r < 0) {
		return -3;
	}
	
	if (t8 != mode) {
		fprintf(stderr, "SPI_SETUP(-4): Mode check fail. Set 0x%X but got 0x%X\n", mode, t8);
		return -4; // mode mismatch
	}
	
	#ifdef _DEBUG_
		std::cout << "SPI.SETUP: Mode set." << std::endl;
	#endif
	
	
	// set bits per word
	r = ioctl(*h, SPI_IOC_WR_BITS_PER_WORD, &bits); // set bits
	if (r < 0) {
		return -5;
	}
	
	r = ioctl(*h, SPI_IOC_RD_BITS_PER_WORD, &t8); // read bits
	if (r < 0) {
		return -6;
	}
	
	if (t8 != bits) {
		fprintf(stderr, "SPI_SETUP(-7): Bits per word check fail. Set 0x%X but got 0x%X\n", bits, t8);
		return -7; // mode mismatch
	}
	
	#ifdef _DEBUG_
		std::cout << "SPI.SETUP: Bits per word set." << std::endl;
	#endif
	
	
	// set SPI clock speed
	r = ioctl(*h, SPI_IOC_WR_MAX_SPEED_HZ, &speed); // set bits
	if (r < 0) {
		return -8;
	}
	
	r = ioctl(*h, SPI_IOC_RD_MAX_SPEED_HZ, &t32); // read bits
	if (r < 0) {
		return -9;
	}
	
	if (t32 != speed) {
		fprintf(stderr, "SPI_SETUP(-10): SPI Clock check fail. Set %d but got %d\n", speed, t32);
		return -10; // mode mismatch
	}
	
	#ifdef _DEBUG_
		std::cout << "SPI.SETUP: Clock speed set." << std::endl;
	#endif
	
	
	return 0; // OK!
	
}

/*
	Name: spi_close
	Description: Close SPI device
	Parameters:
		1. h : pointer int32 - opened device handle. After closing handle is reset to 0.
	Returns:
		0 - on success, non-zero - fail, see function content for error return values. Alos check errno for more information.
*/
int spi_close(int *h) {
	if (*h == 0) return 0; // closed, or alredy closed
	
	int r = close(*h);
	if (r < 0) {
		return -1; // can't close
	}
	
	#ifdef _DEBUG_
		std::cout << "SPI.CLOSE: OK." << std::endl;
	#endif
	
	*h=0; // rest device handle
	return r;
}

/*
	Name: spi_transmit
	Description: Send data over SPI, and receive at the same time. Received data is put into transmitted data buffer.
	Parameters:
		1. h : pointer int32 - opened device handle. After closing handle is reset to 0.
		2. data : pointer(array) uint8_t - data to be sent, and buffer for data to be received
		3. len : int - bytes count to be sent from buffeer
		4. spi_speed : uint32_t - spi speed override for transfer
		5 . spi_bits : uin8_t - spi bits per word override for transfer
	Returns:
		0 - on success, negative - fail, see function content for error return values. Alos check errno for more information.
		positive - number of bytes received
		
*/
int spi_transmit(int *h, uint8_t *data, int len, uint32_t spi_speed, uint8_t spi_bits) {
	int r;
	struct spi_ioc_transfer buf[len];
	if (*h == 0) return -1; // device not opened
	// prepare data
	for (int i=0;i<len;i++) {
		buf[i].tx_buf = (uint64_t)(data + i);
		buf[i].rx_buf = (uint64_t)(data + i);
		buf[i].len = sizeof(*(data+i)); // 1 byte -_-"
		buf[i].delay_usecs = 0; // no delay
		buf[i].speed_hz = spi_speed;
		buf[i].bits_per_word = spi_bits;
		buf[i].cs_change = 0;
		buf[i].pad = 0;
		buf[i].tx_nbits = 1;
		buf[i].rx_nbits = 1;
	}

	// #if defined(_DEBUG_)
		// std::cout << "SPI TRANSMIT(" << len << ") data=";
		// for (int i=0;i<len;i++) printf("%02x ",*(data+i));
		// std::cout << " - SPI_SPEED=" << spi_speed << " BITS_PER_WORD=" << ((int)spi_bits) << " ... ";
	// #endif
	
	//buf[len-1].cs_change=1;
	// send it
	r = ioctl(*h, SPI_IOC_MESSAGE(len), &buf);	
	
	if (r < 0) {
		#if defined(_DEBUG_)
			std::cout << "ERROR" << std::endl;
			fprintf(stderr, "   Return=%d errno=%d str=%s", r, errno, strerror(errno));
			std::cout << std::endl;
		#else 
			fprintf(stderr, "SPI.TRANSMIT ERRROR (%d,%d) : %s", r, errno, strerror(errno));
		#endif
		return -2;
	}
	
	// #if defined(_DEBUG_)
		// std::cout << "OK";
		// std::cout << std::endl;
	// #endif
	
	return r;	
}


/* **********************************************************************************
	LCD ROUTINES
   ********************************************************************************** */

#define LCD_SPI_DEVICE "/dev/spidev0.0"
#define LCD_SPI_MODE SPI_MODE_0
//#define LCD_SPI_SPEED 3500000
#define LCD_SPI_SPEED 25000000
#define LCD_SPI_BITS_PER_WORD 8
   
void lcd_reset(int *spih) {
	uint8_t buff[4] = { 0,0,0,0 };
	int r;
	
	#ifdef _DEBUG_
		printf("LCD_RESET\n");
	#endif	
	
	// set Reset LOW
	r = spi_transmit(spih, &buff[0], 4, LCD_SPI_SPEED, LCD_SPI_BITS_PER_WORD);
	if (r < 0) {		
		fprintf(stderr, "SPI.LCD_RESET_1 error (%d) : %s", errno, strerror(errno));
	}
	
	delayms(50);
	
	// set Reset High
	buff[0]=buff[1]=buff[2] = 0;
	buff[3]=0x02;
	r = spi_transmit(spih, &buff[0], 4, LCD_SPI_SPEED, LCD_SPI_BITS_PER_WORD);
	if (r < 0) {		
		fprintf(stderr, "SPI.LCD_RESET_2 error (%d) : %s", errno, strerror(errno));
	}
	
	#ifdef _DEBUG_
		printf("LCD_RESET end.\n");
	#endif	
	
	
	delayms(200);
}	

void lcd_data(int *spih, uint16_t data) {
	uint8_t buff[4] = { 0,0,0,0};
	int r;
	
	// #ifdef _DEBUG_
		// printf("LCD_DATA(%04X)\n", data);
	// #endif
	
	buff[1] =  data>>8;
	buff[2] = data&0x00ff;
	buff[3] = 0x15; // 0x15 - DATA_BE const from ili9341.c (BE is short form "before")
	r = spi_transmit(spih, &buff[0], 4, LCD_SPI_SPEED, LCD_SPI_BITS_PER_WORD);
	
	if (r < 0) {		
		fprintf(stderr, "SPI.LCD_DATA_1(0x%4X) error (%d,%d) : %s", data, r, errno, strerror(errno));
		return;
	}
	
	buff[1] =  data>>8;
	buff[2] = data&0x00ff;
	buff[3] = 0x1F; // 0x1F - DATA_AF const from ili9341.c (AF is short form "after")
	r = spi_transmit(spih, &buff[0], 4, LCD_SPI_SPEED, LCD_SPI_BITS_PER_WORD);
	if (r < 0) {		
		fprintf(stderr, "SPI.LCD_DATA_2(0x%4X) error (%d,%d) : %s", data, r, errno, strerror(errno));
	}

}

void lcd_cmd(int *spih, uint16_t cmd) {
	uint8_t buff[4] = { 0,0,0,0};
	int r;
	
	// #ifdef _DEBUG_
		// printf("LCD_CMD(%04X)\n", cmd);
	// #endif
	
	
	buff[1] =  cmd>>8;
	buff[2] = cmd&0x00ff;
	buff[3] = 0x11; // 0x15 - DATA_BE const from ili9341.c (BE is short form "before")
	r = spi_transmit(spih, &buff[0], 4, LCD_SPI_SPEED, LCD_SPI_BITS_PER_WORD);
	if (r < 0) {		
		fprintf(stderr, "SPI.LCD_CMD_1(%4X) error (%d,%d) : %s", cmd, r, errno, strerror(errno));
	}
	
	buff[1] =  cmd>>8;
	buff[2] = cmd&0x00ff;
	buff[3] = 0x1B; // 0x1F - DATA_AF const from ili9341.c (AF is short form "after")
	r = spi_transmit(spih, &buff[0], 4, LCD_SPI_SPEED, LCD_SPI_BITS_PER_WORD);
	if (r < 0) {		
		fprintf(stderr, "SPI.LCD_CMD_2(%4X) error (%d,%d) : %s", cmd, r, errno, strerror(errno));
	}

}

void lcd_setptr(int *spih) {
	lcd_cmd(spih, 0x002b);	//Page Address Set 
	lcd_data(spih, 0x0000); 
	lcd_data(spih, 0x0000); // 0
	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x003f); //319
	
	lcd_cmd(spih, 0x002a);	//Column Address Set
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0000); // 0
	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x00df); // 479
	
	lcd_cmd(spih, 0x002c);
}


	
void lcd_setarea(int *spih, uint16_t x, uint16_t y) {
	lcd_cmd(spih, 0x002b);//Set Gamma 
	lcd_data(spih, y>>8);
	lcd_data(spih, 0x00ff&y);
	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x003f);

	lcd_cmd(spih, 0x002a);//Set Gamma 
	lcd_data(spih, x>>8) ;
	lcd_data(spih, 0x00ff&x) ;
	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x00df);
	lcd_cmd(spih, 0x002c);
}

void lcd_setarea2(int *spih, uint16_t sx, uint16_t sy, uint16_t x, uint16_t y) {
	
	if (sx>479)	sx=0;
	if (sy>319) sy=0;
	if (x>479) x=479;
	if (y>319) y=319;
	
	lcd_cmd(spih, 0x002b);
	lcd_data(spih, sy>>8) ;
	lcd_data(spih, 0x00ff&sy);
	lcd_data(spih, y>>8);
	lcd_data(spih, 0x00ff&y);

	lcd_cmd(spih, 0x002a);
	lcd_data(spih, sx>>8) ;
	lcd_data(spih, 0x00ff&sx) ;
	lcd_data(spih, x>>8);
	lcd_data(spih, 0x00ff&x);
	
	lcd_cmd(spih, 0x002c); //Memory Write
}

void lcd_fill(int *spih, uint16_t color565) {
	lcd_setptr(spih);
	for(int x=0; x<153601;x++) {
		lcd_data(spih, color565);
	}
}

void lcd_fill2(int *spih, uint16_t sx, uint16_t sy, uint16_t x, uint16_t y, uint16_t color565) {
	uint16_t tmp=0;
	int cnt;
	if (sx>479) sx=0;
	if (sy>319) sy=0;
	if (x>479)  x=479;
	if (y>319)  y=319;
	
	if (sx>x) {
		tmp=sx;
		sx=x;
		x=tmp;
	}
	
	if (sy>y) {
		tmp=sy;
		sy=y;
		y=tmp;
	}
	
	cnt = (y-sy+1) * (x-sx+1);
	lcd_setarea2(spih, sx,sy,x,y);
	for(int t=0;t<cnt;t++) {
		lcd_data(spih, color565);
	}	
}
	

//void lcd_init(int *spih) {
//	ILI9486L
void lcd_init(void) {
	int *spih = &spi;
	lcd_reset(spih);
	delayms(100);
	lcd_cmd(spih, 0x0000);	//No Operation
	delayms(1);

	lcd_cmd(spih, 0x00B0);	//Interface Mode Control 
	lcd_data(spih, 0x0000);
	lcd_cmd(spih, 0x0011);	//Sleep OUT
	delayms(50); //mdelay(50);

	lcd_cmd(spih, 0x00B3);	//Frame Control
	lcd_data(spih, 0x0002);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0000);

	lcd_cmd(spih, 0x00C0);	//Power Control 1
	lcd_data(spih, 0x0010);//13
	lcd_data(spih, 0x003B);//480
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0002);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x0000);//NW
	lcd_data(spih, 0x0043);

	lcd_cmd(spih, 0x00C1);	//Power Control 2
	lcd_data(spih, 0x0008);//w_data(0x0008);
	lcd_data(spih, 0x0016);//w_data(0x0016);//CLOCK
	lcd_data(spih, 0x0008);
	lcd_data(spih, 0x0008);

	lcd_cmd(spih, 0x00C4);//Power Control 5
	lcd_data(spih, 0x0011);
	lcd_data(spih, 0x0007);
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0003);

	lcd_cmd(spih, 0x00C6);//CABC Control 1 ????
	lcd_data(spih, 0x0000);

	lcd_cmd(spih, 0x00C8); //GAMMA
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0013);
	lcd_data(spih, 0x005C);
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0007);
	lcd_data(spih, 0x0014);
	lcd_data(spih, 0x0008);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0021);
	lcd_data(spih, 0x0008);
	lcd_data(spih, 0x0014);
	lcd_data(spih, 0x0007);
	lcd_data(spih, 0x0053);
	lcd_data(spih, 0x000C);
	lcd_data(spih, 0x0013);
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0021);
	lcd_data(spih, 0x0000);

	lcd_cmd(spih, 0x0035);	//Tearing Effect Line ON
	lcd_data(spih, 0x0000);

	lcd_cmd(spih, 0x0036);  //Memory Access Control
	lcd_data(spih, 0x0028);

	lcd_cmd(spih, 0x003A);	//Pixel Format Set
	lcd_data(spih, 0x0055); //55 lgh

	lcd_cmd(spih, 0x0044);//Set Tear Scanline
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0001);

	lcd_cmd(spih, 0x00B6);	//Display Function Control
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0002); //220 GS SS SM ISC[3:0]
	lcd_data(spih, 0x003B);

	lcd_cmd(spih, 0x00D0);	//NV Memory Write
	lcd_data(spih, 0x0007);
	lcd_data(spih, 0x0007); //VCI1
	lcd_data(spih, 0x001D); //VRH

	lcd_cmd(spih, 0x00D1);	//NV Memory Protection Key
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0003); //VCM
	lcd_data(spih, 0x0000); //VDV

	lcd_cmd(spih, 0x00D2);	//NV Memory Status Read
	lcd_data(spih, 0x0003);
	lcd_data(spih, 0x0014);
	lcd_data(spih, 0x0004);



	lcd_cmd(spih, 0xE0);  		//Positive Gamma Correction
	lcd_data(spih, 0x1f);  
	lcd_data(spih, 0x2C);  
	lcd_data(spih, 0x2C);  
	lcd_data(spih, 0x0B);  
	lcd_data(spih, 0x0C);  
	lcd_data(spih, 0x04);  
	lcd_data(spih, 0x4C);  
	lcd_data(spih, 0x64);  
	lcd_data(spih, 0x36);  
	lcd_data(spih, 0x03);  
	lcd_data(spih, 0x0E);  
	lcd_data(spih, 0x01);  
	lcd_data(spih, 0x10);  
	lcd_data(spih, 0x01);  
	lcd_data(spih, 0x00);  

	lcd_cmd(spih, 0XE1);  	//Negative Gamma Correction
	lcd_data(spih, 0x1f);  
	lcd_data(spih, 0x3f);  
	lcd_data(spih, 0x3f);  
	lcd_data(spih, 0x0f);  
	lcd_data(spih, 0x1f);  
	lcd_data(spih, 0x0f);  
	lcd_data(spih, 0x7f);  
	lcd_data(spih, 0x32);  
	lcd_data(spih, 0x36);  
	lcd_data(spih, 0x04);  
	lcd_data(spih, 0x0B);  
	lcd_data(spih, 0x00);  
	lcd_data(spih, 0x19);  
	lcd_data(spih, 0x14);  
	lcd_data(spih, 0x0F);  

	lcd_cmd(spih, 0xE2);	//Digital Gamma Control 1
	lcd_data(spih, 0x0f);
	lcd_data(spih, 0x0f);

	lcd_data(spih, 0x0f);

	lcd_cmd(spih, 0xE3);	//Digital Gamma Control 2
	lcd_data(spih, 0x0f);
	lcd_data(spih, 0x0f);

	lcd_data(spih, 0x0f);

	lcd_cmd(spih, 0x13);	//Normal Display Mode ON

	lcd_cmd(spih, 0x0029);	//Display ON
	delayms(20); //mdelay(20);

	lcd_cmd(spih, 0x00B4);	//Display Inversion Control
	lcd_data(spih, 0x0000);
	delayms(20); //mdelay(20);
	lcd_cmd(spih, 0x002C);	//Memory Write
	lcd_cmd(spih, 0x002A); 	//Column Address Set
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0000);

	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x000dF);

	lcd_cmd(spih, 0x002B);  //Page Address Set
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0000);
	lcd_data(spih, 0x0001);
	lcd_data(spih, 0x003f); 

	lcd_cmd(spih, 0x002c); //Memory Write
}




/**
 * @brief  Draws single pixel to LCD
 * @param  x: X position for pixel
 * @param  y: Y position for pixel
 * @param  color: Color of pixel
 * @retval None
 */

void lcd_DrawPixel(uint16_t x, uint16_t y, uint32_t color) {
	int *spih = &spi;
	
	// lcd_SetCursorPosition(x, y, x, y);
	
	// lcd_cmd(&spi, 0x002c);
	// lcd_data(&spi,color >> 8);
	// lcd_data(&spi,color & 0xFF);
	lcd_setarea2(spih, x,y,x,y);
	lcd_data(spih, color);
}

/**
 * @brief  Puts single character to LCD
 * @param  x: X position of top left corner
 * @param  y: Y position of top left corner
 * @param  c: Character to be displayed
 * @param  *font: Pointer to @ref TM_FontDef_t used font
 * @param  foreground: Color for char
 * @param  background: Color for char background
 * @retval None
 */

void TM_ILI9341_Putc(uint16_t x, uint16_t y, char c, TM_FontDef_t *font, uint32_t foreground, uint32_t background) {
	uint32_t i, b, j;
	/* Set coordinates */
	ILI9341_x = x;
	ILI9341_y = y;
	
	if ((ILI9341_x + font->FontWidth) > ILI9341_Opts.width) {
		/* If at the end of a line of display, go to new line and set x to 0 position */
		ILI9341_y += font->FontHeight;
		ILI9341_x = 0;
	}
	
	/* Draw rectangle for background */
	if(background != ILI9341_TRANSPARENT)
		lcd_fill2(&spi,ILI9341_x, ILI9341_y, ILI9341_x + font->FontWidth, ILI9341_y + font->FontHeight, background);
	
	/* Draw font data */
	for (i = 0; i < font->FontHeight; i++) {
		b = font->data[(c - 32) * font->FontHeight + i];
		for (j = 0; j < font->FontWidth; j++) {
			if ((b << j) & 0x8000) {
				lcd_DrawPixel(ILI9341_x + j, (ILI9341_y + i), foreground);
			}
		}
	}
	
	/* Set new pointer */
	ILI9341_x += font->FontWidth;
}

/**
 * @brief  Puts string to LCD
 * @param  x: X position of top left corner of first character in string
 * @param  y: Y position of top left corner of first character in string
 * @param  *str: Pointer to first character
 * @param  *font: Pointer to @ref TM_FontDef_t used font
 * @param  foreground: Color for string
 * @param  background: Color for string background
 * @retval None
 */

void TM_ILI9341_Puts(uint16_t x, uint16_t y, char *str, TM_FontDef_t *font, uint32_t foreground, uint32_t background) {
	uint16_t startX = x;
	
	/* Set X and Y coordinates */
	ILI9341_x = x;
	ILI9341_y = y;
	
	while (*str) {
		/* New line */
		if (*str == '\n') {
			ILI9341_y += font->FontHeight + 1;
			/* if after \n is also \r, than go to the left of the screen */
			if (*(str + 1) == '\r') {
				ILI9341_x = 0;
				str++;
			} else {
				ILI9341_x = startX;
			}
			str++;
			continue;
		} else if (*str == '\r') {
			str++;
			continue;
		}
		
		/* Put character to LCD */
		TM_ILI9341_Putc(ILI9341_x, ILI9341_y, *str++, font, foreground, background);
	}
}



int main(int argc, char **argv)
{
	//std::string dev = LCD_SPI_DEVICE;
	//int spi;
	int r;
	/* Set default settings */
	ILI9341_x = ILI9341_y = 0;
	ILI9341_Opts.width = ILI9341_WIDTH;
	ILI9341_Opts.height = ILI9341_HEIGHT;
	ILI9341_Opts.orientation = TM_ILI9341_Portrait;
	
	r = spi_open(&spi, LCD_SPI_DEVICE, LCD_SPI_MODE, LCD_SPI_BITS_PER_WORD, LCD_SPI_SPEED);
	if (r < 0) {
		fprintf(stderr, "Unable to open SPI bus 0.0 , error (%d,%d) : %s", r, errno, strerror(errno));
		return 1;
	} else {
		std::cout << "SPI 0.0 open." << std::endl;
	}
	
	lcd_init();

	std::cout << "Fill black." << std::endl;
	lcd_fill(&spi, 0x0000);
	lcd_fill2(&spi, 200, 130, 280, 190, ILI9341_COLOR_MAGENTA);
	//TM_ILI9341_Putc(10,10,'F',&TM_Font_11x18,ILI9341_COLOR_BLACK,ILI9341_COLOR_WHITE);
	
	
	// std::cout << "Fill black." << std::endl;
	// lcd_fill(&spi, 0x0000);
	// delayms(500);
	
	// std::cout << "Fill whilte." << std::endl;
	// lcd_fill(&spi, 0xffff);
	
	// //fprintf(stdout,"Fill red. ");
	// std::cout << "Fill red." << std::endl;
	// //td_b = get_ticks();
	// lcd_fill(&spi, 0xF800);
	// //td_e = get_ticks();
	// //td = (uint64_t)(td_e - td_b);
	// //fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	// delayms(500);
	
	//fprintf(stdout,"Fill green. ");
	//std::cout << "Fill green." << std::endl;
	//td_b = get_ticks();
	//lcd_fill(&spi, 0x07E0);
	//td_e = get_ticks();
	//td = (uint64_t)(td_e - td_b);
	//fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	// delayms(500);
	
	// //fprintf(stdout,"Fill blue. ");
	// std::cout << "Fill blue." << std::endl;
	// //td_b = get_ticks();
	// lcd_fill(&spi, 0x001F);
	// //td_e = get_ticks();
	// //td = (uint64_t)(td_e - td_b);
	// //fprintf(stdout,"Time: %9.3f" "ms\n",(float)((uint64_t)td/1000.0f));
	// delayms(500);
	
	
	// // std::cout << "Color test..." << std::endl;
	// // for(uint16_t color=0;color <= 0xffff; color++) {
		// // lcd_fill2(&spi, 200, 130, 280, 190, color);
	// // }
	// std::cout << "Fill black." << std::endl;
	// lcd_fill(&spi, 0x0000);
	
	// for(uint16_t color=0;color <= 0xffff; color++) {
		// lcd_fill2(&spi, 200, 130, 280, 190, color);
		// std::cout << "color:" <<color<< std::endl;
		// //delayms(100);
	// }
	
    TM_ILI9341_Puts(30, 30, (char *)"KeDei 3.5 inch 480x320 TFT lcd from ali", &TM_Font_11x18, ILI9341_COLOR_BLUE, ILI9341_COLOR_GREEN);

    TM_ILI9341_Puts(30, 50, (char *)"(~14USD) 3,5 TFT LCD display with touch", &TM_Font_11x18, ILI9341_COLOR_BLACK, ILI9341_COLOR_RED);

    TM_ILI9341_Puts(0, 70, (char *)"The ILI9486L supports parallel CPU 8-/9-/16-/18-bit data bus interface and 3-/4-line serial peripheral interfaces (SPI)", &TM_Font_11x18, ILI9341_COLOR_YELLOW, ILI9341_TRANSPARENT);

	TM_ILI9341_Puts(10, 130, (char *)"by saper_2", &TM_Font_16x26, ILI9341_COLOR_BROWN, ILI9341_COLOR_WHITE);
	
	
	
	
    TM_ILI9341_Puts(455, 308, (char *)"mk9", &TM_Font_7x10, ILI9341_COLOR_BLACK, ILI9341_COLOR_ORANGE);
	
	r = spi_close(&spi);
	std::cout << "SPI 0.0 closed. (" << ((int)r) << ")" << std::endl;
	return 0;
}