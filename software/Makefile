PRJ = tweenpin
MCU = atmega16
CLK = 8000000

SRC = $(PRJ).c display.c fastswitch.c io.c lamp.c slowswitch.c solenoid.c util.c

INCLUDE := $(foreach dir, $(EXT), -I$(dir))
CFLAGS    = -Wall -Os -DF_CPU=$(CLK) -mmcu=$(MCU) $(INCLUDE) -gdwarf-2 -std=gnu99 -funsigned-char -funsigned-bitfields

AVRDUDE = avrdude -P usb -c avrispmkII -p m16
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size --format=avr --mcu=$(MCU)
CC      = avr-gcc

CFILES    = $(filter %.c, $(SRC))
EXTC     := $(foreach dir, $(EXT), $(wildcard $(dir)/*.c))
OBJ       = $(CFILES:.c=.o) $(EXTC:.c=.o) 

all: $(PRJ).hex

test:
	$(AVRDUDE) -v

flash: all
	$(AVRDUDE) -U flash:w:$(PRJ).hex:i

disasm: $(PRJ).elf
	$(OBJDUMP) -d $(PRJ).elf

clean:
	rm -f *.hex *.elf *.o
	$(foreach dir, $(EXT), rm -f $(dir)/*.o;)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(PRJ).elf: $(OBJ)
	$(CC) $(CFLAGS) -o $(PRJ).elf $(OBJ)

$(PRJ).hex: $(PRJ).elf
	rm -f $(PRJ).hex
	$(OBJCOPY) -j .text -j .data -O ihex $(PRJ).elf $(PRJ).hex
	$(SIZE) $(PRJ).elf
