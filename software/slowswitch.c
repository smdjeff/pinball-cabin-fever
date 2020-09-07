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
void targetMiscTrig(slowSwitch theSwitch, boolean active);
void bearCaptureTrig(slowSwitch theSwitch, boolean active);
void gameTrig(slowSwitch theSwitch, boolean active);
void tiltTrig(slowSwitch theSwitch, boolean active);
void laneTrig(slowSwitch theSwitch, boolean active);
void testTrig(slowSwitch theSwitch, boolean active);

// must match fastswitch enum order
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
  targetMiscTrig,
  targetMiscTrig,
  bearCaptureTrig,
  gameTrig,
  gameTrig,
  gameTrig,
  tiltTrig,
  laneTrig,
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
//		scan[i] = readIO( i, IO_INTF );
    scan[i] = readIO( i, IO_GPIO );
//      scan[i] = readIO( i, IO_DIR );
//      scan[i] = readIO( i, IO_GPPU );
//      scan[i] = readIO( i, IO_IPOL );
  }
	writeDisplay( DISPLAY_DIG0, DISP_BLANK);
	writeDisplay( DISPLAY_DIG1, DISP_BLANK);
	writeDisplayNum( DISPLAY_DIG2, scan[2] >> 4);
	writeDisplayNum( DISPLAY_DIG3, scan[2] & 0x0F);
	writeDisplayNum( DISPLAY_DIG4, scan[1] >> 4);
	writeDisplayNum( DISPLAY_DIG5, scan[1] & 0x0F);
	writeDisplayNum( DISPLAY_DIG6, scan[0] >> 4);
	writeDisplayNum( DISPLAY_DIG7, scan[0] & 0x0F);
}

//io interrupt
/*ISR(INT2_vect)
{
  static uint8_t lastScan[IO_INPUT_CHIP_COUNT];
  uint8_t scan[IO_INPUT_CHIP_COUNT];
  uint8_t switchchip, switchmask;
  uint8_t i;


  for ( i = IO_INPUT_CHIP_0; i < IO_INPUT_CHIP_MAX; i++ )
	{
//		scan[i] = readIO( i, IO_INTF );
    scan[i] = readIO( i, IO_GPIO );
  }
  for ( i = 0; i < SWITCH_QTY; i++ ) {
    switchchip = slowSwitchIO[i] >> 3;
    switchmask = BIT(slowSwitchIO[i] & 0x07);
    if ( (scan[switchchip] & switchmask) != (lastScan[switchchip] & switchmask ) ) {
      slowSwitchFunctions[i](i,(scan[switchchip] & switchmask));
writeDisplayNum( DISPLAY_DIG7, i&0x0f);
writeDisplayNum( DISPLAY_DIG6, i>>4);
break;
    }
  }

  for ( i = IO_INPUT_CHIP_0; i < IO_INPUT_CHIP_MAX; i++ )
	{
		lastScan[i] = scan[i];
  }
   
}*/

static uint8_t lastScan[IO_INPUT_CHIP_COUNT];
static boolean switchFlags[SWITCH_QTY];

