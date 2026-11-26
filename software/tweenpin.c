///////////////////////////////////////////////////////////////////////////////
//
// (c)2006  diypinball.com
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>

#include "tweenpin.h"


uint8_t ballInPlay = 0;
boolean tilt = FALSE;
uint8_t tiltSense = 0;
uint8_t multiplier = 1;
boolean gameOn = FALSE;
boolean highScoreExceeded = FALSE;
boolean scoreDisplayDisabled = FALSE;
boolean ballIsLoading = FALSE;
boolean crazyMode = FALSE;

static uint32_t score = 0;
static uint8_t credits = 0;
nonVolatiles_t nonVolatiles = {0,};


int main (void)
{  
  wdt_reset();
  wdt_enable(WDTO_1S);
  
  initIO();
  initSolenoids();
  flipperInitPulse();
  bearInitPulse();
  initFastSwitches();
  initDisplay();
  initSound();
  
  loadNonVolatiles();
  
  initTimers();
  // well inside the ~1 s watchdog timeout (ONE_SECOND fired just after it)
  setTimer(WATCHDOG_TMR, HALF_SECOND, 0);
  sei();
  
  gameOver();

  for (;; )
  {
    // the only things done in interrupt context are incremeting the system tick
    // and the sound uart tx fifo. this increases reliability and makes the 
    // system more deterministic. it also reduces the ATOMIC_BLOCK() clutter 

    driveLamps();
    driveSlowSwitches();
    driveFastSwitches();
    driveSolenoids();
    driveTimers();
  }
}

void watchdogTimer(timerEvent id, uint16_t data) {
  static uint16_t check = ~0;
  if ( check == ~data ) {
    wdt_reset();
    data++;
    check = ~data;
    setTimer(WATCHDOG_TMR, HALF_SECOND, data);
  }
}

const uint8_t vertLamps[15][3] PROGMEM = {
  {
    LAMP_RIVER_ARROW_6,
    LAMP_TARGET_1_B,
    LAMP_TARGET_1_B,
  },
  {
    LAMP_RIVER_ARROW_6,
    LAMP_TARGET_1_E,
    LAMP_TARGET_1_E,
  },
  {
    LAMP_RIVER_ARROW_5,
    LAMP_TARGET_1_A,
    LAMP_TARGET_1_A,
  },
  {
    LAMP_RIVER_ARROW_5,
    LAMP_TARGET_1_R,
    LAMP_TARGET_1_R,
  },
  {
    LAMP_RIVER_ARROW_4,
    LAMP_BEAR_MOUTH,
    LAMP_TARGET_2_L,
  },
  {
    LAMP_RIVER_ARROW_3,
    LAMP_TARGET_2_O,
    LAMP_TARGET_2_O,
  },
  {
    LAMP_RIVER_ARROW_3,
    LAMP_BEAR_ARROW,
    LAMP_TARGET_2_G,
  },
  {
    LAMP_RIVER_ARROW_2,
    LAMP_TARGET_2_J,
    LAMP_TARGET_2_J,
  },
  {
    LAMP_RIVER_ARROW_2,
    LAMP_TARGET_2_A,
    LAMP_TARGET_2_A,
  },
  {
    LAMP_RIVER_ARROW_1,
    LAMP_TARGET_2_M,
    LAMP_TARGET_2_M,
  },
  {
    LAMP_TARGET_UPPER,
    LAMP_BONUS_1,
    LAMP_BONUS_1,
  },
  {
    LAMP_BONUS_2,
    LAMP_BONUS_3,
    LAMP_BONUS_3,
  },
  {
    LAMP_BONUS_4,
    LAMP_BONUS_5,
    LAMP_BONUS_6,
  },
  {
    LAMP_LANE_LEFT,
    LAMP_LANE_RIGHT,
    LAMP_LANE_RIGHT,
  },
  {
    LAMP_TARGET_LOWER,
    LAMP_LANE_LEFT,
    LAMP_LANE_RIGHT,
  }
};

const uint8_t horizLamps[11][6] PROGMEM = {
  {
    LAMP_RIVER_ARROW_4,
    LAMP_RIVER_ARROW_3,
    LAMP_TARGET_LOWER,
    LAMP_TARGET_UPPER,
    LAMP_TARGET_UPPER,
    LAMP_TARGET_UPPER,
  },
  {
    LAMP_RIVER_ARROW_5,
    LAMP_RIVER_ARROW_2,
    LAMP_RIVER_ARROW_2,
    LAMP_RIVER_ARROW_2,
    LAMP_RIVER_ARROW_2,
    LAMP_RIVER_ARROW_2,
  },
  {
    LAMP_RIVER_ARROW_6,
    LAMP_RIVER_ARROW_1,
    LAMP_RIVER_ARROW_1,
    LAMP_RIVER_ARROW_1,
    LAMP_RIVER_ARROW_1,
    LAMP_RIVER_ARROW_1,
  },
  {
    LAMP_BEAR_MOUTH,
    LAMP_BEAR_ARROW,
    LAMP_BONUS_4,
    LAMP_TARGET_1_B,
    LAMP_TARGET_1_B,
    LAMP_TARGET_1_B,
  },
  {
    LAMP_TARGET_1_E,
    LAMP_TARGET_1_A,
    LAMP_BONUS_2,
    LAMP_BONUS_2,
    LAMP_BONUS_2,
    LAMP_BONUS_2,
  },
  {
    LAMP_TARGET_1_R,
    LAMP_BONUS_1,
    LAMP_BONUS_5,
    LAMP_BONUS_5,
    LAMP_BONUS_5,
    LAMP_BONUS_5,
  },    
  {
    LAMP_BONUS_3,
    LAMP_BONUS_3,
    LAMP_BONUS_3,
    LAMP_BONUS_3,
    LAMP_BONUS_3,
    LAMP_BONUS_3,
  },
  {
    LAMP_BONUS_6,
    LAMP_BONUS_6,
    LAMP_BONUS_6,
    LAMP_BONUS_6,
    LAMP_BONUS_6,
    LAMP_BONUS_6,
  },
  {
    LAMP_LANE_LEFT,
    LAMP_LANE_LEFT,
    LAMP_LANE_LEFT,
    LAMP_LANE_LEFT,
    LAMP_LANE_LEFT,
    LAMP_LANE_LEFT,
  },
  {
    LAMP_TARGET_2_L,
    LAMP_TARGET_2_O,
    LAMP_TARGET_2_G,
    LAMP_TARGET_2_J,
    LAMP_TARGET_2_A,
    LAMP_TARGET_2_M,
  },
  {
    LAMP_LANE_RIGHT,
    LAMP_LANE_RIGHT,
    LAMP_LANE_RIGHT,
    LAMP_LANE_RIGHT,
    LAMP_LANE_RIGHT,
    LAMP_LANE_RIGHT,
  }
};


