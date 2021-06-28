PRJ = tweenpin
MCU = atmega16
CLK = 8000000

# avr programmer (and port if necessary)
# e.g. PRG = usbtiny -or- PRG = arduino -P /dev/tty.usbmodem411
PRG = usbtiny

# fuse values for avr: low, high, and extended
# these values are from an Arduino Uno (ATMega328P)
# see http://www.engbedded.com/fusecalc/ for other MCUs and options
LFU = 0xFF
HFU = 0xDE
EFU = 0x05

SRC = $(PRJ).c display.c fastswitch.c io.c lamp.c slowswitch.c solenoid.c util.c
EXT =

INCLUDE := $(foreach dir, $(EXT), -I$(dir))
CFLAGS    = -Wall -Os -DF_CPU=$(CLK) -mmcu=$(MCU) $(INCLUDE)
CPPFLAGS = -gdwarf-2 -std=gnu99 -funsigned-char -funsigned-bitfields

AVRDUDE = avrdude -c $(PRG) -p $(MCU)
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size --format=avr --mcu=$(MCU)
CC      = avr-gcc

CFILES    = $(filter %.c, $(SRC))
EXTC     := $(foreach dir, $(EXT), $(wildcard $(dir)/*.c))
CPPFILES  = $(filter %.cpp, $(SRC))
EXTCPP   := $(foreach dir, $(EXT), $(wildcard $(dir)/*.cpp))
OBJ       = $(CFILES:.c=.o) $(EXTC:.c=.o) $(CPPFILES:.cpp=.o) $(EXTCPP:.cpp=.o)

all: $(PRJ).hex

test:
	$(AVRDUDE) -v

flash: all
	$(AVRDUDE) -U flash:w:$(PRJ).hex:i

fuse:
	$(AVRDUDE) -U lfuse:w:$(LFU):m -U hfuse:w:$(HFU):m -U efuse:w:$(EFU):m

disasm: $(PRJ).elf
	$(OBJDUMP) -d $(PRJ).elf

clean:
	rm -f *.hex *.elf *.o
	$(foreach dir, $(EXT), rm -f $(dir)/*.o;)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.cpp.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(PRJ).elf: $(OBJ)
	$(CC) $(CFLAGS) -o $(PRJ).elf $(OBJ)

$(PRJ).hex: $(PRJ).elf
	rm -f $(PRJ).hex
	$(OBJCOPY) -j .text -j .data -O ihex $(PRJ).elf $(PRJ).hex
	$(SIZE) $(PRJ).elf
