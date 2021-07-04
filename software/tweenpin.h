#ifndef __TWEENPIN_H__
#define __TWEENPIN_H__

#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

typedef uint8_t boolean;
#define TRUE    1
#define FALSE   0

#define BIT(x) (1<<(x))

#define output(ddr,mask)        \
  do {                          \
    ddr |= (mask);              \
  } while (0)
#define input(ddr,mask)         \
  do {                          \
    ddr &= ~(mask);             \
  } while (0)
#define high(port,mask)         \
  do {                          \
    port |= (mask);             \
  } while (0)
#define low(port,mask)          \
  do {                          \
    port &= ~(mask);            \
  } while (0)
#define read(pin,mask)          \
  pin & (mask)

#define ENABLE_INTS()  do{ sei(); } while(0)
#define DECLARE_INT_STATE uint8_t _intState
#define DISABLE_INTS() do{ _intState = (SREG & BIT(SREG_I)); cli(); } while(0)
#define RESTORE_INTS() do{ SREG |= _intState; } while(0)
#define ATOMIC(blah) { DECLARE_INT_STATE; DISABLE_INTS(); blah RESTORE_INTS(); }

//PDA0..7 = highseed solenoids 0-7
//PC6..7 = highspeed solenoids 8-9
//PB0 = cs-sound
//PB1 = cs-display
//PC0 = io-cs[0:3]
//PC1 = io-cs[4:7]
//PD4..7 = highspeed switches


#define SPI_PORT            PORTB
#define SPI_DDR             DDRB
#define SPI_PIN             PINB
#define SPI_MOSI            BIT(5)
#define SPI_MISO            BIT(6)
#define SPI_SCK             BIT(7)

#define FASTSWITCH_PORT     PORTD
#define FASTSWITCH_DDR      DDRD
#define FASTSWITCH_PIN      PIND
#define FASTSWITCH_MASK     (BIT(4)|BIT(5)|BIT(6)|BIT(7))
#define FASTSWITCH_QTY      4

#define BEAR_PORT     PORTB
#define BEAR_DDR      DDRB
#define BEAR_PIN      PINB
#define BEAR_MASK     (BIT(4))

#define SOLENOID_PORT       PORTA
#define SOLENOID_DDR        DDRA
#define SOLENOID_MASK       0xFF
#define SOLENOID_QTY        8
#define SOLENOID_PORT_H     PORTC
#define SOLENOID_DDR_H      DDRC
#define SOLENOID_MASK_H     (BIT(6)|BIT(7))
#define SOLENOID_QTY_H      2

#define BUS_CONTROL_PORT    PORTB
#define BUS_CONTROL_DDR     DDRB
#define BUS_SOUND_CS        BIT(0)
#define BUS_DISPLAY_LOAD    BIT(1)

#define IO_CONTROL_PORT     PORTB
#define IO_CONTROL_DDR      DDRB
#define IO_CONTROL_RESET    BIT(3)
#define IO_CS_PORT          PORTC
#define IO_CS_DDR           DDRC
#define IO_CS_0123          BIT(0)
#define IO_CS_4567          BIT(1)

#define IO_BITS               8
#define IO_INPUT_CHIP_0       0
#define IO_INPUT_CHIP_COUNT   3
#define IO_INPUT_CHIP_MAX     IO_INPUT_CHIP_0 + IO_INPUT_CHIP_COUNT
#define IO_OUTPUT_CHIP_0      3
#define IO_OUTPUT_CHIP_COUNT  5
#define IO_OUTPUT_CHIP_MAX    IO_OUTPUT_CHIP_0 + IO_OUTPUT_CHIP_COUNT
#define IO_CHIP_COUNT         (IO_INPUT_CHIP_COUNT + IO_OUTPUT_CHIP_COUNT)


// io-expander
#define IO_DIR             0x0
#define IO_IPOL            0x1
#define IO_GPINTEN         0x2
#define IO_DEFVAL          0x3
#define IO_INTCON          0x4
#define IO_IOCON           0x5
#define   IOCON_SEQOP       BIT(5)
#define   IOCON_DISSLW      BIT(4)
#define   IOCON_HAEN        BIT(3)
#define   IOCON_ORD         BIT(2)
#define   IOCON_INTPOL      BIT(1)
#define IO_GPPU            0x6
#define IO_INTF            0x7
#define IO_INTCAP          0x8
#define IO_GPIO            0x9
#define IO_OLAT            0xA

