#include <string.h>

#include <display.h>
#include <sysdefs.h>
#include "lpc134x.h"
#include "core/ssp/ssp.h"
#include "gpio/gpio.h"
#include "basic/basic.h"
#include "basic/config.h"
#include "usb/usbmsc.h"


#undef N1600

/**************************************************************************/
/* Utility routines to manage nokia display */
/**************************************************************************/

uint8_t lcdBuffer[RESX*RESY_B];
uint32_t intstatus; // Caches USB interrupt state
                    // (need to disable MSC while displaying)

#define TYPE_CMD    0
#define TYPE_DATA   1

static void lcd_select() {
#if CFG_USBMSC
    if(usbMSCenabled){
        intstatus=USB_DEVINTEN;
        USB_DEVINTEN=0;
    };
#endif
    /* the LCD requires 9-Bit frames */
    uint32_t configReg = ( SSP_SSP0CR0_DSS_9BIT     // Data size = 9-bit
                  | SSP_SSP0CR0_FRF_SPI             // Frame format = SPI
                  | SSP_SSP0CR0_SCR_8);             // Serial clock rate = 8
    SSP_SSP0CR0 = configReg;
    gpioSetValue(RB_LCD_CS, 0);
}

static void lcd_deselect() {
    gpioSetValue(RB_LCD_CS, 1);
    /* reset the bus to 8-Bit frames that everyone else uses */
    uint32_t configReg = ( SSP_SSP0CR0_DSS_8BIT     // Data size = 8-bit
                  | SSP_SSP0CR0_FRF_SPI             // Frame format = SPI
                  | SSP_SSP0CR0_SCR_8);             // Serial clock rate = 8
    SSP_SSP0CR0 = configReg;
#if CFG_USBMSC
    if(usbMSCenabled){
        USB_DEVINTEN=intstatus;
    };
#endif
}

static void lcdWrite(uint8_t cd, uint8_t data) {
    uint16_t frame = 0x0;

    frame = cd << 8;
    frame |= data;

    while ((SSP_SSP0SR & (SSP_SSP0SR_TNF_NOTFULL | SSP_SSP0SR_BSY_BUSY)) != SSP_SSP0SR_TNF_NOTFULL);
    SSP_SSP0DR = frame;
    while ((SSP_SSP0SR & (SSP_SSP0SR_BSY_BUSY|SSP_SSP0SR_RNE_NOTEMPTY)) != SSP_SSP0SR_RNE_NOTEMPTY);
    /* clear the FIFO */
    frame = SSP_SSP0DR;
}

#define CS 2,1
#define SCK 2,11
#define SDA 0,9
#define RST 2,2

#ifdef NOTYET
uint8_t lcdRead(uint8_t data)
{
    uint8_t i;

    gpioSetDir(SDA, 1);
    gpioSetValue(SCK, 0);
    delayms(1);
    gpioSetValue(CS, 0);
    delayms(1);

    gpioSetValue(SDA, 0);
    delayms(1);
    gpioSetValue(SCK, 1);
    delayms(1);
    
    for(i=0; i<8; i++){
        gpioSetValue(SCK, 0);
        delayms(1);
        if( data & 0x80 )
            gpioSetValue(SDA, 1);
        else
            gpioSetValue(SDA, 0);
        data <<= 1;
        gpioSetValue(SCK, 1);
        delayms(1);
    }
    uint8_t ret = 0;

    gpioSetDir(SDA, 0);
    for(i=0; i<8; i++){
        gpioSetValue(SCK, 0);
        delayms(1);
        ret <<= 1;
        ret |= gpioGetValue(SDA);
        gpioSetValue(SCK, 1);
        delayms(1);
    }

    gpioSetValue(CS, 0);
    gpioSetDir(SDA, 1);
    delayms(1);
}
#endif