#define BURST_CENTER_ROW  7     // middle row of vertLamps (15 rows)
#define BURST_REPEATS     4

void attractLamps(timerEvent id, uint16_t state)
{
  static uint8_t stateCount;
  static uint8_t stateRepeat;

  switch(state)
  {
    case 0: 
      stateCount = 0;
      stateRepeat = 0;
      setLampMode( LAMP_BACKBOX_BEAR_3, LAMP_FLASH_STATE, QUARTER_SECOND, 4 );
      setTimer(ATTRACT_LAMP_TMR, ONE_HUNDRETH_SECOND, ++state);
      break;

    case 1:
      // round cycles through all lights on playfield, 
      // double-flash creates chasing effect
      setTimer(ATTRACT_LAMP_TMR, FIVE_HUNDRETH_SECONDS, state);
      setLampMode(stateCount++, LAMP_BLINK_STATE, QUARTER_SECOND, 3 );
      if(stateCount > LAMP_LAST_PLAYFIELD) {
        stateCount = 0;
        if ( stateRepeat++ > 2 ) {
          stateCount = 0;
          stateRepeat = 0;
          setTimer(ATTRACT_LAMP_TMR, QUARTER_SECOND, ++state);
        }
      }
      break;

    case 2:
      // left to right and back, mostly off
      setTimer(ATTRACT_LAMP_TMR, FIVE_HUNDRETH_SECONDS, state);
      for (uint8_t i=0; i<6; i++) {
        lamp light;
        if ( (stateRepeat & 0x01) == 0 ){
          light = pgm_read_byte(&horizLamps[stateCount][i]);
        } else {
          light = pgm_read_byte(&horizLamps[10-stateCount][i]);
        }
        setLampMode( light, LAMP_BLINK_ON_STATE, QUARTER_SECOND, 1 );
      }
      if ( stateCount++ >= 10 ) {
        stateCount = 0;
        if ( stateRepeat++ > 4 ) {
          stateRepeat = 0;
          setTimer(ATTRACT_LAMP_TMR, ONE_HUNDRETH_SECOND, ++state);
        }
      }
      break;
      
    case 3:
      // all on
      for (uint8_t i=0; i<=LAMP_LAST_PLAYFIELD; i++) {
        setLampMode( i, LAMP_ON_STATE, 0, INFINITE );
      }
      setTimer(ATTRACT_LAMP_TMR, ONE_HUNDRETH_SECOND, ++state);
      stateCount = 0;
      break;      
      
    case 4:
      // super fast round cycles through all lights on playfield
      // mostly on
      setTimer(ATTRACT_LAMP_TMR, ONE_HUNDRETH_SECOND, state);
      setLampMode(stateCount++, LAMP_FLASH_STATE, EIGTH_SECOND, 1 );
      if(stateCount > LAMP_LAST_PLAYFIELD) {
        stateCount = 0;
        if ( stateRepeat++ > 3 ) {
          stateCount = 0;
          stateRepeat = 0;
          setTimer(ATTRACT_LAMP_TMR, QUARTER_SECOND, ++state);
        }
      }
      break;

    case 5:
      // top to bottom and back, mostly off
      setTimer(ATTRACT_LAMP_TMR, FIVE_HUNDRETH_SECONDS, state);
      for (uint8_t i=0; i<3; i++) {
        lamp light;
        if ( (stateRepeat & 0x01) == 0 ){
          light = pgm_read_byte(&vertLamps[stateCount][i]);
        } else {
          light = pgm_read_byte(&vertLamps[14-stateCount][i]);
        }
        setLampMode( light, LAMP_BLINK_ON_STATE, QUARTER_SECOND, 1 );
      }
      if ( stateCount++ >= 14 ) {
        stateCount = 0;
        if ( stateRepeat++ > 4 ) {
          stateRepeat = 0;
          setTimer(ATTRACT_LAMP_TMR, ONE_HUNDRETH_SECOND, ++state);
        }
      }
      break;

    case 6:
      // all on
      for (uint8_t i=0; i<=LAMP_LAST_PLAYFIELD; i++) {
        setLampMode( i, LAMP_ON_STATE, 0, INFINITE );
      }
      setTimer(ATTRACT_LAMP_TMR, ONE_HUNDRETH_SECOND, ++state);
      stateCount = 0;
      break;

    case 7:
      // center burst: a ring of light spreads from the middle row of the
      // playfield out to the top and bottom, a few times
      for (uint8_t i=0; i<3; i++) {
        lamp up   = pgm_read_byte(&vertLamps[BURST_CENTER_ROW-stateCount][i]);
        lamp down = pgm_read_byte(&vertLamps[BURST_CENTER_ROW+stateCount][i]);
        setLampMode( up,   LAMP_BLINK_ON_STATE, EIGTH_SECOND, 1 );
        setLampMode( down, LAMP_BLINK_ON_STATE, EIGTH_SECOND, 1 );
      }
      setTimer(ATTRACT_LAMP_TMR, FIVE_HUNDRETH_SECONDS, state);
      if ( ++stateCount > BURST_CENTER_ROW ) {
        stateCount = 0;
        setTimer(ATTRACT_LAMP_TMR, HALF_SECOND, state);   // pause between bursts
        if ( ++stateRepeat >= BURST_REPEATS ) {
          stateRepeat = 0;
          setTimer(ATTRACT_LAMP_TMR, HALF_SECOND, 0);     // back to the start
        }
      }
      break;
  }
}


// animation step for ATTRACT_PLAY / ATTRACT_FEVER / ATTRACT_PINBALL. reset
// whenever the state changes, or attract restarts, so an animation
// interrupted part way (game start, coin, start without credits) starts
// from the beginning.
static uint8_t attractCycles = 0;
static uint16_t attractLastState = 0xFFFF;
static boolean endScoreShown = FALSE;   // end-of-game hold has shown the score

