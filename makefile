CC = @gcc
CFLAGS += -Wall -Wempty-body -Werror -Wstrict-prototypes -Werror=maybe-uninitialized -Warray-bounds
CFLAGS += -g3 -lefence -I./include/

PROGRAM = io500
SEARCHPATH += src
SEARCHPATH += include
SEARCHPATH += test
vpath %.c $(SEARCHPATH)
vpath %.h $(SEARCHPATH)
.SUFFIXES:

DEPS += io500-util.h io500-debug.h
OBJS += debug.o
OBJS += ini-parse.o

TESTS += ini-test
TESTSEXE = $(patsubst %,%.exe,$(TESTS))

all: $(PROGRAM) $(TESTSEXE)

%.exe: %.o $(DEPS)
	@echo LD $@
	$(CC) -o $@ $< $(LDFLAGS) io500.a

clean:
	@echo CLEAN
	@$(RM) *.o io500.a *.exe $(PROGRAM)

io500.a: $(OBJS)
	@echo AR $@
	@ar rcs $@ $(OBJS)

$(PROGRAM): io500.a main.o
	@echo LD $@
	$(CC) -o $@ io500.a main.o $(LDFLAGS)

%.o: %.c $(DEPS)
	@echo CC $@
	$(CC) $(CFLAGS) -c -o $@ $<
