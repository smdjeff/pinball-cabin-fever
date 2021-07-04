///////////////////////////////////////////////////////////////////////////////
//
// (c)2006  diypinball.com
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "tweenpin.h"

// TODO:
// abstract datatype for bcd vars?
// different way to handle display updates? (way to inject messages without changing to attract/game state?)
// sounds support  (sound board, samples)
// figure out ints on the ios?
// hw spi?
// lane switch support
// credits persist accross shutdown (would need wearleveled ee)
//
// VERIFY FIXES:
// left and right buttons for attract text changes
// don't ring bell for increase credits unless it's a coin
// cancel match timer when game is started
// improve brown out protect (knocker/bell timer)
// don't draw coins, or credits on display, if free mode
// brown out protect (knocker trumps bell, never both)
// more attract lamp features
// invert pop bump lamps
// matchgame has its own timer to eliminate attract lamps during matchgame
// cabin lamp is fade lamp type
// bear box_tmr only active when head is open, or during match game
// multi coin game
// statistics
// eeprom checksum
// standard tilt rules
// ATTRACT_NEW_HIGH_SCORE display text lame (removed)
// switches should still score, even if lit
// shoot again, only on first ball
// shoot again, if not tilted
// ball drain needs debounce? seems like balls goes up too fast sometimes.
// ball drain, confused on load retry for "shoot again"
// multipler
// matchgame
// crazyMode (with display)
// spinner support
// don't score (read any slow switches) when tilted
// clear tilt lamp at next ball
// fix flipper pwm stick in tilt (allow flipperISR for !active state)
// added new horizontal playfield lamp attract
// fix non volatiles (high score)
// handle ballIsLoading a little more cleanly, this fixes errant "try again" during ball 1 load retry
// give more time to bear head open before eject
// bear head might not fully open, then it will keep retrying (and scoring!)
// configuration (nv): quiet mode, bear adjust, reset high score, tilt(lib/con), FINALBALLs
// when all targets in the bank are hit, sometimes they don't all blink (doesn't
//   seem related to hit sequence). - fixed by switching from FLASH to BLINK lamp mode?
// new back box lamps
// display sometimes goes blank during game (and stays blank until reset during attract)  
//    addressed with a big cap right on vss/vdd for the display ic (big current spikes with multiplex freq)
// initial swag at sounds
// bear head capture scores MORE than bear target bank complete

// BUGS:
// bear head shouldn't be doing pwm in attract
// bear head makes a pwm noise every 30mins? (timer rollover?)



uint8_t ballInPlay = 0;
boolean tilt = FALSE;
uint8_t tiltSense = 0;
uint8_t multiplier = 1;
boolean gameOn = FALSE;
boolean highScoreExceeded = FALSE;
boolean scoreDisplayDisabled = FALSE;
boolean ballIsLoading = FALSE;
boolean crazyMode;
uint16_t bearTestPWM;

static score_t score;
static uint8_t credits = 0;
nonVolatiles_t nonVolatiles;


int main (void)
{
  initIO();
  bootDisplay();
  resetGame();
  setTimer(BEAR_TMR, QUARTER_SECOND, BEAR_EJECT);

  loadNonVolatiles();
  
  setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_PLAY);
  setTimer(ATTRACT_LAMP_TMR, ONE_HUNDRETH_SECOND, 0);

  ENABLE_INTS();
  
  for (;; )
  {
    driveLamps();
    driveSlowSwitches();
    driveTimers();
    if ( !gameOn )
    {
      // attract mode is driven by the attract timers
    } 
    else 
    {
      // driven by switch routines
    }
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
    LAMP_LANE_LEFT1,
    LAMP_LANE_RIGHT1,
    LAMP_LANE_RIGHT1,
  },
  {
    LAMP_TARGET_LOWER,
    LAMP_LANE_LEFT2,
    LAMP_LANE_RIGHT2,
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
    LAMP_LANE_LEFT1,
    LAMP_LANE_LEFT2,
    LAMP_LANE_LEFT2,
    LAMP_LANE_LEFT2,
    LAMP_LANE_LEFT2,
    LAMP_LANE_LEFT2,
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
    LAMP_LANE_RIGHT1,
    LAMP_LANE_RIGHT2,
    LAMP_LANE_RIGHT2,
    LAMP_LANE_RIGHT2,
    LAMP_LANE_RIGHT2,
    LAMP_LANE_RIGHT2,
  }
};


