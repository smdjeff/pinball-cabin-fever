#include "tweenpin.h"

const uint8_t lampBit[] = {
  35, // LAMP_TARGET_1_B
  19, // LAMP_TARGET_1_E
  26, // LAMP_TARGET_1_A
  14, // LAMP_TARGET_1_R
  18, // LAMP_TARGET_2_L
  23, // LAMP_TARGET_2_O
  11, // LAMP_TARGET_2_G
  12, // LAMP_TARGET_2_J
  22, // LAMP_TARGET_2_A
  25, // LAMP_TARGET_2_M
  32, // LAMP_TARGET_LOWER
  34, // LAMP_TARGET_UPPER
  13, // LAMP_BONUS_1
  24, // LAMP_BONUS_2
  10, // LAMP_BONUS_3
  17, // LAMP_BONUS_4
  36, // LAMP_BONUS_5
   6, // LAMP_BONUS_6
  31, // LAMP_BEAR_ARROW
  29, // LAMP_BEAR_MOUTH
  21, // LAMP_LANE_LEFT
   7, // LAMP_LANE_RIGHT
   8, // LAMP_POP_BUMPER_UPPER
  27, // LAMP_POP_BUMPER_LOWER
  28, // LAMP_RIVER_ARROW_1
  33, // LAMP_RIVER_ARROW_2
  30, // LAMP_RIVER_ARROW_3
   9, // LAMP_RIVER_ARROW_4
  15, // LAMP_RIVER_ARROW_5
  20, // LAMP_RIVER_ARROW_6
  38, // LAMP_BACKBOX_TILT
  39, // LAMP_BACKBOX_GAME_OVER
   4, // LAMP_BACKBOX_RIVER
   2, // LAMP_BACKBOX_CABIN
   3, // LAMP_BACKBOX_BEAR_1
   1, // LAMP_BACKBOX_BEAR_2
   0, // LAMP_BACKBOX_BEAR_3
   5, // LAMP_PLAYFIELD_GI
  37, // LAMP_START_BUTTON
  16, // LAMP_UNUSED_16
};

static lampStates lampState[LAMP_QTY] = {0,};
static uint16_t lampTime[LAMP_QTY] = {0,};
static uint16_t lampStateTime[LAMP_QTY] = {0,};
static uint8_t lampCycles[LAMP_QTY] = {0,};


static uint64_t cache = 0;

static void writeCache(void) {
    writeIO( IO_OUTPUT_CHIP_0, IO_GPIO, cache );
    writeIO( IO_OUTPUT_CHIP_1, IO_GPIO, cache >> 8 );
    writeIO( IO_OUTPUT_CHIP_2, IO_GPIO, cache >> 16 );
    writeIO( IO_OUTPUT_CHIP_3, IO_GPIO, cache >> 24 );
    writeIO( IO_OUTPUT_CHIP_4, IO_OLAT, cache >> 32 );
}

static void setCache(lamp ix, boolean on)
{
  if ( ix >= sizeof(lampBit) ) return;
  if ( on ) {
      cache |= (1ULL << (uint64_t)lampBit[ix]);
  } else {
      cache &= ~(1ULL << (uint64_t)lampBit[ix]);
  }
}

bool isLampOn( lamp theLamp ) {
  if ( theLamp >= sizeof(lampState) ) return false;
  return (lampState[theLamp] != LAMP_OFF_STATE);
}

lampStates getLampMode( lamp theLamp )
{
  if ( theLamp >= sizeof(lampState) ) return 0;
  return lampState[theLamp];
}

void setLampMode( lamp theLamp, lampStates mode, uint16_t time, uint8_t cycles )
{
  if ( theLamp >= sizeof(lampState) ) return;
  if ( theLamp >= sizeof(lampStateTime) ) return;
  if ( theLamp >= sizeof(lampCycles) ) return;
  if ( theLamp >= sizeof(lampTime) ) return;

    setCache(theLamp, (mode >= LAMP_ON_STATE));
    lampState[theLamp] = mode;
    lampStateTime[theLamp] = time;
    lampCycles[theLamp] = cycles;
    lampTime[theLamp] = getSysTime();
    writeCache();
}

void driveLamps(void)
{
  uint8_t i;
  
  for(i=0; i<LAMP_QTY; i++) {
    switch(lampState[i]) {
      case LAMP_BLINK_STATE:
      case LAMP_BLINK_ON_STATE:
        if( (getSysTime() - lampTime[i]) > lampStateTime[i]) {
          lampState[i] = LAMP_BLINK_OFF_STATE;
          lampTime[i] = getSysTime();
          setCache(i,FALSE);
        }
        break;
      case LAMP_BLINK_OFF_STATE:
        if( (getSysTime() - lampTime[i]) > lampStateTime[i]) {
          if( lampCycles[i] == INFINITE || --lampCycles[i] ) {
            lampState[i] = LAMP_BLINK_STATE;
            lampTime[i] = getSysTime();
            setCache(i,TRUE);
          } else {
            lampState[i] = LAMP_OFF_STATE;
            setCache(i,FALSE);
          }
        }
        break;
      case LAMP_FLASH_STATE:
        if( (getSysTime() - lampTime[i]) > lampStateTime[i]) {
          lampState[i] = LAMP_FLASH_INVERT_STATE;
          lampTime[i] = getSysTime();
          setCache(i,TRUE);
        }
        break;
      case LAMP_FLASH_INVERT_STATE:
        if( (getSysTime() - lampTime[i]) > lampStateTime[i]) {
          if( lampCycles[i] == INFINITE || --lampCycles[i] ) {
            lampState[i] = LAMP_FLASH_STATE;
            lampTime[i] = getSysTime();
            setCache(i,FALSE);
          } else {
            lampState[i] = LAMP_ON_STATE;
            setCache(i,TRUE);
          }
        }
        break;
      case LAMP_ON_STATE:
      case LAMP_OFF_STATE:
      default:
        // steady state - do nothing
        break;          
    }
  } 
  
  writeCache();
}
