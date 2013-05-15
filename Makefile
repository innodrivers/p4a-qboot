
-include local.mk

TOP_DIR = $(shell pwd)

CROSS_COMPILE ?=  arm-none-linux-gnueabi-

AS=$(CROSS_COMPILE)as
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
OBJDUMP=$(CROSS_COMPILE)objdump
OBJCOPY=$(CROSS_COMPILE)objcopy

CFLAGS = -I$(TOP_DIR)/inc  -nostdinc -nostdlib -fno-builtin -O2 -g
ASFLAGS = $(CFLAGS) -D__ASSEMBLY__

# for cortex-A8
CFLAGS += -D__LINUX_ARM_ARCH__=7

ifeq ($(strip $(CONFIG_P4A)),y)
CFLAGS += -DCONFIG_P4A -DCONFIG_P4A_CPU2
endif

ifeq ($(strip $(CONFIG_P4A_FPGA)),y)
CFLAGS += -DCONFIG_P4A_FPGA
endif

BUILT_IN_OBJ=built-in.o

export CC LD CFLAGS LDFLAGS ASFLAGS CROSS_COMPILE BUILT_IN_OBJ TOP_DIR

SUBDIRS = src

SUB_BUILT_IN_OBJS := $(foreach n, $(SUBDIRS), $(n)/$(BUILT_IN_OBJ))


TARGET := qboot
TARGETBIN := $(TARGET).bin
TARGETELF := $(TARGET).elf
TARGETDASM := $(TARGET).dasm
LINKER_SCRIPT_TEMP := ld.script.in
LINKER_SCRIPT := $(TARGET).lds

SEDFLAGS = s/TEXT_BASE/$(CONFIG_START_ADDR)/

all : $(TARGET)

$(TARGET) : $(SUBDIRS) $(TARGETBIN) $(TARGETELF) $(TARGETDASM) $(LINKER_SCRIPT)

$(LINKER_SCRIPT) : $(LINKER_SCRIPT_TEMP)
	@sed "$(SEDFLAGS)" < $< > $@

$(TARGETELF)	: $(SUB_BUILT_IN_OBJS) $(LINKER_SCRIPT)
	$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) -Ttext $(CONFIG_START_ADDR) $(SUB_BUILT_IN_OBJS) -o $@
	$(OBJDUMP) -D $@ > $(TARGETDASM)

$(TARGETBIN)	: $(TARGETELF)
	$(OBJCOPY) -O binary -R .comment -S $< $@

clean:
	@for dir in $(SUBDIRS); do \
			make -C $$dir clean;	\
	 done
	@rm -rf *.o *.bak *.*~ *.elf *.bin *.dasm $(LINKER_SCRIPT)
  
$(SUBDIRS) :
	@make -C $@ all

.PHONY: $(SUBDIRS)

