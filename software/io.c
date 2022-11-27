#include "tweenpin.h"


void spiWrite(uint8_t byte)
{
  SPDR = byte;
  while(!(SPSR & _BV(SPIF) ));
}

uint8_t spiRead(void)
{
  SPDR = 0x00;
  while(!(SPSR & _BV(SPIF) ));
  return(SPDR);
}

void writeIO( uint8_t chip, uint8_t reg, uint8_t data )
{
  low( IO_CS_PORT, (chip < 4 ) ? IO_CS_0123 : IO_CS_4567 );
  spiWrite( 0x40 | ((chip & 0x3) << 1) | 0x00 );  // op-code
  spiWrite( reg );
  spiWrite( data );
  high( IO_CS_PORT, (IO_CS_0123|IO_CS_4567) );
}

uint8_t readIO( uint8_t chip, uint8_t reg )
{
  low( IO_CS_PORT, (chip < 4 ) ? IO_CS_0123 : IO_CS_4567 );
  spiWrite( 0x40 | ((chip & 0x3) << 1) | 0x01 );  // op-code
  spiWrite( reg );
  uint8_t data = spiRead();
  high( IO_CS_PORT, (IO_CS_0123|IO_CS_4567) );
  return data;
}

void initIO(void)
{
  // configure spi bus
  output( SPI_DDR, (SPI_SCK|SPI_MOSI) );
  input( SPI_DDR, (SPI_MISO) );
  high( SPI_PORT, SPI_MISO );
  high( SPI_PORT, SPI_SCK );
  low( SPI_PORT, SPI_MOSI );

  // spi_cs has to be asserted to run in spi_master
  // conflicts with bear head pwm but doesn't seem to be a big deal
  output( SPI_DDR, (BEAR_MASK) );
  high( SPI_PORT, BEAR_MASK );
  
  // spi master mode enabled
  SPCR = _BV(SPE)|_BV(MSTR);
  // SPCR |= _BV(SPR1)|_BV(SPR0);  // F_CPU/128 = 125kbps
  // SPCR |= _BV(SPR0);  // F_CPU/16 = 1mbps
  SPCR |= _BV(SPR0); SPSR = _BV(SPI2X); // F_CPU/8 = 2mbps
  // SPCR |= 0; // F_CPU/4 = 4mbps
  // SPCR |= 0; SPSR = _BV(SPI2X); // F_CPU/4 = 8mbps

  // disable jtag so PORTC2,3,4,5  can be used as GPIO
  MCUCR |= _BV(JTD);
  MCUCR |= _BV(JTD);
  
  // reset and release, io-expanders
  low( IO_CONTROL_PORT, IO_CONTROL_RESET );
  output( IO_CONTROL_DDR, IO_CONTROL_RESET );
  high( IO_CS_PORT, (IO_CS_0123|IO_CS_4567) );
  output( IO_CS_DDR, (IO_CS_0123|IO_CS_4567) );
  __delay_us(3);
  high( IO_CONTROL_PORT, IO_CONTROL_RESET);
  
  // configure input chips
  for (uint8_t i=IO_INPUT_CHIP_0; i<IO_INPUT_CHIP_MAX; i++ ) {
    // disable sequential addressing, enable hw addrs, open-drain int pin
    writeIO( i, IO_IOCON, IOCON_SEQOP|IOCON_HAEN|IOCON_ORD );
    writeIO( i, IO_DIR, 0xFF );   //all pins, input
    writeIO( i, IO_GPPU, 0xFF );  //all pins, enable pull-ups
    writeIO( i, IO_IPOL, 0xFF );  //all pins, invert pin logic state
  }

  // configure output chips
  for (uint8_t i=IO_OUTPUT_CHIP_0; i<IO_OUTPUT_CHIP_MAX; i++ ) {
    // disable sequential addressing, enable hw addrs
    writeIO( i, IO_IOCON, IOCON_SEQOP|IOCON_HAEN );
    writeIO( i, IO_OLAT, 0x00 );  //all pins, outputs low
    writeIO( i, IO_DIR, 0x00 );   //all pins, output
  }
}
