#include "tweenpin.h"

typedef void (*timerCallback)(timerEvent, uint16_t);

const timerCallback timerCallbacks[TIMER_QTY] = {
  attractTimer,
  tiltTimer,
  bearTimer,
  gameTimer,
};

uint16_t timerTime[TIMER_QTY];
uint16_t timerLastTime[TIMER_QTY];
uint16_t timerData[TIMER_QTY];


uint16_t sysTime = 0;

void __delay_us(uint32_t us)
{
	volatile uint32_t i;
	for (i=0; i<us; i+=40)
	{
    // not used for precise time, so accuracy is not necessary
	}
}

void __delay_ms(uint32_t ms)
{
	volatile uint32_t i;
	for (i=0; i<ms; i++)
	{
		__delay_us( 1000 );
	}
}


// fast polling timer interrupt vector
//   drives systime, fast and slow switch polling loops
//   and solenoid PWM
//  timer run at 200Hz
ISR(TIMER0_COMP_vect)
{
  sysTime++;
  driveFastSwitchISR();
  driveSolenoidsISR();
  driveSlowSwitchISR();
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

  for( i=0; i< TIMER_QTY; i++) {
    if(timerTime[i]) {
      if( getSysTime() - timerLastTime[i] > timerTime[i] ) {
        timerTime[i] = 0;
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

void cancelTimer(timerEvent timer)
{
  timerTime[timer] = 0;
}
