#include "tweenpin.h"


void initFastSwitches(void) {
    input( FASTSWITCH_BUMPER_DDR, FASTSWITCH_BUMPER_MASK );    
    high( FASTSWITCH_BUMPER_PORT, FASTSWITCH_BUMPER_MASK );
    input( FASTSWITCH_FLIPPER_DDR, FASTSWITCH_FLIPPER_MASK );    
    high( FASTSWITCH_FLIPPER_PORT, FASTSWITCH_FLIPPER_MASK );
}

void driveFastSwitches(void) {
  const uint8_t off = 0;
  const uint8_t hit = 80;
  const uint8_t hold = 5;

  // generally flippers are hit with full strength for 200ms
  // then 10% while held to keep from overheating

  static uint16_t flipperRightTimer = 0;
  static uint16_t flipperLeftTimer = 0;

  uint16_t time = getSysTime();
  
  uint8_t pop = ~read( FASTSWITCH_BUMPER_PIN, FASTSWITCH_BUMPER_MASK );

  if ( pop & FASTSWITCH_POP_BUMPER_UPPER ) {
    if( !isSolenoidOn(SOLENOID_POP_BUMPER_UPPER) ) {
      increaseScore(SCORE_ONE_HUNDRED);
      hitSolenoid( SOLENOID_POP_BUMPER_UPPER );
      setLampMode( LAMP_POP_BUMPER_UPPER, LAMP_FLASH_STATE, EIGTH_SECOND, 1 );
      hitSolenoid( SOLENOID_BELL );
    }
  }
  if ( pop & FASTSWITCH_POP_BUMPER_LOWER ) {
    if( !isSolenoidOn(SOLENOID_POP_BUMPER_LOWER) ) {
      increaseScore(SCORE_ONE_HUNDRED);
      hitSolenoid( SOLENOID_POP_BUMPER_LOWER );
      setLampMode( LAMP_POP_BUMPER_LOWER, LAMP_FLASH_STATE, EIGTH_SECOND, 1 );
      hitSolenoid( SOLENOID_BELL );
    }
  }

  uint8_t flip = ~read( FASTSWITCH_FLIPPER_PIN, FASTSWITCH_FLIPPER_MASK );

  switch ( flipperGetPulseRight() ) {
    case off:
      if ( flip & FASTSWITCH_FLIPPER_RIGHT ) {
        if ( time - flipperRightTimer > QUARTER_SECOND ) {
          flipperRightTimer = time;
          flipperSetPulseRight( hit );
        }
      }
      break;
    case hit:
      if ( time - flipperRightTimer > EIGTH_SECOND ) {
        flipperSetPulseRight( hold );
      }
      break;
    case hold:
      if ( !( flip & FASTSWITCH_FLIPPER_RIGHT) ) {
        flipperSetPulseRight( off );
      }
      break;
  }
  
  switch ( flipperGetPulseLeft() ) {
    case off:
      if ( flip & FASTSWITCH_FLIPPER_LEFT ) {
        if ( time - flipperLeftTimer > QUARTER_SECOND ) {
          flipperLeftTimer = time;
          flipperSetPulseLeft( hit );
        }
      }
      break;
    case hit:
      if ( time - flipperLeftTimer > EIGTH_SECOND ) {
        flipperSetPulseLeft( hold );
      }
      break;
    case hold:
      if ( !( flip & FASTSWITCH_FLIPPER_LEFT) ) {
        flipperSetPulseLeft( off );
      }
      break;
  }
}
