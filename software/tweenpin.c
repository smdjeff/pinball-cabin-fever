#include "tweenpin.h"

const uint8_t FINALBALL = 3;
uint8_t ballInPlay = 0;
boolean tilt = FALSE;
boolean gameOn = FALSE;

static uint8_t score[8];
static uint8_t multiplier = 1;


boolean getSwitch( uint8_t sw );
void attractMode(void);
void gameMode(void);
void gameOver(void);



typedef enum {
  ATTRACT_PLAY,
  ATTRACT_PINBALL,
  ATTRACT_PAUSE,
  ATTRACT_GAMEOVER,
} attractStates;


int main (void)
{
  initIO();
  resetGame();
  setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_PLAY);

  ENABLE_INTS();
  
  for (;; )
  {
		driveLamps();
    driveSlowSwitches();
    driveTimers();
    if ( !gameOn )
    {
      attractMode();
    } 
    else 
    {
      gameMode();
    }
  }
}


void attractMode(void)
{
  static uint8_t count = 0;
	static uint16_t lastTime = 0;

  // cycles through all lights on playfield, double-flash creates chasing effect
	if( (getSysTime() - lastTime) > EIGTH_SECOND ) {
      count++;
	  if(count > LAMP_RIVER_ARROW_6)
	    count = 0;
    setLampMode(count, LAMP_FLASH_STATE, QUARTER_SECOND, 2 );
		lastTime = getSysTime();  
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
    displayString( DISP_BLANK, DISP_BLANK, DISP_P, DISP_L, DISP_A, DISP_Y, DISP_BLANK, DISP_BLANK );
    setTimer(ATTRACT_TMR, TWO_SECONDS, ATTRACT_PINBALL);
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
      // "PINBALL"
      displayString( DISP_BLANK, DISP_P, DISP_I, DISP_N, DISP_B, DISP_A, DISP_L, DISP_L );
      setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_PAUSE);    
    }
    break;
  case ATTRACT_PAUSE:
    displayClear();
    setTimer(ATTRACT_TMR, ONE_SECOND,ATTRACT_PLAY);    
    break;
  case ATTRACT_GAMEOVER:
    if(cycles == 0) cycles = 8;
    if(--cycles) {
      if(cycles & 0x01) {
        // "YOU LOSE"
        displayString( DISP_Y, DISP_O, DISP_U, DISP_BLANK, DISP_L, DISP_O, DISP_S, DISP_E );
      } else {
        displayScore();
      }
      setTimer(ATTRACT_TMR, HALF_SECOND, ATTRACT_GAMEOVER);    
    } else {
      // "YOU LOSE"
      displayString( DISP_Y, DISP_O, DISP_U, DISP_BLANK, DISP_L, DISP_O, DISP_S, DISP_E );
      setTimer(ATTRACT_TMR, ONE_SECOND, ATTRACT_PAUSE);    
    }
    break;
  }
}


static void bearPWM(uint16_t width)
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
    bearPWM(BEAR_PWM_OPEN);
    setTimer(BEAR_TMR, QUARTER_SECOND,BEAR_OPEN);  // keep open
    break;
  case BEAR_CHEW_CLOSE:
    if(cycles == 0) cycles = 3;  // first time
    bearPWM(BEAR_PWM_CLOSED);
    setTimer(BEAR_TMR, HALF_SECOND,BEAR_CHEW_OPEN);
    break;
  case BEAR_CHEW_OPEN:
    bearPWM(BEAR_PWM_OPEN);
    if(--cycles) {
      setTimer(BEAR_TMR, HALF_SECOND,BEAR_CHEW_CLOSE);
    } else {
      setTimer(BEAR_TMR, HALF_SECOND,BEAR_EJECT);
    }
    break;
  case BEAR_EJECT:
    bearPWM(BEAR_PWM_OPEN);
    setSolenoidMode(SOLENOID_BEAR_CAPTURE, SOLENOID_FLASH_PAUSE_STATE, QUARTER_SECOND, 1);
    setTimer(BEAR_TMR, HALF_SECOND,BEAR_CLOSE);
    setLampMode( LAMP_BEAR_ARROW, LAMP_OFF_STATE, 0, INFINITE );
    bearPWM(BEAR_PWM_OPEN);
    break;
  case BEAR_CLOSE:
  default:
    bearPWM(BEAR_PWM_CLOSED);
    setTimer(BEAR_TMR, QUARTER_SECOND,BEAR_CLOSE);  // keep closed
    break;
  }
}


