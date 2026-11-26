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
  1,  // SWITCH_LANE_LEFT
  6,  // SWITCH_LANE_RIGHT
  23, // SWITCH_TEST_ENTER
  21, // SWITCH_TEST_PLUS
  22, // SWITCH_TEST_MINUS
  17, // SWITCH_TEST_ESCAPE
};

typedef struct {
  uint16_t points;
  sounds_t sound;
} switchReturn_t;

typedef switchReturn_t (*callback_t)(slowSwitch, boolean);

typedef struct {
  callback_t callback;
  uint16_t debounce;
  bool enabledInAttract;
  bool enabledInTilt;
} switchConfig_t;


typedef struct {
  bool flag;
  uint16_t lastDebounce;
} switchStatus_t;

static switchStatus_t switchStatus[SWITCH_QTY] = {0,};

switchReturn_t targetBearTrig(slowSwitch theSwitch, boolean active);
switchReturn_t targetLogjamTrig(slowSwitch theSwitch, boolean active);
switchReturn_t targetSnakeTrig(slowSwitch theSwitch, boolean active);
switchReturn_t bearCaptureTrig(slowSwitch theSwitch, boolean active);
switchReturn_t drainTrig(slowSwitch theSwitch, boolean active);
switchReturn_t coinTrig(slowSwitch theSwitch, boolean active);
switchReturn_t startTrig(slowSwitch theSwitch, boolean active);
switchReturn_t tiltTrig(slowSwitch theSwitch, boolean active);
switchReturn_t spinnerTrig(slowSwitch theSwitch, boolean active);
switchReturn_t laneTrig(slowSwitch theSwitch, boolean active);
switchReturn_t doorTrig(slowSwitch theSwitch, boolean active);



