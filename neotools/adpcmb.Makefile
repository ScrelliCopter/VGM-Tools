CC = gcc
CFLAGS = -O3

SRC = .
OBJ = .

OUT_OBJ = $(OBJ)/adpcmb.exe

all:	$(OUT_OBJ)

$(OBJ)/%.exe:	$(SRC)/%.c
	$(CC) $(CCFLAGS) $(MAINFLAGS) $< -o $@

clean:
	rm $(OUT_OBJ)
