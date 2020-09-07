#include "tweenpin.h"

typedef enum
{
  FASTSWITCH_POP_BUMPER_UPPER_PIN = 7,  //sw0
  FASTSWITCH_POP_BUMPER_LOWER_PIN = 6,  //sw1
  FASTSWITCH_FLIPPER_LEFT_PIN = 5,  //sw2
  FASTSWITCH_FLIPPER_RIGHT_PIN = 4  //sw3
} fastSwitchPins;

// must match fastswitch enum order
static const uint8_t fastSwitchPin[FASTSWITCH_QTY] = { 
  FASTSWITCH_POP_BUMPER_UPPER_PIN,
  FASTSWITCH_POP_BUMPER_LOWER_PIN,
  FASTSWITCH_FLIPPER_LEFT_PIN,
  FASTSWITCH_FLIPPER_RIGHT_PIN
};

typedef void (*fastSwitchIsr)(fastSwitch, boolean);

void flipperISR(fastSwitch theSwitch, boolean active);
void bumperISR(fastSwitch theSwitch, boolean active);

// must match fastswitch enum order
static const fastSwitchIsr fastSwitchFunctions[FASTSWITCH_QTY] = {
  bumperISR,
  bumperISR,
  flipperISR,
  flipperISR
};


void driveFastSwitchISR(void)
{
  static uint8_t lastScan = 0;
  uint8_t scan;
  uint8_t bitmask;
  uint8_t i;
  
  scan = ~(FASTSWITCH_PIN & FASTSWITCH_MASK);

  for( i = 0 ; i < FASTSWITCH_QTY; i++ )
  {
    bitmask = BIT(fastSwitchPin[i]);
    if( (bitmask & scan) != (bitmask & lastScan) ) {
      fastSwitchFunctions[i](i,(bitmask & scan));
    }
  }
  lastScan = scan;
}


// must match fastswitch enum order
//  fastswitches are used to directly drive solenoids
static const uint8_t fastSwitchSolenoid[FASTSWITCH_QTY] = { 
  SOLENOID_POP_BUMPER_UPPER,
  SOLENOID_POP_BUMPER_LOWER,
  SOLENOID_FLIPPER_LEFT,
  SOLENOID_FLIPPER_RIGHT,
};

void flipperISR(fastSwitch theSwitch, boolean active)
{
  if(!gameOn || tilt) return;

  if(active) {
    if(getSolenoidMode( fastSwitchSolenoid[theSwitch] ) == SOLENOID_IDLE_STATE) {
      setSolenoidMode( fastSwitchSolenoid[theSwitch], SOLENOID_HOLD_DRIVE_STATE, THREE_HUNDRETH_SECONDS, 1);
    }
  } else {
    setSolenoidMode( fastSwitchSolenoid[theSwitch], SOLENOID_IDLE_STATE, 0, 1);
  }
}

void bumperISR(fastSwitch theSwitch, boolean active)
{
  if(!gameOn || tilt) return;

  if(active) {
    if(getSolenoidMode(fastSwitchSolenoid[theSwitch]) == SOLENOID_IDLE_STATE ) {
      increaseScore(SCORE_ONE_HUNDRED);
      setSolenoidMode( fastSwitchSolenoid[theSwitch] , SOLENOID_ONESHOT_DRIVE_STATE, FOUR_HUNDRETH_SECONDS, 1);
      setLampMode( LAMP_POP_BUMPER_UPPER , LAMP_FLASH_STATE, EIGTH_SECOND, 2 );
      setLampMode( LAMP_POP_BUMPER_LOWER , LAMP_FLASH_STATE, EIGTH_SECOND, 2 );
      setSolenoidMode( SOLENOID_BELL, SOLENOID_FLASH_STATE, THREE_HUNDRETH_SECONDS, 1 );
    }
  } else {
    setSolenoidMode( fastSwitchSolenoid[theSwitch], SOLENOID_IDLE_STATE, 0, 1);
  }
}


