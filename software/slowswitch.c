#include "tweenpin.h"

typedef enum {
  SWITCH_TARGET_1_B_IO = 4,
  SWITCH_TARGET_1_E_IO = 9,
  SWITCH_TARGET_1_A_IO = 2,
  SWITCH_TARGET_1_R_IO = 14,
  SWITCH_TARGET_2_L_IO = 13,
  SWITCH_TARGET_2_O_IO = 8,
  SWITCH_TARGET_2_G_IO = 3,
  SWITCH_TARGET_2_J_IO = 10,
  SWITCH_TARGET_2_A_IO = 11,
  SWITCH_TARGET_2_M_IO = 15,
  SWITCH_TARGET_LOWER_IO = 16,
  SWITCH_TARGET_UPPER_IO = 5,
  SWITCH_BEAR_CAPTURE_IO = 7,
  SWITCH_BALL_DRAIN_IO = 12,
  SWITCH_GAME_START_IO = 20,
  SWITCH_COIN_IO = 18,
  SWITCH_TILT_BOB_IO = 19,
  SWITCH_SPINNER_IO = 0,
  SWITCH_LANE_LEFT_IO = 6,
  SWITCH_LANE_RIGHT_IO = 1,
  SWITCH_TEST_ENTER_IO = 23,
  SWITCH_TEST_PLUS_IO = 21,
  SWITCH_TEST_MINUS_IO = 22,
  SWITCH_TEST_ESCAPE_IO = 17,
} slowSwitchIOs;

// must match slowSwitch enum order
static const uint8_t slowSwitchIO[SWITCH_QTY] = {
  SWITCH_TARGET_1_B_IO,
  SWITCH_TARGET_1_E_IO,
  SWITCH_TARGET_1_A_IO,
  SWITCH_TARGET_1_R_IO,
  SWITCH_TARGET_2_L_IO,
  SWITCH_TARGET_2_O_IO,
  SWITCH_TARGET_2_G_IO,
  SWITCH_TARGET_2_J_IO,
  SWITCH_TARGET_2_A_IO,
  SWITCH_TARGET_2_M_IO,
  SWITCH_TARGET_LOWER_IO,
  SWITCH_TARGET_UPPER_IO,
  SWITCH_BEAR_CAPTURE_IO,
  SWITCH_BALL_DRAIN_IO,
  SWITCH_GAME_START_IO,
  SWITCH_COIN_IO,
  SWITCH_TILT_BOB_IO,
  SWITCH_SPINNER_IO,
  SWITCH_LANE_LEFT_IO,
  SWITCH_LANE_RIGHT_IO,
  SWITCH_TEST_ENTER_IO,
  SWITCH_TEST_PLUS_IO,
  SWITCH_TEST_MINUS_IO,
  SWITCH_TEST_ESCAPE_IO,
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
  uint8_t scan[IO_INPUT_CHIP_COUNT];
  uint8_t i;

  for ( i = IO_INPUT_CHIP_0; i < IO_INPUT_CHIP_MAX; i++ )
    {
//      scan[i] = readIO( i, IO_INTF );
    scan[i] = readIO( i, IO_GPIO );
//      scan[i] = readIO( i, IO_DIR );
//      scan[i] = readIO( i, IO_GPPU );
//      scan[i] = readIO( i, IO_IPOL );
  }
    writeDisplay( DISPLAY_DIG0, DISP_S);
    writeDisplay( DISPLAY_DIG1, DISP_BLANK);
    writeDisplayNum( DISPLAY_DIG2, scan[2] >> 4);
    writeDisplayNum( DISPLAY_DIG3, scan[2] & 0x0F);
    writeDisplayNum( DISPLAY_DIG4, scan[1] >> 4);
    writeDisplayNum( DISPLAY_DIG5, scan[1] & 0x0F);
    writeDisplayNum( DISPLAY_DIG6, scan[0] >> 4);
    writeDisplayNum( DISPLAY_DIG7, scan[0] & 0x0F);
}


static uint8_t lastScan[IO_INPUT_CHIP_COUNT];
static boolean switchFlags[SWITCH_QTY];