// how many ONE_HUNDRETH_SECOND ticks to pause once a sliding word reaches
// the center of the display.
#define SLIDE_WORD_HOLD_TICKS  (HALF_SECOND/ONE_HUNDRETH_SECOND)

// slides `word` onto the display from one edge, pauses centered for half a
// second, then continues off the opposite edge. fromLeft slides left->right
// (entering from the left, exiting right); otherwise it's mirrored. returns
// true while still animating, false once the word has fully exited.
typedef enum { SLIDE_ENTER, SLIDE_HOLD, SLIDE_EXIT } slidePhase_t;

static boolean attractSlideWord(const char *word, boolean fromLeft)
{
  static int8_t slideX = 0;
  static uint8_t holdTicks = 0;
  static slidePhase_t phase = SLIDE_ENTER;

  int8_t w = displayTextWidth(word);
  int8_t centerX = ((LED_BACKPACKS*LED_COLUMNS) - w) / 2;
  int8_t step = fromLeft ? 1 : -1;

  if ( attractCycles == 0 ) {
    attractCycles = 1;   // marks this state as initialized
    slideX = fromLeft ? -w : (LED_BACKPACKS*LED_COLUMNS);
    phase = SLIDE_ENTER;
    holdTicks = 0;
  }

  boolean exited = fromLeft ? ( slideX >= (LED_BACKPACKS*LED_COLUMNS) )
                             : ( slideX + w <= 0 );
  if ( exited ) {
    clearDisplay();   // wipe the last sliver of the glyph left at the edge
    return FALSE;
  }

  displayView( slideX, 1 );
  displayText( "%s", word );

  switch ( phase ) {
    case SLIDE_ENTER:
      if ( slideX == centerX ) {
        phase = SLIDE_HOLD;
        holdTicks = 0;
      } else {
        slideX += step;
      }
      break;
    case SLIDE_HOLD:
      if ( ++holdTicks >= SLIDE_WORD_HOLD_TICKS ) {
        phase = SLIDE_EXIT;
        slideX += step;   // leave center now, so SLIDE_HOLD can't re-trigger
      }
      break;
    case SLIDE_EXIT:
      slideX += step;
      break;
  }
  return TRUE;
}

// shows a word right justified, SCORE_RIGHT_MARGIN in from the right edge
static void displayTextRight(const char *s)
{
  displayView( (LED_BACKPACKS*LED_COLUMNS) - displayTextWidth(s) - SCORE_RIGHT_MARGIN, 1 );
  displayText( "%s", s );
}

// cycles display through attract states
void attractTimer(timerEvent id, uint16_t state)
{
  if(gameOn) return;

  // the lamp shows start with the rest of attract, after the end-of-game
  // hold. any way out of the hold (coin, start without credits) starts them.
  if ( state != ATTRACT_ENDED && state != ATTRACT_ENDED_1 &&
       state != ATTRACT_ENDED_SCORE &&
       !isActiveTimer(ATTRACT_LAMP_TMR) ) {
    setTimer(ATTRACT_LAMP_TMR, ONE_HUNDRETH_SECOND, 0);
  }

  if ( state != attractLastState ) {
    attractCycles = 0;
    attractLastState = state;
  }

  switch(state) {
  case ATTRACT_PLAY:        // "Cabin" slides in from the left
    if ( attractSlideWord("Cabin", TRUE) ) {
      setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_PLAY);
    } else {
      setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_FEVER);
    }
    break;
  case ATTRACT_FEVER:        // "Fever" slides in from the right
    if ( attractSlideWord("Fever", FALSE) ) {
      setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_FEVER);
    } else {
      setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_PINBALL);
    }
    break;
  case ATTRACT_PINBALL:      // "Pinball" slides in from the left
    if ( attractSlideWord("Pinball", TRUE) ) {
      setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_PINBALL);
    } else {
      // straight on: pinball has already left the display, and waiting
      // here left it blank for a second
      setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_SCORE);
    }
    break;
  case ATTRACT_SCORE:       // label first, like "High Score"
    displayText( "Score" );
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_SCORE_1);
    break;
  case ATTRACT_SCORE_1:
    displayScore( 0 );
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_HIGH_SCORE);
    break;
  case ATTRACT_HIGH_SCORE:
    displayText( "High Score" );  // scrolls, give it time to finish
    setTimer(ATTRACT_TMR, TWO_SECONDS, ATTRACT_HIGH_SCORE_1);
    break;
  case ATTRACT_HIGH_SCORE_1:
    {
      char s[12] = {0,};
      sprintf(s,"%ld", nonVolatiles.highScore);
      displayView( (LED_BACKPACKS*LED_COLUMNS) - displayTextWidth(s) - SCORE_RIGHT_MARGIN, 1 );
      displayText("%s", s);
    }
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_GAME_OVER);
    break;
  case ATTRACT_GAME_OVER:
    displayText( "Game" );            // left justified
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_GAME_OVER_1);
    break;
  case ATTRACT_GAME_OVER_1:
    displayTextRight( "Over" );
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_CREDITS);
    break;
  case ATTRACT_CREDITS:
    if ( nonVolatiles.coinsPerGame > 0 ) {
      displayText( "coins" );
      setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_CREDITS_1);
    } else {
      setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_PLAY);
    }
    break;
  case ATTRACT_CREDITS_1:
    displayText("%d of %d", credits, nonVolatiles.coinsPerGame );
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_PLAY);
    break;
  case ATTRACT_INSERT_COINS:
    displayText("%d of %d", credits, nonVolatiles.coinsPerGame );
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_INSERT_COINS_1);
    break;
  case ATTRACT_INSERT_COINS_1:
    displayText( "insert");
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_INSERT_COINS_2);
    break;
  case ATTRACT_INSERT_COINS_2:
    displayText( "coins" );
    // straight to the cabin slide, not ATTRACT_BLANK's second of nothing
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_PLAY);
    break;
  case ATTRACT_BLANK:
    clearDisplay();
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_PLAY);
    break;
  case ATTRACT_FREE:
    displayText( "freegame" );
    setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_SCORE);
    break;
  case ATTRACT_ENDED:
    // music over and the score has been shown at least once: attract
    if ( endScoreShown && !isGameOverStingPlaying() ) {
      setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_PLAY);
      break;
    }
    displayText( "Game" );            // left justified
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_ENDED_1);
    break;
  case ATTRACT_ENDED_1:
    displayTextRight( "Over" );
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_ENDED_SCORE);
    break;
  case ATTRACT_ENDED_SCORE:
    displayScore( 0 );
    endScoreShown = TRUE;
    setTimer(ATTRACT_TMR, TWO_SECONDS,
             isGameOverStingPlaying() ? ATTRACT_ENDED : ATTRACT_PLAY);
    break;
  }
}