void lcdInit(void) {
    sspInit(0, sspClockPolarity_Low, sspClockPhase_RisingEdge);

    gpioSetValue(RB_LCD_CS, 1);
    gpioSetValue(RB_LCD_RST, 1);

    gpioSetDir(RB_LCD_CS, gpioDirection_Output);
    gpioSetDir(RB_LCD_RST, gpioDirection_Output);

    delayms(100);
    gpioSetValue(RB_LCD_RST, 0);
    delayms(100);
    gpioSetValue(RB_LCD_RST, 1);
    delayms(100);

    lcd_select();

/* Small Nokia 1200 LCD docs:
 *           clear/ set
 *  on       0xae / 0xaf
 *  invert   0xa6 / 0xa7
 *  mirror-x 0xA0 / 0xA1
 *  mirror-y 0xc7 / 0xc8
 *
 *  0x20+x contrast (0=black - 0x2e)
 *  0x40+x offset in rows from top (-0x7f)
 *  0x80+x contrast? (0=black -0x9f?)
 *  0xd0+x black lines from top? (-0xdf?)
 *
 */
#ifndef N1600
    lcdWrite(TYPE_CMD,0xE2);
    delayms(5);
    lcdWrite(TYPE_CMD,0xAF); // Display ON
    lcdWrite(TYPE_CMD,0xA1); // Mirror-X
    lcdWrite(TYPE_CMD,0xA4);
    lcdWrite(TYPE_CMD,0x2F);
    lcdWrite(TYPE_CMD,0xB0);
    lcdWrite(TYPE_CMD,0x10);
    // lcdWrite(TYPE_CMD,0x00);
#else
    delayms(10);
    lcdWrite(TYPE_CMD,0x01); //sw reset
    delayms(10);
    lcdWrite(TYPE_CMD,0x11); //sleepout
    delayms(10);
    
    lcdWrite(TYPE_CMD,0x36); //MADCTL MY MX V LAO RGB X X X  
    lcdWrite(TYPE_DATA,0x00);

    lcdWrite(TYPE_CMD,0x25); // contrast...
    lcdWrite(TYPE_DATA,0x3F);
    delayms(10);
    
    lcdWrite(TYPE_CMD,0x29);  //display on
    
    lcdWrite(TYPE_CMD,0xBA); //data order
    lcdWrite(TYPE_DATA,0x07);
    lcdWrite(TYPE_DATA,0x15);

    lcdWrite(TYPE_CMD,0x25); //contrast... again?
    lcdWrite(TYPE_DATA,0x3f);
    
    lcdWrite(TYPE_CMD,0x11); //Sleepout
    lcdWrite(TYPE_CMD,0x13); //display mode normal

    lcdWrite(TYPE_CMD,0X37); //vscroll addr
    lcdWrite(TYPE_DATA,0x00);

    lcdWrite(TYPE_CMD,0x3A); // COLMOD pixel format 4=12, 5=16, 6=18
    lcdWrite(TYPE_DATA,0x05);
    
    lcdWrite(TYPE_CMD,0x2A); //no clue... I think it's setting up the size of the display?
    lcdWrite(TYPE_DATA,0);   
    lcdWrite(TYPE_DATA,98-1); //98 = width
    
    lcdWrite(TYPE_CMD,0x2B);
    lcdWrite(TYPE_DATA,0);
    lcdWrite(TYPE_DATA,70-1); //70 = height
    
#endif
    /*
    uint16_t i;
    for(i=0; i<100; i++)
        lcdWrite(TYPE_DATA,0x00);
    */
    lcd_deselect();
}

void lcdFill(char f){
    int x;
    for(x=0;x<RESX*RESY_B;x++) {
        lcdBuffer[x]=f;
    }
};

void lcdSafeSetPixel(char x, char y, bool f){
    if (x>=0 && x<RESX && y>=0 && y < RESY)
        lcdSetPixel(x, y, f);
}

void lcdSetPixel(char x, char y, bool f){
    if (x<0 || x> RESX || y<0 || y > RESY)
        return;
    char y_byte = (RESY-(y+1)) / 8;
    char y_off = (RESY-(y+1)) % 8;
    char byte = lcdBuffer[y_byte*RESX+(RESX-(x+1))];
    if (f) {
        byte |= (1 << y_off);
    } else {
        byte &= ~(1 << y_off);
    }
    lcdBuffer[y_byte*RESX+(RESX-(x+1))] = byte;
}

