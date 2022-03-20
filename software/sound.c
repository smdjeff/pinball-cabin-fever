#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "tweenpin.h"

#include "uart.h"

void initSound( void ) {}
void playSound( sounds_t sound ) {}

#if 0

void initSound( void )
{
  uart_init( UART_BAUD_SELECT( 115200, F_CPU ) );
  uart_puts_P( "\r\n" );
  uart_puts_P( "AT+VOL=30\r\n" );
  
  #ifdef USART1_ENABLED  
    uart1_init( UART_BAUD_SELECT( 115200, F_CPU ) );
    uart1_puts_P( "\r\n" );
  #endif
}

#define playEffect( str ) do { \
    uart_puts_P( "AT+PLAYFILE=" ); \
    uart_puts_P( str ); \
    uart_puts_P( "\r\n" ); \
  } while(0)

#ifdef USART1_ENABLED  

  #define stopMusic() do { \
    uart1_puts_P( "AT+VOL=0\r\n" ); \
    uart1_puts_P( "AT+PLAYMODE=0\r\n" ); \
  } while(0)
  
  #define playMusic( str ) do { \
      uart1_puts_P( "AT+VOL=30\r\n" ); \
      uart1_puts_P( "AT+PLAYMODE=1\r\n" ); \
      uart1_puts_P( "AT+PLAYFILE=" ); \
      uart1_puts_P( str ); \
      uart1_puts_P( "\r\n" ); \
    } while(0)
#else  

  #define stopMusic() do { } while(0)
  #define playMusic( str ) do { } while(0)
  
#endif



void playSound( sounds_t sound )
{
  if ( gameOn && !tilt ) return;
  if ( !nonVolatiles.soundBoard ) return;

  switch ( sound ) {
    case SOUND_STOP:
      break;
    case SOUND_SNAKE: // rattler
      break;
    case SOUND_SPINNER: // water park screams
      playEffect( "Drip.wav" );
      break;
    case SOUND_BEAR_TARGETS: // large animal grunt
      switch ( rand() % 3 ) {
        case 0: playEffect( "BearRoar0.wav" ); break;
        case 1: playEffect( "BearRoar1.wav" ); break;
        case 2: playEffect( "BearRoar2.wav" ); break;
      }
      break;
    case SOUND_BEAR_OPEN: // grizzly growl
      playEffect( "BearMouth.wav" );
      playMusic( "BearMusic.mp3" );
      break;
    case SOUND_BEAR_CHEW: // large animal chew
      break;
    case SOUND_LOGS: // beavers (or music)
      switch ( rand() % 3 ) {
        case 0: playEffect( "LogHit0.mp3" ); break;
        case 1: playEffect( "LogHit1.wav" ); break;
        case 2: playEffect( "LogHit2.wav" ); break;
      }
      break;
    case SOUND_BONUS: // buzz saw
      break;
    case SOUND_LIZARD: // lizard
      break;
    case SOUND_DRAIN: // drop into water
      switch ( rand() % 2 ) {
        case 0: playEffect( "Drain0.wav" ); break;
        case 1: playEffect( "Drain1.wav" ); break;
      }
      break;
    case SOUND_GAME_START:
      playEffect( "StartButton.wav" );
      break;
    case SOUND_GAME_END:
      switch ( rand() % 2 ) {
        case 0: playEffect( "GameOver0.mp3" ); break;
        case 1: playEffect( "GameOver1.wav" ); break;
      }
      break;
    case SOUND_MATCHING:    // clicks
      break;
    case SOUND_BACKGROUND_STOP:
      stopMusic();
      break;
    case SOUND_BACKGROUND: // babbling brook
      playMusic( "MainMusic.wav" );
      break;
  }
  
}

#endif
