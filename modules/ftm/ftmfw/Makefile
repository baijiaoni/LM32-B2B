LD        := lm32-elf-ld
CC        := lm32-elf-gcc
DEBUGLVL  = 0
TARGET    = ftm
MYPATH    = .
CHECKOUT  = /home/mkreider/hdlprojects/new_bel_projects
INCPATH   = $(CHECKOUT)/modules/lm32-include
W1        = $(CHECKOUT)/ip_cores/wrpc-sw
CFLAGS    = -I$(INCPATH) -mmultiply-enabled -mbarrel-shift-enabled -Os -I$(W1)/include -I$(W1)/pp_printf -DDEBUGLEVEL=$(DEBUGLVL)

all: $(TARGET).bin $(TARGET).elf

$(TARGET).bin: $(TARGET).elf
	lm32-elf-objcopy -O binary $< $@

$(TARGET).elf:	$(INCPATH)/dbg.c $(INCPATH)/aux.c $(INCPATH)/ebm.c $(INCPATH)/timer.c $(INCPATH)/irq.c \
		$(INCPATH)/mini_sdb.c $(MYPATH)/main.c $(INCPATH)/crt0.S $(INCPATH)/mprintf.c \
		$(W1)/dev/uart.c $(MYPATH)/ftm.c
		$(CC) $(CFLAGS) -o $@ -nostdlib -T linker.ld $^ 


clean:
	rm -f *.o *.elf *.bin
