TARGET := neoadpcmextract
SOURCE := autoextract.c neoadpcmextract.c
CFLAGS := -std=c99 -O2 -pipe -Wall -Wextra -pedantic
LDFLAGS := $(CFLAGS)


OBJECT := $(SOURCE:%.c=%.o)
DEPEND := $(OBJECT:%.o=%.d)

.PHONY: default all clean
default: $(TARGET)
all: $(TARGET)

$(TARGET): $(OBJECT)
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

-include: $(DEPEND)

clean:
	rm -f $(TARGET) $(OBJECT) $(DEPEND)
