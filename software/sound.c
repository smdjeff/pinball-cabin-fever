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
  // transmit only. the receive buffers are size 0 (see Makefile), but
  // uart_init() turns the receivers and their interrupts on anyway. any byte
  // received (sound board replies on RXD0, noise or the old sw-int diodes on
  // RXD1/PD2) would then be written past the empty buffer and corrupt RAM.
  #ifdef USART0_ENABLED
    uart_init( UART_BAUD_SELECT( 9600, F_CPU ) );
    UCSR0B &= ~( _BV(RXCIE0) | _BV(RXEN0) );
    uart_puts_P( "\nq\n+\n" );
  #endif
  #ifdef USART1_ENABLED  
    uart1_init( UART_BAUD_SELECT( 9600, F_CPU ) );
    UCSR1B &= ~( _BV(RXCIE1) | _BV(RXEN1) );
    uart1_puts_P( "\nq\n+\n" );
  #endif
}

static void playEffect( const char *progmem_s ) 
{
  #ifdef USART0_ENABLED
    uart_puts_P( "q\n" );
    if ( !progmem_s ) return;
    if ( tilt ) return;
    // if ( !gameOn ) return;
    if ( nonVolatiles.volume < 1 ) return;
    char s[] = "P        OGG\n";
    strncpy_P( &s[1], progmem_s, strlen_P(progmem_s) );
    // __delay_ms(20); /// can't be shorter?
    uart_puts( s );
  #endif
}

static void playEffectCh2( const char *progmem_s ) 
{
  #ifdef USART1_ENABLED  
    // always stop the current track first, like the effects channel does.
    // this also makes a replay that arrives a little before the track ends
    // restart it, instead of being ignored and leaving a loop of silence.
    uart1_puts_P( "q\n" );
    if ( !progmem_s ) return;
    if ( tilt ) return;
    if ( !gameOn ) return;
    if ( nonVolatiles.volume < 2 ) return;
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
      playMusic( SOUND_STOP );
      switch ( rand() % 2 ) {
        case 0: playEffectCh2( PSTR("OVER0") ); break;
        case 1: playEffectCh2( PSTR("OVER1") ); break;
      }
      break;
    case SOUND_GAME_TILT:
    case SOUND_STOP:
      playMusic( SOUND_STOP );
      playEffect( NULL );
      break;
    default:
      break;
  }
}

// starts, switches or stops the background music.
// the track is only remembered when it can actually play. a request while
// no game is running, while tilted, or at low volume counts as a stop, so a
// later request for the same track isn't skipped as "already playing".
void playMusic( sounds_t sound ) {
  static sounds_t currentTrack = SOUND_STOP;
  if ( sound != SOUND_BEAR_MUSIC && sound != SOUND_MAIN_MUSIC ) sound = SOUND_STOP;
  if ( !gameOn || tilt || nonVolatiles.volume < 2 ) sound = SOUND_STOP;
  if ( sound == currentTrack ) return;
  currentTrack = sound;
  cancelTimer( MUSIC_TMR );
  if ( sound == SOUND_STOP ) {
    playEffectCh2( NULL );
  } else {
    musicTimer( MUSIC_TMR, sound );
  }
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
