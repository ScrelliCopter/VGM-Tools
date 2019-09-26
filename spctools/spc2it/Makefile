TARGET := spc2it
SOURCE := emu.c it.c main.c sound.c spc700.c
CFLAGS ?= -O2 -pipe


BUILD_CFLAGS  := $(CFLAGS)
BUILD_LDFLAGS := $(CFLAGS) $(LDFLAGS) -lm
PREFIX  := /usr/local
OBJDIR  := obj
OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(SOURCE))
DEPENDS := $(OBJECTS:%.o=%.d)

default: all $(TARGET)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(BUILD_CFLAGS) -MMD -c $< -o $@

$(OBJDIR):
	mkdir -p $@

$(TARGET): $(OBJECTS)
	$(CC) $(BUILD_LDFLAGS) $^ -o $@

-include $(DEPENDS)

all: $(TARGET)

.PHONY: install
install: $(TARGET)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	
.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPENDS)