void attractLamps(timerEvent evt, uint16_t state)
{
  static uint8_t stateCount;
  static uint8_t stateRepeat;
  uint8_t i;

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
      for (i=0; i<6; i++) {
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
      for (i=0; i<=LAMP_LAST_PLAYFIELD; i++) {
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
      for (i=0; i<3; i++) {
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
      for (i=0; i<=LAMP_LAST_PLAYFIELD; i++) {
        setLampMode( i, LAMP_ON_STATE, 0, INFINITE );
      }
      setTimer(ATTRACT_LAMP_TMR, ONE_HUNDRETH_SECOND, 0/*state*/);
      stateCount = 0;
      break;      

  }
}

static uint8_t displayState = 0;

void attractTimerButton( fastSwitch theSwitch )
{
  if ( theSwitch == FASTSWITCH_FLIPPER_RIGHT ) {

    // advance the attract display state
    switch(displayState) {
      case ATTRACT_PLAY:
        setIfActiveTimer(ATTRACT_TMR, ATTRACT_SCORE);
        break;
      case ATTRACT_SCORE:
        setIfActiveTimer(ATTRACT_TMR, ATTRACT_HIGH_SCORE);
        break;
      case ATTRACT_HIGH_SCORE:
        setIfActiveTimer(ATTRACT_TMR, ATTRACT_GAME_OVER);
        break;
      case ATTRACT_GAME_OVER:
        setIfActiveTimer(ATTRACT_TMR, ATTRACT_PLAY);
        break;
      default:
        break;
    }
  }
  else
  {
    // reverse the attract display state
    switch(displayState) {
      case ATTRACT_PLAY:
        setIfActiveTimer(ATTRACT_TMR, ATTRACT_GAME_OVER);
        break;
      case ATTRACT_SCORE:
        setIfActiveTimer(ATTRACT_TMR, ATTRACT_PLAY);
        break;
      case ATTRACT_HIGH_SCORE:
        setIfActiveTimer(ATTRACT_TMR, ATTRACT_SCORE);
        break;
      case ATTRACT_GAME_OVER:
        setIfActiveTimer(ATTRACT_TMR, ATTRACT_HIGH_SCORE);
        break;
      default:
        break;
    }
  } 
}


// cycles display through attract states
void attractTimer(timerEvent evt, uint16_t state)
{
  static uint8_t cycles = 0;

  if(gameOn) return;

  switch(state) {
  case ATTRACT_PLAY:
    // "PLAY"
    displayState = ATTRACT_PLAY;
    resetDisplay(); // just in case
    displayString( DISP_BLANK, DISP_BLANK, DISP_P, DISP_L, DISP_A, DISP_Y, DISP_BLANK, DISP_BLANK );
    setTimer(ATTRACT_TMR, TWO_SECONDS, ATTRACT_PINBALL);
    cycles = 0;
    break;
  case ATTRACT_PINBALL:
    if(cycles == 0) cycles = 8;
    if(--cycles) {
      if(cycles & 0x01) {
        // "PINBALL"
        displayString( DISP_BLANK, DISP_P, DISP_I, DISP_N, DISP_B, DISP_A, DISP_L, DISP_L );
      } else {
        displayClear();
      }
      setTimer(ATTRACT_TMR, QUARTER_SECOND, ATTRACT_PINBALL);    
    } else {
      setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_SCORE);    
    }
    break;
  case ATTRACT_SCORE:
    displayState = ATTRACT_SCORE;
    displayScore();
    setTimer(ATTRACT_TMR, FIVE_SECONDS, ATTRACT_HIGH_SCORE);
    break;
  case ATTRACT_HIGH_SCORE:
    displayState = ATTRACT_HIGH_SCORE;
    displayString( DISP_H, DISP_I, DISP_BLANK, DISP_S, DISP_C, DISP_O, DISP_R, DISP_E );
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_HIGH_SCORE_1);
    break;
  case ATTRACT_HIGH_SCORE_1:
    displayBCD(nonVolatiles.highScore,sizeof(nonVolatiles.highScore));
    setTimer(ATTRACT_TMR, THREE_SECONDS, ATTRACT_GAME_OVER);
    break;
  case ATTRACT_GAME_OVER:
    displayState = ATTRACT_GAME_OVER;
    displayString( DISP_G, DISP_A, DISP_M, DISP_E, DISP_O, DISP_V, DISP_E, DISP_R );
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_CREDITS);    
    break;
  case ATTRACT_CREDITS:
    if ( nonVolatiles.coinsPerGame > 0 ) {
      displayString( DISP_BLANK, DISP_BLANK, DISP_C, DISP_O, DISP_I, DISP_N, DISP_S, DISP_BLANK );
      setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_CREDITS_1);
    } else {
      setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_PLAY);
    }
    break;
  case ATTRACT_CREDITS_1:
    displayCredits();
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_PLAY);
    break;
  case ATTRACT_INSERT_COINS:
    displayCredits();
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_INSERT_COINS_1);
    break;
  case ATTRACT_INSERT_COINS_1:
    displayString( DISP_BLANK, DISP_I, DISP_N, DISP_S, DISP_E, DISP_R, DISP_T, DISP_BLANK );
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_INSERT_COINS_2);
    break;
  case ATTRACT_INSERT_COINS_2:
    displayString( DISP_BLANK, DISP_BLANK, DISP_C, DISP_O, DISP_I, DISP_N, DISP_S, DISP_BLANK );
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_BLANK);
    break;
  case ATTRACT_BLANK:
    displayClear();
    setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_PLAY);
    break;
  case ATTRACT_FREE: 
    displayState = ATTRACT_FREE;
    displayString( DISP_F, DISP_R, DISP_E, DISP_E, DISP_G, DISP_A, DISP_M, DISP_E );
    setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_SCORE);
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