void cancelBoxTimer(void)
{
  cancelTimer( BOX_TMR );
  setLampMode( LAMP_BACKBOX_BEAR_1, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_BACKBOX_BEAR_2, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_BACKBOX_BEAR_3, LAMP_ON_STATE, 0, INFINITE );
} 


void boxTimer(timerEvent evt, uint16_t state)
{
  switch ( state )
  {
    case 0:
      setLampMode( LAMP_BACKBOX_BEAR_3, LAMP_ON_STATE, 0, INFINITE );
      setLampMode( LAMP_BACKBOX_BEAR_1, LAMP_OFF_STATE, 0, INFINITE );
      state++;
      break;
    case 1:
      setLampMode( LAMP_BACKBOX_BEAR_1, LAMP_ON_STATE, 0, INFINITE );
      setLampMode( LAMP_BACKBOX_BEAR_2, LAMP_OFF_STATE, 0, INFINITE );
      state++;
      break;
    case 2:
      setLampMode( LAMP_BACKBOX_BEAR_2, LAMP_ON_STATE, 0, INFINITE );
      setLampMode( LAMP_BACKBOX_BEAR_3, LAMP_OFF_STATE, 0, INFINITE );
      state=0;
      break;
  }

  setTimer( BOX_TMR, FIVE_HUNDRETH_SECONDS, state );
}


void crazyModeTimer(timerEvent id, uint16_t state)
{
  if (state++>4) {
    state = 0;
  }
  setLampMode( LAMP_BONUS_1 + state, LAMP_BLINK_STATE, EIGTH_SECOND, 3 );
  setTimer(CRAZY_TMR, TEN_HUNDRETH_SECONDS, state);
}


void spinTimer(timerEvent id, uint16_t state)
{
  static uint8_t countDown = 0;
  static uint8_t count = 0;
  
  if ( state > 0 ) {
    if ( countDown < 4 ) {
      countDown++;
    }
  }
  
  setLampMode( LAMP_RIVER_ARROW_1 + count, LAMP_BLINK_ON_STATE, TEN_HUNDRETH_SECONDS, 2 );

  setTimer(SPIN_TMR, FOUR_HUNDRETH_SECONDS, 0);

  if ( count++ > 4 ) {
    count = 0;
    if ( --countDown == 0 ) {
      cancelTimer(SPIN_TMR);
      // shoot the blinking light
      setLampMode(LAMP_RIVER_ARROW_1, LAMP_BLINK_ON_STATE, EIGTH_SECOND, INFINITE);
    }
  }
}



static uint8_t loadRetries = 0;

// bear mode: from the head opening (BEAR targets completed) until it closes
// again. it carries across a drain, so the next ball keeps the bear music.
static boolean bearMode = FALSE;

// the background music for the game as it stands right now
static sounds_t gameMusic(void)
{
  return bearMode ? SOUND_BEAR_MUSIC : SOUND_MAIN_MUSIC;
}

// the high score was beaten while a ball was loading. GAME_TMR is busy with
// the load then, so the "freegame" message waits until GAME_SCORE.
static boolean freeCreditPending = FALSE;

void gameTimer(timerEvent id, uint16_t state)
{
  if(!gameOn) return;

  switch(state) {
  case GAME_BALL_LOAD:
    ballIsLoading = TRUE;
    playMusic( gameMusic() );   // bear music if the bear is still open
    hitSolenoid( SOLENOID_BALL_LOADER );
    displayText( "ball %d", ballInPlay );
    setTimer(GAME_TMR, TWO_SECONDS, GAME_BALL_CHECK);
    break;    
  case GAME_BALL_CHECK:
    // the ball rests on the drain switch until it's loaded. if the switch
    // is still closed, the loader kick didn't move it and nothing else will
    // retry (the drain switch never sees the ball arrive again). a few tries
    // only, so a stuck switch doesn't fire the loader all game.
    if ( isSwitchClosed( SWITCH_BALL_DRAIN ) && loadRetries < 3 ) {
      loadRetries++;
      setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_BALL_LOAD);
    } else {
      loadRetries = 0;
      setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_SCORE);
    }
    break;
  case GAME_BALL_AGAIN:
    hitSolenoid( SOLENOID_BALL_LOADER );
    displayText( "shoot" );
    setTimer(GAME_TMR, ONE_SECOND, GAME_BALL_AGAIN_1);
    break;
  case GAME_BALL_AGAIN_1:
    displayText( "again" );
    setTimer(GAME_TMR, ONE_SECOND, GAME_BALL_CHECK);
    break;
  case GAME_SCORE:
    // one shot. timers repeat until cancelled, and this used to redraw the
    // score every few ticks, wiping out the green score flash.
    cancelTimer(GAME_TMR);
    ballIsLoading = FALSE;
    if ( freeCreditPending ) {
      freeCreditPending = FALSE;
      setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_FREE_CREDIT);
      break;
    }
    scoreDisplayDisabled = FALSE;
    displayScore( 0 );
    break;
  case GAME_FREE_CREDIT:
    scoreDisplayDisabled = TRUE;
    displayText( "freegame" );
    setTimer(GAME_TMR, TWO_SECONDS, GAME_SCORE);
    break;
  case GAME_TILT:
    scoreDisplayDisabled = TRUE;
    // eject rather than just close: if a ball was being chewed when the
    // game tilted, closing would trap it in the bear and the game could
    // never continue. an eject with no ball just kicks the empty capture.
    setTimer(BEAR_TMR, ONE_SECOND, BEAR_EJECT);
    cancelTimer(BOX_TMR);   // backbox bear chase would keep lighting lamps
    for (int i=0; i<=LAMP_LAST; i++) {
      setLampMode( i, LAMP_OFF_STATE, 0, INFINITE );
    }
    setLampMode( LAMP_PLAYFIELD_GI, LAMP_FLASH_STATE, THREE_SECONDS, 1 );
    setLampMode( LAMP_BACKBOX_TILT, LAMP_ON_STATE, 0, INFINITE );
    playSound( SOUND_GAME_TILT );
    flipperEnableSolenoids( false );
    allSolenoidsOff();
    displayText( "Tilt" );
    setTimer(GAME_TMR, THREE_SECONDS, GAME_BALL_WAIT);
    break;
  case GAME_BALL_WAIT:
  default:
    break;
  }
}

