#ifndef __TWEENPIN_H__
#define __TWEENPIN_H__

#include <stdbool.h>
#include <stdio.h>
#include <ctype.h> 
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include "timers.h"
#include "uart.h"

typedef bool    boolean;
#define TRUE    true
#define FALSE   false

#define BIT(x)  _BV(x)

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif


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

#define ATOMIC(blah) { DECLARE_INT_STATE; DISABLE_INTS(); blah RESTORE_INTS(); }



#define SPI_PORT            PORTB
#define SPI_DDR             DDRB
#define SPI_PIN             PINB
#define SPI_MOSI            BIT(5)
#define SPI_MISO            BIT(6)
#define SPI_SCK             BIT(7)

#define FASTSWITCH_BUMPER_PORT      PORTD
#define FASTSWITCH_BUMPER_DDR       DDRD
#define FASTSWITCH_BUMPER_PIN       PIND
#define FASTSWITCH_BUMPER_MASK      (FASTSWITCH_POP_BUMPER_UPPER|FASTSWITCH_POP_BUMPER_LOWER)
#define FASTSWITCH_POP_BUMPER_UPPER (BIT(4))
#define FASTSWITCH_POP_BUMPER_LOWER (BIT(5)) 

#define FASTSWITCH_FLIPPER_PORT     PORTC
#define FASTSWITCH_FLIPPER_DDR      DDRC
#define FASTSWITCH_FLIPPER_PIN      PINC
#define FASTSWITCH_FLIPPER_MASK     (FASTSWITCH_FLIPPER_LEFT|FASTSWITCH_FLIPPER_RIGHT)
#define FASTSWITCH_FLIPPER_RIGHT    (BIT(6))
#define FASTSWITCH_FLIPPER_LEFT     (BIT(7))

#define SOLENOID_FLIPPER_PORT       PORTD
#define SOLENOID_FLIPPER_DDR        DDRD
#define SOLENOID_FLIPPER_MASK       (SOLENOID_FLIPPER_LEFT|SOLENOID_FLIPPER_RIGHT)
#define SOLENOID_FLIPPER_LEFT       BIT(6)
#define SOLENOID_FLIPPER_RIGHT      BIT(7)
#define SOLENOID_FLIPPER_LEFT_OCR   OCR2B
#define SOLENOID_FLIPPER_RIGHT_OCR  OCR2A

#define BEAR_PORT           PORTB
#define BEAR_DDR            DDRB
#define BEAR_PIN            PINB
#define BEAR_MASK           (BIT(4))
#define BEAR_OCR            OCR0B

#define SOLENOID_PORT       PORTA
#define SOLENOID_DDR        DDRA
#define SOLENOID_MASK       0xFF
#define SOLENOID_QTY        8

#define DISPLAY_PORT        PORTC
#define DISPLAY_DDR         DDRC
#define DISPLAY_SCL         BIT(0)
#define DISPLAY_SCA         BIT(1)

#define IO_CONTROL_PORT     PORTB
#define IO_CONTROL_DDR      DDRB
#define IO_CONTROL_RESET    BIT(3)
#define IO_CS_PORT          PORTC
#define IO_CS_DDR           DDRC
#define IO_CS_0123          BIT(2)
#define IO_CS_4567          BIT(3)

#define IO_INPUT_CHIP_0       0
#define IO_INPUT_CHIP_1       1
#define IO_INPUT_CHIP_2       2
#define IO_INPUT_CHIP_MAX     IO_INPUT_CHIP_2 + 1
#define IO_INPUT_CHIP_COUNT   IO_INPUT_CHIP_MAX + 1
#define IO_OUTPUT_CHIP_0      3
#define IO_OUTPUT_CHIP_1      4
#define IO_OUTPUT_CHIP_2      5
#define IO_OUTPUT_CHIP_3      6
#define IO_OUTPUT_CHIP_4      7
#define IO_OUTPUT_CHIP_MAX    IO_OUTPUT_CHIP_4 + 1


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
  BEAR_TMR,
  GAME_TMR,
  CRAZY_TMR,
  SPIN_TMR,
  BOX_TMR,
  MUSIC_TMR,
  WATCHDOG_TMR,
  TIMER_QTY,
} timerEvent;

void attractLamps(timerEvent id, uint16_t data);
void attractTimer(timerEvent id, uint16_t data);
void bearTimer(timerEvent id, uint16_t data);
void gameTimer(timerEvent id, uint16_t state);
void crazyModeTimer(timerEvent id, uint16_t state);
void spinTimer(timerEvent id, uint16_t state);
void boxTimer(timerEvent id, uint16_t state);
void musicTimer(timerEvent id, uint16_t state);
void watchdogTimer(timerEvent id, uint16_t data);
void systemTickISR(void);
void flipperTickISR(void);
void initTimers(void);


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

void driveSlowSwitches(void);
void initSlowSwitches(void);

//////////////////////////////////
// Solenoids
//////////////////////////////////

typedef enum
{
  SOLENOID_KNOCKER,
  SOLENOID_BEAR_CAPTURE,
  SOLENOID_BALL_LOADER,
  SOLENOID_POP_BUMPER_LOWER,
  SOLENOID_POP_BUMPER_UPPER,
  SOLENOID_WOODBLOCK,
  // SOLENOID_6,
  // SOLENOID_7
} solenoid;