void crazyModeTimer(timerEvent evt, uint16_t state)
{
  if (state++>4) {
    state = 0;
  }
  setLampMode( LAMP_BONUS_1 + state, LAMP_BLINK_STATE, EIGTH_SECOND, 3 );
  setTimer(CRAZY_TMR, TEN_HUNDRETH_SECONDS, state);
}


void spinTimer(timerEvent evt, uint16_t state)
{
  static uint8_t countDown = 0;
  static uint8_t count = 0;
  
  if ( (state==0) && (countDown==0) ) {
    playSound( SOUND_SPINNER );
  }
  
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
    }
  }
}



static score_t match;

void matchTimerButton( fastSwitch theSwitch )
{
  match[0] = rand() % 9;
}

void matchTimer(timerEvent evt, uint16_t state)
{
  static uint8_t stateCount;
  static uint8_t repeatStateCount;
  
  playSound( SOUND_MATCHING );
  
  switch (state)
  {
    case 0:
      memcpy( match, score, sizeof(score) );
      setTimer(BOX_TMR, QUARTER_SECOND, 0);
      setTimer(MATCH_TMR, TEN_HUNDRETH_SECONDS, ++state);
      break;
    case 1: 
      // when the score is nnnn ny0n
      // we're matching against y0 so show those in the right side
      repeatStateCount = 0;
      match[6] = score[5];
      match[7] = 0;
      setTimer(MATCH_TMR, TEN_HUNDRETH_SECONDS, ++state);
      break;
    case 2:
      // pick a new match 
      // 01234567
      // i0....n0
      match[0] = rand() % 9;
      displayClear();
      writeDisplayNum( DISPLAY_DIG0, match[0] );
      writeDisplayNum( DISPLAY_DIG1, 0 );
      writeDisplayNum( DISPLAY_DIG6, match[6] );
      writeDisplayNum( DISPLAY_DIG7, 0 );
      stateCount = 0;
      setTimer(MATCH_TMR, TEN_HUNDRETH_SECONDS, ++state);
      break;
    case 3:
      // move the match right
      // 01234567
      // .i0...n0
      writeDisplay(    DISPLAY_DIG0 + stateCount, DISP_BLANK );
      writeDisplayNum( DISPLAY_DIG1 + stateCount, match[0] );
      writeDisplayNum( DISPLAY_DIG2 + stateCount, 0 );
      if ( stateCount++>2 ) {
        stateCount = 0;
        if (repeatStateCount++<10) {
          state++;
        } else {
          state+=2;
        }
      }
      setTimer(MATCH_TMR, FIVE_HUNDRETH_SECONDS, state);
      break;
    case 4:
      // move the match left
      // 01234567
      // ....i0n0
      writeDisplayNum( DISPLAY_DIG3 - stateCount, match[0] );
      writeDisplayNum( DISPLAY_DIG4 - stateCount, 0 );
      writeDisplay(    DISPLAY_DIG5 - stateCount, DISP_BLANK );
      if ( stateCount++>2 ) {
        state = 2;
      }
      setTimer(MATCH_TMR, FIVE_HUNDRETH_SECONDS, state);
      break;
    case 5:
      // match end
      if ( match[0] == match[6] ) {
        setSolenoidMode( SOLENOID_KNOCKER, SOLENOID_FLASH_STATE, TEN_HUNDRETH_SECONDS, 1);
        increaseCredits( nonVolatiles.coinsPerGame, FALSE );
        nonVolatiles.freeGameMatch++;
        setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_FREE);
      } else {
        setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_SCORE);
      } 
      setTimer(ATTRACT_LAMP_TMR, ONE_SECOND, 0);
      cancelBoxTimer();
      break;
  }
}


