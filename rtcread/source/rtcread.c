#include <gba.h>
#include <gba_console.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <stdio.h>
#include <stdlib.h>

//above includes are from DevKitPro

//Cobbled together from this:
//http://pastebin.com/ApcR878P  Thank you POPSDECO
//and from the source for Vana'Diel Clock Advance:
//http://sourceforge.net/projects/vca/  thanks golddbz2000 and travistyoj
//here is the original license for rtc.c, thanks Jonas Minnberg
/********************************************************************************
 * All the following RTC functions were taken from pogoshell as per the GNU
 * license. Here is the license from pogoshell. Thanks Jonas, I could not have
 * figured this out on my own. :)
 *
 * Copyright (c) 2002-2004 Jonas Minnberg

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *******************************************************************************/

#define UNBCD(x) (((x) & 0xF) + (((x) >> 4) * 10))
#define _BCD(x) ((((x) / 10)<<4) + ((x) % 10))
#define RTC_DATA ((vu16 *)0x080000C4)
#define RTC_RW ((vu16 *)0x080000C6)
#define RTC_ENABLE ((vu16 *)0x080000C8)
#define CART_NAME ((vu8 *)0x080000A0)
#define RTC_CMD_READ(x) (((x)<<1) | 0x61)
#define RTC_CMD_WRITE(x) (((x)<<1) | 0x60)
#define _YEAR	0
#define _MONTH	1
#define _DAY	2
#define _WKD	3
#define _HOUR	4
#define _MIN	5
#define _SEC	6

//this sends commands to the RTC so it knows what to send/receive
//see the data sheet for the Seiko S-3511 for more details
//0x65 to read the 7byte date/time BCDs and 0x64 to write them
void rtc_cmd(int v)
{
	int l;
	u16 b;
	v = v<<1;
	for(l=7; l>=0; l--)
	{
		b = (v>>l) & 0x2;
		*RTC_DATA = b | 4;
		*RTC_DATA = b | 4;
		*RTC_DATA = b | 4;
		*RTC_DATA = b | 5;
	}
}

//this pipes data out to the RTC
//remember that data must be BCDs
void rtc_data(int v)
{
	int l;
	u16 b;
	v = v<<1;
	for(l=0; l<8; l++)
	{
		b = (v>>l) & 0x2;
		*RTC_DATA = b | 4;
		*RTC_DATA = b | 4;
		*RTC_DATA = b | 4;
		*RTC_DATA = b | 5;
	}
}

//this pipes data in from the RTC
int rtc_read(void)
{
	int j,l;
	u16 b;
	int v = 0;
	for(l=0; l<8; l++)
	{
		for(j=0;j<5; j++)
			*RTC_DATA = 4;
		*RTC_DATA = 5;
		b = *RTC_DATA;
		v = v | ((b & 2)<<l);
	}
	v = v>>1;
	return v;
}

static int check_val = 0;

void rtc_enable(void)
{
	*RTC_ENABLE = 1;
	*RTC_DATA = 1;
	*RTC_DATA = 5;
	*RTC_RW = 7;
	rtc_cmd(RTC_CMD_READ(1));
	*RTC_RW = 5;
	check_val =  rtc_read();
}

// Normally returns 0x40
int rtc_check(void)
{
	return (check_val & 0x40); //01000000
}

int rtc_get(u8 *data)
{
	int i;
	*RTC_DATA = 1;
	*RTC_RW = 7;
	*RTC_DATA = 1;
	*RTC_DATA = 5;
	rtc_cmd(RTC_CMD_READ(2));
	*RTC_RW = 5;
	for(i=0; i<4; i++)
		data[i] = (u8)rtc_read();
	*RTC_RW = 5;
	for(i=4; i<7; i++)
		data[i] = (u8)rtc_read();
	return 0;
}

void rtc_set(u8 *data) {
	int i; u8 newdata[7];
	
	for(i=0;i<7;i++) {
		newdata[i] = _BCD(data[i]);
	}
	
	*RTC_ENABLE = 1;
	*RTC_DATA = 1;
	*RTC_DATA = 5;
	*RTC_RW = 7;
	rtc_cmd(RTC_CMD_WRITE(2));
	//*RTC_RW = 0;
	for(i=0;i<4;i++) {
		rtc_data(newdata[i]);
	}
	//*RTC_RW = 0;
	for(i=4;i<7;i++) {
		rtc_data(newdata[i]);
	}
}

