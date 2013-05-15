
OBJS_C = vsprintf.o
SRCS_C := $(OBJS_C:.o=.c)

SUBDIRS = arm stdlib

SUB_BUILT_IN_OBJS := $(foreach n, $(SUBDIRS), $(n)/$(BUILT_IN_OBJ))

all: $(SUBDIRS) $(BUILT_IN_OBJ)

$(SUBDIRS): 
	@make -C $@ all

$(BUILT_IN_OBJ): $(SUB_BUILT_IN_OBJS) $(OBJS_C)
	$(LD) $(LDFLAGS) -r $^ -o $@

$(OBJS_C) : $(SRCS_C)
	$(CC) $(CFLAGS) -c $^

clean:
	@for dir in $(SUBDIRS); do \
		make -C $$dir clean; \
	 done	
	@rm -vf *.o

.PHONY: $(SUBDIRS)