void gameTimer(timerEvent evt, uint16_t state)
{
  if(!gameOn) return;

  switch(state) {
  case GAME_BALL_LOAD:
    ballIsLoading = TRUE;
    setSolenoidMode( SOLENOID_BALL_LOADER, SOLENOID_FLASH_STATE, QUARTER_SECOND, 1 );
    displayString( DISP_BLANK, DISP_B, DISP_A, DISP_L, DISP_L, DISP_BLANK, DISP_BLANK, DISP_BLANK );
    writeDisplayNum( DISPLAY_DIG7, ballInPlay );
    setTimer(GAME_TMR, TWO_SECONDS, GAME_SCORE);
    break;    
  case GAME_BALL_AGAIN:
    setSolenoidMode( SOLENOID_BALL_LOADER, SOLENOID_FLASH_STATE, QUARTER_SECOND, 1 );
    displayString( DISP_BLANK, DISP_BLANK, DISP_S, DISP_H, DISP_O, DISP_O, DISP_T, DISP_BLANK );
    setTimer(GAME_TMR, ONE_SECOND, GAME_BALL_AGAIN_1);
    break;
  case GAME_BALL_AGAIN_1:
    displayString( DISP_BLANK, DISP_BLANK, DISP_A, DISP_G, DISP_A, DISP_I, DISP_N, DISP_BLANK );
    setTimer(GAME_TMR, ONE_SECOND, GAME_SCORE);
    break;
  case GAME_SCORE:
    scoreDisplayDisabled = FALSE;
    ballIsLoading = FALSE;
    displayScore();
    setTimer(GAME_TMR, QUARTER_SECOND, GAME_SCORE);
    break;
  case GAME_FREE_CREDIT:
    scoreDisplayDisabled = TRUE;
    displayString( DISP_F, DISP_R, DISP_E, DISP_E, DISP_G, DISP_A, DISP_M, DISP_E );
    setTimer(GAME_TMR, TWO_SECONDS, GAME_SCORE);
    break;
  case GAME_TILT:
    scoreDisplayDisabled = TRUE;
    setTimer(BEAR_TMR, QUARTER_SECOND, BEAR_EJECT);
    setLampMode( LAMP_PLAYFIELD_GI, LAMP_OFF_STATE, 0, INFINITE );
    setLampMode( LAMP_POP_BUMPER_UPPER, LAMP_OFF_STATE, 0, INFINITE );
    setLampMode( LAMP_POP_BUMPER_LOWER, LAMP_OFF_STATE, 0, INFINITE );
    setSolenoidMode( SOLENOID_FLIPPER_LEFT, SOLENOID_IDLE_STATE, 0, 1);
    setSolenoidMode( SOLENOID_FLIPPER_RIGHT, SOLENOID_IDLE_STATE, 0, 1);
    displayString( DISP_BLANK, DISP_BLANK, DISP_T, DISP_I, DISP_L, DISP_T, DISP_BLANK, DISP_BLANK );
    break;
  default:
    break;
  }
}


