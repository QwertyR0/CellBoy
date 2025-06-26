GBDK = c:/gbdk/bin # go ahead and change this
SRC = src
BIN = bin
CC = $(GBDK)/lcc

OBJS = $(BIN)/main.o $(BIN)/saveState.o
OUT = $(BIN)/main.gb

all: $(OUT)

$(BIN)/%.o: $(SRC)/%.c
	$(CC) -Wa-l -Wf-ba0 -c -o $@ $<

$(OUT): $(OBJS)
	$(CC) -Wl-yt3 -Wl-yo4 -Wl-ya4 -o $@ $(OBJS)

clean:
	del /Q $(BIN)\*.o $(OUT)

.PHONY: all clean
