CC = mpicc
CFLAGS += -std=gnu99 -Wall -Wempty-body -Werror -Wstrict-prototypes -Werror=maybe-uninitialized -Warray-bounds

IORCFLAGS = $(shell grep CFLAGS ./build/ior/src/build.conf | cut -d "=" -f 2-)
CFLAGS += -g3 -lefence -I./include/ -I./src/ -I./build/pfind/src/ -I./build/ior/src/
IORLIBS = $(shell grep LDFLAGS ./build/ior/src/build.conf | cut -d "=" -f 2-)
LDFLAGS += -lm $(IORCFLAGS) $(IORLIBS) # -lgpfs # may need some additional flags as provided to IOR

VERSION_GIT=$(shell git describe --always --abbrev=12)
VERSION_HASH=$(shell git log --pretty=format:'%h' -n 1)
VERSION_TREE=$(shell git diff src | wc -l | sed -e 's/   *//g' -e 's/^0//' | sed "s/\([0-9]\)/-\1/")
VERSION=$(VERSION_HASH)$(VERSION_TREE)
CFLAGS += -DVERSION="\"$(VERSION)\""
PROGRAM = io500
VERIFIER = io500-verify
SEARCHPATH += src
SEARCHPATH += include
SEARCHPATH += test
vpath %.c $(SEARCHPATH)
vpath %.h $(SEARCHPATH)
.SUFFIXES:

DEPS += io500-util.h io500-debug.h io500-opt.h
OBJS += util.o
OBJS += ini-parse.o phase_dbg.o phase_opt.o phase_timestamp.o
OBJS += phase_find.o phase_ior_easy.o phase_ior_easy_read.o phase_mdtest.o phase_ior.o phase_ior_easy_write.o phase_ior_hard.o phase_ior_hard_read.o phase_ior_hard_write.o phase_mdtest_easy.o phase_mdtest_easy_delete.o phase_mdtest_easy_stat.o phase_mdtest_easy_write.o phase_mdtest_hard.o phase_mdtest_hard_delete.o phase_mdtest_hard_read.o phase_mdtest_hard_stat.o phase_mdtest_hard_write.o

TESTS += ini-test
TESTSEXE = $(patsubst %,%.exe,$(TESTS))

all: $(VERIFIER) $(PROGRAM) $(TESTSEXE)

%.exe: %.o $(DEPS) io500.a
	@echo LD $@
	$(CC) -o $@ $< $(LDFLAGS) io500.a

clean:
	@echo CLEAN
	@$(RM) *.o io500.a *.exe $(PROGRAM)

io500.a: $(OBJS)
	@echo AR $@
	ar rcsT $@ $(OBJS)

$(VERIFIER): verifier.o io500.a
	@echo LD $@
	$(CC) -o $@ verifier.o io500.a $(LDFLAGS)

$(PROGRAM): io500.a main.o
	@echo LD $@
	$(CC) -o $@ main.o $(LDFLAGS) io500.a ./build/pfind/pfind.a ./build/ior/src/libaiori.a  $(LDFLAGS)

.PHONY: main.o
main.o: main.c $(DEPS)
	@echo CC $@
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c $(DEPS)
	@echo CC $@
	$(CC) $(CFLAGS) -c -o $@ $<
