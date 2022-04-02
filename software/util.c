#include "tweenpin.h"

typedef void (*timerCallback)(timerEvent, uint16_t);

const timerCallback timerCallbacks[TIMER_QTY] = {
  attractLamps,
  attractTimer,
  matchTimer,
  bearTimer,
  gameTimer,
  crazyModeTimer,
  spinTimer,
  boxTimer,
  solenoidTimer
};

uint16_t timerTime[TIMER_QTY];
uint16_t timerLastTime[TIMER_QTY];
uint16_t timerData[TIMER_QTY];


uint16_t sysTime = 0;

// fast polling timer interrupt vector
//   drives systime, fast and slow switch polling loops
//   and solenoid PWM
//  timer run at 200Hz
ISR(TIMER0_COMPA_vect)
{
  sysTime++;
  driveFastSwitchISR();
  driveSolenoidsISR();
  driveSlowSwitchISR();
}

ISR(TIMER2_COMPA_vect)
{
  driveLampISR();
}

uint16_t getSysTime(void)
{
  uint16_t temp;
  // use atomic since time is 16-bit and modified by interrupt
  ATOMIC(
    temp = sysTime;
  )
  return temp;
}


// general purpose timer capabilities used by game play functionality
void driveTimers(void)
{
  uint8_t i;
  for( i=0; i<TIMER_QTY; i++) {
    if(timerTime[i]) {
      uint16_t t = getSysTime();
      if( t - timerLastTime[i] > timerTime[i] ) {
        timerTime[i] = t;
        timerCallbacks[i](i, timerData[i]);
      }
    }
  }
}

void setTimer( timerEvent timer, uint16_t time, uint16_t data )
{
  timerTime[timer] = time;
  timerLastTime[timer] = getSysTime();
  timerData[timer] = data;
}

void setIfActiveTimer( timerEvent timer, uint16_t data )
{
  if ( timerTime[timer] > 0) {
    timerTime[timer] = 1;
    timerData[timer] = data;
  }
}

boolean isActiveTimer( timerEvent timer )
{
  return ( timerTime[timer] );
}

void cancelTimer(timerEvent timer)
{
  timerTime[timer] = 0;
}
