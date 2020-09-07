#include "tweenpin.h"


void playSound( sounds_t sound )
{
  if ( gameOn && !tilt ) return;
  if ( !nonVolatiles.soundBoard ) return;

  DECLARE_INT_STATE;
  DISABLE_INTS();
  
  low( BUS_CONTROL_PORT, BUS_SOUND_CS );

  spiWrite( sound );
  
  high( BUS_CONTROL_PORT, BUS_SOUND_CS );

  RESTORE_INTS();
}

// Bit-bang spi routines used for simplicity
void spiWrite(uint8_t byte)
{
  uint8_t i = 8;
  do {
    i--;
    low( SPI_PORT, SPI_SCK );
    if ( byte & BIT(i) ) 
    {
      high( SPI_PORT, SPI_MOSI ); 
    } else {
      low( SPI_PORT, SPI_MOSI );
    }
    __delay_us(10);
    high( SPI_PORT, SPI_SCK );
    __delay_us(10);
  } while ( i != 0 );
  low( SPI_PORT, SPI_MOSI );
}

uint8_t spiRead(void)
{
  uint8_t data = 0;
  uint8_t i = 8;
  do {
    low( SPI_PORT, SPI_SCK );
    __delay_us(10);
    high( SPI_PORT, SPI_SCK );
    __delay_us(10);
    i--;
    if ( read( SPI_PIN, SPI_MISO ) )
      data |= BIT( i );
  } while ( i != 0 );
  return data;
}

void writeIO( uint8_t chip, uint8_t reg, uint8_t data )
{
  DECLARE_INT_STATE;
  DISABLE_INTS();
  
  low( IO_CS_PORT, (chip < 4 ) ? IO_CS_0123 : IO_CS_4567 );
  
  spiWrite( 0x40 | ((chip & 0x3) << 1) | 0x00 );  // op-code
  spiWrite( reg );
  spiWrite( data );

  high( IO_CS_PORT, (IO_CS_0123|IO_CS_4567) );
    
  RESTORE_INTS();
}


uint8_t readIO( uint8_t chip, uint8_t reg )
{
  uint8_t data;
  DECLARE_INT_STATE;
  DISABLE_INTS();
  
  low( IO_CS_PORT, (chip < 4 ) ? IO_CS_0123 : IO_CS_4567 );
  
  spiWrite( 0x40 | ((chip & 0x3) << 1) | 0x01 );  // op-code
  spiWrite( reg );
  data = spiRead();
  
  high( IO_CS_PORT, (IO_CS_0123|IO_CS_4567) );
    
  RESTORE_INTS(); 
  return data;
}


void initIO(void)
{
  // configure solenoid outputs
  low( SOLENOID_PORT, SOLENOID_MASK );
  output( SOLENOID_DDR, SOLENOID_MASK );
  low( SOLENOID_PORT_H, SOLENOID_MASK_H );
  output( SOLENOID_DDR_H, SOLENOID_MASK_H );
  
  // configure highspeed sw inputs
  high( FASTSWITCH_PORT, FASTSWITCH_MASK ); // pull up
  input( FASTSWITCH_DDR, FASTSWITCH_MASK );

  // configure bit-banged spi
  //pb5 = mosi
  //pb6 = miso
  //pb7 = sck
  DDRB |= (BIT(5) | BIT(7));
  DDRB &= ~BIT(6);
  high( SPI_PORT, SPI_SCK );

  // reset and release, io-expanders
  low( IO_CONTROL_PORT, IO_CONTROL_RESET );
  output( IO_CONTROL_DDR, IO_CONTROL_RESET );
  high( IO_CS_PORT, (IO_CS_0123|IO_CS_4567) );
  output(  IO_CS_DDR, (IO_CS_0123|IO_CS_4567) );
  __delay_us(3);
  high( IO_CONTROL_PORT, IO_CONTROL_RESET);
  
  uint8_t i;
  // configure input chips
  for ( i = IO_INPUT_CHIP_0; i < IO_INPUT_CHIP_MAX; i++ )
  {
    // disable sequential addressing, enable hw addrs, open-drain int pin
    writeIO( i, IO_IOCON, IOCON_SEQOP|IOCON_HAEN|IOCON_ORD );
    writeIO( i, IO_DIR, 0xFF );   //all pins, input
    writeIO( i, IO_GPPU, 0xFF );  //all pins, enable pull-ups
    writeIO( i, IO_IPOL, 0xFF );  //all pins, invert pin logic state
    /* Interrupt features of IO Expanders not currently used
    //  writeIO( i, IO_DEFVAL, 0x00 );
      // all pins, configure for change int
    //  writeIO( i, IO_INTCON, 0x00 );
    //  readIO( i, IO_GPIO );  // clear any flags
    //  writeIO( i, IO_GPINTEN, 0xFF );
    */
  }

  /* Interrupt features of IO Expanders not currently used
  // enable and config io expander interrupt
  //  MCUCSR &= ~BIT(ISC2); // falling edge
  //  PORTB |= BIT(2);    // pull up on int2 pin
  //  GICR |= BIT(INT2);
  */

  // configure output chips
  for ( i = IO_OUTPUT_CHIP_0; i < IO_OUTPUT_CHIP_MAX; i++ )
  {
    // disable sequential addressing, enable hw addrs
    writeIO( i, IO_IOCON, IOCON_SEQOP|IOCON_HAEN );
    writeIO( i, IO_OLAT, 0x00 );  //all pins, outputs low
    writeIO( i, IO_DIR, 0x00 );   //all pins, output
  }
    
  // configure board bus
  low( BUS_CONTROL_PORT, BUS_DISPLAY_LOAD );
  high( BUS_CONTROL_PORT, BUS_SOUND_CS );
  output( BUS_CONTROL_DDR, (BUS_SOUND_CS|BUS_DISPLAY_LOAD) );
  
  //setup fast polling timer
  OCR0 = 40;   // 195.3125Hz = 8000000 / 1024 / 40
  TCCR0 = BIT(WGM01) | BIT(CS02) | BIT(CS00);    // CTC mode, 1024 prescaler
  TIMSK |= BIT(OCIE0);  // enable compare interrupt

  // setup lamp fade timer
  OCR2 = 62;   // 2000Hz = 8000000 / 64 / 125
  TCCR2 = BIT(WGM21) | BIT(CS22);    // CTC mode, 64 prescaler
  TIMSK |= BIT(OCIE2);  // enable compare interrupt

  /* Unused
  // enable and config fast switch interrupt
  //  MCUCR |= BIT(ISC00);  // any change
  //  MCUCR |= BIT(ISC01);  // falling edge
  //  GICR |= BIT(INT0);
  */

  // timer for bear head pwm
  TCCR1B = CS12; // 64 prescaler - 128kHz @ 8mhz,
  low(BEAR_PORT, BEAR_MASK);
  output(BEAR_DDR, BEAR_MASK);
}


//board bus interrupt - Currently Unused
ISR(INT1_vect)
{
}

