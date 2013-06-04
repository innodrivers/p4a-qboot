
OBJS_C = string.o stdio.o eabi.o
SRCS_C := $(OBJS_C:.o=.c)

all: $(BUILT_IN_OBJ)

$(BUILT_IN_OBJ): $(OBJS_C)
	$(LD) -r $(LDFLAGS) -o $@ $^

$(OBJS_C):$(SRCS_C)
	$(CC) $(CFLAGS) -c $^

clean:
	@rm -vf *.o