static const switchConfig_t switchConfigs[] = {
  { targetBearTrig, TEN_HUNDRETH_SECONDS, false, false },
  { targetBearTrig, TEN_HUNDRETH_SECONDS, false, false },
  { targetBearTrig, TEN_HUNDRETH_SECONDS, false, false },
  { targetBearTrig, TEN_HUNDRETH_SECONDS, false, false },
  { targetLogjamTrig, TEN_HUNDRETH_SECONDS, false, false },
  { targetLogjamTrig, TEN_HUNDRETH_SECONDS, false, false },
  { targetLogjamTrig, TEN_HUNDRETH_SECONDS, false, false },
  { targetLogjamTrig, TEN_HUNDRETH_SECONDS, false, false },
  { targetLogjamTrig, TEN_HUNDRETH_SECONDS, false, false },
  { targetLogjamTrig, TEN_HUNDRETH_SECONDS, false, false },
  { targetSnakeTrig, TEN_HUNDRETH_SECONDS, false, false },
  { targetSnakeTrig, TEN_HUNDRETH_SECONDS, false, false },
  { bearCaptureTrig, QUARTER_SECOND, false, true },
  { drainTrig, QUARTER_SECOND, false, true },
  { startTrig, QUARTER_SECOND, true, false },
  { coinTrig, QUARTER_SECOND, true, true },
  { tiltTrig, TEN_HUNDRETH_SECONDS, false, false },
  { spinnerTrig, TEN_HUNDRETH_SECONDS, false, false },
  { laneTrig, TEN_HUNDRETH_SECONDS, false, false },
  { laneTrig, TEN_HUNDRETH_SECONDS, false, false },
  { doorTrig, QUARTER_SECOND, true, true },
  { doorTrig, QUARTER_SECOND, true, true },
  { doorTrig, QUARTER_SECOND, true, true },
  { doorTrig, QUARTER_SECOND, true, true },
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


static void scanSwitches(void)
{
  static uint8_t lastScan[IO_INPUT_CHIP_COUNT] = {0,};
  uint8_t scan[IO_INPUT_CHIP_COUNT] = {0,};
  uint8_t switchchip, switchmask = 0;
  bool doCheck = false;

  for ( uint8_t i = IO_INPUT_CHIP_0; i < IO_INPUT_CHIP_MAX; i++ ) {
    scan[i] = readIO( i, IO_GPIO );
    if(scan[i]) { doCheck = true; }
  }
  for ( uint8_t i = 0; doCheck && i < SWITCH_QTY ; i++ ) {
    switchchip = switchBit[i] >> 3;
    switchmask = BIT(switchBit[i] & 0x07);
    if( (scan[switchchip] & switchmask) && !(lastScan[switchchip] & switchmask) ) { 
      switchStatus[i].flag = true;
    }
  }  
  for ( uint8_t i = IO_INPUT_CHIP_0; i < IO_INPUT_CHIP_MAX; i++ ) {
    lastScan[i] = scan[i];
  }
}

void driveSlowSwitches(void)
{
  scanSwitches();
  for ( uint8_t i = 0; i < SWITCH_QTY; i++ ) {
    switchStatus_t *s = &switchStatus[i];
    if ( s->flag ) {
      s->flag = false;
      const switchConfig_t *c = &switchConfigs[i];
      uint16_t t = getSysTime();
      if ( t - s->lastDebounce > c->debounce ) {
        s->lastDebounce = t;

        if ( !tilt || c->enabledInTilt ) {
          if ( gameOn || c->enabledInAttract ) {

            switchReturn_t r = c->callback(i,true);

            if ( r.points ) increaseScore( r.points );
            if ( r.sound ) playSound( r.sound );
          }
        }
      }
    }
  }
}


switchReturn_t targetBearTrig(slowSwitch theSwitch, boolean active)
{
  switchReturn_t ret = { SCORE_ONE_HUNDRED, SOUND_BEAR_TARGETS };
  if(getLampMode(theSwitch) != LAMP_ON_STATE) {
    setLampMode(theSwitch, LAMP_ON_STATE, INFINITE, 1);
  } else {
    return ret;
  }

  // all lamps must be lit
  for(int i=LAMP_TARGET_1_B; i<=LAMP_TARGET_1_R; i++) {
    if(getLampMode(i) != LAMP_ON_STATE) {
      return ret;
    }
  }
  
  setTimer(BEAR_TMR, ONE_HUNDRETH_SECOND, BEAR_OPEN);
  setTimer(BOX_TMR, QUARTER_SECOND, 0);
  hitSolenoid( SOLENOID_WOODBLOCK );
  return (switchReturn_t){ SCORE_ONE_THOUSAND, SOUND_BONUS };
}

switchReturn_t targetLogjamTrig(slowSwitch theSwitch, boolean active)
{
  switchReturn_t ret = { SCORE_ONE_HUNDRED, SOUND_LOGS };
  if(getLampMode(theSwitch) != LAMP_ON_STATE) {
    setLampMode(theSwitch, LAMP_ON_STATE, INFINITE, 1);
  } else {
    return ret;
  }

  // all lamps must be lit
  for(int i=LAMP_TARGET_2_L; i<=LAMP_TARGET_2_M; i++) {
    if(getLampMode(i) != LAMP_ON_STATE) {
      return ret;
    }
  }
  
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

  return (switchReturn_t){ SCORE_ONE_THOUSAND, SOUND_BONUS };
}

// upper and lower snake switch
switchReturn_t targetSnakeTrig(slowSwitch theSwitch, boolean active)
{
  switchReturn_t ret = { SCORE_ONE_HUNDRED, SOUND_SNAKE };
  if (getLampMode(theSwitch) != LAMP_ON_STATE) {
    setLampMode(theSwitch, LAMP_ON_STATE, INFINITE, 1);
  }

  // all lamps must be lit
  if( (getLampMode(LAMP_TARGET_LOWER) != LAMP_ON_STATE ) || 
      (getLampMode(LAMP_TARGET_UPPER) != LAMP_ON_STATE ) ) {
    return ret;
  }

  setLampMode(LAMP_TARGET_LOWER, LAMP_BLINK_STATE, EIGTH_SECOND, 5);
  setLampMode(LAMP_TARGET_UPPER, LAMP_BLINK_STATE, EIGTH_SECOND, 5);
  setLampMode(LAMP_POP_BUMPER_LOWER, LAMP_FLASH_STATE, EIGTH_SECOND, 5);
  return (switchReturn_t){ SCORE_ONE_THOUSAND, SOUND_BONUS };
}

switchReturn_t bearCaptureTrig(slowSwitch theSwitch, boolean active)
{
  setTimer(BEAR_TMR, EIGTH_SECOND, BEAR_CHEW);
  return (switchReturn_t){ SCORE_FIVE_THOUSAND, SOUND_BEAR_CHEW };
}

switchReturn_t coinTrig(slowSwitch theSwitch, boolean active)
{
  increaseCredits(1,TRUE);
  return (switchReturn_t){ 0, SOUND_COIN };
}

switchReturn_t startTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) {
    if ( decreaseCredits() ) {
      startGame();
      setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_BALL_LOAD);
    } else {
      setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_INSERT_COINS);    
    }
  }
  return (switchReturn_t){0,SOUND_BONUS};
}

