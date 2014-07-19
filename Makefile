all: minir_dummy

CC = gcc
CFLAGS =
LD = gcc
LFLAGS =

#Native toolchain, aka not cross compiler. Used because parts of minir are autogenerated, and the generator is also written in C.
#The generated file doesn't change between platforms.
NATCC = $(CC)
NATCFLAGS = $(CFLAGS) -std=c99
NATLD = $(LD)
NATLFLAGS = $(LFLAGS)

#Windows only code.
TRUE_CFLAGS = $(CFLAGS)
TRUE_LFLAGS = $(LFLAGS) -lgdi32 -lcomctl32 -lcomdlg32 -ldinput8 -ldxguid -lopengl32 -ldsound
EXESUFFIX = .exe
EXTRAOBJ = obj/resource$(OBJSUFFIX).o
RC = windres
RCFLAGS =
OBJSUFFIX =
obj/resource$(OBJSUFFIX).o: ico/*
	$(RC) $(RCFLAGS) ico/minir.rc obj/resource$(OBJSUFFIX).o

#On Linux, this undoes most of the above. On Windows, this file doesn't exist because 'configure' wasn't executed.�
-include Makefile.custom

OUTNAME = minir$(EXESUFFIX)

TESTSRC = memory.c
TESTSEPSRC = test-*.c window-*.c

OBJS = $(patsubst %.c,obj/%$(OBJSUFFIX).o,$(wildcard *.c)) $(EXTRAOBJ)
TESTOBJS = $(patsubst %.c,obj/%.o,$(wildcard $(TESTSRC))) $(patsubst %.c,obj/%-test.o,$(wildcard $(TESTSEPSRC))) $(EXTRAOBJ)

TRUE_CFLAGS += -std=c99
TRUE_LFLAGS +=


#$(CC) $(TRUE_CFLAGS) -DTEST -DNO_ICON test*.c window*.c $(TRUE_LFLAGS) -otest $(RESOBJ)
test: $(TESTOBJS)
	$(LD) $+ $(TRUE_LFLAGS) -o $@

#On Windows, cleaning up the object directory is expected to be done with a separate batch script.
#The contents are typically
#del /q obj\*
#del minir.exe
clean:
	rm obj/* || true

obj:
	mkdir obj

obj/config$(OBJSUFFIX).o: config.c obj/generated.c | obj
obj/main$(OBJSUFFIX).o: main.c obj/generated.c minir.h | obj
obj/%$(OBJSUFFIX).o: %.c | obj obj/generated.c
	$(CC) $(TRUE_CFLAGS) -c $< -o $@

obj/%-test.o: %.c | obj obj/config.c
	$(CC) $(TRUE_CFLAGS) -c $< -o $@ -DTEST -DNO_ICON

obj/generated.c: obj/rescompile$(EXESUFFIX) minir.cfg.tmpl
	obj/rescompile$(EXESUFFIX)
obj/rescompile$(EXESUFFIX): rescompile.c miniz.c | obj
	$(NATCC) $(NATCFLAGS) $(NATLFLAGS) -DRESCOMPILE rescompile.c miniz.c -o obj/rescompile$(EXESUFFIX)

$(OUTNAME): $(OBJS)
	$(LD) $+ $(TRUE_LFLAGS) -o $@ -lm

minir_dummy: $(OUTNAME)