// display
#define DISPLAY_NOOP      0x0
#define DISPLAY_DIG0      0x1
#define DISPLAY_DIG1      0x2
#define DISPLAY_DIG2      0x3
#define DISPLAY_DIG3      0x4
#define DISPLAY_DIG4      0x5
#define DISPLAY_DIG5      0x6
#define DISPLAY_DIG6      0x7
#define DISPLAY_DIG7      0x8
#define DISPLAY_MODE      0x9
#define DISPLAY_INTENSITY 0xA
#define DISPLAY_SCAN      0xB
#define DISPLAY_SHUTDOWN  0xC
#define DISPLAY_TEST      0xF

#define DISPLAY_FONT_DASH   10
#define DISPLAY_FONT_E      11
#define DISPLAY_FONT_H      12
#define DISPLAY_FONT_L      13
#define DISPLAY_FONT_P      14
#define DISPLAY_FONT_BLANK  15



typedef enum
{
  ONE_HUNDRETH_SECOND = 2,
  TWO_HUNDRETH_SECONDS = 4,
  THREE_HUNDRETH_SECONDS = 6,
  FOUR_HUNDRETH_SECONDS = 8,
  FIVE_HUNDRETH_SECONDS = 10,
  TEN_HUNDRETH_SECONDS = 20,
  EIGTH_SECOND = 25,
  QUARTER_SECOND = 50,
  HALF_SECOND = 100,
  ONE_SECOND = 200,
  TWO_SECONDS = 400,
  THREE_SECONDS = 600,
  FOUR_SECONDS = 800,
  FIVE_SECONDS = 1000
} systimeDurations;

typedef enum {
  ATTRACT_LAMP_TMR,
  ATTRACT_TMR,
  MATCH_TMR,
  BEAR_TMR,
  GAME_TMR,
  CRAZY_TMR,
  SPIN_TMR,
  BOX_TMR,
  SOLENOID_TMR,
  TIMER_QTY,
} timerEvent;

void attractLamps(timerEvent evt, uint16_t data);
void attractTimer(timerEvent evt, uint16_t data);
void matchTimer(timerEvent evt, uint16_t data);
void bearTimer(timerEvent evt, uint16_t data);
void gameTimer(timerEvent evt, uint16_t state);
void crazyModeTimer(timerEvent evt, uint16_t state);
void spinTimer(timerEvent evt, uint16_t state);
void boxTimer(timerEvent evt, uint16_t state);
void solenoidTimer(timerEvent evt, uint16_t state);


//////////////////////////////////
// Slow Switches
//////////////////////////////////


typedef enum 
{
  // keep targets in same order as lamps
  SWITCH_TARGET_1_B = 0,
  SWITCH_TARGET_1_E = 1,
  SWITCH_TARGET_1_A = 2,
  SWITCH_TARGET_1_R = 3,
  SWITCH_TARGET_2_L = 4,
  SWITCH_TARGET_2_O = 5,
  SWITCH_TARGET_2_G = 6,
  SWITCH_TARGET_2_J = 7,
  SWITCH_TARGET_2_A = 8,
  SWITCH_TARGET_2_M = 9,
  SWITCH_TARGET_LOWER = 10,
  SWITCH_TARGET_UPPER = 11,
  SWITCH_BEAR_CAPTURE = 12,
  SWITCH_BALL_DRAIN = 13,
  SWITCH_GAME_START = 14,
  SWITCH_COIN = 15,
  SWITCH_TILT_BOB = 16,
  SWITCH_SPINNER = 17,
  SWITCH_LANE_LEFT = 18,
  SWITCH_LANE_RIGHT = 19,
  SWITCH_TEST_ENTER = 20,
  SWITCH_TEST_PLUS = 21,
  SWITCH_TEST_MINUS = 22,
  SWITCH_TEST_ESCAPE = 23,
  SWITCH_QTY = 24
} slowSwitch;

void driveSlowSwitchISR(void);
void driveSlowSwitches(void);


