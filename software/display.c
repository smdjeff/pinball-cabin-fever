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

void bootDisplay(void)
{
  writeDisplay( DISPLAY_TEST, 0x1 );  
  __delay_ms(10000);
  writeDisplay( DISPLAY_TEST, 0x0 );  
  
  resetDisplay();
}

void resetDisplay(void)
{
  writeDisplay( DISPLAY_MODE, 0x00 );   // no font decode all digits
  writeDisplay( DISPLAY_SHUTDOWN, 1 );    // normal op
  writeDisplay( DISPLAY_SCAN, 0x7 );    // scan all digits
  writeDisplay( DISPLAY_INTENSITY, 0xF ); // max

  displayClear();
}

void displayBCD(uint8_t *bcd, uint8_t size)
{
  uint8_t i;
  for (i=0; i<8; i++) {
    writeDisplayNum(DISPLAY_DIG0 + i, bcd[i]);
  }
}


// show large numeric value, this update is slow and
//  incurs significant overhead due to divides and modulus
//  use for debugging and initialization only
void displayLong(uint32_t value, uint8_t digits)
{
  uint8_t i;
  uint32_t j = 10;
  uint32_t k = 1;
  for (i=0; i<8 && i<digits; i++) {
    writeDisplayNum( DISPLAY_DIG7 - i, (value % j) / k );
    j *= 10;
    k *= 10;
  }
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

/*
non-functioning
void displayText(const uint8_t* str)
{
  const displayChars displayLetters[] = {
    DISP_A, DISP_B, DISP_C, DISP_D, DISP_E, DISP_F, DISP_G, DISP_H, DISP_I, DISP_J, DISP_K, DISP_L, DISP_M,
    DISP_N, DISP_O, DISP_P, DISP_Q, DISP_R, DISP_S, DISP_T, DISP_U, DISP_V, DISP_W, DISP_X, DISP_Y, DISP_Z
  };
  const displayChars displayNumbers[] = {
    DISP_ZERO, DISP_ONE, DISP_TWO, DISP_THREE, DISP_FOUR, DISP_FIVE, 
    DISP_SIX, DISP_SEVEN, DISP_EIGHT, DISP_NINE
  };
  uint8_t i;
  
  for (i=0;(i<8)&&(str[i]!='\0'); i++) {
    displayChars dispChar = DISP_BLANK;
    uint8_t ch = str[i];
    if ( ch>='a' && ch<='z' ) {
      dispChar = displayLetters[ch-'a'];
    } else if ( ch>='A' && ch<='Z' ) {
      dispChar = displayLetters[ch-'A'];
    } else if ( ch>= '0' && ch<='9' ) {
      dispChar = displayNumbers[ch-'0'];
    } else if ( ch=='-' ) {
      dispChar = DISP_DASH;
    } else if ( ch=='=' ) {
      dispChar = DISP_BARS;
    }
    
    writeDisplay(DISPLAY_DIG0 + i, dispChar);
  }
}
*/

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
