#include "tweenpin.h"

const uint8_t switchBit[] = {
  4,  // SWITCH_TARGET_1_B
  9,  // SWITCH_TARGET_1_E
  2,  // SWITCH_TARGET_1_A
  14, // SWITCH_TARGET_1_R
  13, // SWITCH_TARGET_2_L
  8,  // SWITCH_TARGET_2_O
  3,  // SWITCH_TARGET_2_G
  10, // SWITCH_TARGET_2_J
  11, // SWITCH_TARGET_2_A
  15, // SWITCH_TARGET_2_M
  16, // SWITCH_TARGET_LOWER
  5,  // SWITCH_TARGET_UPPER
  7,  // SWITCH_BEAR_CAPTURE
  12, // SWITCH_BALL_DRAIN
  20, // SWITCH_GAME_START
  18, // SWITCH_COIN
  19, // SWITCH_TILT_BOB
  0,  // SWITCH_SPINNER
  6,  // SWITCH_LANE_LEFT
  1,  // SWITCH_LANE_RIGHT
  23, // SWITCH_TEST_ENTER
  21, // SWITCH_TEST_PLUS
  22, // SWITCH_TEST_MINUS
  17, // SWITCH_TEST_ESCAPE
};

typedef void (*slowSwitchTrig)(slowSwitch, boolean);

void targetBearTrig(slowSwitch theSwitch, boolean active);
void targetLogjamTrig(slowSwitch theSwitch, boolean active);
void targetSnakeTrig(slowSwitch theSwitch, boolean active);
void bearCaptureTrig(slowSwitch theSwitch, boolean active);
void drainTrig(slowSwitch theSwitch, boolean active);
void coinTrig(slowSwitch theSwitch, boolean active);
void startTrig(slowSwitch theSwitch, boolean active);
void tiltTrig(slowSwitch theSwitch, boolean active);
void spinnerTrig(slowSwitch theSwitch, boolean active);
void laneTrig(slowSwitch theSwitch, boolean active);
void testTrig(slowSwitch theSwitch, boolean active);

// must match slowswitch enum order
static const slowSwitchTrig slowSwitchFunctions[SWITCH_QTY] = {
  targetBearTrig,
  targetBearTrig,
  targetBearTrig,
  targetBearTrig,
  targetLogjamTrig,
  targetLogjamTrig,
  targetLogjamTrig,
  targetLogjamTrig,
  targetLogjamTrig,
  targetLogjamTrig,
  targetSnakeTrig,
  targetSnakeTrig,
  bearCaptureTrig,
  drainTrig,
  startTrig,
  coinTrig,
  tiltTrig,
  spinnerTrig,
  laneTrig,
  laneTrig,
  testTrig,
  testTrig,
  testTrig,
  testTrig
};


void testSwitch(void)
{
//   uint8_t scan[IO_INPUT_CHIP_COUNT];
//   uint8_t i;
// 
//   for ( i = IO_INPUT_CHIP_0; i < IO_INPUT_CHIP_MAX; i++ )
//     {
// //      scan[i] = readIO( i, IO_INTF );
//     scan[i] = readIO( i, IO_GPIO );
// //      scan[i] = readIO( i, IO_DIR );
// //      scan[i] = readIO( i, IO_GPPU );
// //      scan[i] = readIO( i, IO_IPOL );
//   }
//     writeDisplay( DISPLAY_DIG0, DISP_S);
//     writeDisplay( DISPLAY_DIG1, DISP_BLANK);
//     writeDisplayNum( DISPLAY_DIG2, scan[2] >> 4);
//     writeDisplayNum( DISPLAY_DIG3, scan[2] & 0x0F);
//     writeDisplayNum( DISPLAY_DIG4, scan[1] >> 4);
//     writeDisplayNum( DISPLAY_DIG5, scan[1] & 0x0F);
//     writeDisplayNum( DISPLAY_DIG6, scan[0] >> 4);
//     writeDisplayNum( DISPLAY_DIG7, scan[0] & 0x0F);
}


static uint8_t lastScan[IO_INPUT_CHIP_COUNT];
static boolean switchFlags[SWITCH_QTY];

static void scanSwitches(void)
{
  uint8_t scan[IO_INPUT_CHIP_COUNT];
  uint8_t switchchip, switchmask;
  boolean doCheck = FALSE;
  uint8_t i;

  for ( i = IO_INPUT_CHIP_0; i < IO_INPUT_CHIP_MAX; i++ ) {
    scan[i] = readIO( i, IO_GPIO );
    if(scan[i]) { doCheck = TRUE; }
  }
  for ( i = 0; doCheck && i < SWITCH_QTY ; i++ ) {
    switchchip = switchBit[i] >> 3;
    switchmask = BIT(switchBit[i] & 0x07);
    if( (scan[switchchip] & switchmask) && !(lastScan[switchchip] & switchmask) ) { 
      switchFlags[i] = TRUE;
    }
  }  
  for ( i = IO_INPUT_CHIP_0; i < IO_INPUT_CHIP_MAX; i++ ) {
    lastScan[i] = scan[i];
  }
}