//////////////////////////////////
// Solenoids
//////////////////////////////////

typedef enum
{
  SOLENOID_POP_BUMPER_UPPER = 0,
  SOLENOID_POP_BUMPER_LOWER = 1,
  SOLENOID_FLIPPER_RIGHT = 2,
  SOLENOID_FLIPPER_LEFT = 3,
  SOLENOID_BEAR_CAPTURE = 4,
  SOLENOID_BALL_LOADER = 5,
  SOLENOID_KNOCKER = 6,
  SOLENOID_BELL = 7,
  SOLENOID_UNUSED_1 = 8,
  SOLENOID_UNUSED_2 = 9
} solenoid;

typedef enum {
  SOLENOID_IDLE_STATE = 0,
  SOLENOID_HOLD_PWMOFF_STATE,
  SOLENOID_ONESHOT_COMPLETE_STATE,
  SOLENOID_FLASH_PAUSE_STATE,
  // all states prior to active have solenoid driven off
  SOLENOID_ACTIVE,
  SOLENOID_HOLD_DRIVE_STATE,
  SOLENOID_HOLD_PWMON_STATE,
  SOLENOID_ONESHOT_DRIVE_STATE,
  SOLENOID_FLASH_STATE
} solenoidStates;

solenoidStates getSolenoidMode( solenoid sol );
void setSolenoidMode( solenoid sol, solenoidStates mode, uint16_t time, uint8_t cycles );
void driveSolenoidsISR(void);


//////////////////////////////////
// Fast Switches
//////////////////////////////////

typedef enum
{
  FASTSWITCH_POP_BUMPER_UPPER = 0,
  FASTSWITCH_POP_BUMPER_LOWER = 1,
  FASTSWITCH_FLIPPER_LEFT = 2,
  FASTSWITCH_FLIPPER_RIGHT = 3
} fastSwitch;

void driveFastSwitchISR(void);


//////////////////////////////////
// Lamps
//////////////////////////////////

typedef enum
{
  // keep targets in same order as switches
  LAMP_TARGET_1_B = 0,
  LAMP_TARGET_1_E = 1,
  LAMP_TARGET_1_A = 2,
  LAMP_TARGET_1_R = 3,
  LAMP_TARGET_2_L = 4,
  LAMP_TARGET_2_O = 5,
  LAMP_TARGET_2_G = 6,
  LAMP_TARGET_2_J = 7,
  LAMP_TARGET_2_A = 8,
  LAMP_TARGET_2_M = 9,
  LAMP_TARGET_LOWER = 10,
  LAMP_TARGET_UPPER = 11,
  LAMP_BONUS_1 = 12,
  LAMP_BONUS_2 = 13,
  LAMP_BONUS_3 = 14,
  LAMP_BONUS_4 = 15,
  LAMP_BONUS_5 = 16,
  LAMP_BONUS_6 = 17,
  LAMP_BEAR_ARROW = 18,
  LAMP_BEAR_MOUTH = 19,
  LAMP_LANE_LEFT1 = 20,
  LAMP_LANE_LEFT2 = 21,
  LAMP_LANE_RIGHT1 = 22,
  LAMP_LANE_RIGHT2 = 23,
  LAMP_POP_BUMPER_UPPER = 24,
  LAMP_POP_BUMPER_LOWER = 25,
  LAMP_RIVER_ARROW_1 = 26,
  LAMP_RIVER_ARROW_2 = 27,
  LAMP_RIVER_ARROW_3 = 28,
  LAMP_RIVER_ARROW_4 = 29,
  LAMP_RIVER_ARROW_5 = 30,
  LAMP_RIVER_ARROW_6 = 31,
  LAMP_LAST_PLAYFIELD = 31,
  LAMP_BACKBOX_TILT = 32,
  LAMP_BACKBOX_GAME_OVER = 33,
  LAMP_BACKBOX_RIVER = 34,
  LAMP_BACKBOX_CABIN = 35,
  LAMP_BACKBOX_BEAR_1 = 36,
  LAMP_BACKBOX_BEAR_2 = 37,
  LAMP_BACKBOX_BEAR_3 = 38,
  LAMP_PLAYFIELD_GI = 39,  
  LAMP_LAST = 39,
  LAMP_QTY = 40
} lamp;

