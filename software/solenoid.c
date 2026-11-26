#include "tweenpin.h"

// solenoids fire for a fixed pulse, then driveSolenoids() turns them off.
//
// timing uses elapsed time ( now - onTime ), which stays correct when the
// 16-bit system tick wraps every ~5.5 minutes. the old code stored an
// absolute off time and compared it with '<', so a hit just before the wrap
// could leave a coil on forever.

#define SOLENOID_PULSE      TEN_HUNDRETH_SECONDS
#define SOLENOID_RETRIGGER  QUARTER_SECOND

static uint16_t onTime[SOLENOID_QTY] = {0,};
static uint8_t  active = 0;   // bit per solenoid that is currently pulsing

void initSolenoids(void) {
    low( SOLENOID_PORT, SOLENOID_MASK );
    output( SOLENOID_DDR, SOLENOID_MASK );
}

static uint8_t solenoidBit( solenoid sol )
{
  switch ( sol ) {
    case SOLENOID_WOODBLOCK:          return BIT(0);
    case SOLENOID_KNOCKER:            return BIT(1);
    case SOLENOID_BALL_LOADER:        return BIT(2);
    case SOLENOID_POP_BUMPER_LOWER:   return BIT(3);
    case SOLENOID_POP_BUMPER_UPPER:   return BIT(4);
    case SOLENOID_BEAR_CAPTURE:       return BIT(5);
    default:                          return 0;
  }
}

static void setSolenoid( solenoid sol, boolean on )
{
  uint8_t bit = solenoidBit( sol );
  if ( !bit ) return;
  if ( on ) {
    // woodblock and knocker are noise makers, muted at low volume
    if ( ( sol == SOLENOID_WOODBLOCK || sol == SOLENOID_KNOCKER ) &&
         nonVolatiles.volume < 2 ) return;
    high( SOLENOID_PORT, bit );
  } else {
    low( SOLENOID_PORT, bit );
  }
}

bool isSolenoidOn( solenoid sol )
{
  if ( sol >= SOLENOID_QTY ) return false;
  return ( active & BIT(sol) ) != 0;
}

void hitSolenoid( solenoid sol ) 
{
  uint16_t time = getSysTime();
  if ( sol >= SOLENOID_QTY ) return;
  if ( (uint16_t)(time - onTime[sol]) < SOLENOID_RETRIGGER ) return;
  onTime[sol] = time;
  active |= BIT(sol);
  setSolenoid( sol, true );
}

void allSolenoidsOff(void)
{
  active = 0;
  low( SOLENOID_PORT, SOLENOID_MASK );
}

void driveSolenoids(void)
{
  uint16_t time = getSysTime();
  uint8_t allowed = 0;

  for ( uint8_t i = 0; i < SOLENOID_QTY; i++ ) {
    if ( active & BIT(i) ) {
      if ( (uint16_t)(time - onTime[i]) > SOLENOID_PULSE ) {
        active &= ~BIT(i);
        setSolenoid( i, false );
      } else {
        allowed |= solenoidBit( i );
      }
    }
  }

  // safety net: any coil output that isn't in the middle of a timed pulse
  // gets forced off, whatever turned it on.
  low( SOLENOID_PORT, SOLENOID_MASK & ~allowed );
}