void driveSlowSwitches(void)
{
  scanSwitches();
  for ( uint8_t i = 0; i < SWITCH_QTY; i++ ) {
    boolean flag;
      flag = switchFlags[i];
      if( flag ) {
        switchFlags[i] = FALSE;
      }
    if ( flag ) {
      slowSwitchTrig func = slowSwitchFunctions[i];
      if ( (!tilt) || (func==drainTrig) ) {
        func(i,TRUE);
      }
    }
  }
}



void targetBearTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) return;

  increaseScore(SCORE_ONE_HUNDRED);
  playSound( SOUND_BEAR_TARGETS );
  
  if(getLampMode(theSwitch) != LAMP_ON_STATE) {
    setLampMode(theSwitch, LAMP_ON_STATE, INFINITE, 1);
  } else {
    return;
  }

  for(int i=LAMP_TARGET_1_B; i<=LAMP_TARGET_1_R; i++) {
    if(getLampMode(i) != LAMP_ON_STATE) {
      return;
    }
  }
  
  // all lamps must be lit
  increaseScore(SCORE_ONE_THOUSAND);
  setTimer(BEAR_TMR, ONE_HUNDRETH_SECOND, BEAR_OPEN);
  setTimer(BOX_TMR, QUARTER_SECOND, 0);
}

void targetLogjamTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) return;

  increaseScore(SCORE_ONE_HUNDRED);
  playSound( SOUND_LOGS );

  if(getLampMode(theSwitch) != LAMP_ON_STATE) {
    setLampMode(theSwitch, LAMP_ON_STATE, INFINITE, 1);
  } else {
    return;
  }

  for(int i=LAMP_TARGET_2_L; i<=LAMP_TARGET_2_M; i++) {
    if(getLampMode(i) != LAMP_ON_STATE) {
      return;
    }
  }

  // all lamps must be lit
  increaseScore(SCORE_ONE_THOUSAND);
  playSound( SOUND_BONUS );
  
  // shoot the blinking light
  for(int i=LAMP_TARGET_2_L; i<=LAMP_TARGET_2_M; i++) {
    setLampMode(i, LAMP_BLINK_ON_STATE, EIGTH_SECOND, INFINITE);
  }

  if ( multiplier < 5 ) 
  {
    multiplier++;
    setLampMode(LAMP_BONUS_1 + multiplier - 1, LAMP_ON_STATE, INFINITE, 1);
    setLampMode(LAMP_BONUS_1 + multiplier, LAMP_BLINK_ON_STATE, EIGTH_SECOND, INFINITE); // shoot the blinking light
    hitSolenoid( SOLENOID_WOODBLOCK );
  } else {
    if ( !crazyMode ) {
      crazyMode = TRUE;
      setTimer(CRAZY_TMR, ONE_HUNDRETH_SECOND, 0);
      hitSolenoid( SOLENOID_WOODBLOCK );
    }
  }

}

// upper and lower snake switch
void targetSnakeTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) return;
  
  increaseScore(SCORE_ONE_HUNDRED);
 
  playSound( SOUND_SNAKE );
 
  if (getLampMode(theSwitch) != LAMP_ON_STATE) {
    setLampMode(theSwitch, LAMP_ON_STATE, INFINITE, 1);
  }

  if( getLampMode(LAMP_TARGET_LOWER) != LAMP_ON_STATE ) {
    return;
  }
  if( getLampMode(LAMP_TARGET_UPPER) != LAMP_ON_STATE ) {
    return;
  }

  // all lamps must be lit
  increaseScore(SCORE_ONE_THOUSAND);
  setLampMode(LAMP_TARGET_LOWER, LAMP_BLINK_STATE, EIGTH_SECOND, 5);
  setLampMode(LAMP_TARGET_UPPER, LAMP_BLINK_STATE, EIGTH_SECOND, 5);
  setLampMode(LAMP_POP_BUMPER_LOWER, LAMP_FLASH_STATE, EIGTH_SECOND, 5);
}

void bearCaptureTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) return;
  
  increaseScore(SCORE_FIVE_THOUSAND);
  playSound( SOUND_BEAR_CHEW );
  setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_CHEW);
}

void coinTrig(slowSwitch theSwitch, boolean active)
{
  playSound( SOUND_COIN );
  increaseCredits(1,TRUE);
}

void startTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) {
    if ( decreaseCredits() ) {
      startGame();
      setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_BALL_LOAD);
    } else {
      setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_INSERT_COINS);    
    }
  }
}

void drainTrig(slowSwitch theSwitch, boolean active)
{
  if(gameOn) {

    // hit the loader again (possibly a retry)   
    setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_BALL_LOAD);

    // debounce for about 2 seconds for this logic...
    if ( !ballIsLoading )
    {
      if( (getScore()==0) && (ballInPlay==1) && (!tilt) ) {
        setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_BALL_AGAIN);
      } else {
        nextBall();
      }
    }
  }
}

void tiltTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) return;
  
  if ( ++tiltSense > nonVolatiles.tiltSensitivity )
  { 
    tilt = TRUE;
    setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_TILT);
  }
}

void spinnerTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) return;

  increaseScore(SCORE_ONE_HUNDRED);

  playSound( SOUND_SPINNER );

  setTimer(SPIN_TMR, ONE_HUNDRETH_SECOND, 1);
}

void laneTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) return;
  // TODO 
}

void testTrig(slowSwitch theSwitch, boolean active)
{
  configMode(theSwitch);
}  
  
  