typedef enum {
  LAMP_OFF_STATE = 0,
  LAMP_BLINK_OFF_STATE,
  LAMP_FLASH_INVERT_STATE, 
  LAMP_FADE_UP_STATE,
  LAMP_FADE_DOWN_STATE,
  LAMP_FLASH_STATE, // on->off->on
  LAMP_BLINK_STATE, // off->on->off
  LAMP_ON_STATE,
  LAMP_BLINK_ON_STATE, // on->off
} lampStates;

// Use infinite for time and/or cycles
#define INFINITE 0

lampStates getLampMode( lamp theLamp );
void setLampMode( lamp theLamp, lampStates mode, uint16_t time, uint8_t cycles );
void driveLamps(void);
void driveLampISR(void);

//////////////////////////////////
// display
//////////////////////////////////

//  -    a
// | |  f b 
//  -    g
// | |  e c
//  - .  d  
//
// bits
// 7 6 5 4 3 2 1 0
// p a b c d e f g

typedef enum {
  DISP_BLANK = 0x00,
  DISP_ZERO = 0x7E,
  DISP_ONE = 0x30,
  DISP_TWO = 0x6d,
  DISP_THREE = 0x79,
  DISP_FOUR = 0x33,
  DISP_FIVE = 0x5b,
  DISP_SIX = 0x5f,
  DISP_SEVEN = 0x70,
  DISP_EIGHT = 0x7f,
  DISP_NINE = 0x7b,
  DISP_A = 0x77,
  DISP_B = 0x1f,
  DISP_C = 0x4e,
  DISP_D = 0x3d,
  DISP_E = 0x4f,
  DISP_F = 0x47,
  DISP_G = 0x5e,
  DISP_H = 0x37,
  DISP_I = 0x30,
  DISP_J = 0x3c,
  DISP_K = 0x37,  //better representation possible??
  DISP_L = 0x0e,
  DISP_M = 0x15, //better representation possible??
  DISP_M2 = 0x11,
  DISP_N = 0x15,
  DISP_O = 0x7e,
  DISP_P = 0x67,
  DISP_Q = 0xfe, //better representation possible??
  DISP_R = 0x05,
  DISP_S = 0x5b,
  DISP_T = 0x70, //better representation possible??
  DISP_U = 0x1c,  // short
  DISP_U2 = 0x3e, // tall
  DISP_V = 0x3e, //better representation possible??
  DISP_W = 0x1c, //better representation possible??
  DISP_W2 = 0x18,
  DISP_X = 0x37,  //better representation possible??
  DISP_Y = 0x3b,
  DISP_Z = 0x6d,  //better representation possible??
  DISP_DASH = 0x01,
  DISP_BARS = 0x49
} displayChars;

void displayBCD(uint8_t *bcd, uint8_t size);
void displayBinary(uint8_t value);
void writeDisplay( uint8_t reg, uint8_t data );
void writeDisplayNum( uint8_t reg, uint8_t data );
void bootDisplay(void);
void resetDisplay(void);
void displayLong(uint32_t value,uint8_t digits);
void displayText(const uint8_t* str);
void displayString(uint8_t dig0, uint8_t dig1, uint8_t dig2, uint8_t dig3, 
                   uint8_t dig4, uint8_t dig5, uint8_t dig6, uint8_t dig7);
void displayClear(void);


//////////////////////////////////
// sounds
//////////////////////////////////

typedef enum {
  SOUND_STOP = 0x00,
  SOUND_SNAKE = 0x01,       // rattler
  SOUND_SPINNER = 0x02,     // water park screams
  SOUND_BEAR_TARGETS = 0x03,  // large animal grunt
  SOUND_BEAR_OPEN = 0x04,   // grizzly growl
  SOUND_BEAR_CHEW = 0x05,   // large animal chew
  SOUND_LOGS = 0x06,        // beavers (or music)
  SOUND_BONUS = 0x07,       // buzz saw
  SOUND_LIZARD = 0x08,      // lizard?
  SOUND_DRAIN = 0x09,       // drop into water
  SOUND_GAME_START = 0x0A,  // ?
  SOUND_GAME_END = 0x0B,    // ?
  SOUND_MATCHING = 0x0C,    // clicks
  SOUND_BACKGROUND_STOP = 0x80,  
  SOUND_BACKGROUND = 0x81,  // babbling brook
} sounds_t;

