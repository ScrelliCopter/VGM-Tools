# Standard makefile to use as a base for DJGPP projects (not anymore lol)
# By MARTINEZ Fabrice aka SNK of SUPREMACY

# Programs to use during make
LD := $(CC)

TARGET := adpcm
SOURCE := adpcm.c

# Flags for compilation
CFLAGS := -fomit-frame-pointer -O3 -Werror -Wall \
	-W -Wno-sign-compare -Wno-unused \
	-Wpointer-arith -Wbad-function-cast -Wcast-align -Waggregate-return \
	-pedantic \
	-Wshadow \
	-Wstrict-prototypes
LDFLAGS := -lm

# Object files
OBJECT := $(SOURCE:%.c=%.o)

# Make rules
all: $(TARGET)

$(TARGET): $(OBJECT)
		$(LD) $(CFLAGS) $(LDFLAGS) $^ -o $@

%.o: %.c
		$(CC) $(CFLAGS) -c $< -o $@

# Rules to manage files
.PHONY: clean
clean:
		rm -f $(TARGET) $(OBJECT)
