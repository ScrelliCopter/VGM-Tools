TARGET := neoadpcmextract
SOURCE := autoextract.c neoadpcmextract.c
CFLAGS := -std=c99 -O2 -pipe -Wall -Wextra -pedantic
LDFLAGS := $(CFLAGS)


OBJDIR := .obj_$(TARGET)
OBJECT := $(patsubst %.c, $(OBJDIR)/%.o, $(SOURCE))
DEPEND := $(OBJECT:%.o=%.d)

.PHONY: default all clean
default: $(TARGET)
all: $(TARGET)

$(TARGET): $(OBJECT)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

-include: $(DEPEND)

clean:
	rm -rf $(OBJDIR)
