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

static uint8_t view_x = 0;
static uint8_t view_y = 0;

void displayView(uint8_t x, uint8_t y) {
  view_x = x;
  view_y = y;
}

static int display_putchar_printf(char ch, FILE *stream) {
    static uint8_t x0 = 0;
    static uint8_t y0 = 0;
    uint8_t w = displayChar( ch, x0-view_x, y0+view_y );
    if ( ch == '\n' ) {
      ht16k33DisplayFrame();
      x0 = (LED_BACKPACKS*LED_COLUMNS);
    } else {
      x0 -= w;
      for (int y=0; y<LED_ROWS; y++) {
          ht16k33DrawPixel( x0-view_x, y, 0 );
      }
      x0 -= 1;
    }
    return 0;
}

void displayText(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  clearDisplay();
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void clearDisplay(void) {
    ht16k33Clear();
}

void initDisplay(void) {
    displayView( 0, 0 );
    ht16k33Init();
    printf("\n");
    
    // hook stdio printf() to the led matrix
    static FILE mystdout = FDEV_SETUP_STREAM(display_putchar_printf, NULL, _FDEV_SETUP_WRITE);
    stdout = &mystdout;
}
