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

// music channel: stop whatever is playing
static void stopCh2( void )
{
  #ifdef USART1_ENABLED
    uart1_puts_P( "q\n" );
  #endif
}

// music channel: play a track. playMusic() decides whether it may play
// and sends the stop first.
static void playEffectCh2( const char *progmem_s )
{
  #ifdef USART1_ENABLED
    if ( !progmem_s ) return;
    char s[] = "P        OGG\n";
    strncpy_P( &s[1], progmem_s, strlen_P(progmem_s) );
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
      // played on the music channel, in place of the music
      playMusic( SOUND_GAME_END );
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

// the music board drops a play command that arrives while it's still
// stopping the previous track. the game over sting was sent right behind
// the stop for the main music, so it never played. every play now waits
// MUSIC_STOP_GAP after its stop. MUSIC_TMR's data carries the track, with
// MUSIC_PLAY set once the stop has been sent.
#define MUSIC_PLAY      0x8000
// first play attempt after the stop. see MUSIC_TRIES for the repeats.
#define MUSIC_STOP_GAP  TEN_HUNDRETH_SECONDS

static sounds_t currentTrack = SOUND_STOP;

// the game over sting: which one, and when it started and how long it runs,
// so attract mode can wait for it to finish. stingLength 0 means none.
#define OVER0_LENGTH    ((uint16_t)(16.9*ONE_SECOND))
#define OVER1_LENGTH    ((uint16_t)(4.9*ONE_SECOND))
static uint8_t  stingTrack = 0;
static uint16_t stingStart = 0;
static uint16_t stingLength = 0;

boolean isGameOverStingPlaying(void)
{
  if ( !stingLength ) return FALSE;
  if ( (uint16_t)(getSysTime() - stingStart) < stingLength ) return TRUE;
  stingLength = 0;
  return FALSE;
}

// starts, switches or stops the background music, or plays the game over
// sting. the track is only remembered when it can actually play. a request
// while no game is running, while tilted, or at low volume counts as a stop,
// so a later request for the same track isn't skipped as "already playing".
void playMusic( sounds_t sound ) {
  if ( sound != SOUND_BEAR_MUSIC && sound != SOUND_MAIN_MUSIC &&
       sound != SOUND_GAME_END ) sound = SOUND_STOP;
  if ( tilt || nonVolatiles.volume < 2 ) sound = SOUND_STOP;
  // the sting is requested just before gameOver(), and plays on into attract
  if ( !gameOn && sound != SOUND_GAME_END ) {
    // gameOver() asks for a stop right after the sting is requested
    if ( currentTrack == SOUND_GAME_END ) return;
    sound = SOUND_STOP;
  }
  if ( sound == currentTrack ) return;
  currentTrack = sound;
  if ( sound == SOUND_GAME_END ) {
    stingTrack = rand() % 2;
    stingLength = ( stingTrack ? OVER1_LENGTH : OVER0_LENGTH ) + MUSIC_STOP_GAP;
    stingStart = getSysTime();
  } else {
    stingLength = 0;
  }
  cancelTimer( MUSIC_TMR );
  stopCh2();
  if ( sound != SOUND_STOP ) {
    setTimer( MUSIC_TMR, MUSIC_STOP_GAP, sound | MUSIC_PLAY );
  }
}

// the board often drops a play that arrives soon after a stop (100 and
// 250 ms both failed), and would stay silent until the next loop. so the
// play is sent MUSIC_TRIES times, MUSIC_RETRY_DELAY apart. the board ignores
// a play while it's already playing (that's why replays need a stop first),
// so the music starts at the first one the board accepts and the rest do
// nothing. MUSIC_TMR's data counts the plays sent in MUSIC_TRIES_MASK.
#define MUSIC_TRIES        4
#define MUSIC_RETRY_DELAY  QUARTER_SECOND
#define MUSIC_TRIES_SHIFT  8
#define MUSIC_TRIES_MASK   0x0F00

static void playTrack( sounds_t sound )
{
  switch ( sound ) {
    case SOUND_BEAR_MUSIC: playEffectCh2( PSTR("BEARMUS") ); break;
    case SOUND_MAIN_MUSIC: playEffectCh2( PSTR("MAIN") ); break;
    case SOUND_GAME_END:
      if ( stingTrack ) {
        playEffectCh2( PSTR("OVER1") );
      } else {
        playEffectCh2( PSTR("OVER0") );
      }
      break;
    default: break;
  }
}

// when to replay a looping track, measured from its play. 0 = plays once.
static uint16_t trackLoopTime( sounds_t sound )
{
  switch ( sound ) {
    case SOUND_BEAR_MUSIC: return (uint16_t)(28.2*ONE_SECOND);
    case SOUND_MAIN_MUSIC: return (uint16_t)(43.2*ONE_SECOND);
    default:               return 0;
  }
}

// MUSIC_TMR steps: stop -> (gap) -> play x MUSIC_TRIES, (retry delay) apart
// -> (rest of the loop time) -> stop ...
void musicTimer(timerEvent id, uint16_t data)
{
  sounds_t sound = (sounds_t)( data & ~(MUSIC_PLAY|MUSIC_TRIES_MASK) );

  if ( data & MUSIC_PLAY ) {
    uint8_t tries = (uint8_t)( ( data & MUSIC_TRIES_MASK ) >> MUSIC_TRIES_SHIFT );
    playTrack( sound );
    if ( ++tries < MUSIC_TRIES ) {
      setTimer( MUSIC_TMR, MUSIC_RETRY_DELAY,
                sound | MUSIC_PLAY | ( (uint16_t)tries << MUSIC_TRIES_SHIFT ) );
      return;
    }
    // all tries sent. the loop time counts from the first one.
    uint16_t loop = trackLoopTime( sound );
    if ( loop ) {
      setTimer( MUSIC_TMR,
                loop - MUSIC_STOP_GAP - (MUSIC_TRIES-1)*MUSIC_RETRY_DELAY, sound );
    } else {
      // plays once. forget it, so the next game's sting isn't skipped as
      // "already playing" and a later stop request goes through.
      cancelTimer( MUSIC_TMR );
      currentTrack = SOUND_STOP;
    }
  } else {
    // loop replay. stop first, so a replay that arrives a little before the
    // track ends restarts it instead of being ignored, then play after the gap
    stopCh2();
    setTimer( MUSIC_TMR, MUSIC_STOP_GAP, sound | MUSIC_PLAY );
  }
}