/*
struct tm get_time()
{
	struct tm tmp_tm;
	uchar data[7];

	rtc_get(data);
	tmp_tm.tm_sec  = UNBCD(data[6]);
	tmp_tm.tm_min  = UNBCD(data[5]);
	tmp_tm.tm_hour = UNBCD(data[4] & 0x3F); //The & 0x3F was added to work with my EFA cart

	tmp_tm.tm_mday = UNBCD(data[2] & 0x3F); //The & 0x3F was added to work with my EFA cart
	tmp_tm.tm_mon  = UNBCD(data[1]-1);  // Subtract one since C time functins expect months 0-11
	tmp_tm.tm_year = UNBCD(data[0]) + 100; //C expects years from 1900, 1e 1999 = 99, 2000 = 100
	return tmp_tm;
}
*/
/*
void refreshClockDate() {
	enableRTC(0);
	enableRTC(1);
	setRTCcommand(0x65);
	iprintf("DtTm:%u/%02u/%02u %d %02d:%02d:%02d\n",getRTCbyte()+1900+100,getRTCbyte(),getRTCbyte(),getRTCbyte(),getRTCbyte(),getRTCbyte(),getRTCbyte());

}
void refreshClockTime() {
	enableRTC(0);
	enableRTC(1);
	setRTCcommand(0x67);
	iprintf("Time:%02d:%02d:%02d\n",getRTCbyte(),getRTCbyte(),getRTCbyte());

}
void refreshClockStatus() {
	enableRTC(0);
	enableRTC(1);
	setRTCcommand(0x63);
	iprintf("Stat:%02X\n",getRTCbyte());

}
*/

void getGameString(u8 *gametitle) {
	int i;
	for(i=0;i<12;i++) {
		gametitle[i] = CART_NAME[i];
	}
	gametitle[12] = '\0';
}