void bearPWM(uint16_t width)
{
  // Needs to remain atomic in order to ensure accurate timing
  ATOMIC(
    TCNT1 = 0;
    high(BEAR_PORT, BEAR_MASK);
    while(TCNT1 < width) {
      ;
    }
    low(BEAR_PORT, BEAR_MASK);      
  )
}

// cycles the bear through its various motions
//   modes are activated by "BEAR" targets and bear capture
void bearTimer(timerEvent evt, uint16_t state)
{
  static uint8_t cycles=0;

  switch(state) {
  case BEAR_OPEN:
    bearPWM(nonVolatiles.headOpen);
    setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_OPEN);  // keep open
    break;
  case BEAR_CHEW_CLOSE:
    if(cycles == 0) cycles = 3;  // first time
    bearPWM(nonVolatiles.headClose);
    setTimer(BEAR_TMR, QUARTER_SECOND, BEAR_CHEW_OPEN);
    bearPWM(nonVolatiles.headClose);
    break;
  case BEAR_CHEW_OPEN:
    bearPWM(nonVolatiles.headOpen);
    if(--cycles) {
      setTimer(BEAR_TMR, QUARTER_SECOND, BEAR_CHEW_CLOSE);
    } else {
      setTimer(BEAR_TMR, QUARTER_SECOND, BEAR_EJECT);
    }
    bearPWM(nonVolatiles.headOpen);
    break;
  case BEAR_EJECT:
    setLampMode(LAMP_BEAR_MOUTH, LAMP_BLINK_STATE, EIGTH_SECOND, 8);  // debounce timer
    setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_EJECT_1);
    cycles = 0;
    break;
  case BEAR_EJECT_1:
    bearPWM(nonVolatiles.headOpen);
    if ( cycles++ > 4 ) {
      setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_EJECT_2);
    } else {
      setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_EJECT_1);
    }
    break;
  case BEAR_EJECT_2:
    cycles = 0;
    setSolenoidMode(SOLENOID_BEAR_CAPTURE, SOLENOID_FLASH_PAUSE_STATE, QUARTER_SECOND, 1);
    setTimer(BEAR_TMR, HALF_SECOND, BEAR_CLOSE);
    setLampMode( LAMP_BEAR_ARROW, LAMP_OFF_STATE, 0, INFINITE );
    cancelBoxTimer();
    break;
  case BEAR_TEST:
    bearPWM(bearTestPWM);
    setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_TEST);
    break;
  case BEAR_CLOSE:
  default:
    bearPWM(nonVolatiles.headClose);
    setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_CLOSE);  // keep closed
    break;
  }
}


void resetGame(void)
{
  uint8_t i;
  gameOn = FALSE;
  for ( i = 0; i < LAMP_QTY; i++ )
  {
    setLampMode(i, LAMP_OFF_STATE, 0, 1);
  }

  setSolenoidMode( SOLENOID_FLIPPER_LEFT, SOLENOID_IDLE_STATE, 0, 1);
  setSolenoidMode( SOLENOID_FLIPPER_RIGHT, SOLENOID_IDLE_STATE, 0, 1);
  setLampMode( LAMP_BACKBOX_GAME_OVER, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_PLAYFIELD_GI, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_BACKBOX_RIVER, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_BACKBOX_CABIN, LAMP_FADE_UP_STATE, 200, INFINITE );
  
  cancelTimer(CRAZY_TMR);
  cancelBoxTimer();
  setTimer(BEAR_TMR, QUARTER_SECOND, BEAR_CLOSE);
}

void startGame(void)
{
  resetGame();
  resetScore();
  gameOn = TRUE;
  highScoreExceeded = FALSE;
  ballInPlay = 1;
  tilt = FALSE;
  tiltSense = 0;
  multiplier = 1;
  crazyMode = FALSE;
  cancelTimer(MATCH_TMR);
  cancelTimer(ATTRACT_TMR);
  cancelTimer(ATTRACT_LAMP_TMR);
  setLampMode( LAMP_BACKBOX_GAME_OVER, LAMP_OFF_STATE, 0, INFINITE );
  setLampMode( LAMP_BONUS_1, LAMP_ON_STATE, 0, INFINITE );  
  setLampMode( LAMP_POP_BUMPER_UPPER, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_POP_BUMPER_LOWER, LAMP_ON_STATE, 0, INFINITE );
  playSound( SOUND_BACKGROUND );
  playSound( SOUND_GAME_START );
}