bool lcdGetPixel(char x, char y){
    char y_byte = (RESY-(y+1)) / 8;
    char y_off = (RESY-(y+1)) % 8;
    char byte = lcdBuffer[y_byte*RESX+(RESX-(x+1))];
    return byte & (1 << y_off);
}

#define THECOLOR_R 0x0
#define THECOLOR_G 0x80
#define THECOLOR_B 0x0

void lcdDisplay(void) {
    char byte;
    lcd_select();

#ifndef N1600
    lcdWrite(TYPE_CMD,0xB0);
    lcdWrite(TYPE_CMD,0x10);
    lcdWrite(TYPE_CMD,0x00);
#else
    lcdWrite(TYPE_CMD,0x2C);
    
#endif
    

#ifndef N1600
    uint16_t i,page;
    for(page=0; page<RESY_B;page++) {
        for(i=0; i<RESX; i++) {
            if (GLOBAL(lcdmirror))
                byte=lcdBuffer[page*RESX+RESX-1-(i)];
            else
                byte=lcdBuffer[page*RESX+(i)];

            if (GLOBAL(lcdinvert))
                byte=~byte;
    
            lcdWrite(TYPE_DATA,byte);
        }
    }
#else
    unsigned char r=THECOLOR_R,g=THECOLOR_G,b=THECOLOR_B;
    unsigned char br=0xFF, bg=0xFF, bb=0xFF;
    unsigned char frame_r=0x00, frame_g=0x00, frame_b=0x80;
    uint16_t color,framecolor,backcolor;
    uint16_t x,y,i;
    bool px;
    uint16_t actualcolor;
    color = ((r&0xF8) << 8) | ((g&0xFC)<<3) | ((b&0xF8) >> 3);
    framecolor= ((frame_r&0xF8) << 8) | ((frame_g&0xFC)<<3) | ((frame_b&0xF8) >> 3);
    backcolor= ((br&0xF8) << 8) | ((bg&0xFC)<<3) | ((bb&0xF8) >> 3);

    //top line of the frame...
    for(i=0;i<98;i++){
        lcdWrite(TYPE_DATA,framecolor>>8);
        lcdWrite(TYPE_DATA,framecolor&0xFF);
    }

    for(y=RESY;y>0;y--){
        //left line of the frame
        lcdWrite(TYPE_DATA,framecolor>>8);
        lcdWrite(TYPE_DATA,framecolor&0xFF);

        for(x=RESX;x>0;x--){
            if(GLOBAL(lcdmirror))
                px=lcdGetPixel(RESX-x+1,y-1);
            else
                px=lcdGetPixel(x-1,y-1);
                
            if((!px)^(!GLOBAL(lcdinvert))) actualcolor=color;
            else actualcolor=backcolor; /* white */

            lcdWrite(TYPE_DATA,actualcolor>>8);
            lcdWrite(TYPE_DATA,actualcolor&0xFF);
        }
        //right line of the frame
        lcdWrite(TYPE_DATA,framecolor>>8);
        lcdWrite(TYPE_DATA,framecolor&0xFF);
    }
    
    //bottom line of the frame
    for(i=0;i<98;i++){
        lcdWrite(TYPE_DATA,framecolor>>8);
        lcdWrite(TYPE_DATA,framecolor&0xFF);
    }
#endif

    lcd_deselect();
}
void lcdRefresh() __attribute__ ((weak, alias ("lcdDisplay")));

inline void lcdInvert(void) {
    GLOBAL(lcdinvert)=!GLOBAL(lcdinvert);
}

void lcdSetContrast(int c) {
    #ifndef N1600
    c+=0x80;
    if(c>0x9F)
        return;
    lcd_select();
    lcdWrite(TYPE_CMD,c);
    lcd_deselect();
    #else
    if(c>=0x40)
        return;
    lcd_select();
    lcdWrite(TYPE_CMD,0x25);
    lcdWrite(TYPE_DATA,4*c);
    lcd_deselect();
    #endif
};