void playSound( sounds_t sound );

//////////////////////////////////
// IO
//////////////////////////////////

void initIO(void);
void writeIO( uint8_t chip, uint8_t reg, uint8_t data );
uint8_t readIO( uint8_t chip, uint8_t reg );

uint8_t spiRead(void);
void spiWrite(uint8_t byte);



//////////////////////////////////
// other utilities
//////////////////////////////////

void __delay_us(uint32_t us);
void __delay_ms(uint32_t ms);

uint16_t getSysTime(void);
void setTimer( timerEvent timer, uint16_t time, uint16_t data );
void cancelTimer(timerEvent timer);
void setIfActiveTimer( timerEvent timer, uint16_t data );
boolean isActiveTimer( timerEvent timer );
void driveTimers(void);

void bearPWM(uint16_t width);

//////////////////////////////////
// game play
//////////////////////////////////

typedef uint8_t score_t[8];

typedef struct {
  score_t highScore;
  uint16_t headOpen;
  uint16_t headClose;
  uint8_t quiteMode;
  uint8_t tiltSensitivity;
  uint8_t ballsPerGame;
  uint8_t soundBoard;
  uint8_t coinsPerGame;
  uint16_t gamesPlayed;
  uint16_t freeGameMatch;
  uint16_t freeGameHighScore;
  uint8_t checkSum;
} nonVolatiles_t;

extern const uint8_t tiltSenseMax;
extern uint8_t ballInPlay;
extern boolean tilt;
extern uint8_t tiltSense;
extern boolean gameOn;
extern boolean ballIsLoading;
extern uint8_t multiplier;
extern boolean crazyMode;
extern uint16_t bearTestPWM;
extern nonVolatiles_t nonVolatiles;

typedef enum {
  BEAR_OPEN,
  BEAR_CHEW_CLOSE,
  BEAR_CHEW_OPEN,
  BEAR_EJECT,
  BEAR_EJECT_1,
  BEAR_EJECT_2,
  BEAR_CLOSE,
  BEAR_TEST
} bearStates;

typedef enum {
  GAME_BALL_LOAD,
  GAME_BALL_AGAIN,
  GAME_BALL_AGAIN_1,
  GAME_SCORE,
  GAME_FREE_CREDIT,
  GAME_TILT
} gameStates;

typedef enum {
  ATTRACT_PLAY,
  ATTRACT_PINBALL,
  ATTRACT_SCORE,
  ATTRACT_NEW_HIGH_SCORE,
  ATTRACT_HIGH_SCORE,
  ATTRACT_HIGH_SCORE_1,
  ATTRACT_GAME_OVER,
  ATTRACT_CREDITS,
  ATTRACT_CREDITS_1,
  ATTRACT_INSERT_COINS,
  ATTRACT_INSERT_COINS_1,
  ATTRACT_INSERT_COINS_2,
  ATTRACT_BLANK,
  ATTRACT_FREE
} attractStates;

void resetGame(void);
void startGame(void);
void nextBall(void);
void gameOver(void);


extern const uint8_t SCORE_TEN[8];
extern const uint8_t SCORE_ONE_HUNDRED[8];
extern const uint8_t SCORE_FIVE_HUNDRED[8];
extern const uint8_t SCORE_ONE_THOUSAND[8];
extern const uint8_t SCORE_FIVE_THOUSAND[8];

void displayCredits(void);
void increaseCredits(uint8_t i, boolean coinMode);
boolean decreaseCredits(void);
void resetCredits(void);

void displayScore(void);
void increaseScore(const uint8_t *bcdVal);
uint32_t getScore(void);
void resetScore(void);

void loadNonVolatiles(void);
void saveNonVolatiles(void);
void resetNonVolatiles(void);

void setMultiplier(uint8_t mult);

void configMode(slowSwitch sw);
void cancelBoxTimer(void);
void attractTimerButton( fastSwitch theSwitch );
void matchTimerButton( fastSwitch theSwitch );


#endif //__TWEENPIN_H__
