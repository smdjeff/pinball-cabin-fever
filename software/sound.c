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

static void playEffect( const char *progmem_s ) 
{
  #ifdef USART0_ENABLED
    char s[] = "P        OGG\n";
    strncpy_P( &s[1], progmem_s, strlen_P(progmem_s) );
    uart_puts_P( "q\n" );
    if ( tilt ) return;
    // if ( !gameOn ) return;
    if ( nonVolatiles.volume < 1 ) return;
    if ( !progmem_s ) return;
    // __delay_ms(20); /// can't be shorter?
    uart_puts( s );
  #endif
}

static void playEffectCh2( const char *progmem_s ) 
{
  #ifdef USART1_ENABLED  
    // uart1_puts_P( "q\n" );
    if ( tilt ) return;
    if ( !gameOn ) return;
    if ( nonVolatiles.volume < 2 ) return;
    if ( !progmem_s ) return;
    char s[] = "P        OGG\n";
    strncpy_P( &s[1], progmem_s, strlen_P(progmem_s) );
     // __delay_ms(100); /// can't be shorter?
    uart1_puts( s );
  #endif
}

void playSound( sounds_t sound )
{
 
  switch ( sound ) {
    case SOUND_BEAR_TARGETS:
    case SOUND_LIZZARD:
    case SOUND_SNAKE:
      playEffect( PSTR("HIT") );
      break;
    case SOUND_SPINNER:
    case SOUND_DOOR_BUTTON:
    case SOUND_LANE:
      playEffect( PSTR("DRIP") );
      break;
    case SOUND_COIN:
    case SOUND_BEAR_CHEW:
      switch ( rand() % 3 ) {
        case 0: playEffect( PSTR("ROAR0") ); break;
        case 1: playEffect( PSTR("ROAR1") ); break;
        case 2: playEffect( PSTR("ROAR2") ); break;
      }
      break;
    case SOUND_BEAR_OPEN:
      playEffect( PSTR("BEARMOU") );
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
    case SOUND_GAME_END:
      playEffectCh2( NULL );
      switch ( rand() % 2 ) {
        case 0: playEffectCh2( PSTR("OVER0") ); break;
        case 1: playEffectCh2( PSTR("OVER1") ); break;
      }
      break;
    case SOUND_GAME_TILT:
    case SOUND_STOP:
      cancelTimer( MUSIC_TMR );
      playEffect( NULL );
      playEffectCh2( NULL );
      break;
    default:
      break;
  }
}

void playMusic( sounds_t sound ) {
  static sounds_t currentTrack = SOUND_STOP;
  if ( sound != currentTrack ) {
    playEffectCh2( NULL );
    cancelTimer( MUSIC_TMR );
    if ( sound == SOUND_BEAR_MUSIC || sound == SOUND_MAIN_MUSIC ) {
      musicTimer( MUSIC_TMR, sound );
    }
  }
  currentTrack = sound;
}

void musicTimer(timerEvent id, uint16_t sound) 
{
  switch ( sound ) {
    case SOUND_BEAR_MUSIC:
      playEffectCh2( PSTR("BEARMUS") );
      setTimer( MUSIC_TMR, (28.2*ONE_SECOND), (uint16_t)sound );
      break;
    case SOUND_MAIN_MUSIC:
      playEffectCh2( PSTR("MAIN") );
      setTimer( MUSIC_TMR, (43.2*ONE_SECOND), (uint16_t)sound );
      break;
    default:
      break;
  }
}