void nextBall(void)
{
  tilt = FALSE;
  tiltSense = 0;
  setLampMode( LAMP_BACKBOX_TILT, LAMP_OFF_STATE, 0, INFINITE );
  setLampMode( LAMP_PLAYFIELD_GI, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_POP_BUMPER_UPPER, LAMP_ON_STATE, 0, INFINITE );
  setLampMode( LAMP_POP_BUMPER_LOWER, LAMP_ON_STATE, 0, INFINITE );

  resetDisplay(); // just in case
  
  if (ballInPlay++ < nonVolatiles.ballsPerGame) {
    setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_BALL_LOAD);
  } else {
    gameOver();
  }
}

void gameOver(void)
{
  playSound( SOUND_BACKGROUND_STOP );
  playSound( SOUND_GAME_END );
  resetGame();
  if ( highScoreExceeded ) {
    nonVolatiles.freeGameHighScore++;
    increaseCredits( nonVolatiles.coinsPerGame, FALSE );
    memcpy( nonVolatiles.highScore, score, sizeof(score) );
    saveNonVolatiles();
  }
  setTimer(MATCH_TMR, ONE_HUNDRETH_SECOND, 0);
}


//////////////////////////////////
// credits
//////////////////////////////////
void displayCredits(void)
{
  displayString( DISP_BLANK, DISP_BLANK, DISP_BLANK, DISP_BLANK, DISP_O, DISP_F, DISP_BLANK, DISP_BLANK);

  uint8_t high = (credits%100)/10;
  uint8_t low = credits%10;
  if ( high > 0 ) writeDisplayNum( DISPLAY_DIG1, high );
  writeDisplayNum( DISPLAY_DIG2, low );
  writeDisplayNum( DISPLAY_DIG7, nonVolatiles.coinsPerGame );
}

void increaseCredits(uint8_t value, boolean coinMode)
{
  credits += value;
  if  (credits > 99) credits = 99;
  if (coinMode ) {
    setSolenoidMode( SOLENOID_BELL, SOLENOID_FLASH_STATE, THREE_HUNDRETH_SECONDS, 1 );
    setIfActiveTimer(ATTRACT_TMR, ATTRACT_CREDITS_1);  
  }
}

boolean decreaseCredits(void)
{
  nonVolatiles.gamesPlayed++;
  
  if ( credits < nonVolatiles.coinsPerGame ) 
    return FALSE;
  
  credits -= nonVolatiles.coinsPerGame;
  return TRUE;
}


//////////////////////////////////
// scoring and non volatiles
//////////////////////////////////

// "BCD-style" used for scoring in order to avoid divides and modulus when updating display
const uint8_t SCORE_TEN[8]  = { 0, 0, 0, 0, 0, 0, 1, 0 };
const uint8_t SCORE_ONE_HUNDRED[8]  = { 0, 0, 0, 0, 0, 1, 0, 0 };
const uint8_t SCORE_FIVE_HUNDRED[8]  = { 0, 0, 0, 0, 0, 5, 0, 0 };
const uint8_t SCORE_ONE_THOUSAND[8]  = { 0, 0, 0, 0, 1, 0, 0, 0 };
const uint8_t SCORE_FIVE_THOUSAND[8]  = { 0, 0, 0, 0, 5, 0, 0, 0 };

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
  memset( &nonVolatiles, 0x00, sizeof(nonVolatiles) );
  nonVolatiles.headOpen = 1400;
  nonVolatiles.headClose = 1750;
  nonVolatiles.quiteMode = 0;
  nonVolatiles.soundBoard = 0;
  nonVolatiles.tiltSensitivity = 3;
  nonVolatiles.ballsPerGame = 3;
  nonVolatiles.coinsPerGame = 1;
  nonVolatiles.gamesPlayed = 0;
  nonVolatiles.freeGameMatch = 0;
  nonVolatiles.freeGameHighScore = 0;

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
  eeprom_write_block( (uint8_t*)&nonVolatiles, (uint8_t*)0, sizeof(nonVolatiles) );
}


