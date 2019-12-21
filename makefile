CC = @mpicc
CFLAGS += -Wall -Wempty-body -Werror -Wstrict-prototypes -Werror=maybe-uninitialized -Warray-bounds
CFLAGS += -g3 -lefence -I./include/

PROGRAM = io500
SEARCHPATH += src
SEARCHPATH += include
SEARCHPATH += test
vpath %.c $(SEARCHPATH)
vpath %.h $(SEARCHPATH)
.SUFFIXES:

DEPS += io500-util.h io500-debug.h io500-opt.h
OBJS += util.o
OBJS += ini-parse.o
OBJS += phase_find.o phase_ior_easy.o phase_ior_easy_read.o phase_ior_easy_write.o phase_ior_hard.o phase_ior_hard_read.o phase_ior_hard_write.o phase_mdtest_easy.o phase_mdtest_easy_delete.o phase_mdtest_easy_stat.o phase_mdtest_easy_write.o phase_mdtest_hard.o phase_mdtest_hard_delete.o phase_mdtest_hard_read.o phase_mdtest_hard_stat.o phase_mdtest_hard_write.o phase_opt.o


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
	$(CC) -o $@ main.o $(LDFLAGS) io500.a

%.o: %.c $(DEPS)
	@echo CC $@
	$(CC) $(CFLAGS) -c -o $@ $<