void gameMode(void)
{
  // not currenlty used - all activity driven via switches
}


void gameTimer(timerEvent evt, uint16_t state)
{
  if(!gameOn) return;

  switch(state) {
  case GAME_SCORE:
    displayScore();
    setTimer(GAME_TMR, QUARTER_SECOND, GAME_SCORE);
    break;
  case GAME_TILT:
    tilt = TRUE;
    setSolenoidMode( FASTSWITCH_FLIPPER_LEFT, SOLENOID_IDLE_STATE, 0, 1);
    setSolenoidMode( FASTSWITCH_FLIPPER_RIGHT, SOLENOID_IDLE_STATE, 0, 1);
    displayString( DISP_BLANK, DISP_BLANK, DISP_T, DISP_I, DISP_L, DISP_T, DISP_BLANK, DISP_BLANK );
    break;

  default:
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
  ballInPlay=0;
  tilt = FALSE;  
  setSolenoidMode( FASTSWITCH_FLIPPER_LEFT, SOLENOID_IDLE_STATE, 0, 1);
  setSolenoidMode( FASTSWITCH_FLIPPER_RIGHT, SOLENOID_IDLE_STATE, 0, 1);
  setLampMode( LAMP_BACKBOX_GAME_OVER, LAMP_ON_STATE, 0, INFINITE );
  setTimer(BEAR_TMR, ONE_HUNDRETH_SECOND, BEAR_EJECT);
}

void startGame(void)
{
  resetGame();
  resetScore();
  gameOn = TRUE;
  ballInPlay = 1;
  cancelTimer(ATTRACT_TMR);
  setLampMode( LAMP_BACKBOX_GAME_OVER, LAMP_OFF_STATE, 0, INFINITE );
  setLampMode( LAMP_BONUS_1, LAMP_ON_STATE, 0, INFINITE );  
}

void gameOver(void)
{
  resetGame();
  setSolenoidMode( SOLENOID_KNOCKER, SOLENOID_FLASH_STATE, QUARTER_SECOND, 3);
  setTimer(ATTRACT_TMR, ONE_HUNDRETH_SECOND, ATTRACT_GAMEOVER);
}



//////////////////////////////////
// scoring
//////////////////////////////////

// "BCD-style" used for scoring in order to avoid divides and modulus when updating display
const uint8_t SCORE_TEN[8] = { 0, 0, 0, 0, 0, 0, 1, 0 };
const uint8_t SCORE_ONE_HUNDRED[8] = { 0, 0, 0, 0, 0, 1, 0, 0 };
const uint8_t SCORE_FIVE_HUNDRED[8] = { 0, 0, 0, 0, 0, 5, 0, 0 };
const uint8_t SCORE_ONE_THOUSAND[8] = { 0, 0, 0, 0, 1, 0, 0, 0 };
const uint8_t SCORE_FIVE_THOUSAND[8] = { 0, 0, 0, 0, 5, 0, 0, 0 };

void displayScore(void)
{
  boolean pastZeroes=FALSE;
  uint8_t i;

  for( i = 0; i < 8; i++ ) {
    if( (i==7) || score[i] > 0 || pastZeroes) {
      writeDisplayNum(DISPLAY_DIG0+i, score[i]);
      pastZeroes = TRUE;
    } else {
      writeDisplay(DISPLAY_DIG0+i, DISP_BLANK);
    }
  }
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
  displayScore();  
}