void lcdSetInvert(int c) {
    if(c>1)
        c=1;
    if(c<0)
        c=1;

    c+=0xa6;
    lcd_select();
    lcdWrite(TYPE_CMD,c);
    lcd_deselect();
};

/* deprecated */
void __attribute__((__deprecated__)) lcdToggleFlag(int flag) {
    if(flag==LCD_MIRRORX)
        GLOBAL(lcdmirror)=!GLOBAL(lcdmirror);
    if(flag==LCD_INVERTED)
        GLOBAL(lcdinvert)=!GLOBAL(lcdinvert);
}


void lcdShiftH(bool right, bool wrap) {
	uint8_t tmp;
	for (int yb = 0; yb<RESY_B; yb++) {
		if (right) {
			tmp = lcdBuffer[yb*RESX];
			memmove(lcdBuffer + yb*RESX,lcdBuffer + yb*RESX+1 ,RESX-1);
            lcdBuffer[yb*RESX+(RESX-1)] = wrap?tmp:0;
		} else {
			tmp = lcdBuffer[yb*RESX+(RESX-1)];
			memmove(lcdBuffer + yb*RESX+1,lcdBuffer + yb*RESX ,RESX-1);
			lcdBuffer[yb*RESX] = wrap?tmp:0;
		}
	}
}

void lcdShiftV8(bool up, bool wrap) {
	uint8_t tmp[RESX];
	if (!up) {
		if (wrap)
            memmove(tmp, lcdBuffer, RESX);
        else
            memset(tmp,0,RESX);
		memmove(lcdBuffer,lcdBuffer+RESX ,RESX*(RESY_B-1));
		memmove(lcdBuffer+RESX*(RESY_B-1),tmp,RESX);
	} else {
		if (wrap)
            memmove(tmp, lcdBuffer+RESX*(RESY_B-1), RESX);
        else
            memset(tmp,0,RESX);
		memmove(lcdBuffer+RESX,lcdBuffer ,RESX*(RESY_B-1));
		memmove(lcdBuffer,tmp,RESX);
	}
}

void lcdShiftV(bool up, bool wrap) {
	uint8_t tmp[RESX];
	if (up) {
		if (wrap) 
            memmove(tmp,lcdBuffer+((RESY_B-1)*RESX),RESX);
        else
            memset(tmp,0,RESX);
		for (int x = 0; x<RESX; x++){
			for (int y = RESY_B-1; y > 0; y--){
				lcdBuffer[x+(y*RESX)] = (lcdBuffer[x+(y*RESX)] << 1) |( lcdBuffer[x+((y-1)*RESX)] >> 7);
			}
			lcdBuffer[x] = ( lcdBuffer[x] << 1) | ((tmp[x]>>3)&1);
		}
				
	} else {
		if (wrap)
            memmove(tmp,lcdBuffer,RESX);
        else
            memset(tmp,0,RESX);
		for (int x = 0; x<RESX; x++){
			for (int y = 0; y < (RESY_B-1); y++){
				lcdBuffer[x+(y*RESX)] = (lcdBuffer[x+(y*RESX)] >> 1) |( lcdBuffer[x+((y+1)*RESX)] << 7);
			}
			lcdBuffer[x+((RESY_B-1)*RESX)] = ( lcdBuffer[x+((RESY_B-1)*RESX)] >> 1) | ((tmp[x]<<3)&8);
		}	

	}
}	

void lcdShift(int x, int y, bool wrap) {
	bool dir=true;

    if(x<0){
        dir=false;
        x=-x;
    };

    while(x-->0)
			lcdShiftH(dir, wrap);

    if(y<0){
        dir=false;
        y=-y;
    }else{
        dir=true;
    };

    while(y>=8){
        y-=8;
        lcdShiftV8(dir, wrap);
    };

    while(y-->0)
			lcdShiftV(dir, wrap);
}