void displayScore(void)
{
  displayBCD( score, sizeof(score) );
}

void resetScore(void)
{
  uint8_t i;
  for(i = 0; i< 8; i++) {
    score[i] = 0;
  }
  displayScore();
}

uint32_t getScore(void)
{
  uint32_t temp = 0;
  uint8_t i=7;

  do {
    temp = temp * 10 + score[i];
  } while(i--);

  return temp;
}

void increaseScore(const uint8_t *bcdVal)
{
  uint8_t i,j;
  uint8_t temp, carry=0;

  if ( (crazyMode) || (nonVolatiles.quiteMode == 0) ) {
    setSolenoidMode( SOLENOID_BELL, SOLENOID_FLASH_STATE, THREE_HUNDRETH_SECONDS, 1 );
  }

  for(j=0; j<multiplier; j++) {
    i=7;
    do {
      temp = score[i] + bcdVal[i] + carry;
      if(temp >= 10) {
        carry = 1;
        score[i] = temp - 10;
      } else {
        carry = 0;
        score[i] = temp;
      }
    } while( i-- );
  }

  if ( !scoreDisplayDisabled ) {
    displayScore();  
  }
  
  if ( !highScoreExceeded )
  {
    for(i=0; i<8; i++)
    {
      if ( score[i] > nonVolatiles.highScore[i] )
      {
        highScoreExceeded = TRUE;
        setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_FREE_CREDIT);
        setSolenoidMode( SOLENOID_KNOCKER, SOLENOID_FLASH_STATE, TEN_HUNDRETH_SECONDS, 1);
        break;
      }
      if ( score[i] < nonVolatiles.highScore[i] )
      {
        break;
      }
    }
  }
}


