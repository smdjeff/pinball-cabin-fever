#include "tweenpin.h"

// note: this requires 5 8x8 bi-color displays 
// each has a separate i2c address.
// https://www.adafruit.com/product/902

typedef struct __attribute__((packed)) {
  uint8_t ascii;
  uint8_t width;
  uint8_t height;
  uint8_t data[];
} font_t;

const uint8_t font_d3[] = {
  #include "fonts/d3.txt"
};


static font_t *findChar( char ch, font_t *font, int font_sz ) {
  if ( font && font_sz ) {
    uint8_t *p = (uint8_t*)font; 
    do {
      font_t *f = (font_t*)p;
      if ( f->ascii == ch ) {
        return f;
      }
      p += sizeof(font_t) + (f->width * f->height);
    } while ( p < (uint8_t*)font + font_sz );
  }
  return NULL;
}

uint8_t displayChar(char ch, uint8_t x0, uint8_t y0) {
  font_t *font = findChar(ch, (font_t*)font_d3, sizeof(font_d3) );
  if ( font ) {
    for (int x=0; x<font->width; x++) {
        for (int y=0; y<font->height; y++) {
            int color = font->data[ (y*font->width) + x ];
            ht16k33DrawPixel( x0-x, y0+y, color );
        }
    }
    return font->width;
  }
  return 0;
}

// the display is LED_BACKPACKS 8x8 panels side by side. column 0 is the
// right most, so x counts up to the left.
#define DISPLAY_WIDTH        (LED_BACKPACKS*LED_COLUMNS)
#define DISPLAY_DEFAULT_Y    1
#define SCROLL_START_PAUSE   HALF_SECOND
#define SCROLL_STEP          FIVE_HUNDRETH_SECONDS

static int8_t view_x = 0;           // + moves text right, - moves it left
static int8_t view_y = DISPLAY_DEFAULT_Y;
static bool   viewSet = false;      // caller set the view for the next text

static char    text[24];
static int16_t textWidth = 0;
static int16_t scroll = 0;

// position the next displayText() call. used by animations, which redraw
// the text themselves, so text drawn with an explicit view never auto scrolls.
// the view only applies to the next displayText() call.
void displayView(uint8_t x, uint8_t y) {
  view_x = (int8_t)x;
  view_y = (int8_t)y;
  viewSet = true;
}

static int16_t measureText( const char *s ) {
  int16_t w = 0;
  for ( ; *s; s++ ) {
    font_t *f = findChar( *s, (font_t*)font_d3, sizeof(font_d3) );
    if ( f ) w += f->width + 1;   // character plus one blank column
  }
  return w ? w - 1 : 0;
}

// pixel width text would occupy, for right-justifying before displayText().
uint16_t displayTextWidth(const char *s) {
  return measureText(s);
}

// draws text with its left edge shifted 'left' columns to the left
static void drawText( const char *s, int16_t left, int8_t down ) {
  ht16k33ClearFrame();
  int16_t x0 = DISPLAY_WIDTH - 1 + left;   // left most column of this character
  for ( ; *s; s++ ) {
    font_t *f = findChar( *s, (font_t*)font_d3, sizeof(font_d3) );
    if ( !f ) continue;
    for ( int16_t x = 0; x < f->width; x++ ) {
      int16_t px = x0 - x;
      if ( px < 0 || px >= DISPLAY_WIDTH ) continue;
      for ( int16_t y = 0; y < f->height; y++ ) {
        int16_t py = down + y;
        if ( py < 0 || py >= LED_ROWS ) continue;
        ht16k33DrawPixel( px, py, f->data[ (y*f->width) + x ] );
      }
    }
    x0 -= f->width + 1;
  }
  ht16k33DisplayFrame();
}

// left-justified text starts TEXT_LEFT_MARGIN in from the left edge. text
// too wide for that scrolls left after a short pause, stopping when its end
// is SCORE_RIGHT_MARGIN in from the right edge.
#define TEXT_FIT_WIDTH   (DISPLAY_WIDTH - TEXT_LEFT_MARGIN - SCORE_RIGHT_MARGIN)

// shows text, left-justified unless displayView() set a position.
void displayText(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf( text, sizeof(text), fmt, args );
  va_end(args);

  cancelTimer( DISPLAY_TMR );
  textWidth = measureText( text );

  if ( viewSet ) {
    viewSet = false;
    drawText( text, -view_x, view_y );
    return;
  }

  scroll = 0;
  drawText( text, -TEXT_LEFT_MARGIN, DISPLAY_DEFAULT_Y );
  if ( textWidth > TEXT_FIT_WIDTH ) {
    setTimer( DISPLAY_TMR, SCROLL_START_PAUSE, 0 );
  }
}

void displayScrollTimer(timerEvent id, uint16_t state) {
  scroll++;
  drawText( text, scroll - TEXT_LEFT_MARGIN, DISPLAY_DEFAULT_Y );
  if ( scroll < textWidth - TEXT_FIT_WIDTH ) {
    setTimer( DISPLAY_TMR, SCROLL_STEP, 0 );
  } else {
    cancelTimer( DISPLAY_TMR );
  }
}

void clearDisplay(void) {
    cancelTimer( DISPLAY_TMR );
    ht16k33Clear();
}

void initDisplay(void) {
    ht16k33Init();
}
