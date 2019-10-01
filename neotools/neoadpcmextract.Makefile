TARGET := neoadpcmextract
SOURCE := neoadpcmextract.c
CFLAGS := -std=c99 -O2 -pipe -Wall -Wextra -pedantic

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET)