void driveSlowSwitchISR(void)
{
  uint8_t scan[IO_INPUT_CHIP_COUNT];
  uint8_t switchchip, switchmask;
  boolean doCheck = FALSE;
  uint8_t i;

  for ( i = IO_INPUT_CHIP_0; i < IO_INPUT_CHIP_MAX; i++ )
	{
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
  for ( i = IO_INPUT_CHIP_0; i < IO_INPUT_CHIP_MAX; i++ )
	{
		lastScan[i] = scan[i];
  }
}

void driveSlowSwitches(void)
{
  uint8_t i;
  DECLARE_INT_STATE;

  for ( i = 0; i < SWITCH_QTY; i++ ) {
    DISABLE_INTS();
    if( switchFlags[i] ) {
      switchFlags[i] = FALSE;
      RESTORE_INTS();
      slowSwitchFunctions[i](i,TRUE);
    } else {
      RESTORE_INTS();
    }
  }
}



void targetBearTrig(slowSwitch theSwitch, boolean active)
{
  uint8_t i;

  if(!gameOn) return;

  if(getLampMode(theSwitch) == LAMP_OFF_STATE) {
    setLampMode(theSwitch, LAMP_ON_STATE, INFINITE, 1);
    increaseScore(SCORE_ONE_HUNDRED);
  } else {
    return;
  }

  for(i = LAMP_TARGET_1_B; i <= LAMP_TARGET_1_R; i++) {
    if(getLampMode(i) != LAMP_ON_STATE) {
      return;
    }
  }
  // all lamps must be lit
  increaseScore(SCORE_ONE_THOUSAND);
  setTimer(BEAR_TMR, HALF_SECOND,BEAR_OPEN);
  setLampMode( LAMP_BEAR_ARROW, LAMP_BLINK_ON_STATE, HALF_SECOND, INFINITE );
  for(i = LAMP_TARGET_1_B; i <= LAMP_TARGET_1_R; i++) {
    setLampMode(i, LAMP_FLASH_STATE, EIGTH_SECOND, 5);
  }
}

void targetLogjamTrig(slowSwitch theSwitch, boolean active)
{
  uint8_t i;

  if(!gameOn) return;

  if(getLampMode(theSwitch) == LAMP_OFF_STATE) {
    setLampMode(theSwitch, LAMP_ON_STATE, INFINITE, 1);
    increaseScore(SCORE_ONE_HUNDRED);
  } else {
    return;
  }

  for(i = LAMP_TARGET_2_L; i <= LAMP_TARGET_2_M; i++) {
    if(getLampMode(i) != LAMP_ON_STATE) {
      return;
    }
  }

  // all lamps must be lit
  increaseScore(SCORE_ONE_THOUSAND);
  for(i = LAMP_TARGET_2_L; i <= LAMP_TARGET_2_M; i++) {
    setLampMode(i, LAMP_FLASH_STATE, EIGTH_SECOND, 5);
  }


}

// upper and lower
void targetMiscTrig(slowSwitch theSwitch, boolean active)
{
  uint8_t i;

  if(!gameOn) return;
 
  if(getLampMode(theSwitch) == LAMP_OFF_STATE) {
    setLampMode(theSwitch, LAMP_ON_STATE, INFINITE, 1);
    increaseScore(SCORE_ONE_HUNDRED);
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
  setLampMode(LAMP_TARGET_LOWER, LAMP_FLASH_STATE, EIGTH_SECOND, 5);
  setLampMode(LAMP_TARGET_UPPER, LAMP_FLASH_STATE, EIGTH_SECOND, 5);
  setLampMode(LAMP_POP_BUMPER_LOWER, LAMP_FLASH_STATE, EIGTH_SECOND, 5);
}

void bearCaptureTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) return;
  
  increaseScore(SCORE_ONE_HUNDRED);
  setLampMode(LAMP_BEAR_MOUTH, LAMP_FLASH_STATE, EIGTH_SECOND, 5);
  setTimer(BEAR_TMR, EIGTH_SECOND,BEAR_CHEW_CLOSE);
}


// drain, coin, start
void gameTrig(slowSwitch theSwitch, boolean active)
{
  static uint32_t lastScore = 0;
  uint32_t thisScore;

  if(!gameOn && theSwitch == SWITCH_GAME_START) {
    setSolenoidMode( SOLENOID_BALL_LOADER, SOLENOID_FLASH_STATE, QUARTER_SECOND, 1 );
    startGame();
    lastScore = 0;
  }
  if(theSwitch == SWITCH_BALL_DRAIN && gameOn) {
    thisScore = getScore();
    if( thisScore == lastScore) {
      displayString( DISP_T, DISP_R, DISP_Y, DISP_A, DISP_G, DISP_A, DISP_I, DISP_N );      
      setSolenoidMode( SOLENOID_BALL_LOADER, SOLENOID_FLASH_STATE, QUARTER_SECOND, 1 );
    } else {
      gameOver();
    }
  }
}

typedef enum {
  NO_TILT,
  TILT_RISK,
  TILTING,
} tiltStates;
tiltStates tiltState = NO_TILT;

void tiltTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) return;
  
  if(tiltState == NO_TILT) {
    setTimer(TILT_TMR, QUARTER_SECOND, 0);
    tiltState = TILT_RISK;
  } else {  
//    setTimer(GAME_TMR, ONE_HUNDRETH_SECOND, GAME_TILT);
    tilt = TRUE;
    setSolenoidMode( FASTSWITCH_FLIPPER_LEFT, SOLENOID_IDLE_STATE, 0, 1);
    setSolenoidMode( FASTSWITCH_FLIPPER_RIGHT, SOLENOID_IDLE_STATE, 0, 1);
    displayString( DISP_BLANK, DISP_BLANK, DISP_T, DISP_I, DISP_L, DISP_T, DISP_BLANK, DISP_BLANK );
    setLampMode( LAMP_BACKBOX_TILT, LAMP_ON_STATE, 0, INFINITE );
    tiltState = TILTING;
    setTimer(TILT_TMR, ONE_SECOND, 0);
  }
}

void tiltTimer(timerEvent evt, uint16_t data)
{
  tilt = FALSE;
  tiltState = NO_TILT;
  setLampMode( LAMP_BACKBOX_TILT, LAMP_OFF_STATE, 0, INFINITE );
  displayScore();
}

void laneTrig(slowSwitch theSwitch, boolean active)
{
  if(!gameOn) return;
  
}

void testTrig(slowSwitch theSwitch, boolean active)
{
  static uint16_t width = 1500;

  if(theSwitch == SWITCH_TEST_PLUS) {
    width=BEAR_PWM_OPEN;
  }
  if(theSwitch == SWITCH_TEST_MINUS) {
    width=BEAR_PWM_CLOSED;
  } 
  updateDisplay(width);
  ATOMIC(
    TCNT1 = 0;
    high(BEAR_PORT, BEAR_MASK);
    while(TCNT1 < width) {
      ;
    }
    low(BEAR_PORT, BEAR_MASK);      
  )

}