int main(){
	int keys_pressed;
	u8 gamename[13];
	int gamestate;
	int currstate;
	/*
	u8 edit_hour;
	u8 edit_min;
	u8 edit_sec;
	u8 edit_day;
	u8 edit_month;
	u8 edit_year;
	u8 edit_wkd;
	*/
	
	
	u8 edit_pos;
	edit_pos=0; 
	
	//edit_hour=0; edit_min=0; edit_sec=0; edit_day=0; edit_month=0; edit_year=0; edit_wkd=0;
	
	irqInit();
	irqEnable(IRQ_VBLANK);
	consoleDemoInit();

	iprintf("\n Real Time Clock Reader\n ----------------------------\n\n 1. Remove Flash Cart\n 2. Insert PKMN R/S/E Cart\n 3. Press START");
	iprintf("\n\n\n\n\n\n\n\n\n\n\n Cobbled together by adam\n http://furlocks-forest.net");
	
	u8 datetime[7];
	u8 edit_datetime[7];
	
	edit_datetime[_HOUR]=0; edit_datetime[_MIN]=0; edit_datetime[_SEC]=0;
	edit_datetime[_DAY]=0; edit_datetime[_MONTH]=0; edit_datetime[_YEAR]=0; edit_datetime[_WKD]=0;
	
	gamestate = 0;
	
	while(1){
		currstate = gamestate; //not sure how switch handles changes to gamestate mid-flow
		switch(currstate) {
			case 0: //initial state
				scanKeys();
				keys_pressed = keysDown();
				
				if ( keys_pressed & KEY_START ) {
					//edit_hour=0; edit_min=0; edit_sec=0; edit_day=0; edit_month=0; edit_year=0; edit_pos=0; edit_wkd=0;
					edit_datetime[_HOUR]=0; edit_datetime[_MIN]=0; edit_datetime[_SEC]=0;
					edit_datetime[_DAY]=0; edit_datetime[_MONTH]=0; edit_datetime[_YEAR]=0; edit_datetime[_WKD]=0;
					
					//update data from cart
					getGameString(gamename);
					rtc_enable();
					rtc_get(datetime);
					//print to screen
					iprintf("\x1b[2J"); //clear the screen
					iprintf("\n Real Time Clock Reader\n ----------------------------\n\n Cart Inserted is:\n   %s\n\n",gamename);
					if(check_val & 0x80) { 
						//power flag is raised - everything else is invalid
						iprintf(" Power flag raised!\n\n Battery probably dead.\n\n");
					} else {
						//probably okay to read the clock
						iprintf(" Current date is:\n   %02u/%02u/%u  wkday: % 2d\n Current time is:\n   %02d:%02d:%02d\n",UNBCD(datetime[2]&0x3F),UNBCD(datetime[1]),UNBCD(datetime[0])+1900+100,UNBCD(datetime[3]),UNBCD(datetime[4]&0x3F),UNBCD(datetime[5]),UNBCD(datetime[6]));
					}
					iprintf(" Power: %u  12/24: %u  IntAE: %u\n IntME: %u  IntFE: %u\n",(check_val&0x80)>>7,(check_val&0x40)>>6,(check_val&0x20)>>5,(check_val&0x08)>>3,(check_val&0x02)>>1);
					iprintf("\n\n Press START to refresh.");
					if(!(check_val & 0x80)) {
						iprintf("\n\n Press SELECT to edit.");
					}
				} else if((keys_pressed & KEY_SELECT) && !(check_val & 0x80)) {
					gamestate = 1;
				}
			break;
			
			case 1: //edit clock state
				scanKeys();
				keys_pressed = keysDown();

				if(edit_datetime[_DAY]==0) {
					//edit_* are not set - set them from datetime
					rtc_enable();
					rtc_get(datetime);
					/*
					edit_hour  = UNBCD(datetime[4]&0x3F);
					edit_min   = UNBCD(datetime[5]);
					edit_sec   = UNBCD(datetime[6]);
					edit_day   = UNBCD(datetime[2]&0x3F);
					edit_month = UNBCD(datetime[1]);
					edit_year  = UNBCD(datetime[0]);
					edit_wkd   = UNBCD(datetime[3]);
					*/
					edit_datetime[_HOUR] 	= UNBCD(datetime[_HOUR]&0x3F);
					edit_datetime[_MIN]		= UNBCD(datetime[_MIN]);
					edit_datetime[_SEC] 	= UNBCD(datetime[_SEC]);
					edit_datetime[_DAY] 	= UNBCD(datetime[_DAY]&0x3F);
					edit_datetime[_MONTH] 	= UNBCD(datetime[_MONTH]);
					edit_datetime[_YEAR] 	= UNBCD(datetime[_YEAR]);
					edit_datetime[_WKD] 	= UNBCD(datetime[_WKD]);
				}
				
				iprintf("\x1b[2J"); //clear the screen
				
				iprintf("\n Edit Real-Time Clock\n ----------------------------\n\n Date (dd/mm/yyyy)\n\n  ");
					
				//line to show cursor position
				iprintf("%s   %s     %s       %s",(edit_pos==0?"--":"  "),(edit_pos==1?"--":"  "),(edit_pos==2?"--":"  "),(edit_pos==3?"-":" "));
				
				iprintf("\n  %02d / %02d / 20%02d  wkd: %d\n  ",edit_datetime[_DAY],edit_datetime[_MONTH],edit_datetime[_YEAR],edit_datetime[_WKD]);
				
				//line to show cursor position
				iprintf("%s   %s     %s       %s",(edit_pos==0?"--":"  "),(edit_pos==1?"--":"  "),(edit_pos==2?"--":"  "),(edit_pos==3?"-":" "));
				
				iprintf("\n\n Time (hh:mm:ss) \n\n  ");
				
				//line to show cursor position
				iprintf("%s   %s   %s",(edit_pos==4?"--":"  "),(edit_pos==5?"--":"  "),(edit_pos==6?"--":"  "));
				
				iprintf("\n  %02d : %02d : %02d\n  ",edit_datetime[_HOUR],edit_datetime[_MIN],edit_datetime[_SEC]);
				
				//line to show cursor position
				iprintf("%s   %s   %s",(edit_pos==4?"--":"  "),(edit_pos==5?"--":"  "),(edit_pos==6?"--":"  "));
				
				iprintf("\n\n Press START to save to RTC.");
				
				//iprintf("\n\n Press START to save to RTC.\n Values: %02X/%02X/%02X %02X %02X:%02X:%02X",_BCD(edit_datetime[_DAY]),_BCD(edit_datetime[_MONTH]),_BCD(edit_datetime[_YEAR]),_BCD(edit_datetime[_WKD]),_BCD(edit_datetime[_HOUR]),_BCD(edit_datetime[_MIN]),_BCD(edit_datetime[_SEC]));
				
				if(keys_pressed & KEY_UP) {
					switch(edit_pos) {
						case 0:
							//day
							switch(edit_datetime[_MONTH]) {
								case 1: case 3: case 5: case 7: case 8: case 10: case 12:

									if(edit_datetime[_DAY]==31) {edit_datetime[_DAY]=1;} else {edit_datetime[_DAY]++;}
								break;
								case 4: case 6: case 9: case 11:
									if(edit_datetime[_DAY]==30) {edit_datetime[_DAY]=1;} else {edit_datetime[_DAY]++;}
								break;
								case 2:
									if((edit_datetime[_YEAR]%4==0 && edit_datetime[_DAY]==9) || (edit_datetime[_YEAR]%4>0 && edit_datetime[_DAY]==28)) {edit_datetime[_DAY]=1;} else {edit_datetime[_DAY]++;}
								break;
							}
						break;
						case 1:
							//month
							if(edit_datetime[_MONTH]==12) {edit_datetime[_MONTH]=1;} else {edit_datetime[_MONTH]++;}
						break;
						case 2:
							//year
							if(edit_datetime[_YEAR]==99) {edit_datetime[_YEAR]=0;} else {edit_datetime[_YEAR]++;}
						break;
						case 3:
							//week day
							if(edit_datetime[_WKD]==6) {edit_datetime[_WKD]=0;} else {edit_datetime[_WKD]++;}
						break;
						case 4:
							//hour
							if(edit_datetime[_HOUR]==23) {edit_datetime[_HOUR]=0;} else {edit_datetime[_HOUR]++;}
						break;
						case 5:
							//minute
							if(edit_datetime[_MIN]==59) {edit_datetime[_MIN]=0;} else {edit_datetime[_MIN]++;}
						break;
						case 6:
							//second
							if(edit_datetime[_SEC]==59) {edit_datetime[_SEC]=0;} else {edit_datetime[_SEC]++;}
						break;
					
					}
				} else if(keys_pressed & KEY_DOWN) {
					switch(edit_pos) {
						case 0:
							switch(edit_datetime[_MONTH]) {
								case 1: case 3: case 5: case 7: case 8: case 10: case 12:
									if(edit_datetime[_DAY]==1) {edit_datetime[_DAY]=31;} else {edit_datetime[_DAY]--;}
								break;
								case 4: case 6: case 9: case 11:
									if(edit_datetime[_DAY]==0) {edit_datetime[_DAY]=1;} else {edit_datetime[_DAY]--;}
								break;
								case 2:
									if(edit_datetime[_DAY]==1) {
										if(edit_datetime[_YEAR]%4==0) {
											edit_datetime[_DAY]=29;
										} else {
											edit_datetime[_DAY]=28;
										}
									} else {
										edit_datetime[_DAY]--;
									}
								break;
							}
						break;
						case 1:
							if(edit_datetime[_MONTH]==1) {edit_datetime[_MONTH]=12;} else {edit_datetime[_MONTH]--;}
						break;
						case 2:
							if(edit_datetime[_YEAR]==0) {edit_datetime[_YEAR]=99;} else {edit_datetime[_YEAR]--;}
						break;
						case 3:
							//week day
							if(edit_datetime[_WKD]==0) {edit_datetime[_WKD]=6;} else {edit_datetime[_WKD]--;}
						break;
						case 4:
							//hour
							if(edit_datetime[_HOUR]==0) {edit_datetime[_HOUR]=23;} else {edit_datetime[_HOUR]--;}
						break;
						case 5:
							//minute
							if(edit_datetime[_MIN]==0) {edit_datetime[_MIN]=59;} else {edit_datetime[_MIN]--;}
						break;
						case 6:
							//second
							if(edit_datetime[_SEC]==0) {edit_datetime[_SEC]=59;} else {edit_datetime[_SEC]--;}
						break;
					
					}
				} else if(keys_pressed & KEY_RIGHT) {
					if(edit_pos==6) {
						edit_pos=0;
					} else {
						edit_pos++;
					}
				} else if(keys_pressed & KEY_LEFT) {
					if(edit_pos==0) {
						edit_pos=6;
					} else {
						edit_pos--;
					}
				} else if(keys_pressed & KEY_START) {
					//totally save the shit out of that RTC value
					//iprintf("\n Updating Real-Time Clock\n ----------------------------\n\n Update to:\n %02d/%02d/%04d wkd %d\n %02d:%02d:%02d
					
					rtc_set(edit_datetime);
					gamestate = 0;
					iprintf("\x1b[2J"); //clear the screen
					iprintf("\n Clock updated.\n Press START to continue.");
					
				} else if(keys_pressed & KEY_SELECT) {
				
					gamestate = 0;
					iprintf("\x1b[2J"); //clear the screen
					iprintf("\n Real Time Clock Reader\n ----------------------------\n\n 1. Remove Flash Cart\n 2. Insert PKMN R/S/E Cart\n 3. Press START");
					iprintf("\n\n\n\n\n\n\n\n\n\n\n Cobbled together by adam\n http://furlocks-forest.net");
				}
				
			break;
		}
		VBlankIntrWait();
	}
}
