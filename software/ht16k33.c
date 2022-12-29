#include "tweenpin.h"


#define HT16K33_CMD_OSC         0b0010
#define HT16K33_OSC_ON          1
#define HT16K33_OSC_OFF         0
#define HT16K33_CMD_BRIGHTNESS  0b1110
#define HT16K33_CMD_SETUP       0b1000
#define HT16K33_DISPLAY_ON      0b0001

static uint8_t column_red[LED_BACKPACKS*LED_ROWS] = {0,};
static uint8_t column_green[LED_BACKPACKS*LED_ROWS] = {0,};

static void i2cInit(void)
{
    TWSR = 0x00; // prescale 1
    // TWSR = _BV(TWPS0); // prescale 4
    // TWSR = _BV(TWPS1); // prescale 16
    // TWSR = _BV(TWPS1)|_BV(TWPS0); // prescale 64
    TWBR = 0x0C; // 400kHz
    TWCR = (1<<TWEN);
}

static void i2cStart(void)
{
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
}

static void i2cStop(void)
{
    TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

static void i2cWrite(uint8_t val)
{
    TWDR = val;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
}

static void writeReg(uint8_t addr, uint8_t reg, uint8_t val) 
{
    i2cStart();
    i2cWrite( (0x70|(addr&0x07)) << 1 );
    i2cWrite( ((reg & 0x0F) << 4) | (val & 0x0F) );
    i2cStop();
}

void ht16k33DrawPixel(uint8_t x, uint8_t y, uint8_t color) 
{
  if ( x >= LED_BACKPACKS*LED_COLUMNS ) return;
  if ( y >= LED_ROWS ) return;
  switch (color) {
    case 0:
      column_red[ x ] &= ~_BV(y);
      column_green[ x ] &= ~_BV(y);
      break;
    case 1:
      column_red[ x ] |= _BV(y);
      column_green[ x ] &= ~_BV(y);
      break;
    case 2:
      column_red[ x ] &= ~_BV(y);
      column_green[ x ] |= _BV(y);
      break;
    case 3:
      column_red[ x ] |= _BV(y);
      column_green[ x ] |= _BV(y);
      break;
  }
}

static void resetBackpack( uint8_t addr, uint16_t color ) 
{
  i2cStart();
  i2cWrite( (0x70|(addr&0x07)) << 1 );
  i2cWrite( 0x00 ); // RAM address increment, start right most column 0
  for (int i=0; i<LED_COLUMNS; i++) { // all columns, right to left
    i2cWrite( (color&0xFF) );  // green all 8 in row
    i2cWrite( (color>>8)&0xFF );  // red all 8 in row 
  }
  i2cStop();
}

void ht16k33Clear(void) 
{
  memset(column_red, 0x00, sizeof(column_red));
  memset(column_green, 0x00, sizeof(column_green));
  for (int i=0; i<LED_BACKPACKS; i++) {
    resetBackpack( i, 0x0000 ); // all colors off
  }
}

uint8_t invertDisplayColors = 0;

void ht16k33DisplayFrame(void)
{
  for (uint8_t addr=0; addr<LED_BACKPACKS; addr++) {
    i2cStart();
    i2cWrite( (0x70|(addr&0x07)) << 1 );
    i2cWrite( 0x00 ); // start at address $00 (right most column 0)
    for (int i=0; i<LED_COLUMNS; i++) { // all columns, right to left
      i2cWrite( invertDisplayColors ^ column_green[ (addr*LED_ROWS)+i ] );
      i2cWrite( column_red[ (addr*LED_ROWS)+i ] );
    }
    i2cStop();
  }
}

void ht16k33Init(void) 
{
  output( DISPLAY_PORT, (DISPLAY_SCL|DISPLAY_SCA) );
  high( DISPLAY_DDR, (DISPLAY_SCL|DISPLAY_SCA) );

  i2cInit();

  for (int i=0; i<LED_BACKPACKS; i++) {
    writeReg( i, HT16K33_CMD_OSC, HT16K33_OSC_ON );
    writeReg( i, HT16K33_CMD_SETUP, HT16K33_DISPLAY_ON );
    writeReg( i, HT16K33_CMD_BRIGHTNESS, 6 );
  }
  ht16k33Clear();
}
