#include "tweenpin.h"


static uint16_t offTime[SOLENOID_QTY] = {0,};
static uint16_t onTime[SOLENOID_QTY] = {0,};

void initSolenoids(void) {
    low( SOLENOID_PORT, SOLENOID_MASK );
    output( SOLENOID_DDR, SOLENOID_MASK );
}

static void setSolenoid( solenoid sol, boolean on )
{
  uint8_t bit = 0;
  switch ( sol ) {
    case SOLENOID_WOODBLOCK:
      if ( nonVolatiles.quiteMode == 2 ) return;
      bit = BIT(0); 
      break;
    case SOLENOID_KNOCKER:
      if ( nonVolatiles.quiteMode == 2 ) return;
      bit = BIT(1); 
      break;
    case SOLENOID_BALL_LOADER:
      bit = BIT(2); 
      break;
    case SOLENOID_POP_BUMPER_LOWER: 
      bit = BIT(3);
      break;
    case SOLENOID_POP_BUMPER_UPPER:
      bit = BIT(4);
      break;
    case SOLENOID_BEAR_CAPTURE:
      bit = BIT(5); 
      break;
    default:
      return;
  }
  if(on) {
      high(SOLENOID_PORT, bit );
   } else {
      low(SOLENOID_PORT, bit );
  }   
}

bool isSolenoidOn( solenoid sol )
{
  if ( sol >= sizeof(offTime)/sizeof(offTime[0]) ) return false;
  return ( offTime[sol] );
}

void hitSolenoid( solenoid sol ) 
{
  uint16_t time = getSysTime();
  if ( sol >= sizeof(offTime)/sizeof(offTime[0]) ) return;
  if ( time - onTime[sol] < QUARTER_SECOND ) return;
  setSolenoid( sol, true );
  onTime[sol] = time;
  offTime[sol] = time + TEN_HUNDRETH_SECONDS;
}

void driveSolenoids(void)
{
  uint16_t time = getSysTime();
  for(uint8_t i=0; i<sizeof(offTime)/sizeof(offTime[0]); i++) {
    if ( offTime[i] ) {
      if ( offTime[i] < time ) {
        offTime[i] = 0;
        setSolenoid( i, false );
      }
    }
  }
}
