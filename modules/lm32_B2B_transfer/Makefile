TARGET		  = B2B_main
DEVICE		  = EP2AGX125
FLASH		    = EPCS128
RAM_SIZE	  = 65536
SHARED_SIZE = 24K
USRCPUCLK   = 125000

MYPATH   = ./
CFLAGS    = -I$(PSCU)/include -I$(MYPATH)

include /home/jbai/test/bel_projects_scu/bel_projects/syn/build.mk

$(TARGET).elf: $(MYPATH)/B2B_main.c

clean::
	rm -f *.o *.elf *.bin

                
