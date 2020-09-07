#include "tweenpin.h"

// numeric font translation table
const uint8_t displayNum[16] = {
  DISP_ZERO,
  DISP_ONE,
  DISP_TWO,
  DISP_THREE,
  DISP_FOUR,
  DISP_FIVE,
  DISP_SIX,
  DISP_SEVEN,
  DISP_EIGHT,
  DISP_NINE,
  DISP_A,
  DISP_B,
  DISP_C,
  DISP_D,
  DISP_E,
  DISP_F,
};

void writeDisplayNum( uint8_t reg, uint8_t data )
{
  writeDisplay(reg, displayNum[data]);
}

void writeDisplay( uint8_t reg, uint8_t data )
{
  DECLARE_INT_STATE;
  DISABLE_INTS();
  
  spiWrite( reg );
  spiWrite( data );
  
  high( BUS_CONTROL_PORT, BUS_DISPLAY_LOAD );
  __delay_us(1);
  low( BUS_CONTROL_PORT, BUS_DISPLAY_LOAD );

  RESTORE_INTS();
}

void resetDisplay(void)
{
  writeDisplay( DISPLAY_TEST, 0x1 );  
  __delay_ms(1000);
  writeDisplay( DISPLAY_TEST, 0x0 );  

  writeDisplay( DISPLAY_MODE, 0x00 );		// no font decode all digits
  writeDisplay( DISPLAY_SHUTDOWN, 1 );		// normal op
  writeDisplay( DISPLAY_SCAN, 0x7 ); 		// scan all digits
  writeDisplay( DISPLAY_INTENSITY, 0xF );	// max

  displayClear();
}

// show large numeric value, this update is slow and
//  incurs significant overhead due to divides and modulus
//  use for debugging and initialization only
void updateDisplay(uint32_t value)
{
	writeDisplayNum( DISPLAY_DIG7, value % 10 );
	writeDisplayNum( DISPLAY_DIG6, (value % 100) / 10 );
	writeDisplayNum( DISPLAY_DIG5, (value % 1000) / 100 );
	writeDisplayNum( DISPLAY_DIG4, (value % 10000) / 1000 );
	writeDisplayNum( DISPLAY_DIG3, (value % 100000) / 10000 );
	writeDisplayNum( DISPLAY_DIG2, (value % 1000000) / 100000 );
	writeDisplayNum( DISPLAY_DIG1, (value % 10000000) / 1000000 );
	writeDisplayNum( DISPLAY_DIG0, (value % 100000000) / 10000000 );
}


void displayBinary(uint8_t value)
{
	writeDisplayNum( DISPLAY_DIG7, (value & BIT(0))?1:0 );
	writeDisplayNum( DISPLAY_DIG6, (value & BIT(1))?1:0 );
	writeDisplayNum( DISPLAY_DIG5, (value & BIT(2))?1:0 );
	writeDisplayNum( DISPLAY_DIG4, (value & BIT(3))?1:0 );
	writeDisplayNum( DISPLAY_DIG3, (value & BIT(4))?1:0 );
	writeDisplayNum( DISPLAY_DIG2, (value & BIT(5))?1:0 );
	writeDisplayNum( DISPLAY_DIG1, (value & BIT(6))?1:0 );
	writeDisplayNum( DISPLAY_DIG0, (value & BIT(7))?1:0 );
}


void displayString(uint8_t dig0, uint8_t dig1, uint8_t dig2, uint8_t dig3, 
                   uint8_t dig4, uint8_t dig5, uint8_t dig6, uint8_t dig7)
{
  writeDisplay(DISPLAY_DIG0,dig0);
  writeDisplay(DISPLAY_DIG1,dig1);
  writeDisplay(DISPLAY_DIG2,dig2);
  writeDisplay(DISPLAY_DIG3,dig3);
  writeDisplay(DISPLAY_DIG4,dig4);
  writeDisplay(DISPLAY_DIG5,dig5);
  writeDisplay(DISPLAY_DIG6,dig6);
  writeDisplay(DISPLAY_DIG7,dig7);
}

void displayClear(void)
{
  writeDisplay(DISPLAY_DIG0,DISP_BLANK);
  writeDisplay(DISPLAY_DIG1,DISP_BLANK);
  writeDisplay(DISPLAY_DIG2,DISP_BLANK);
  writeDisplay(DISPLAY_DIG3,DISP_BLANK);
  writeDisplay(DISPLAY_DIG4,DISP_BLANK);
  writeDisplay(DISPLAY_DIG5,DISP_BLANK);
  writeDisplay(DISPLAY_DIG6,DISP_BLANK);
  writeDisplay(DISPLAY_DIG7,DISP_BLANK);
}