// "bear attack" while the bear chews and ejects a ball: music off, every
// lamp off (except the start button), and the roars in chewSteps. the
// game's lamps are saved first and put back when the bear closes, which
// also restarts the main music.
static boolean bearAttack = FALSE;

// playfield sparkle while the bear is open waiting for the ball, until the
// chew starts. it's an output overlay (lampSparkle), so the game's lamps
// keep working underneath.
static boolean bearSparkle = FALSE;

static void bearSparkleStart(void)
{
  if ( bearSparkle || !gameOn || tilt ) return;
  bearSparkle = TRUE;
  lampSparkleBegin();
  setTimer( SPARKLE_TMR, ONE_HUNDRETH_SECOND, 0 );
}

static void bearSparkleEnd(void)
{
  bearSparkle = FALSE;
  cancelTimer( SPARKLE_TMR );
  lampSparkleClear();
}

// chew timing: jaw open (roar playing), then jaw shut, three times
// BEAR_CHEW_SLOWDOWN > 1 slows the chew down for debugging
#define BEAR_CHEW_SLOWDOWN     1
#define BEAR_CHEW_OPEN_TIME    (BEAR_CHEW_SLOWDOWN*((QUARTER_SECOND + HALF_SECOND + EIGTH_SECOND)/2))   // ~435 ms
#define BEAR_CHEW_CLOSED_TIME  (BEAR_CHEW_SLOWDOWN*QUARTER_SECOND)

// the bear runs fixed sequences of steps: chewSteps from a capture, and
// ejectSteps to free the capture (tilt). each step sets the jaw (and the
// mouth / arrow lamps with it: mouth lit while open, arrow lit while shut),
// optionally cuts the roar that's playing, starts a sound, fires the eject
// coil, then holds for `time`. after the last step the bear closes. sounds
// and lamps only happen during a bear attack (in play, not tilted).
typedef struct {
  boolean  open;     // jaw position
  uint8_t  slew;     // jaw speed: ticks per servo step (see bearMoveTo)
  boolean  cut;      // stop the roar that's playing first
  sounds_t sound;    // sound to start, SOUND_STOP for none
  boolean  kick;     // fire the eject coil
  uint16_t time;     // hold before the next step
} bearStep_t;

// jaw speeds, ticks per servo step. open <-> shut is ~5 steps.
#define SLEW_SLAM   1    // ~10 ms a step, ~50 ms: snaps shut on the ball
#define SLEW_CHOMP  3    // ~20 ms a step, ~100 ms: chomp shutting
#define SLEW_CHOMP_OPEN  15   // ~80 ms a step, ~400 ms: chomp opening
#define SLEW_FINAL_CHOMP 15   // ~80 ms a step, ~400 ms: final chomp, spits the ball
#define SLEW_EJECT  4    // ~25 ms a step, ~125 ms: plain eject (tilt)

// capture: slam shut on the ball (silent), three roaring chomps, then the
// final chomp, which opens slowly and spits the ball out with ROAR2
static const bearStep_t chewSteps[] = {
  { FALSE, SLEW_SLAM,        TRUE,  SOUND_STOP,       FALSE, BEAR_CHEW_CLOSED_TIME },
  { TRUE,  SLEW_CHOMP_OPEN,  FALSE, SOUND_BEAR_CHOMP, FALSE, BEAR_CHEW_OPEN_TIME   },   // ROAR1
  { FALSE, SLEW_CHOMP,       TRUE,  SOUND_STOP,       FALSE, BEAR_CHEW_CLOSED_TIME },
  { TRUE,  SLEW_CHOMP_OPEN,  FALSE, SOUND_BEAR_CHOMP, FALSE, BEAR_CHEW_OPEN_TIME   },   // ROAR1
  { FALSE, SLEW_CHOMP,       TRUE,  SOUND_STOP,       FALSE, BEAR_CHEW_CLOSED_TIME },
  { TRUE,  SLEW_CHOMP_OPEN,  FALSE, SOUND_BEAR_CHOMP, FALSE, BEAR_CHEW_OPEN_TIME   },   // ROAR1
  { FALSE, SLEW_CHOMP,       TRUE,  SOUND_STOP,       FALSE, BEAR_CHEW_CLOSED_TIME },
  // final chomp: ROAR2 (played out) as it starts to open, HALF_SECOND to
  // open before the kick
  { TRUE,  SLEW_FINAL_CHOMP, FALSE, SOUND_BEAR_SPIT,  FALSE, HALF_SECOND           },   // ROAR2
  { TRUE,  SLEW_FINAL_CHOMP, FALSE, SOUND_STOP,       TRUE,  HALF_SECOND           },   // kick
};

// plain eject (tilt): open, kick whatever is in the capture, close. silent.
static const bearStep_t ejectSteps[] = {
  { TRUE,  SLEW_EJECT,       FALSE, SOUND_STOP,       FALSE, HALF_SECOND           },
  { TRUE,  SLEW_EJECT,       FALSE, SOUND_STOP,       TRUE,  HALF_SECOND           },   // kick
};

// BEAR_TMR data while a sequence runs: sequence id + step number
#define BEAR_SEQ_CHEW    0x100
#define BEAR_SEQ_EJECT   0x200
#define BEAR_SEQ_MASK    0xFF00

