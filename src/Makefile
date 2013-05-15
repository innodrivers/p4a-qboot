OBJS_C = clock.o iomux.o gpio.o main.o delay.o console.o
SRCS_C := $(OBJS_C:.o=.c)

OBJS_S = head.o
SRCS_S :=$(OBJS_S:.o=.S)

SUBDIRS = board drivers cmd lib
SUB_BUILT_IN_OBJS := $(foreach n, $(SUBDIRS), $(n)/$(BUILT_IN_OBJ))

all : $(BUILT_IN_OBJ)

$(BUILT_IN_OBJ) : $(OBJS_S) $(SUB_BUILT_IN_OBJS) $(OBJS_C)
	$(LD) $(LDFLAGS) -r $^ -o $@

$(SUB_BUILT_IN_OBJS) : $(SUBDIRS)

$(SUBDIRS):
	@make -C $@ all

$(OBJS_C) : $(SRCS_C)
	$(CC) $(CFLAGS) -c $^

$(OBJS_S) : $(SRCS_S)
	$(CC) $(ASFLAGS) -c $^

clean:
	@for dir in $(SUBDIRS); do \
			make -C $$dir clean;	\
	 done
	@rm -vf *.o

.PHONY : $(SUBDIRS)
