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

// sparkle overlay: each twinkle briefly flips a lamp's output (off lamps
// flash on, lit lamps flash off) for SPARKLE_ON_TIME, on top of whatever the
// game set. it only touches the outputs, never lampState, so game logic that
// reads and sets lamp modes is unaffected.
#define SPARKLE_ON_TIME  FIVE_HUNDRETH_SECONDS
static boolean  sparkleActive = false;
static uint64_t sparkleMask = 0;      // lamps twinkling right now
static uint16_t sparkleStart[LAMP_QTY] = {0,};

static void writeCache(void) {
    uint64_t out = cache ^ sparkleMask;
    writeIO( IO_OUTPUT_CHIP_0, IO_GPIO, out );
    writeIO( IO_OUTPUT_CHIP_1, IO_GPIO, out >> 8 );
    writeIO( IO_OUTPUT_CHIP_2, IO_GPIO, out >> 16 );
    writeIO( IO_OUTPUT_CHIP_3, IO_GPIO, out >> 24 );
    writeIO( IO_OUTPUT_CHIP_4, IO_OLAT, out >> 32 );
}

static uint64_t lampMask( uint8_t ix ) {
  return 1ULL << lampBit[ix];
}

// allows twinkles until lampSparkleClear()
void lampSparkleBegin(void)
{
  sparkleActive = true;
}

// twinkle one lamp (flip its output) for SPARKLE_ON_TIME
void lampSparkle( lamp theLamp )
{
  if ( !sparkleActive || theLamp >= LAMP_QTY ) return;
  sparkleMask |= lampMask( theLamp );
  sparkleStart[theLamp] = getSysTime();
  writeCache();
}

// back to the lamps the game set
void lampSparkleClear(void)
{
  sparkleActive = false;
  sparkleMask = 0;
  writeCache();
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

// note: these checked sizeof(array), a byte count. lampState and the
// uint16_t arrays are 2 bytes per lamp, so lamps past LAMP_QTY got through.
bool isLampOn( lamp theLamp ) {
  if ( theLamp >= LAMP_QTY ) return false;
  return (lampState[theLamp] != LAMP_OFF_STATE);
}

lampStates getLampMode( lamp theLamp )
{
  if ( theLamp >= LAMP_QTY ) return 0;
  return lampState[theLamp];
}

void setLampMode( lamp theLamp, lampStates mode, uint16_t time, uint8_t cycles )
{
  if ( theLamp >= LAMP_QTY ) return;

    setCache(theLamp, (mode >= LAMP_ON_STATE));
    lampState[theLamp] = mode;
    lampStateTime[theLamp] = time;
    lampCycles[theLamp] = cycles;
    lampTime[theLamp] = getSysTime();
    writeCache();
}

// snapshot of every lamp, so a lamp show can take over the playfield during
// a game and put the game's lamps back afterwards
static lampStates savedState[LAMP_QTY];
static uint16_t   savedStateTime[LAMP_QTY];
static uint8_t    savedCycles[LAMP_QTY];
static uint64_t   savedCache;

void lampSaveAll(void)
{
  memcpy( savedState, lampState, sizeof(lampState) );
  memcpy( savedStateTime, lampStateTime, sizeof(lampStateTime) );
  memcpy( savedCycles, lampCycles, sizeof(lampCycles) );
  savedCache = cache;
}

// blinking lamps restart their current phase from now
void lampRestoreAll(void)
{
  memcpy( lampState, savedState, sizeof(lampState) );
  memcpy( lampStateTime, savedStateTime, sizeof(lampStateTime) );
  memcpy( lampCycles, savedCycles, sizeof(lampCycles) );
  cache = savedCache;
  uint16_t now = getSysTime();
  for ( uint8_t i = 0; i < LAMP_QTY; i++ ) {
    lampTime[i] = now;
  }
  writeCache();
}

void driveLamps(void)
{
  uint8_t i;
  
  for(i=0; i<LAMP_QTY; i++) {
    if ( sparkleMask ) {   // skip the 64-bit work when nothing twinkles
      uint64_t m = lampMask(i);
      if ( ( sparkleMask & m ) &&
           (uint16_t)(getSysTime() - sparkleStart[i]) > SPARKLE_ON_TIME ) {
        sparkleMask &= ~m;   // twinkle over
      }
    }
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