static void bearStep( uint16_t seq, uint8_t i )
{
  const bearStep_t *steps = ( seq == BEAR_SEQ_EJECT ) ? ejectSteps : chewSteps;
  uint8_t qty = ( seq == BEAR_SEQ_EJECT )
                  ? sizeof(ejectSteps)/sizeof(ejectSteps[0])
                  : sizeof(chewSteps)/sizeof(chewSteps[0]);
  if ( i >= qty ) {
    bearTimer( BEAR_TMR, BEAR_CLOSE );
    return;
  }
  const bearStep_t *s = &steps[i];
  bearMoveTo( s->open ? nonVolatiles.headOpen : nonVolatiles.headClose, s->slew );
  if ( bearAttack ) {
    if ( s->cut ) stopEffects();
    if ( s->sound != SOUND_STOP ) playSound( s->sound );
    setLampMode( LAMP_BEAR_MOUTH, s->open ? LAMP_ON_STATE : LAMP_OFF_STATE, 0, INFINITE );
    setLampMode( LAMP_BEAR_ARROW, s->open ? LAMP_OFF_STATE : LAMP_ON_STATE, 0, INFINITE );
  }
  if ( s->kick ) hitSolenoid( SOLENOID_BEAR_CAPTURE );
  setTimer( BEAR_TMR, s->time, seq + i + 1 );
}

// one twinkle every ~25 ms, each ~50 ms. stops itself on a tilt or game
// over; the bear closing stops it too.
void sparkleTimer(timerEvent id, uint16_t state)
{
  if ( !bearSparkle || !gameOn || tilt ) {
    bearSparkleEnd();
    return;
  }
  lampSparkle( rand() % (LAMP_LAST_PLAYFIELD+1) );
  setTimer( SPARKLE_TMR, TWO_HUNDRETH_SECONDS, 0 );
}

static void bearAttackStart(void)
{
  // a bouncing capture switch restarts the chew. saving again then would
  // save the darkened lamps instead of the game's.
  if ( bearAttack || !gameOn || tilt ) return;
  bearAttack = TRUE;
  bearSparkleEnd();   // the chew is dark, no sparkle
  lampSaveAll();
  cancelTimer( BOX_TMR );   // backbox bear chase from bear mode
  for ( uint8_t i = 0; i <= LAMP_LAST; i++ ) {
    if ( i != LAMP_START_BUTTON ) {
      setLampMode( i, LAMP_OFF_STATE, 0, INFINITE );
    }
  }
  playMusic( SOUND_STOP );
}

// restore: put the game's lamps back. not after a tilt or game over, which
// have set the lamps they want.
static void bearAttackEnd(boolean restore)
{
  if ( !bearAttack ) return;
  bearAttack = FALSE;
  if ( restore ) lampRestoreAll();
}

// cycles the bear through its various motions
//   modes are activated by "BEAR" targets and bear capture
void bearTimer(timerEvent id, uint16_t state)
{
  if ( state & BEAR_SEQ_MASK ) {   // in a chew or eject sequence
    bearStep( state & BEAR_SEQ_MASK, state & 0xFF );
    return;
  }

  switch(state) {
  case BEAR_OPEN:
    bearMode = TRUE;
    playSound( SOUND_BEAR_OPEN );
    playMusic( SOUND_BEAR_MUSIC );
    setLampMode( LAMP_BEAR_ARROW, LAMP_BLINK_STATE, EIGTH_SECOND, INFINITE );
    setLampMode( LAMP_BEAR_MOUTH, LAMP_BLINK_STATE, EIGTH_SECOND, INFINITE );
    bearSparkleStart();   // while waiting for the ball
    // thru...
  case BEAR_HOLD_OPEN:
    bearSetPulse(nonVolatiles.headOpen);
    setTimer(BEAR_TMR, HALF_SECOND, BEAR_HOLD_OPEN);  // keep open
    break;
  case BEAR_CHEW:     // ball captured: chew, spit it out, close
    bearAttackStart();
    bearStep( BEAR_SEQ_CHEW, 0 );
    break;
  case BEAR_EJECT:    // plain eject (tilt), to free a captured ball
    bearStep( BEAR_SEQ_EJECT, 0 );
    break;
  case BEAR_TEST:
    setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_TEST);
    break;
  case BEAR_CLOSE:
    bearMode = FALSE;
    bearSparkleEnd();
    bearAttackEnd( gameOn && !tilt );
    // the backbox bear chase ends with the bear mode
    cancelTimer( BOX_TMR );
    // lamps only during play. gameOver() and a tilt also close the bear,
    // and resetting the target lamps then disturbed attract / tilt lamps.
    if ( gameOn && !tilt ) {
      cancelBoxTimer();   // backbox bears back to steady on
      setLampMode( LAMP_BEAR_ARROW, LAMP_OFF_STATE, 0, INFINITE );
      setLampMode( LAMP_BEAR_MOUTH, LAMP_OFF_STATE, 0, INFINITE );
      // and back to shoot the blinking lights
      for(int i = LAMP_TARGET_1_B; i <= LAMP_TARGET_1_R; i++) {
        setLampMode(i, LAMP_BLINK_ON_STATE, EIGTH_SECOND, INFINITE);
      }
      playMusic( SOUND_MAIN_MUSIC );
    }
    // thru...
  case BEAR_HOLD_CLOSE:
  default:
    bearSetPulse(nonVolatiles.headClose);
    setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_HOLD_CLOSE);  // keep closed
    break;
  }
}


void startGame(void)
{
  resetScore();
  gameOn = TRUE;
  highScoreExceeded = FALSE;
  freeCreditPending = FALSE;
  bearAttack = FALSE;
  bearMode = FALSE;
  bearSparkleEnd();
  ballInPlay = 0;
  multiplier = 1;
  crazyMode = FALSE;

  cancelTimer(ATTRACT_TMR);
  cancelTimer(ATTRACT_LAMP_TMR);
  // all lamps off first. this loop used to run after the start button was
  // lit, so the start button lamp was always off during a game.
  for (uint8_t i=0; i<LAMP_QTY; i++) {
    setLampMode(i, LAMP_OFF_STATE, 0, 1);
  }
  setLampMode( LAMP_BACKBOX_GAME_OVER, LAMP_OFF_STATE, 0, INFINITE );
  setLampMode( LAMP_START_BUTTON, LAMP_ON_STATE, 0, INFINITE );

  nextBall(FALSE);
}

