#include "tweenpin.h"

typedef enum
{
  SOLENOID_POP_BUMPER_UPPER_PIN = 4, //sol4
  SOLENOID_POP_BUMPER_LOWER_PIN = 3, //sol3
  SOLENOID_FLIPPER_RIGHT_PIN = 1, //sol1
  SOLENOID_FLIPPER_LEFT_PIN = 5, //sol5
  SOLENOID_BEAR_CAPTURE_PIN = 0, //sol0
  SOLENOID_BALL_LOADER_PIN = 2, //sol2
  SOLENOID_KNOCKER_PIN = 7, //sol7
  SOLENOID_BELL_PIN = 6,    //sol6
  SOLENOID_UNUSED_1_PIN = 6,
  SOLENOID_UNUSED_2_PIN = 7
} solenoidPins;

static const uint8_t solenoidPin[SOLENOID_QTY+SOLENOID_QTY_H] = { 
  SOLENOID_POP_BUMPER_UPPER_PIN,
  SOLENOID_POP_BUMPER_LOWER_PIN,
  SOLENOID_FLIPPER_RIGHT_PIN,
  SOLENOID_FLIPPER_LEFT_PIN,
  SOLENOID_BEAR_CAPTURE_PIN,
  SOLENOID_BALL_LOADER_PIN,
  SOLENOID_KNOCKER_PIN,
  SOLENOID_BELL_PIN,
  SOLENOID_UNUSED_1_PIN,
  SOLENOID_UNUSED_2_PIN
};

static solenoidStates solenoidState[SOLENOID_QTY];
static uint16_t solenoidTime[SOLENOID_QTY];
static uint16_t solenoidStateTime[SOLENOID_QTY];
static uint8_t solenoidCycles[SOLENOID_QTY];


typedef struct {
  boolean active;
  solenoid sol;
  solenoidStates mode;
  uint16_t time;
  uint8_t cycles; 
} solenoidState_t;

static solenoidState_t queuedSolenoid = {0,};


static void setSolenoid( solenoid sol, boolean on )
{
  if ( nonVolatiles.quiteMode == 2 ) {
    if ( sol==SOLENOID_BELL_PIN || sol==SOLENOID_KNOCKER_PIN ) {
      return;
    }
  }
  
  if(on) {
    if(sol < SOLENOID_QTY) {
      high(SOLENOID_PORT, BIT(solenoidPin[sol]));
    } else if (sol < SOLENOID_QTY + SOLENOID_QTY_H) {
      high(SOLENOID_PORT_H, BIT(solenoidPin[sol]));
    }
   } else {
    if(sol < SOLENOID_QTY) {
      low(SOLENOID_PORT, BIT(solenoidPin[sol]));
    } else if (sol < SOLENOID_QTY + SOLENOID_QTY_H) {
      low(SOLENOID_PORT_H, BIT(solenoidPin[sol]));
    }  
  }   
}

solenoidStates getSolenoidMode( solenoid sol )
{
  return solenoidState[sol];
}


void solenoidTimer(timerEvent evt, uint16_t state)
{
  if ( queuedSolenoid.active ) {
    queuedSolenoid.active = FALSE;
    setSolenoidMode( queuedSolenoid.sol, queuedSolenoid.mode, 
                     queuedSolenoid.time, queuedSolenoid.cycles );
  }
}


// Modes supported:
//  Idle - solenoid inactive
//  Hold - for hitting a solenoid, then holding it active with PWM
//  Oneshot - for hitting a solenoid, then releasing it and keeping it released
//     until the mode is returned to idle
//  Flash - for hitting a solenoid multiple times in succession
void setSolenoidMode( solenoid sol, solenoidStates mode, uint16_t time, uint8_t cycles )
{
  
  // prevent brown outs
  if ( (sol == SOLENOID_BELL) || (sol == SOLENOID_KNOCKER) ) {
    if ( isActiveTimer( SOLENOID_TMR ) ) {
      if ( sol == SOLENOID_KNOCKER ) {
        queuedSolenoid.active = TRUE;
        queuedSolenoid.sol = sol;
        queuedSolenoid.mode = mode;
        queuedSolenoid.time = time;
        queuedSolenoid.cycles = cycles;
      }
      return;
    } else {
      setTimer( SOLENOID_TMR, QUARTER_SECOND, 0 );
    }
  }

  ATOMIC(
    setSolenoid(sol, (mode > SOLENOID_ACTIVE));
    solenoidState[sol] = mode;
    solenoidTime[sol] = getSysTime();
    solenoidStateTime[sol] = time;
    solenoidCycles[sol] = cycles;
  )
}

// Solenoids driven by ISR to reliably support PWM modes used when flippers
//   are held in their "up" states
void driveSolenoidsISR(void)
{
  uint8_t i;

  for( i = 0; i < SOLENOID_QTY; i++ )
  {
    switch( solenoidState[i] ) {
      case SOLENOID_HOLD_DRIVE_STATE:
        if( getSysTime() - solenoidTime[i] > solenoidStateTime[i] ) {
          solenoidState[i] = SOLENOID_HOLD_PWMOFF_STATE;
          solenoidTime[i] = getSysTime();
          setSolenoid( i, FALSE);
        }
        break;
      case SOLENOID_ONESHOT_DRIVE_STATE:
        if( getSysTime() - solenoidTime[i] > solenoidStateTime[i] ) {
            solenoidState[i] = SOLENOID_ONESHOT_COMPLETE_STATE;
            setSolenoid( i, FALSE);
        }
        break;
      case SOLENOID_HOLD_PWMON_STATE:  
        if( getSysTime() - solenoidTime[i] > ONE_HUNDRETH_SECOND ) {
          solenoidState[i] = SOLENOID_HOLD_PWMOFF_STATE;
          setSolenoid( i, FALSE);
          solenoidTime[i] = getSysTime();
        }
        break;
      case SOLENOID_HOLD_PWMOFF_STATE:
        if( getSysTime() - solenoidTime[i] > FOUR_HUNDRETH_SECONDS ) {
          solenoidState[i] = SOLENOID_HOLD_PWMON_STATE;
          setSolenoid( i, TRUE);
          solenoidTime[i] = getSysTime();
        }
        break;
      case SOLENOID_FLASH_STATE:
        if( getSysTime() - solenoidTime[i] > solenoidStateTime[i] ) {
          if( --solenoidCycles[i] ) {
            solenoidState[i] = SOLENOID_FLASH_PAUSE_STATE;
            solenoidTime[i] = getSysTime();          
          } else {
            solenoidState[i] = SOLENOID_IDLE_STATE;
          }
          setSolenoid( i, FALSE);        
        } 
        break;
      case SOLENOID_FLASH_PAUSE_STATE:
        if( getSysTime() - solenoidTime[i] > solenoidStateTime[i] ) {
            solenoidState[i] = SOLENOID_FLASH_STATE;
            solenoidTime[i] = getSysTime();          
            setSolenoid( i, TRUE);                
        }
        break;
      case SOLENOID_IDLE_STATE:
      case SOLENOID_ACTIVE:
      case SOLENOID_ONESHOT_COMPLETE_STATE:
      default:
        // do nothing - steady state
        break;
    }
  }
}