typedef enum {
  SOLENOID_OFF = 0,
  SOLENOID_ON
} solenoidStates;

bool isSolenoidOn( solenoid sol );
void hitSolenoid( solenoid sol );
void driveSolenoids(void);
void initSolenoids(void);

//////////////////////////////////
// Fast Switches
//////////////////////////////////

typedef uint8_t fastSwitch;
void driveFastSwitches(void);
void initFastSwitches(void);


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
  LAMP_LANE_LEFT = 20,
  LAMP_LANE_RIGHT = 21,
  LAMP_POP_BUMPER_UPPER = 22,
  LAMP_POP_BUMPER_LOWER = 23,
  LAMP_RIVER_ARROW_1 = 24,
  LAMP_RIVER_ARROW_2 = 25,
  LAMP_RIVER_ARROW_3 = 26,
  LAMP_RIVER_ARROW_4 = 27,
  LAMP_RIVER_ARROW_5 = 28,
  LAMP_RIVER_ARROW_6 = 29,
  
  LAMP_LAST_PLAYFIELD = 29,
  
  LAMP_BACKBOX_TILT = 30,
  LAMP_BACKBOX_GAME_OVER = 31,
  LAMP_BACKBOX_RIVER = 32,
  LAMP_BACKBOX_CABIN = 33,
  LAMP_BACKBOX_BEAR_1 = 34,
  LAMP_BACKBOX_BEAR_2 = 35,
  LAMP_BACKBOX_BEAR_3 = 36,
  LAMP_PLAYFIELD_GI = 37,  
  LAMP_START_BUTTON = 38,
  
  LAMP_LAST = 38,
  
  LAMP_QTY = 39
} lamp;

typedef enum {
  LAMP_OFF_STATE = 0,
  LAMP_BLINK_OFF_STATE,
  LAMP_FLASH_INVERT_STATE, 
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

#define LED_BACKPACKS   5
#define LED_COLUMNS     8
#define LED_ROWS        8
void ht16k33Init(void);
void ht16k33Clear(void);
void ht16k33DisplayFrame(void);
void ht16k33DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void initDisplay(void);
void clearDisplay(void);
void displayText(const char *fmt, ...);
void displayView(uint8_t x, uint8_t y);

//////////////////////////////////
// sounds
//////////////////////////////////

typedef enum {
  SOUND_STOP,
  SOUND_SNAKE,
  SOUND_LIZZARD,
  SOUND_SPINNER,
  SOUND_BEAR_TARGETS,
  SOUND_BEAR_OPEN,
  SOUND_BEAR_CHEW,
  SOUND_BEAR_MUSIC,
  SOUND_LOGS,
  SOUND_BONUS,
  SOUND_DRAIN,
  SOUND_GAME_START,
  SOUND_GAME_END,
  SOUND_GAME_TILT,
  SOUND_COIN,
  SOUND_LANE,
  SOUND_DOOR_BUTTON,
  SOUND_MAIN_MUSIC
} sounds_t;

void initSound( void );
void playSound( sounds_t sound );
void playMusic( sounds_t sound );

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

#define __delay_us _delay_us
#define __delay_ms _delay_ms

uint16_t getSysTime(void);
void setTimer( timerEvent id, uint16_t time, uint16_t data );
void cancelTimer(timerEvent id);
void setIfActiveTimer( timerEvent id, uint16_t data );
boolean isActiveTimer( timerEvent id );
void driveTimers(void);

void flipperInitPulse(void);
void flipperEnableSolenoids(bool enable);
void flipperSetPulseLeft(uint8_t perc);
void flipperSetPulseRight(uint8_t perc);
uint8_t flipperGetPulseLeft(void);
uint8_t flipperGetPulseRight(void);

void bearInitPulse(void);
void bearSetPulse(uint8_t pulse);

//////////////////////////////////
// game play
//////////////////////////////////

typedef struct {
  uint32_t highScore;
  uint8_t headOpen;
  uint8_t headClose;
  uint8_t volume;
  uint8_t tiltSensitivity;
  uint8_t ballsPerGame;
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
extern nonVolatiles_t nonVolatiles;

typedef enum {
  BEAR_OPEN,
  BEAR_HOLD_OPEN,
  BEAR_CHEW,
  BEAR_CHEW_1,
  BEAR_EJECT,
  BEAR_EJECT_1,
  BEAR_CLOSE,
  BEAR_HOLD_CLOSE,
  BEAR_TEST
} bearStates;

typedef enum {
  GAME_BALL_WAIT,
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


#define SCORE_TEN             10
#define SCORE_ONE_HUNDRED    100
#define SCORE_FIVE_HUNDRED   500
#define SCORE_ONE_THOUSAND  1000
#define SCORE_FIVE_THOUSAND 5000

void increaseCredits(uint8_t i, boolean coinMode);
boolean decreaseCredits(void);
void resetCredits(void);

void displayScore(uint8_t color);
void increaseScore(uint16_t value);
uint32_t getScore(void);
void resetScore(void);

void loadNonVolatiles(void);
void saveNonVolatiles(void);
void resetNonVolatiles(void);

void setMultiplier(uint8_t mult);

void laneToggle(void);
void doorMenu(slowSwitch sw);
void cancelBoxTimer(void);
void attractTimerButton( fastSwitch theSwitch );


#endif //__TWEENPIN_H__