void nextBall(boolean drained)
{
  tilt = FALSE;
  tiltSense = 0;
  loadRetries = 0;
  flipperEnableSolenoids( true );

  setLampMode( LAMP_BACKBOX_TILT, LAMP_OFF_STATE, 0, INFINITE );
  setLampMode( LAMP_BACKBOX_BEAR_1, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_BACKBOX_BEAR_2, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_BACKBOX_BEAR_3, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_PLAYFIELD_GI, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_POP_BUMPER_UPPER, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_POP_BUMPER_LOWER, LAMP_ON_STATE, 0, INFINITE );
  
  // shoot the blinking lights
  lamp lamps[] = { LAMP_RIVER_ARROW_1, 
                   LAMP_TARGET_1_B, LAMP_TARGET_1_E, LAMP_TARGET_1_A, LAMP_TARGET_1_R,
                   LAMP_TARGET_2_L, LAMP_TARGET_2_O, LAMP_TARGET_2_G, LAMP_TARGET_2_J, LAMP_TARGET_2_A, LAMP_TARGET_2_M };
  for (int i=0; i<sizeof(lamps)/sizeof(lamps[0]); i++) {
    if ( getLampMode( lamps[i] ) == LAMP_OFF_STATE ) {
      setLampMode(lamps[i], LAMP_BLINK_ON_STATE, EIGTH_SECOND, INFINITE);
    }
  }
  // shoot the blinking light
  for (int i=LAMP_BONUS_1; i<LAMP_BONUS_1+multiplier; i++) {
    if ( (getLampMode(i) == LAMP_OFF_STATE) ) {
      setLampMode(i, LAMP_ON_STATE, INFINITE, 1);
    }
  }
  if ( (getLampMode(LAMP_BONUS_1 + multiplier) != LAMP_BLINK_ON_STATE) ) {
    setLampMode(LAMP_BONUS_1 + multiplier, LAMP_BLINK_ON_STATE, EIGTH_SECOND, INFINITE);
  }

  if (ballInPlay++ < nonVolatiles.ballsPerGame) {
    if ( drained ) playSound( SOUND_DRAIN );
    setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_BALL_LOAD);
  } else {
    // game over sound first, while gameOn is still set so it can play
    playSound( SOUND_GAME_END );
    gameOver();
  }
}

void gameOver(void)
{
  if ( highScoreExceeded ) {
    // clear it so another gameOver() (door menu escape in attract)
    // doesn't award the free game and count the stat again
    highScoreExceeded = FALSE;
    nonVolatiles.freeGameHighScore++;
    increaseCredits( nonVolatiles.coinsPerGame, FALSE );
    nonVolatiles.highScore = score;
  }
  // save every game so the games played stat isn't lost at power off.
  // saveNonVolatiles() only writes bytes that changed.
  saveNonVolatiles();

  gameOn = FALSE;
  freeCreditPending = FALSE;
  bearAttack = FALSE;   // gameOver() sets its own lamps, nothing to restore
  bearMode = FALSE;
  bearSparkleEnd();
  // a game ended from the door menu while tilted would otherwise leave
  // tilt set in attract mode, where it blocks the start button
  tilt = FALSE;

  // flippers mashed right after drain/game-over shouldn't immediately growl
  resetBearNoiseCooldown();

  flipperEnableSolenoids( false );
  allSolenoidsOff();
  playMusic( SOUND_STOP );

  for (uint8_t i=0; i<LAMP_QTY; i++) {
    setLampMode(i, LAMP_OFF_STATE, 0, 1);
  }

  setLampMode( LAMP_BACKBOX_GAME_OVER, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_PLAYFIELD_GI, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_BACKBOX_RIVER, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_BACKBOX_CABIN, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_START_BUTTON, LAMP_BLINK_STATE, HALF_SECOND, INFINITE );
    
  cancelTimer(CRAZY_TMR);
  cancelTimer(SPIN_TMR);
  cancelTimer(TILT_TMR);
  cancelTimer(GAME_TMR);
  cancelTimer(SCORE_FLASH_TMR);
  cancelBoxTimer();
  setTimer(BEAR_TMR, QUARTER_SECOND, BEAR_CLOSE);
  attractLastState = 0xFFFF;   // start the attract animations fresh
  // hold on "Game" "Over" and the final score while the game over music plays.
  // attract (display and lamp shows) starts once it finishes.
  endScoreShown = FALSE;
  cancelTimer(ATTRACT_LAMP_TMR);
  setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_ENDED);
}


//////////////////////////////////
// credits
//////////////////////////////////
void increaseCredits(uint8_t value, boolean coinMode)
{
  credits += value;
  if  (credits > 99) credits = 99;
  if (coinMode ) {
    setIfActiveTimer(ATTRACT_TMR, ATTRACT_CREDITS_1);  
  }
}

boolean decreaseCredits(void)
{
  if ( credits < nonVolatiles.coinsPerGame ) 
    return FALSE;
  
  credits -= nonVolatiles.coinsPerGame;
  nonVolatiles.gamesPlayed++;   // was counted even when there weren't enough credits
  return TRUE;
}


//////////////////////////////////
// scoring and non volatiles
//////////////////////////////////

static uint8_t checkCalc( uint8_t sum, uint8_t *data, uint8_t size )
{
  uint8_t i;
  for (i=0; i<size; i++) {
    sum += data[ i ];
  }
  return sum;
}

void resetNonVolatiles(void)
{
  nonVolatiles_t nv = {
    .highScore = 50000,
    .headOpen = 20,
    .headClose = 25,
    .volume = 2,
    .tiltSensitivity = 3,
    .ballsPerGame = 3,
    .coinsPerGame = 0,
    .gamesPlayed = 0, // stats
    .freeGameMatch = 0, // stats
    .freeGameHighScore = 0, // stats
    .checkSum = 0
  };
  memcpy( &nonVolatiles, &nv, sizeof(nv) );
}

void loadNonVolatiles(void)
{
  uint8_t sum;
  eeprom_read_block( (uint8_t*)&nonVolatiles, (uint8_t*)0, sizeof(nonVolatiles) );
  sum = checkCalc( 0, (uint8_t*)&nonVolatiles, sizeof(nonVolatiles) - sizeof(nonVolatiles.checkSum ) );
  if ( sum != nonVolatiles.checkSum ) {
    resetNonVolatiles();
  }
}

void saveNonVolatiles(void) 
{
  nonVolatiles.checkSum = 
      checkCalc( 0, (uint8_t*)&nonVolatiles, sizeof(nonVolatiles) - sizeof(nonVolatiles.checkSum ) );
  eeprom_update_block( (uint8_t*)&nonVolatiles, (uint8_t*)0, sizeof(nonVolatiles) );
}


