CC = gcc
CFLAGS = -Wall -Wextra -pedantic
SRC = main.c
HEADERS = define.h
OUT_DIR = outputs
TARGET = $(OUT_DIR)/network_switch
BETTY ?= betty

.PHONY: all run betty clean

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

betty:
	@command -v $(BETTY) >/dev/null 2>&1 || { \
		echo "Error: Betty is not installed or not in PATH."; \
		echo "See README.md for installation instructions."; \
		exit 1; \
	}
	$(BETTY) $(SRC) $(HEADERS)

clean:
	rm -f $(TARGET)