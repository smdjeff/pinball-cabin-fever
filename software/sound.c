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
    uart_puts_P( "\nq\n" );
  #endif
  #ifdef USART1_ENABLED
    uart1_init( UART_BAUD_SELECT( 9600, F_CPU ) );
    UCSR1B &= ~( _BV(RXCIE1) | _BV(RXEN1) );
    uart1_puts_P( "\nq\n" );
  #endif
  // volume up, once the boards have booted. see volumeTimer().
  setTimer( VOLUME_TMR, VOLUME_BOOT_WAIT, VOLUME_STEPS );
}

// the boards power up at their default volume, and we can't read it back.
// '+' raises it one step and does nothing at the maximum, so enough of them
// leaves both boards at full volume. sent one at a time from a timer: the
// boards are still booting when initSound() runs, and initSound() runs
// before interrupts are on, so a long string would fill the tx buffer and
// hang (uart_puts waits for room) until the watchdog reset.
void volumeTimer(timerEvent id, uint16_t remaining)
{
  #ifdef USART0_ENABLED
    uart_puts_P( "+\n" );
  #endif
  #ifdef USART1_ENABLED
    uart1_puts_P( "+\n" );
  #endif
  if ( --remaining ) {
    setTimer( VOLUME_TMR, VOLUME_STEP_TIME, remaining );
  } else {
    cancelTimer( VOLUME_TMR );
  }
}

// like the music board, the effects board sometimes drops a play sent
// right after the stop that cuts off the previous effect (the bear's ROAR0
// went missing behind the target bonus sound). the play is sent at once, so
// effects stay instant, then again EFFECT_RETRY_DELAY apart. the board
// ignores a play while one is playing, so repeats only matter if the first
// was dropped. a new effect or a stop replaces the pending repeats, so a
// fast spinner doesn't flood the serial line. an effect shorter than the
// last repeat (~210 ms) would be replayed by it, so those are sent once:
// only DRIP (80 ms). the next shortest, HIT, is 370 ms.
#define EFFECT_TRIES        3
#define EFFECT_RETRY_DELAY  TEN_HUNDRETH_SECONDS

static const char *pendingEffect = NULL;   // PROGMEM name being repeated

static void sendEffectPlay( const char *progmem_s )
{
  #ifdef USART0_ENABLED
    char s[] = "P        OGG\n";
    strncpy_P( &s[1], progmem_s, strlen_P(progmem_s) );
    uart_puts( s );
  #endif
}

static void playEffectTries( const char *progmem_s, uint8_t tries )
{
  #ifdef USART0_ENABLED
    uart_puts_P( "q\n" );
    pendingEffect = NULL;
    cancelTimer( EFFECT_TMR );
    if ( !progmem_s ) return;
    if ( tilt ) return;
    // if ( !gameOn ) return;
    if ( nonVolatiles.volume < 1 ) return;
    sendEffectPlay( progmem_s );
    if ( tries > 1 ) {
      pendingEffect = progmem_s;
      setTimer( EFFECT_TMR, EFFECT_RETRY_DELAY, tries - 1 );
    }
  #endif
}

static void playEffect( const char *progmem_s )
{
  playEffectTries( progmem_s, EFFECT_TRIES );
}

void effectTimer(timerEvent id, uint16_t remaining)
{
  if ( pendingEffect && !tilt ) {
    sendEffectPlay( pendingEffect );
  }
  if ( pendingEffect && --remaining ) {
    setTimer( EFFECT_TMR, EFFECT_RETRY_DELAY, remaining );
  } else {
    pendingEffect = NULL;
    cancelTimer( EFFECT_TMR );
  }
}

// stop whatever sound effect is playing
void stopEffects( void )
{
  playEffect( NULL );   // sends the stop, then returns
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
      playEffectTries( PSTR("DRIP"), 1 );   // 80 ms, shorter than a repeat
      break;
    case SOUND_COIN:
    case SOUND_BEAR_CHEW:
      switch ( rand() % 3 ) {
        case 0: playEffect( PSTR("ROAR0") ); break;
        case 1: playEffect( PSTR("ROAR1") ); break;
        case 2: playEffect( PSTR("ROAR2") ); break;
      }
      break;
    // bear sequence: ROAR0 as the head opens, ROAR1 on each chomp (cut
    // short), ROAR2 as the ball is spat out (played to the end)
    case SOUND_BEAR_OPEN:
      playEffect( PSTR("ROAR0") );
      break;
    case SOUND_BEAR_CHOMP:
      playEffect( PSTR("ROAR1") );
      break;
    case SOUND_BEAR_SPIT:
      playEffect( PSTR("ROAR2") );
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
