# modified 1/31/16 by Chris Martin
# make flash now finds where STM board is mounted and
# directly copies the TARGET.bin without the need for stlink
TARGET = lab
OBJS   = main.o

INSTALLDIR = /usr/local/stmdev/


ARCH = STM32L476xx

CC=arm-none-eabi-gcc
LD=arm-none-eabi-ld
AS=arm-none-eabi-as
OBJCOPY=arm-none-eabi-objcopy

INCDIRS = -I$(INSTALLDIR)/include -I.
LIBDIRS = -L$(INSTALLDIR)/lib

LIBS=  -lece486_$(ARCH) -l$(ARCH) -lcmsis_dsp_$(ARCH) -lm

LINKSCRIPT = $(INSTALLDIR)/lib/$(ARCH)_FLASH.ld

CFLAGS = -mcpu=cortex-m4 -mthumb -O3 -Wall  \
         -fomit-frame-pointer -fno-strict-aliasing -fdata-sections \
         -include stm32l4xx_hal_conf.h -DARM_MATH_CM4 -D$(ARCH) \
         -mfpu=fpv4-sp-d16 -mfloat-abi=softfp $(INCDIRS) \
         -fsingle-precision-constant -ffunction-sections

LDFLAGS = -u _printf_float \
	  -Wl,-T$(LINKSCRIPT) \
	  -static \
          -Wl,--gc-sections $(LIBDIRS)


.PHONY : all st-flash flash clean debug $(TEST_TARGET)

all: $(TARGET) $(TARGET).bin

debug : CFLAGS += -DDEBUG -g -Og
debug : LDFLAGS += -Wl,-Map,$(TARGET).map

debug : all

$(TARGET): $(OBJS)
	$(CC)  -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)

$(TEST_TARGET): $(TEST_SRC)
	cc -Wall -o $(TEST_TARGET) $(TEST_SRC) && ./$(TEST_TARGET)

$(TARGET).bin: $(TARGET)
	$(OBJCOPY) -Obinary $(TARGET) $(TARGET).bin

st-flash: $(TARGET).bin
	st-flash write $(TARGET).bin 0x08000000

flash: $(TARGET).bin
	cp $(TARGET).bin $(shell mount|grep DIS_L476VG|awk '{print $$3}')

clean:
	rm -f $(OBJS) $(TARGET) $(TEST_TARGET) $(TARGET).bin $(TARGET).map
