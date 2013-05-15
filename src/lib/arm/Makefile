
OBJS_S = lib1funcs.o
SRCS_S := $(OBJS_S:.o=.S)

OBJS_C = div0.o
SRCS_C := $(OBJS_C:.o=.c)

all : $(BUILT_IN_OBJ)

$(BUILT_IN_OBJ) : $(OBJS_C) $(OBJS_S)
	$(LD) $(LDFLAGS) -r $^ -o $@

$(OBJS_S) : $(SRCS_S)
	$(CC) $(ASFLAGS) -c $^

$(OBJS_C) : $(SRCS_C)
	$(CC) $(CFLAGS) -c $^

clean:
	@rm -vf *.o
