TARGET := neoadpcmextract
SOURCE := neoadpcmextract.cpp
CXXFLAGS := -O2 -pipe -Wall -Wextra -pedantic

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET)
