#include <stdlib.h>
#include <string.h>

#include "tweenpin.h"

#include "uart.h"

// note: this requires 2x2w wav/ogg sound boards
// they're triggered by two separate uart channels
// one handles music while the other handles sound effects.
// https://www.adafruit.com/product/2210

void initSound( void )
{
  #ifdef USART0_ENABLED
    uart_init( UART_BAUD_SELECT( 9600, F_CPU ) );
    uart_puts_P( "\nq\n+\n" );
  #endif
  #ifdef USART1_ENABLED  
    uart1_init( UART_BAUD_SELECT( 9600, F_CPU ) );
    uart1_puts_P( "\nq\n+\n" );
  #endif
}

void playEffect( const char *progmem_s ) 
{
  #ifdef USART0_ENABLED
    char s[] = "P        OGG\n";
    strncpy_P( &s[1], progmem_s, strlen_P(progmem_s) );
    uart_puts_P( "q\n" );
    __delay_ms(20);
    uart_puts( s );
  #endif
}

void playMusic( const char *progmem_s ) 
{
  if (!gameOn) return;
  #ifdef USART1_ENABLED  
    char s[] = "P        OGG\n";
    strncpy_P( &s[1], progmem_s, strlen_P(progmem_s) );
    uart1_puts_P( "q\n" );
    __delay_ms(100);
    uart1_puts( s );
  #endif
}

void stopMusic( void )
{
  cancelTimer( MUSIC_TMR );
  #ifdef USART1_ENABLED  
    uart1_puts_P( "q\n" );
  #endif
}
  
void playSound( sounds_t sound )
{
  // if ( !gameOn && !tilt ) return;
  // if ( !nonVolatiles.soundBoard ) return;

  switch ( sound ) {
    case SOUND_SNAKE:
    case SOUND_LIZARD:
      playEffect( PSTR("HIT") );
      break;
    case SOUND_SPINNER:
      playEffect( PSTR("DRIP") );
      break;
    case SOUND_COIN:
    case SOUND_BEAR_TARGETS:
      switch ( rand() % 3 ) {
        case 0: playEffect( PSTR("ROAR0") ); break;
        case 1: playEffect( PSTR("ROAR1") ); break;
        case 2: playEffect( PSTR("ROAR2") ); break;
      }
      break;
    case SOUND_BEAR_OPEN:
      playEffect( PSTR("BEARMOU") );
      break;
    case SOUND_BEAR_CHEW:
      break;
    case SOUND_LOGS:
      switch ( rand() % 3 ) {
        case 0: playEffect( PSTR("LOG0") ); break;
        case 1: playEffect( PSTR("LOG1") ); break;
        case 2: playEffect( PSTR("LOG2") ); break;
      }
      break;
    case SOUND_BONUS:
      playEffect( PSTR("START") );
      break;
    case SOUND_DRAIN:
      switch ( rand() % 2 ) {
        case 0: playEffect( PSTR("DRAIN0") ); break;
        case 1: playEffect( PSTR("DRAIN1") ); break;
      }
      break;
    case SOUND_GAME_START:
      playEffect( PSTR("start") );
      break;
    case SOUND_GAME_TILT:
    case SOUND_GAME_END:
      stopMusic();
      switch ( rand() % 2 ) {
        case 0: playEffect( PSTR("OVER0") ); break;
        case 1: playEffect( PSTR("OVER1") ); break;
      }
      break;
    case SOUND_BEAR_MUSIC:
      playMusic( PSTR("BEARMUS") );
      setTimer( MUSIC_TMR, (28.2*ONE_SECOND), (uint16_t)sound );
      break;
    case SOUND_MAIN_MUSIC:
      playMusic( PSTR("MAIN") );
      setTimer( MUSIC_TMR, (43.2*ONE_SECOND), (uint16_t)sound );
      break;
    case SOUND_STOP:
      stopMusic();
      break;
  }
}

void musicTimer(timerEvent id, uint16_t state) 
{
  if ( state == SOUND_MAIN_MUSIC ) {
    playMusic( PSTR("MAIN") );
  } else {
    playMusic( PSTR("BEARMUS") );
  }
}
