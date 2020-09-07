#include "tweenpin.h"

typedef enum  
{
  LAMP_TARGET_1_B_IO = 35,
  LAMP_TARGET_1_E_IO = 19,
  LAMP_TARGET_1_A_IO = 26,
  LAMP_TARGET_1_R_IO = 14,
  LAMP_TARGET_2_L_IO = 9,
  LAMP_TARGET_2_O_IO = 23,
  LAMP_TARGET_2_G_IO = 11,
  LAMP_TARGET_2_J_IO = 12,
  LAMP_TARGET_2_A_IO = 22,
  LAMP_TARGET_2_M_IO = 25,
  LAMP_TARGET_LOWER_IO = 32,
  LAMP_TARGET_UPPER_IO = 34,
  LAMP_BONUS_1_IO = 13,
  LAMP_BONUS_2_IO = 24,
  LAMP_BONUS_3_IO = 10,
  LAMP_BONUS_4_IO = 17,
  LAMP_BONUS_5_IO = 37,
  LAMP_BONUS_6_IO = 6,
  LAMP_BEAR_ARROW_IO = 31,
  LAMP_BEAR_MOUTH_IO = 29,
  LAMP_LANE_LEFT1_IO = 21,
	LAMP_LANE_LEFT2_IO = 36,
  LAMP_LANE_RIGHT1_IO = 7,
	LAMP_LANE_RIGHT2_IO = 16,
  LAMP_POP_BUMPER_UPPER_IO = 8,
  LAMP_POP_BUMPER_LOWER_IO = 27,
  LAMP_RIVER_ARROW_1_IO = 28,
  LAMP_RIVER_ARROW_2_IO = 33,
  LAMP_RIVER_ARROW_3_IO = 30,
  LAMP_RIVER_ARROW_4_IO = 18,
  LAMP_RIVER_ARROW_5_IO = 15,
  LAMP_RIVER_ARROW_6_IO = 20,
  LAMP_BACKBOX_TILT_IO = 39,
  LAMP_BACKBOX_GAME_OVER_IO = 38,
  LAMP_BACKBOX_MATCH_IO = 0,
  LAMP_UNUSED_1_IO = 1,
  LAMP_UNUSED_2_IO = 2,
  LAMP_UNUSED_3_IO = 3,
  LAMP_UNUSED_4_IO = 4,
} lampIOs;

// must match lamp enum order
static const uint8_t lampIO[LAMP_QTY] = { 
  LAMP_TARGET_1_B_IO,
  LAMP_TARGET_1_E_IO,
  LAMP_TARGET_1_A_IO,
  LAMP_TARGET_1_R_IO,
  LAMP_TARGET_2_L_IO,
  LAMP_TARGET_2_O_IO,
  LAMP_TARGET_2_G_IO,
  LAMP_TARGET_2_J_IO,
  LAMP_TARGET_2_A_IO,
  LAMP_TARGET_2_M_IO,
  LAMP_TARGET_LOWER_IO,
  LAMP_TARGET_UPPER_IO,
  LAMP_BONUS_1_IO,
  LAMP_BONUS_2_IO,
  LAMP_BONUS_3_IO,
  LAMP_BONUS_4_IO,
  LAMP_BONUS_5_IO,
  LAMP_BONUS_6_IO,
  LAMP_BEAR_ARROW_IO,
  LAMP_BEAR_MOUTH_IO,
  LAMP_LANE_LEFT1_IO,
	LAMP_LANE_LEFT2_IO,
  LAMP_LANE_RIGHT1_IO,
	LAMP_LANE_RIGHT2_IO,
  LAMP_POP_BUMPER_UPPER_IO,
  LAMP_POP_BUMPER_LOWER_IO,
  LAMP_RIVER_ARROW_1_IO,
  LAMP_RIVER_ARROW_2_IO,
  LAMP_RIVER_ARROW_3_IO,
  LAMP_RIVER_ARROW_4_IO,
  LAMP_RIVER_ARROW_5_IO,
  LAMP_RIVER_ARROW_6_IO,
  LAMP_BACKBOX_TILT_IO,
  LAMP_BACKBOX_GAME_OVER_IO,
};