void displayScore(uint8_t color)
{
  extern uint8_t invertDisplayColors;

  char s[12] = {0,};
  sprintf(s,"%ld", score);
  displayView( (LED_BACKPACKS*LED_COLUMNS) - displayTextWidth(s) - SCORE_RIGHT_MARGIN, 1 );
  // the colour only applies to this frame. left set, any message drawn
  // during a flash (tilt, freegame, GameOver) came out on solid green.
  invertDisplayColors = color;
  displayText("%s", s);
  invertDisplayColors = 0;
}

// ends the green score flash triggered by increaseScore(), unless some
// other message (tilt, freegame, ...) has taken over the display since.
void scoreFlashTimer(timerEvent id, uint16_t state)
{
  // one shot. left repeating, it redrew the score every half second and
  // overwrote "ball N", "shoot again" and the door menu.
  cancelTimer(SCORE_FLASH_TMR);
  if ( !gameOn ) return;   // belt and suspenders; gameOver() also cancels this
  if ( !scoreDisplayDisabled ) {
    displayScore( 0 );
  }
}

void resetScore(void)
{
  score = 0;
  displayScore( 0 );
}

uint32_t getScore(void)
{
  return score;
}

void increaseScore(uint16_t value)
{
  score += value * multiplier;

  if ( !scoreDisplayDisabled ) {
    displayScore( 0xff );
    setTimer(SCORE_FLASH_TMR, SCORE_FLASH_DURATION, 0);
  }

  if ( !highScoreExceeded )
  {
    if ( score > nonVolatiles.highScore )
    {
      highScoreExceeded = TRUE;
      if ( ballIsLoading ) {
        // don't replace the pending ball check / load on GAME_TMR
        freeCreditPending = TRUE;
      } else {
        setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_FREE_CREDIT);
      }
      hitSolenoid( SOLENOID_KNOCKER );
    }
  }
}


void doorMenu(slowSwitch sw)
{
  typedef enum {
    modeNone,
    modeBalls,
    modeCoins,
    modeStats_1,
    modeStats_2,
    modeStats_3,
    modeTiltSensitivity,
    modeHeadOpen,
    modeHeadClose,
    modeReset,
    modeMax
  } configModes_t;
  
  static configModes_t mode = 0;

  switch(sw) {
    case SWITCH_TEST_ENTER:
      if ( mode == 0 ) {
        cancelTimer(BEAR_TMR);
        cancelTimer(ATTRACT_TMR);
        cancelTimer(ATTRACT_LAMP_TMR);
        cancelTimer(GAME_TMR);
      } 
      if ( ++mode >= modeMax ) {
        mode = 1;
      }
      break;
    case SWITCH_TEST_ESCAPE:
      mode = 0;
      saveNonVolatiles();
      gameOver();
      break;
    case SWITCH_TEST_PLUS:
      switch(mode) {
        case modeNone:
          if (nonVolatiles.volume<2) {
            nonVolatiles.volume++;
            if ( nonVolatiles.volume > 1 ) {
              playMusic( gameMusic() );
            }
          }
          break;
        case modeHeadOpen:
          if (nonVolatiles.headOpen<31)
            nonVolatiles.headOpen++;
          break;
        case modeHeadClose:
          if (nonVolatiles.headClose<31)
            nonVolatiles.headClose++;
          break;
        case modeTiltSensitivity:
          if (nonVolatiles.tiltSensitivity<99)
            nonVolatiles.tiltSensitivity++;
          break;
        case modeBalls:
          if (nonVolatiles.ballsPerGame<9)
            nonVolatiles.ballsPerGame++;
          break;
        case modeCoins:
          if (nonVolatiles.coinsPerGame<9)
            nonVolatiles.coinsPerGame++;
          break;
        case modeReset:
          hitSolenoid( SOLENOID_KNOCKER );
          resetNonVolatiles();
          break;
        default:
          break;
      }
      break;
    case SWITCH_TEST_MINUS:
      switch(mode) {
        case modeNone:
          if (nonVolatiles.volume>0) {
            nonVolatiles.volume--;
            if ( nonVolatiles.volume < 2 ) {
              playMusic( SOUND_STOP );
            }
          }
          break;
        case modeHeadOpen:
          if (nonVolatiles.headOpen>15)
            nonVolatiles.headOpen--;
          break;
        case modeHeadClose:
          if (nonVolatiles.headClose>15)
            nonVolatiles.headClose--;
          break;
        case modeTiltSensitivity:
          if (nonVolatiles.tiltSensitivity > 1)
            nonVolatiles.tiltSensitivity--;
          break;
        case modeBalls:
          if (nonVolatiles.ballsPerGame > 1)
            nonVolatiles.ballsPerGame--;
          break;
        case modeCoins:
          if (nonVolatiles.coinsPerGame > 0)
            nonVolatiles.coinsPerGame--;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  } 

  switch(mode) {
    case modeNone:
      displayText("Vol %d", nonVolatiles.volume );
      break;
    case modeHeadOpen:
      bearSetPulse( nonVolatiles.headOpen );
      setTimer(BEAR_TMR, QUARTER_SECOND, BEAR_TEST);
      displayText("Br.O %04d", nonVolatiles.headOpen );
      break;
    case modeHeadClose:
      bearSetPulse( nonVolatiles.headClose );
      setTimer(BEAR_TMR, QUARTER_SECOND, BEAR_TEST);
      displayText("Br.C %04d", nonVolatiles.headClose );
      break;
    case modeTiltSensitivity:
      displayText("TiltS %d", nonVolatiles.tiltSensitivity );
      break;
    case modeBalls:
      displayText("Balls %d", nonVolatiles.ballsPerGame );
      break;
    case modeCoins:
      displayText("Coins %d", nonVolatiles.coinsPerGame );
      break;
    case modeStats_1:
      displayText("Plays %d", nonVolatiles.gamesPlayed );
      break;
    case modeStats_2:
      displayText("F.High %d", nonVolatiles.freeGameHighScore );
      break;
    case modeStats_3:
      displayText("F.Matc %d", nonVolatiles.freeGameMatch );
      break;
    case modeReset:
      displayText("RESET");
      break;
    default:
      break;
  }
}