void configMode(slowSwitch sw)
{
  typedef enum {
    modeNone = 0,
    modeBalls = 1,
    modeCoins = 2,
    modeStats_1 = 3,
    modeStats_2 = 4,
    modeStats_3 = 5,
    modeQuiet = 6,
    modeSound = 7,
    modeTiltSensitivity = 8,
    modeHeadOpen = 9,
    modeHeadClose = 10,
    modeReset = 11,
    maxModes = 11
  } configModes_t;
  
  static uint8_t mode = 0;

  switch(sw) {
    case SWITCH_TEST_ENTER:
      if ( mode == 0 ) {
        cancelTimer(BEAR_TMR);
        cancelTimer(ATTRACT_TMR);
        cancelTimer(ATTRACT_LAMP_TMR);
        cancelTimer(GAME_TMR);
      } 
      if ( ++mode > maxModes ) {
        mode = 1;
      }
      break;
    case SWITCH_TEST_ESCAPE:
      mode = 0;
      saveNonVolatiles();
      resetGame();
      setTimer(ATTRACT_LAMP_TMR, ONE_SECOND, 0);
      setTimer(ATTRACT_TMR, ONE_SECOND, 0);
      break;
    case SWITCH_TEST_PLUS:
      switch(mode) {
        case modeHeadOpen:
          if (nonVolatiles.headOpen<1975)
            nonVolatiles.headOpen += 25;
          break;
        case modeHeadClose:
          if (nonVolatiles.headClose<1975)
            nonVolatiles.headClose += 25;
          break;
        case modeQuiet:
          if (nonVolatiles.quiteMode<2)
            nonVolatiles.quiteMode++;
          break;
        case modeSound:
          nonVolatiles.soundBoard = 1;
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
          setSolenoidMode( SOLENOID_KNOCKER, SOLENOID_FLASH_STATE, TEN_HUNDRETH_SECONDS, 1);
          resetNonVolatiles();
          break;
      }
      break;
    case SWITCH_TEST_MINUS:
      switch(mode) {
        case modeHeadOpen:
          if (nonVolatiles.headOpen>1025)
            nonVolatiles.headOpen -= 25;
          break;
        case modeHeadClose:
          if (nonVolatiles.headClose>1025)
            nonVolatiles.headClose -= 25;
          break;
        case modeQuiet:
          if (nonVolatiles.quiteMode>0)
            nonVolatiles.quiteMode--;
          break;
        case modeSound:
          nonVolatiles.soundBoard = 0;
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
      }
      break;
    default:
      break;
  } 

  switch(mode) {
    case modeHeadOpen:
      displayString(DISP_H,DISP_D,DISP_O,DISP_BLANK,DISP_BLANK,DISP_BLANK,DISP_BLANK,DISP_BLANK);
      bearTestPWM = nonVolatiles.headOpen;
      setTimer(BEAR_TMR, QUARTER_SECOND, BEAR_TEST);
      displayLong( nonVolatiles.headOpen, 4 );
      break;
    case modeHeadClose:
      displayString(DISP_H,DISP_D,DISP_C,DISP_BLANK,DISP_BLANK,DISP_BLANK,DISP_BLANK,DISP_BLANK);
      bearTestPWM = nonVolatiles.headClose;
      setTimer(BEAR_TMR, QUARTER_SECOND, BEAR_TEST);
      displayLong( nonVolatiles.headClose, 4 );
      break;
    case modeQuiet:
      displayString(DISP_Q,DISP_U,DISP_I,DISP_E,DISP_T,DISP_BLANK,DISP_BLANK,DISP_BLANK);
      writeDisplayNum( DISPLAY_DIG7, nonVolatiles.quiteMode );
      break;
    case modeTiltSensitivity:
      displayString(DISP_T,DISP_I,DISP_L,DISP_T,DISP_Y,DISP_BLANK,DISP_BLANK,DISP_BLANK);
      writeDisplayNum( DISPLAY_DIG6, (nonVolatiles.tiltSensitivity%100)/10 );
      writeDisplayNum( DISPLAY_DIG7, nonVolatiles.tiltSensitivity%10 );
      break;
    case modeBalls:
      displayString(DISP_B,DISP_A,DISP_L,DISP_L,DISP_S,DISP_BLANK,DISP_BLANK,DISP_BLANK);
      writeDisplayNum( DISPLAY_DIG7, nonVolatiles.ballsPerGame );
      break;
    case modeSound:
      displayString(DISP_S,DISP_O,DISP_U,DISP_N,DISP_D,DISP_BLANK,DISP_BLANK,DISP_BLANK);
      writeDisplayNum( DISPLAY_DIG7, nonVolatiles.soundBoard );
      break;
    case modeCoins:
      displayString(DISP_C,DISP_O,DISP_I,DISP_N,DISP_S,DISP_BLANK,DISP_BLANK,DISP_BLANK);
      writeDisplayNum( DISPLAY_DIG7, nonVolatiles.coinsPerGame );
      break;
    case modeStats_1:
      displayString(DISP_P,DISP_L,DISP_A,DISP_Y,DISP_BLANK,DISP_BLANK,DISP_BLANK,DISP_BLANK);
      writeDisplayNum( DISPLAY_DIG5, (nonVolatiles.gamesPlayed%1000)/100 );
      writeDisplayNum( DISPLAY_DIG6, (nonVolatiles.gamesPlayed%100)/10 );
      writeDisplayNum( DISPLAY_DIG7, nonVolatiles.gamesPlayed%10 );
      break;
    case modeStats_2:
      displayString(DISP_F,DISP_G,DISP_H,DISP_S,DISP_BLANK,DISP_BLANK,DISP_BLANK,DISP_BLANK);
      writeDisplayNum( DISPLAY_DIG5, (nonVolatiles.freeGameHighScore%1000)/100 );
      writeDisplayNum( DISPLAY_DIG6, (nonVolatiles.freeGameHighScore%100)/10 );
      writeDisplayNum( DISPLAY_DIG7, nonVolatiles.freeGameHighScore%10 );
      break;
    case modeStats_3:
      displayString(DISP_F,DISP_G,DISP_M,DISP_A,DISP_BLANK,DISP_BLANK,DISP_BLANK,DISP_BLANK);
      writeDisplayNum( DISPLAY_DIG5, (nonVolatiles.freeGameMatch%1000)/100 );
      writeDisplayNum( DISPLAY_DIG6, (nonVolatiles.freeGameMatch%100)/10 );
      writeDisplayNum( DISPLAY_DIG7, nonVolatiles.freeGameMatch%10 );
      break;
    case modeReset:
      displayString(DISP_R,DISP_E,DISP_S,DISP_E,DISP_T,DISP_BLANK,DISP_BLANK,DISP_BLANK);
      break;
  }
}
