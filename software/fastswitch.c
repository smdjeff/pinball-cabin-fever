#include "tweenpin.h"

// starts already "expired" so a press shortly after power-up still growls,
// instead of being muted until SIXTY_SECONDS has passed since boot.
static uint16_t lastBearNoiseTime = -(SIXTY_SECONDS + 1);
static uint8_t  prevAttractFlip = 0;

// restarts the attract bear-noise cooldown, as if one had just played.
// called when a game ends so flippers mashed right after drain/game-over
// don't immediately growl.
void resetBearNoiseCooldown(void) {
  lastBearNoiseTime = getSysTime();
}

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

  // flippers are hit with 80% power for about 130 ms (FLIPPER_HIT_TIME),
  // then drop to 5% while held to keep from overheating. a new flip needs the button to
  // have been released for FLIPPER_REARM, which ignores release bounce
  // without blocking quick re-flips.
  const uint16_t FLIPPER_HIT_TIME = EIGTH_SECOND;
  const uint16_t FLIPPER_REARM = TWO_HUNDRETH_SECONDS;

  static uint16_t flipperRightTimer = 0;
  static uint16_t flipperLeftTimer = 0;
  static uint16_t flipperRightReleased = 0;
  static uint16_t flipperLeftReleased = 0;

  uint16_t time = getSysTime();
  
  uint8_t pop = ~read( FASTSWITCH_BUMPER_PIN, FASTSWITCH_BUMPER_MASK );
  if ( !gameOn || tilt ) pop = 0;   // pops are dead when tilted, like the flippers

  // score only when the coil actually fires. checking isSolenoidOn() let a
  // switch still closed between the end of the pulse and the retrigger time
  // score (and restart the sound) on every pass of the main loop.
  if ( pop & FASTSWITCH_POP_BUMPER_UPPER ) {
    if ( hitSolenoid( SOLENOID_POP_BUMPER_UPPER ) ) {
      increaseScore(SCORE_ONE_HUNDRED);
      playSound( SOUND_LIZZARD );
      setLampMode( LAMP_POP_BUMPER_UPPER, LAMP_FLASH_STATE, EIGTH_SECOND, 1 );
    }
  }
  if ( pop & FASTSWITCH_POP_BUMPER_LOWER ) {
    if ( hitSolenoid( SOLENOID_POP_BUMPER_LOWER ) ) {
      increaseScore(SCORE_ONE_HUNDRED);
      playSound( SOUND_SNAKE );
      setLampMode( LAMP_POP_BUMPER_LOWER, LAMP_FLASH_STATE, EIGTH_SECOND, 1 );
    }
  }

  uint8_t flip = ~read( FASTSWITCH_FLIPPER_PIN, FASTSWITCH_FLIPPER_MASK );

  // attract mode: a flipper press growls a random bear noise, but no more
  // often than once every 60 seconds so it doesn't get obnoxious.
  if ( !gameOn ) {
    uint8_t attractFlip = flip & FASTSWITCH_FLIPPER_MASK;
    if ( attractFlip & ~prevAttractFlip ) {   // newly pressed, not held
      if ( (uint16_t)(time - lastBearNoiseTime) > SIXTY_SECONDS ) {
        lastBearNoiseTime = time;
        playSound( SOUND_BEAR_CHEW );   // random bear roar
      }
    }
    prevAttractFlip = attractFlip;
  }

  if ( !gameOn ) flip = 0;
  if ( crazyMode ) {
    flip = ((flip&FASTSWITCH_FLIPPER_LEFT)?FASTSWITCH_FLIPPER_RIGHT:0) | 
           ((flip&FASTSWITCH_FLIPPER_RIGHT)?FASTSWITCH_FLIPPER_LEFT:0) ;
  }

  switch ( flipperGetPulseRight() ) {
    case off:
      if ( flip & FASTSWITCH_FLIPPER_RIGHT ) {
        if ( (uint16_t)(time - flipperRightReleased) > FLIPPER_REARM ) {
          flipperRightTimer = time;
          flipperSetPulseRight( hit );
          laneToggle();
        }
      }
      break;
    case hit:
      if ( (uint16_t)(time - flipperRightTimer) > FLIPPER_HIT_TIME ) {
        flipperSetPulseRight( hold );
      }
      break;
    case hold:
      if ( !( flip & FASTSWITCH_FLIPPER_RIGHT) ) {
        flipperSetPulseRight( off );
        flipperRightReleased = time;
      }
      break;
  }
  
  switch ( flipperGetPulseLeft() ) {
    case off:
      if ( flip & FASTSWITCH_FLIPPER_LEFT ) {
        if ( (uint16_t)(time - flipperLeftReleased) > FLIPPER_REARM ) {
          flipperLeftTimer = time;
          flipperSetPulseLeft( hit );
          laneToggle();
        }
      }
      break;
    case hit:
      if ( (uint16_t)(time - flipperLeftTimer) > FLIPPER_HIT_TIME ) {
        flipperSetPulseLeft( hold );
      }
      break;
    case hold:
      if ( !( flip & FASTSWITCH_FLIPPER_LEFT) ) {
        flipperSetPulseLeft( off );
        flipperLeftReleased = time;
      }
      break;
  }
}
