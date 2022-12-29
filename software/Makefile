PRJ = tweenpin

MCU = atmega1284
CLK = 16000000

SRC = $(PRJ).c display.c ht16k33.c fastswitch.c io.c uart.c timers.c sound.c lamp.c slowswitch.c solenoid.c timer.c

INCLUDE := $(foreach dir, $(EXT), -I$(dir))
CFLAGS   = -Wall -Os -DF_CPU=$(CLK) -mmcu=$(MCU) $(INCLUDE) -gdwarf-2 -std=gnu99 -funsigned-char -funsigned-bitfields
CFLAGS  += -DUSART0_ENABLED=1 -DUART_TX0_BUFFER_SIZE=64 -DUART_RX0_BUFFER_SIZE=0
CFLAGS  += -DUSART1_ENABLED=1 -DUART_TX1_BUFFER_SIZE=64 -DUART_RX1_BUFFER_SIZE=0

AVRDUDE = avrdude -P usb -c avrispmkII -p $(MCU)
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size --format=avr --mcu=$(MCU)
CC      = avr-gcc

CFILES    = $(filter %.c, $(SRC))
EXTC     := $(foreach dir, $(EXT), $(wildcard $(dir)/*.c))
OBJ       = $(CFILES:.c=.o) $(EXTC:.c=.o) 

all: $(PRJ).hex

test-isp:
	$(AVRDUDE) -v -B 125kHz
	
read-fuse:
	$(AVRDUDE) -U lfuse:r:-:i -v

write-fuse-external-xtal:
	# clksel 0111 full swing xtal
	$(AVRDUDE) -U lfuse:w:0xF7:m
	
write-fuse-internal-rc:
	$(AVRDUDE) -U lfuse:w:0xe2:m
	
flash: all
	$(AVRDUDE) -V -u -U flash:w:$(PRJ).hex:i

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
