#include "tweenpin.h"

typedef void (*timerCallback_t)(timerEvent, uint16_t);

typedef struct {
    uint16_t time;
    uint16_t lastTime;
    uint16_t data;
} timer_t;

static timer_t timers[TIMER_QTY] = {0,};


void initTimers(void) 
{
  // atmega1284
  // TIMER0 8BIT  PWM OC0A:PB3 OC0B:PB4  <- bear head pwm servo
  // TIMER1 16BIT PWM OC1A:PD5 OC1B:PD4  <- system tick
  // TIMER2 8BIT  PWM OC2A:PD7 OC2B:PD6  <- flipper pwm
  // TIMER3 16BIT PWM OC3A:PB6 OC3B:PB7  <- don't use conflicts with isp programmer / spi
  
  // // note: changes to systick require systimeDurations enum adjustments
  timer1( TIMER1_PRESCALER_1024, (F_CPU / 1024) / ONE_SECOND, systemTickISR );
}

void bearInitPulse(void) {
  low(BEAR_PORT, BEAR_MASK);
  output(BEAR_DDR, BEAR_MASK);
  TCCR0A = _BV(COM0B1); // clear OC0B on Compare Match, set OC0B at BOTTOM
  TCCR0A |= _BV(WGM01) | _BV(WGM00);  // fast pwm mode
  TCCR0B = _BV(CS02) | _BV(CS00); // 1024 prescaller
}

void bearSetPulse(uint8_t pulse)
{
  // 1024/16 MHz = 64 us units. 
  // servo control range is about 15 for 1 ms servo pulse
  // and 31 for 2 ms servo control pulse
  BEAR_OCR = pulse;
}

void flipperInitPulse(void) {
  high(  SOLENOID_FLIPPER_DDR, SOLENOID_FLIPPER_MASK );
  output( SOLENOID_FLIPPER_DDR, SOLENOID_FLIPPER_MASK );
  TCCR2A = _BV(COM2A1)|_BV(COM2B1); // Clear OC2A/B on Compare Match, set OC2A/B at BOTTOM
  TCCR2A |= _BV(WGM22)|_BV(WGM20);  // phase correct pwm mode (for lower noise)
  // higher frequencies are less audible, but may cause the FET to stay in the
  // transition zone and overheat.
  // TCCR2B = _BV(CS02)|_BV(CS00); // 128 prescaller ( ~500Hz)
  TCCR2B = _BV(CS02)|_BV(CS01); // 256 prescaller ( ~250Hz)
  // TCCR2B = _BV(CS02)|_BV(CS01)|_BV(CS00); // 1024 prescaller ( ~61Hz)
}

static bool flipEnabled = false;
static uint8_t pulseLeft = 0;
static uint8_t pulseRight = 0;

void flipperEnableSolenoids(bool enable) {
  flipperSetPulseRight(0);
  flipperSetPulseLeft(0);
  flipEnabled = enable;
}

uint8_t flipperGetPulseLeft(void)
 {
    return pulseLeft;
}

uint8_t flipperGetPulseRight(void) 
{
    return pulseRight;
}

void flipperSetPulseLeft(uint8_t perc)
{
    if ( !flipEnabled && perc ) return;
    pulseLeft = perc;
    SOLENOID_FLIPPER_LEFT_OCR = (perc * 255) / 100;
}

void flipperSetPulseRight(uint8_t perc)
{
    if ( !flipEnabled && perc ) return;
    pulseRight = perc;
    SOLENOID_FLIPPER_RIGHT_OCR = (perc * 255) / 100;
}

static uint16_t int_context_sysTime = 0;
// system tick ( 200Hz, 5ms tick )
void systemTickISR(void)
{
  int_context_sysTime++;
}

uint16_t getSysTime(void)
{
  uint16_t temp;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    temp = int_context_sysTime;
  }
  return temp;
}

// general purpose timer capabilities used by game play functionality
void driveTimers(void)
{
  for( int i=0; i<sizeof(timers)/sizeof(timers[0]); i++) {
    timer_t *tmr = &timers[i];
    if( tmr->time ) {
      uint16_t t = getSysTime();
      if( t - tmr->lastTime > tmr->time ) {
        tmr->lastTime = t;
        timerCallback_t cb = NULL;
        switch( i ) {
          case ATTRACT_LAMP_TMR: cb=attractLamps; break;
          case ATTRACT_TMR:      cb=attractTimer; break;
          case MATCH_TMR:        cb=matchTimer; break;
          case BEAR_TMR:         cb=bearTimer; break;
          case GAME_TMR:         cb=gameTimer; break;
          case CRAZY_TMR:        cb=crazyModeTimer; break;
          case SPIN_TMR:         cb=spinTimer; break;
          case BOX_TMR:          cb=boxTimer; break;
          case MUSIC_TMR:        cb=musicTimer; break;
          case WATCHDOG_TMR:     cb=watchdogTimer; break;
        }
        if (cb) cb( i, tmr->data );
      }
    }
  }
}

void setTimer( timerEvent timer, uint16_t time, uint16_t data )
{
    if( timer >= sizeof(timers)/sizeof(timers[0]) ) return;
    timer_t *tmr = &timers[timer];
    tmr->time = time;
    tmr->lastTime = getSysTime();
    tmr->data = data;
}

void setIfActiveTimer( timerEvent timer, uint16_t data )
{
    if( timer >= sizeof(timers)/sizeof(timers[0]) ) return;
    timer_t *tmr = &timers[timer];
    if ( tmr->time ) {
      tmr->time = 1;
      tmr->data = data;
    }
}

boolean isActiveTimer( timerEvent timer )
{
  if( timer >= sizeof(timers)/sizeof(timers[0]) ) return 0;
  timer_t *tmr = &timers[timer];
  return ( tmr->time );
}

void cancelTimer(timerEvent timer)
{
  if( timer >= sizeof(timers)/sizeof(timers[0]) ) return;
  timer_t *tmr = &timers[timer];
  tmr->time = 0;
}