switchReturn_t drainTrig(slowSwitch theSwitch, boolean active)
{
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
  return (switchReturn_t){0,0};
}

switchReturn_t tiltTrig(slowSwitch theSwitch, boolean active)
{
  if ( ++tiltSense > nonVolatiles.tiltSensitivity )
  { 
    tilt = TRUE;
    setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_TILT);
  }
  return (switchReturn_t){0,0};
}

switchReturn_t spinnerTrig(slowSwitch theSwitch, boolean active)
{
  setTimer(SPIN_TMR, ONE_HUNDRETH_SECOND, 1);
  return (switchReturn_t){SCORE_ONE_HUNDRED, SOUND_SPINNER};
}

switchReturn_t laneTrig(slowSwitch theSwitch, boolean active)
{
  switchReturn_t ret = { SCORE_ONE_HUNDRED, SOUND_LANE };
  switch ( theSwitch ) {
    case SWITCH_LANE_LEFT:
      if(getLampMode(LAMP_LANE_LEFT) != LAMP_ON_STATE) {
        setLampMode(LAMP_LANE_LEFT, LAMP_ON_STATE, INFINITE, 1);
      }
      break;
    case SWITCH_LANE_RIGHT:
      if(getLampMode(LAMP_LANE_RIGHT) != LAMP_ON_STATE) {
        setLampMode(LAMP_LANE_RIGHT, LAMP_ON_STATE, INFINITE, 1);
      }
      break;
    default:
      break;
  }

  // all lamps must be lit
  if( (getLampMode(LAMP_LANE_LEFT) != LAMP_ON_STATE ) || 
      (getLampMode(LAMP_LANE_RIGHT) != LAMP_ON_STATE ) ) {
    return ret;
  }

  setLampMode(LAMP_LANE_LEFT, LAMP_BLINK_STATE, EIGTH_SECOND, 5);
  setLampMode(LAMP_LANE_RIGHT, LAMP_BLINK_STATE, EIGTH_SECOND, 5);
  return (switchReturn_t){ SCORE_ONE_THOUSAND, SOUND_BONUS };
}

void laneToggle(void) {
  if ( getLampMode(LAMP_LANE_LEFT) != getLampMode(LAMP_LANE_RIGHT) ) {
    setLampMode( LAMP_LANE_LEFT, !getLampMode(LAMP_LANE_LEFT), 0, 0 );
    setLampMode( LAMP_LANE_RIGHT, !getLampMode(LAMP_LANE_RIGHT), 0, 0 );
  }
}

switchReturn_t doorTrig(slowSwitch theSwitch, boolean active)
{
  doorMenu(theSwitch);
  return (switchReturn_t){0, SOUND_DOOR_BUTTON};
}  
  
  
