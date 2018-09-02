# Standard makefile to use as a base for DJGPP projects
# By MARTINEZ Fabrice aka SNK of SUPREMACY

# Programs to use during make
AR = ar
CC = gcc
LD = gcc
ASM = nasm
PACKER = upx

# Flags for debugging
#DEBUG=1
#SYMBOLS=1

# Flags for compilation
ASMFLAGS = -f coff
ifdef SYMBOLS
CFLAGS = -o -mpentium -Wall -Werror -g
LDFLAGS = -s
else
CFLAGS = -fomit-frame-pointer -O3 -mpentium -Werror -Wall \
	-W -Wno-sign-compare -Wno-unused \
	-Wpointer-arith -Wbad-function-cast -Wcast-align -Waggregate-return \
	-pedantic \
	-Wshadow \
	-Wstrict-prototypes
LDFLAGS =
endif
CDEFS =
ASMDEFS =

# Object files
MAINOBJS =

# Library files
MAINLIBS = obj/adpcm.a

# Make rules
all: adpcm.exe

adpcm.exe:	$(MAINOBJS) $(MAINLIBS)
		$(LD) $(LDFLAGS) $(MAINOBJS) $(MAINLIBS) -o $@

src/%.asm:

obj/%.o:	src/%.c
		$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

obj/%.oa:	src/%.asm
		$(ASM) -o $@ $(ASMFLAGS) $(ASMDEFS) $<

obj/%.a:
		$(AR) cr $@ $^

# Rules to manage files
pack:		adpcm.exe
		$(PACKER) adpcm.exe

mkdir:
		md obj

clean:
		del obj\*.o
		del obj\*.a
		del obj\*.oa
		del *.exe

# Rules to make libraries
obj/adpcm.a:	obj/adpcm.o
# obj/decode.oa \