static uint8_t lampShadow[IO_CHIP_COUNT];
static lampStates lampState[LAMP_QTY];
static uint16_t lampTime[LAMP_QTY];
static uint16_t lampStateTime[LAMP_QTY];
static uint8_t lampCycles[LAMP_QTY];



static void setLamp(lamp theLamp, boolean on)
{
  uint8_t lampChip = ((IO_OUTPUT_CHIP_0 * IO_BITS) + lampIO[theLamp]) / IO_BITS;
  uint8_t lampBit = ((IO_OUTPUT_CHIP_0 * IO_BITS) + lampIO[theLamp]) % IO_BITS;
  uint8_t data;

  // lamp active flags are shadowed so that lamps can be set/cleared without
  //  disturbing other lamps on the same io expander, and avoiding the 
  //  overhead of reading out the state over the spi
  data = lampShadow[ lampChip ];
	if(on) {
	  data |= BIT(lampBit);
  } else {
	  data &= ~BIT(lampBit);
	}

  writeIO( lampChip, IO_OLAT, data );
  lampShadow[lampChip] = data;
}

lampStates getLampMode( lamp theLamp )
{
  return lampState[theLamp];
}


// Modes supported:
//  Off - lamp inactive
//  On - lamp permanently on
//  Flash - for flashing a lamp multiple times in succession
//  Blink - lamp continuously cycling on and off
void setLampMode( lamp theLamp, lampStates mode, uint16_t time, uint8_t cycles )
{
  ATOMIC(
    setLamp(theLamp, (mode >= LAMP_ON_STATE));
    lampState[theLamp] = mode;
    lampTime[theLamp] = getSysTime();
    lampStateTime[theLamp] = time;
    lampCycles[theLamp] = cycles;
  )
}

void driveLamps(void)
{
	uint8_t i;

	for(i=0; i<LAMP_QTY; i++) {
		switch(lampState[i]) {
		  case LAMP_BLINK_ON_STATE:
				if( (getSysTime() - lampTime[i]) > lampStateTime[i]) {
					lampState[i] = LAMP_BLINK_OFF_STATE;
					lampTime[i] = getSysTime();
					setLamp(i,FALSE);
				}
				break;
			case LAMP_BLINK_OFF_STATE:
				if( (getSysTime() - lampTime[i]) > lampStateTime[i]) {
          if( lampCycles[i] == INFINITE || --lampCycles[i] ) {
            lampState[i] = LAMP_BLINK_ON_STATE;
            lampTime[i] = getSysTime();
            setLamp(i,TRUE);
          } else {
            lampState[i] = LAMP_OFF_STATE;
  					setLamp(i,FALSE);
          }
				}
				break;
			case LAMP_FLASH_STATE:
				if( (getSysTime() - lampTime[i]) > lampStateTime[i]) {
					if (--lampCycles[i]) {
            lampState[i] = LAMP_FLASH_PAUSE_STATE;
            lampTime[i] = getSysTime();
          } else {
            lampState[i] = LAMP_OFF_STATE;
          }
					setLamp(i,FALSE);
				}
				break;
      case LAMP_FLASH_PAUSE_STATE:
        if( getSysTime() - lampTime[i] > lampStateTime[i] ) {
            lampState[i] = LAMP_FLASH_STATE;
  				  lampTime[i] = getSysTime();          
            setLamp( i, TRUE);                
        }
		  case LAMP_ON_STATE:
        if(lampStateTime[i] != INFINITE) {
          if( (getSysTime() - lampTime[i]) > lampStateTime[i]) {
            lampState[i] = LAMP_OFF_STATE;
            setLamp(i,FALSE);
          }
        }
		  case LAMP_OFF_STATE:
			default:
			  // steady state - do nothing
				break;					
		}
  }	
}