void driveSlowSwitchISR(void)
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
    switchchip = slowSwitchIO[i] >> 3;
    switchmask = BIT(slowSwitchIO[i] & 0x07);
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
  uint8_t i;
  slowSwitchTrig func;
  for ( i = 0; i < SWITCH_QTY; i++ ) {
    boolean flag;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      flag = switchFlags[i];
      if( flag ) {
        switchFlags[i] = FALSE;
      }
    }
    if ( flag ) {
      func = slowSwitchFunctions[i];
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
  playSound( SOUND_BEAR_OPEN );
  setTimer(BEAR_TMR, ONE_HUNDRETH_SECOND, BEAR_OPEN);
  setTimer(BOX_TMR, QUARTER_SECOND, 0);
  setLampMode( LAMP_BEAR_ARROW, LAMP_BLINK_STATE, EIGTH_SECOND, INFINITE );
  setLampMode( LAMP_BEAR_MOUTH, LAMP_ON_STATE, 0, INFINITE );
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

  if ( multiplier < 6 ) 
  {
    multiplier++;
    setLampMode(LAMP_BONUS_1 + multiplier - 1, LAMP_ON_STATE, INFINITE, 1);
    setLampMode(LAMP_BONUS_1 + multiplier, LAMP_BLINK_ON_STATE, EIGTH_SECOND, INFINITE); // shoot the blinking light
    setSolenoidMode( SOLENOID_BELL, SOLENOID_FLASH_STATE, THREE_HUNDRETH_SECONDS, 1 );
  } else {
    if ( !crazyMode ) {
      crazyMode = TRUE;
      setTimer(CRAZY_TMR, ONE_HUNDRETH_SECOND, 0);
      setSolenoidMode( SOLENOID_BELL, SOLENOID_FLASH_STATE, EIGTH_SECOND, 3);
    }
  }

}

// upper and lower snake switch
void targetSnakeTrig(slowSwitch theSwitch, boolean active)
{
  uint8_t i;

  if(!gameOn) return;
  
  increaseScore(SCORE_ONE_HUNDRED);
 
  playSound( SOUND_SNAKE );
 
  if(getLampMode(theSwitch) != LAMP_ON_STATE) {
    setLampMode(theSwitch, LAMP_ON_STATE, INFINITE, 1);
  } else {
    return;
  }

  for(i = LAMP_TARGET_LOWER; i <= LAMP_TARGET_UPPER; i++) {
    if(getLampMode(i) != LAMP_ON_STATE) {
      return;
    }
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
  
  // debounce (blinking means, we're trying to eject
  if ( (getLampMode(LAMP_BEAR_MOUTH)!=LAMP_OFF_STATE) &&
       (getLampMode(LAMP_BEAR_MOUTH)!=LAMP_ON_STATE) ) {
    
    // eject retry? maybe we need more opening
    setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_EJECT);
  } else {
    
    if ( getLampMode(LAMP_BEAR_ARROW) == LAMP_OFF_STATE ) {
      // reject shots into bear if the bear isn't actually open.
    // having trouble with the head not being closed?
    setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_EJECT);
  } else {
    increaseScore(SCORE_FIVE_THOUSAND);
    playSound( SOUND_BEAR_CHEW );
    setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_CHEW_CLOSE);
  }
    
  }
}

void coinTrig(slowSwitch theSwitch, boolean active)
{
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
        playSound( SOUND_DRAIN );
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
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    tilt = TRUE;
    setSolenoidMode( SOLENOID_FLIPPER_LEFT, SOLENOID_IDLE_STATE, 0, 1);
    setSolenoidMode( SOLENOID_FLIPPER_RIGHT, SOLENOID_IDLE_STATE, 0, 1);
    }
    setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_TILT);
    setLampMode( LAMP_BACKBOX_BEAR_1, LAMP_OFF_STATE, 0, INFINITE );
    setLampMode( LAMP_BACKBOX_BEAR_2, LAMP_OFF_STATE, 0, INFINITE );
    setLampMode( LAMP_BACKBOX_BEAR_3, LAMP_OFF_STATE, 0, INFINITE );
    setLampMode( LAMP_BACKBOX_TILT, LAMP_ON_STATE, 0, INFINITE );
  }
}

void spinnerTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) return;

  increaseScore(SCORE_ONE_HUNDRED);

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
  
  
